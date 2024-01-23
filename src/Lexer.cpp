#include "Lexer.h"

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

    int data_start = 0;

    int index=0;
    while(index<filesize) {
        char chr = text[index];
        index++;

        if((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z')) {
            if(!is_id)
                data_start = index-1;
            is_id = true;
            continue;
        }

        if((chr >= '0' && chr <= '9')) {
            if(!is_num)
                data_start = index-1;
            is_num = true;
            continue;
        }

        if(is_num) {
            auto str = std::string(text + data_start, index-1 - data_start);
            int num = atoi(str.c_str());
            stream->add_int(num);
            is_num = false;
        }

        if(is_id) {
            auto str = std::string(text + data_start, index-1 - data_start);

            #define CASE(T) if(str == NAME_OF_TOKEN(T)) stream->add(T);
            
            CASE(TOKEN_STRUCT)
            else CASE(TOKEN_FUNCTION)
            else CASE(TOKEN_WHILE)
            else CASE(TOKEN_CONTINUE)
            else CASE(TOKEN_BREAK)
            else CASE(TOKEN_IF)
            else CASE(TOKEN_ELSE)
            else {

                stream->add_id(str);
            }
            is_id = false;
        }

        if(chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t')
            continue;

        stream->add((TokenType)chr);
    }

    free(text);

    return stream;
}

const char* token_names[] {
    "id", // TOKEN_ID,
    "integer", // TOKEN_INTEGER,
    "struct", // TOKEN_STRUCT,
    "fun", // TOKEN_FUNCTION,
    "while", // TOKEN_WHILE,
    "continue", // TOKEN_CONTINUE,
    "break", // TOKEN_BREAK,
    "if", // TOKEN_IF,
    "else", // TOKEN_ELSE,
};

void TokenStream::print() {
    for(auto tok : tokens) {
        if(tok.type < 256) {
            printf("%c ",(char)tok.type);
        } else if(tok.type == TOKEN_ID){
            printf("%s (id) ",strings[tok.data_index].c_str());
        } else if(tok.type == TOKEN_INTEGER) {
            printf("%d (int) ",integers[tok.data_index]);
        } else {
            printf("%s ",NAME_OF_TOKEN(tok.type));
        }
    }
    printf("\n");
}