#include "Parser.h"

/* TODO:
    A function for every statement type, function, struct, expression.
*/

ASTStatement* ParseContext::parseIf() {
    // if expression { } else expression
    
    ASTStatement* out = ast->createStatement();

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