#include "Parser.h"

/* TODO:
    A function for every statement type, function, struct, expression.
*/



ASTStatement * ParseContext::parseWhile(){
    // while expr {}

    ASTStatement* out = ast->createStatement(ASTStatement::Type::WHILE);
    
    Token* token = gettok();

    if (token->type != TOKEN_WHILE){
        return nullptr;
    }
    advance();
    ASTExpression * expr= parseExpr();
    if (!expr){
        printf("Error in Expresion!");
    }
    out->expression= expr;

    token= gettok();



}

ASTStatement* ParseContext::parseIf() {
    // if expression { } else expression
    
    ASTStatement* out = ast->createStatement(ASTStatement::Type::IF);

    Token* token = gettok();
    if(token->type != TOKEN_IF) {
        return nullptr;
    }

    advance();

    ASTExpression* expr = parseExpression();
    if(!expr) {
        // error
    }

    out->expression = expr;

    token = gettok();

    out->body = parseBody();

    token = gettok();
    if(token->type != TOKEN_ELSE) {
        return out;
    }

    out->elseBody = parseBody();

    return out;
}

void ParseTokenStream(TokenStream* stream) {
    // read token by token possible more depending
    // on how hard the EBNF is to parse

    ParseContext context{};


}