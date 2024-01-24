#pragma once

enum TokenType {

    // 0-256
    // 48 '0'
    // 65 'A'
    
    TOKEN_ID = 256,
    TOKEN_LITERAL_INTEGER,
    TOKEN_LITERAL_STRING,
    TOKEN_STRUCT,
    TOKEN_FUNCTION,
    TOKEN_WHILE,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_GLOBAL,
};
struct Token {
    TokenType type;
    int data_index;
};
struct TokenStream {
    std::vector<Token> tokens;
    std::vector<std::string> strings;
    std::vector<int> integers;

    Token* getToken(int index, std::string** str, int** num) {
        if(str && (tokens[index].type == TOKEN_ID || tokens[index].type == TOKEN_LITERAL_STRING))
            *str = &strings[tokens[index].data_index];
        else if(tokens[index].type == TOKEN_LITERAL_INTEGER) 
            *num = &integers[tokens[index].data_index];
        return &tokens[index];
    }

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
        tokens.push_back({TOKEN_LITERAL_INTEGER, index});
    }
    void add_string(const std::string& str) {
        int index = strings.size();
        strings.push_back(str);
        tokens.push_back({TOKEN_LITERAL_STRING, index});
    }

    void print();
};

TokenStream* lex_file(const std::string& path);

#define NAME_OF_TOKEN(TOKEN_TYPE) token_names[TOKEN_TYPE-256]
extern const char* token_names[];