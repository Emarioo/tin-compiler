#include "Lexer.h"

void TokenStream::Destroy(TokenStream* stream) {
    delete stream;
}
TokenStream* lex_file(const std::string& path) {
    std::ifstream file(path, std::ifstream::binary);
    if(!file.is_open())
        return nullptr;

    file.seekg(0, file.end);
    int filesize = file.tellg();
    file.seekg(0, file.beg);

    char* text = (char*)malloc(filesize);
    file.read(text, filesize);
    file.close();

    // printf("size: %d\n", filesize);

    TokenStream* stream = new TokenStream();
    
    bool is_id = false;
    bool is_num = false;
    bool is_str = false;

    int data_start = 0;

    int index=0;
    while(index<filesize) {
        char chr = text[index];
        index++;

        bool ending = index == filesize;
        bool delim = chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t';

        if(chr == '"') {
            if(!is_str) {
                is_str = true;
                data_start = index;
                continue;
            }
            int data_end = index-1 - data_start;
            auto str = std::string(text + data_start, data_end);
            stream->add_string(str);
            is_str = false;
            continue;
        }
        if(is_str)
            continue;

        if((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z')) {
            if(!is_id)
                data_start = index-1;
            is_id = true;
            if(!ending)
                continue;
        }

        if((chr >= '0' && chr <= '9')) {
            if(!is_num)
                data_start = index-1;
            is_num = true;
            if(!ending)
                continue;
        }

        if(is_num) {
            int data_end = index-1 - data_start;
            if(ending)
                data_end++;
            auto str = std::string(text + data_start, data_end);
            int num = atoi(str.c_str());
            stream->add_int(num);
            is_num = false;
        }

        if(is_id) {
            int data_end = index-1 - data_start;
            if(ending)
                data_end++;
            auto str = std::string(text + data_start, data_end);

            #define CASE(T) if(str == NAME_OF_TOKEN(T)) stream->add(T);
            
            CASE(TOKEN_STRUCT)
            else CASE(TOKEN_FUNCTION)
            else CASE(TOKEN_WHILE)
            else CASE(TOKEN_CONTINUE)
            else CASE(TOKEN_BREAK)
            else CASE(TOKEN_IF)
            else CASE(TOKEN_ELSE)
            else CASE(TOKEN_GLOBAL)
            else {

                stream->add_id(str);
            }
            is_id = false;
        }

        if(delim)
            continue;

        if(!ending)
            stream->add((TokenType)chr);
    }

    free(text);

    return stream;
}

const char* token_names[] {
    "id",        // TOKEN_ID,
    "lit_int",   // TOKEN_LITERAL_INTEGER,
    "lit_str",   // TOKEN_LITERAL_STRING,
    "struct",    // TOKEN_STRUCT,
    "fun",       // TOKEN_FUNCTION,
    "while",     // TOKEN_WHILE,
    "continue",  // TOKEN_CONTINUE,
    "break",     // TOKEN_BREAK,
    "if",        // TOKEN_IF,
    "else",      // TOKEN_ELSE,
    "global",      // TOKEN_ELSE,
};

void TokenStream::print() {
    for(auto tok : tokens) {
        if(tok.type < 256) {
            printf("%c ",(char)tok.type);
        } else if(tok.type == TOKEN_ID){
            printf("%s (id) ",strings[tok.data_index].c_str());
        } else if(tok.type == TOKEN_LITERAL_STRING){
            printf("%s (str) ",strings[tok.data_index].c_str());
        } else if(tok.type == TOKEN_LITERAL_INTEGER) {
            printf("%d (int) ",integers[tok.data_index]);
        } else {
            printf("%s ",NAME_OF_TOKEN(tok.type));
        }
    }
    printf("\n");
}