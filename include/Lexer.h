#pragma once

#include "Util.h"

enum TokenType {

    // 0-256
    // 48 '0'
    // 65 'A'
    
    TOKEN_EOF = 256,
    TOKEN_ID,
    TOKEN_LITERAL_INTEGER,
    TOKEN_LITERAL_STRING,
    TOKEN_STRUCT,
    TOKEN_FUNCTION,
    TOKEN_WHILE,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_GLOBAL,
    TOKEN_INCLUDE,
};
struct Token {
    TokenType type;
    int data_index;

    int line;
    int column;
    std::string* file;

    char debug_type;
};
struct TokenStream {
    static void Destroy(TokenStream* stream);

    std::string path;
    
    std::vector<Token> tokens;
    std::vector<std::string> strings;
    std::vector<int> integers;

    Token* getToken(int index, std::string* str, int* num) {
        static Token eof{TOKEN_EOF};
        if(tokens.size() <= index)
            return &eof;

        if(str && (tokens[index].type == TOKEN_ID || tokens[index].type == TOKEN_LITERAL_STRING))
            *str = strings[tokens[index].data_index];
        else if(num && tokens[index].type == TOKEN_LITERAL_INTEGER) 
            *num = integers[tokens[index].data_index];
        return &tokens[index];
    }

    void add(TokenType t, int line, int column) {
        tokens.push_back({t});
        auto& tok = tokens.back();
        tok.line = line;
        tok.column = column;
        tok.file = &path;
        if(t < 256)
            tok.debug_type = (char)t;
        else
            tok.debug_type = 0;
    }
    void add_id(const std::string& id, int line, int column) {
        int index = strings.size();
        strings.push_back(id);
        tokens.push_back({TOKEN_ID, index});

        auto& tok = tokens.back();
        tok.line = line;
        tok.column = column;
        tok.file = &path;
    }
    void add_int(int number, int line, int column) {
        int index = integers.size();
        integers.push_back(number);
        tokens.push_back({TOKEN_LITERAL_INTEGER, index});

        auto& tok = tokens.back();
        tok.line = line;
        tok.column = column;
        tok.file = &path;
    }
    void add_string(const std::string& str, int line, int column) {
        int index = strings.size();
        strings.push_back(str);
        tokens.push_back({TOKEN_LITERAL_STRING, index});

        auto& tok = tokens.back();
        tok.line = line;
        tok.column = column;
        tok.file = &path;
    }

    void print();
};

TokenStream* lex_file(const std::string& path);

#define NAME_OF_TOKEN(TOKEN_TYPE) token_names[TOKEN_TYPE-256]
extern const char* token_names[];