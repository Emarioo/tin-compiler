#include "Lexer.h"

void TokenStream::Destroy(TokenStream* stream) {
    delete stream;
}
TokenStream* lex_file(const std::string& path) {
    ZoneScopedC(tracy::Color::Gold);
    
    int filesize = 0;
    char* text = nullptr;
    defer {
        if(text) { Free(text); text = nullptr; }
    };
    {
        ZoneNamedNC(zone0,"lexer_io",tracy::Color::Gold3, true);

        #ifndef AVOID_STDLIB
        std::ifstream file(path, std::ifstream::binary);
        if(!file.is_open())
            return nullptr;

        file.seekg(0, file.end);
        filesize = file.tellg();
        file.seekg(0, file.beg);

        text = (char*)Alloc(filesize);
        Assert(text);
        file.read(text, filesize);
        file.close();
        #else
        bool yes = ReadEntireFile(path,text,filesize);
        Assert(yes);
        #endif
    }

    TokenStream* stream = new TokenStream();
    stream->path = path;
    stream->processed_bytes = filesize; // comments are counted too, is that fine?
    
    bool is_id = false;
    bool is_num = false;
    bool is_decimal = false;
    bool is_str = false;
    bool is_char = false;
    bool is_comment = false;
    bool is_multicomment = false;

    int start_ln = 0;
    int start_col = 0;

    int line=1;
    int column=1;

    int data_start = 0;

    std::string string_content{};
    bool only_whitespace = true;

    int index=0;
    while(index<filesize) {
        char chr = text[index];
        char nextChr = 0;
        if(index+1 < filesize)
            nextChr = text[index+1];
        index++;

        int col = column;
        int ln = line;

        if(chr == '\t')
            column+=4;
        else
            column++;
        if(chr == '\n') {
            column = 1;
            line++;
        }

        bool ending = index == filesize;
        bool delim = chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t';
        
        if(!delim && only_whitespace) {
            only_whitespace = false;
            stream->non_blank_lines++;
        }
        if(chr == '\n') {
            only_whitespace = true;
        }

        if(stream->tokens.size() > 0) {
            if(chr == '\n') {
                stream->tokens.back().flags |= TOKEN_FLAG_SUFFIX_NEWLINE;
                stream->tokens.back().flags &= ~TOKEN_FLAG_SUFFIX_SPACE;
            }
            if(chr == ' ') {
                if(0 == (stream->tokens.back().flags & TOKEN_FLAG_SUFFIX_NEWLINE)) {
                    stream->tokens.back().flags |= TOKEN_FLAG_SUFFIX_SPACE;
                }
            }
        }


        if(is_comment) {
            if(chr == '\n') {
                is_comment = false;
            }
            continue;
        }
        if(is_multicomment) {
            if(chr == '*' && nextChr == '/') {
                index++;
                is_multicomment = false;
            }
            continue;
        }

        if(chr == '/' && nextChr == '/') {
            index++; // skip the second slash
            is_comment=true;
            continue;
        }
        if(chr == '/' && nextChr == '*') {
            index++; // skip the asterix
            is_multicomment=true;
            continue;
        }

        if(chr == '"') {
            if(!is_str) {
                is_str = true;
                data_start = index;
                start_col = col;
                start_ln = ln;
                continue;
            }
            // This code does not handle newlines in quotes
            // int data_end = index-1 - data_start;
            // auto str = std::string(text + data_start, data_end);
            // stream->add_string(str, start_ln, start_col);

            stream->add_string(string_content, start_ln, start_col);
            string_content.clear();
            is_str = false;
            continue;
        }
        if(chr == '\'') {
            if(!is_char) {
                is_char = true;
                data_start = index;
                start_col = col;
                start_ln = ln;
                continue;
            }
            // This code does not handle newlines in quotes
            // int data_end = index-1 - data_start;
            // auto str = std::string(text + data_start, data_end);
            // stream->add_string(str, start_ln, start_col);

            stream->add_char(string_content[0], start_ln, start_col);
            string_content.clear();
            is_char = false;
            continue;
        }
        if(is_str || is_char) {
            if(chr == '\\' && nextChr == '\\') {
                index++;
                string_content += '\\';
            } else if(chr == '\\' && nextChr == 'n') {
                index++;
                string_content += '\n';
            } else {
                string_content += chr;
            }
            continue;
        }

        if((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || chr == '_') {
            if(!is_id) {
                is_id = true;
                data_start = index-1;
                start_col = col;
                start_ln = ln;
            }
            if(!ending)
                continue;
        }

        if((chr >= '0' && chr <= '9')) {
            if(!is_id) {
                if(!is_num && !is_decimal) {
                    data_start = index-1;
                    is_num = true;
                    start_col = col;
                    start_ln = ln;
                }
            }
            if(!ending)
                continue;
        }
        if((chr == '.' && (is_num || (nextChr >= '0' && nextChr <= '9')))) {
            // if we find . then switch state to decimal
            if(!is_decimal) {
                if(!is_num) {
                    data_start = index-1; // don't set start if is_num code parsed the early digits
                    start_col = col;
                    start_ln = ln;
                }
                is_decimal = true;
                is_num = false;
            }
            if(!ending)
                continue;
        }
        if(is_decimal) {
            int data_end = index-1 - data_start;
            if(ending)
                data_end++;
            auto str = std::string(text + data_start, data_end);
            double num = atof(str.c_str());
            stream->add_float(num, start_ln, start_col);
            is_decimal = false;
            if(ending)
                continue;
        }
        if(is_num) {
            int data_end = index-1 - data_start;
            if(ending)
                data_end++;
            auto str = std::string(text + data_start, data_end);
            int num = atoi(str.c_str());
            stream->add_int(num, start_ln, start_col);
            is_num = false;
            if(ending)
                continue;
        }

        if(is_id) {
            int data_end = index-1 - data_start;
            if(ending)
                data_end++;
            auto str = std::string(text + data_start, data_end);

            #define CASE(T) if(str == NAME_OF_TOKEN(T)) stream->add(T, start_ln, start_col);
            
            CASE(TOKEN_STRUCT)
            else CASE(TOKEN_FUNCTION)
            else CASE(TOKEN_WHILE)
            else CASE(TOKEN_CONTINUE)
            else CASE(TOKEN_BREAK)
            else CASE(TOKEN_RETURN)
            else CASE(TOKEN_IF)
            else CASE(TOKEN_ELSE)
            else CASE(TOKEN_GLOBAL)
            else CASE(TOKEN_CONST)
            else CASE(TOKEN_IMPORT)
            else CASE(TOKEN_CAST)
            else CASE(TOKEN_SIZEOF)
            else CASE(TOKEN_TRUE)
            else CASE(TOKEN_FALSE)
            else CASE(TOKEN_NULL)
            else {
                stream->add_id(str, start_ln, start_col);
            }
            is_id = false;
            if(ending)
                continue;
        }

        if(delim) {
            if(stream->tokens.size()) {
                if(chr == '\n') {
                    stream->tokens.back().flags |= TOKEN_FLAG_SUFFIX_NEWLINE;
                    stream->tokens.back().flags &= ~TOKEN_FLAG_SUFFIX_SPACE;
                }
                if(chr == ' ') {
                    if(0 == (stream->tokens.back().flags & TOKEN_FLAG_SUFFIX_NEWLINE)) {
                        stream->tokens.back().flags |= TOKEN_FLAG_SUFFIX_SPACE;
                    }
                }
            }
            continue;
        }

        start_col = col;
        start_ln = ln;
        stream->add((TokenKind)chr, start_ln, start_col);
    }
    
    stream->total_lines = line;

    return stream;
}

const char* token_names[] {
    "EOF",        // TOKEN_EOF,
    "id",        // TOKEN_ID,
    "lit_int",   // TOKEN_LITERAL_INTEGER,
    "lit_dec", // TOKEN_LITERAL_DECIMAL,
    "lit_str",   // TOKEN_LITERAL_STRING,
    "lit_char",   // TOKEN_LITERAL_CHAR,
    "struct",    // TOKEN_STRUCT,
    "fun",       // TOKEN_FUNCTION,
    "while",     // TOKEN_WHILE,
    "continue",  // TOKEN_CONTINUE,
    "break",     // TOKEN_BREAK,
    "return",    // TOKEN_RETURN,
    "if",        // TOKEN_IF,
    "else",      // TOKEN_ELSE,
    "global",    // TOKEN_GLOBAL,
    "const",     // TOKEN_CONST,
    "import",   // TOKEN_IMPORT,
    "cast",   // TOKEN_CAST,
    "sizeof",   // TOKEN_SIZEOF,
    "true",   // ,
    "false",   // ,
    "null",   // ,
};

void TokenStream::print() {
    for(auto& tok : tokens) {
        if(tok.type < 256) {
            printf("%c ",(char)tok.type);
        } else if(tok.type == TOKEN_ID){
            printf("%s (id) ",strings[tok.data_index].c_str());
        } else if(tok.type == TOKEN_LITERAL_STRING){
            printf("%s (str) ",strings[tok.data_index].c_str());
        } else if(tok.type == TOKEN_LITERAL_CHAR){
            printf("%s (char) ",strings[tok.data_index].c_str());
        } else if(tok.type == TOKEN_LITERAL_INTEGER) {
            printf("%d (int) ",integers[tok.data_index]);
        } else if(tok.type == TOKEN_LITERAL_DECIMAL) {
            printf("%f (float) ",floats[tok.data_index]);
        } else {
            printf("%s ",NAME_OF_TOKEN(tok.type));
        }
    }
    printf("\n");
}
std::string TokenStream::feed(int start, int end) {
    std::string out = "";
    for(int i=start;i<end;i++) {
        auto& tok = tokens[i];
        int len0 = out.length();
        if(tok.type < 256) {
            out += (char)tok.type;
        } else if(tok.type == TOKEN_ID){
            out += strings[tok.data_index];
        } else if(tok.type == TOKEN_LITERAL_STRING){
            out += "\"";
            auto& s = strings[tok.data_index];
            for(int j=0;j<s.size();j++) {
                char chr = s[j];
                if(chr == '\n') {
                    out += "\\n";
                } else {
                    out += chr;
                }
            }
            out += "\"";
        } else if(tok.type == TOKEN_LITERAL_CHAR){
            out += "'";
            auto& s = strings[tok.data_index];
            for(int j=0;j<s.size();j++) {
                char chr = s[j];
                if(chr == '\n') {
                    out += "\\n";
                } else {
                    out += chr;
                }
            }
            out += "'";
        } else if(tok.type == TOKEN_LITERAL_INTEGER) {
            out += std::to_string(integers[tok.data_index]);
        } else if(tok.type == TOKEN_LITERAL_DECIMAL) {
            out += std::to_string(floats[tok.data_index]);
        } else {
            out += NAME_OF_TOKEN(tok.type);
        }
        int len1 = out.length();

        if(i+1 < end) {
            auto& next_tok = tokens[i+1];
            for(int j=0;j<next_tok.line - tok.line;j++) {
                out += "\n";
            }
            if(next_tok.line == tok.line) {
                int spaces = next_tok.column - tok.column - (len1 - len0);
                for(int j=0;j<spaces;j++) {
                    out += " ";
                }
            }
        }
    }
    return out;
}
std::string TokenStream::getline(int index) {
    Token* base_tok = getToken(index, nullptr, nullptr, nullptr);
    int line = base_tok->line;
    
    int start = index;
    while(start - 1 >= 0) {
        auto tok = getToken(start - 1, nullptr, nullptr, nullptr);
        if(tok->line != line)
            break;
        start--;
    }
    
    int end = index;
    while(end + 1 < tokens.size()) {
        auto tok = getToken(end + 1, nullptr, nullptr, nullptr);
        if(tok->line != line)
            break;
        end++;
    }
    end++; // exclusive
    
    return feed(start, end);
}