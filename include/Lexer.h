
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

struct Tokens {
    std::vector<TokenType> tokenTypes;


    void add(TokenType t) {
        tokenTypes.push_back(t);
    }

    void print();
};

Tokens* lex_file(std::string path);