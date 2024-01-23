#pragma once

enum TokenType {

    // 0-256
    // 48 '0'
    // 65 'A'
    
    TOKEN_ID = 256,
    TOKEN_INTEGER,
    TOKEN_STRUCT,
    TOKEN_FUNCTION,
    TOKEN_WHILE,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_IF,
    TOKEN_ELSE,
};
struct Token {
    TokenType type;
    int data_index;
};
struct TokenStream {
    std::vector<Token> tokens;
    std::vector<std::string> strings;
    std::vector<int> integers;

    void add(TokenType t) {
        tokens.push_back({t});
    }
    void add_id(const std::string& id) {
        int index = strings.size();
        strings.push_back(id);
        tokens.push_back({TOKEN_ID, index});
    }
    void add_int(int number) {
        int index = integers.size();
        integers.push_back(number);
        tokens.push_back({TOKEN_INTEGER, index});
    }

    void print();
};

TokenStream* lex_file(const std::string& path);

#define NAME_OF_TOKEN(TOKEN_TYPE) token_names[TOKEN_TYPE-256]
extern const char* token_names[];