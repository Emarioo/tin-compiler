#pragma once

#include "Util.h"
#include "Lexer.h"

struct ASTExpression;
struct ASTStatement;
struct ASTBody;
struct ASTFunction;
struct ASTStructure;

struct ASTExpression {
    enum Type {
        INVALID,
        IDENTIFIER,
        FUNCTION_CALL,
        LITERAL_INT,
        LITERAL_STR,
        ADD,
        SUB,
        DIV,
        MUL,
        AND,
        OR,
        NOT,
        EQUAL,
        NOT_EQUAL,
        LESS,
        GREATER,
        LESS_EQUAL,
        GREATER_EQUAL,
    };
    Type type;

    std::string literal_string;
    int literal_integer;
    
    std::string name; // used with IDENTIFIER, FUNCTION_CALL
    
    ASTExpression* left;
    ASTExpression* right;
    
    std::vector<ASTExpression*> arguments; // used with FUNCTION_CALL
    
    Token* location;
};

struct ASTStatement {
    enum Type {
        INVALID,
        VAR_DECLARATION, // variable declaration
        EXPRESSION,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
        RETURN,
    };
    Type type;
    
    std::string declaration_type; // used by VAR_DECLARATION
    std::string declaration_name; // used by VAR_DECLARATION
    
    ASTExpression* expression; // used by RETURN, EXPRESSION, IF, WHILE, VAR_DECLARATION
    ASTBody* body; // used by IF, WHILE
    ASTBody* elseBody; // used by IF sometimes
    
    Token* location;
};

struct ASTBody {
    std::vector<ASTStatement*> statements;
};

struct ASTFunction {
    std::string name;
    
    struct Parameter {
        std::string name;
        std::string type;
    };
    std::vector<Parameter> parameters;
    
    std::string returnType;

    ASTBody* body;
};
struct ASTStructure {
    std::string name;
    
    struct Member {
        std::string name;
        std::string type;
    };
    std::vector<Member> members;
    
    // size of struct?
};

struct AST {
    std::vector<ASTFunction*> functions;
    std::vector<ASTStructure*> structures;
    // TODO: Global variables
    
    ASTExpression* createExpression(ASTExpression::Type type);
    ASTStatement* createStatement(ASTStatement::Type type);
    ASTBody* createBody();
    ASTFunction* createFunction();
    ASTStructure* createStructure();

    void print(ASTExpression* expr, int depth = 0);
    void print(ASTBody* body, int depth = 0);
    void print();
};