#include "Lexer.h"

Tokens* lex_file(std::string path) {
    std::ifstream file(path, std::ifstream::binary);
    if(!file.is_open())
        return nullptr;

    file.seekg(0, file.end);
    int filesize = file.tellg();
    file.seekg(0, file.beg);

    char* text = (char*)malloc(filesize);
    file.read(text, filesize);
    file.close();

    printf("size: %d\n", filesize);

    Tokens* tokens = new Tokens();
    
    bool is_id = false;

    int index=0;
    while(index<filesize) {
        char chr = text[index];
        index++;

        if((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z')) {
            
            is_id = true;
            continue;
        }

        if(chr == ' ' || chr == '\n' || chr == '\r' || chr == '\t')
            continue;

        if(is_id) {
            tokens->add(TOKEN_ID);
            is_id = false;
        }

        tokens->add((TokenType)chr);

        // printf("%c",chr);
    }

    return tokens;
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

void Tokens::print() {
    for(auto type : tokenTypes) {
        if(type < 256) {
            printf("%c ",(char)type);
        } else {
            printf("%s ",token_names[type - 256]);
        }
    }
    printf("\n");
}