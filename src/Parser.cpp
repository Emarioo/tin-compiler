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

std::string ParseContext::parseType() {
    std::string out = "";
    Token* token = gettok(&out);
    if(token->type != TOKEN_ID) {
        return "";
    }
    advance();

    while(true){
        token = gettok();
        if(token->type != '*')
            break;

        out+="*";
    }
    return out;
}
ASTExpression* ParseContext::parseExpression() {
    return nullptr;
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
ASTBody* ParseContext::parseBody() {
    return nullptr;
}
ASTFunction* ParseContext::parseFunction() {
    return nullptr;
}
ASTStructure* ParseContext::parseStruct() {
    Token* token = gettok();
    Assert(token->type == TOKEN_STRUCT);
    advance();

    ASTStructure* out = ast->createStructure();

    std::string name="";
    token = gettok(&name);
    if(token->type != TOKEN_ID) {
        reporter->err(token, "Should be an identifier");
        return nullptr;
    }
    advance();
    out->name = name;

    token = gettok();
    if(token->type != '{') {
        reporter->err(token, "Should be a curly brace");
        return nullptr;
    }
    advance();

    while(true) {
        token = gettok();
        if(token->type == TOKEN_EOF) {
            reporter->err(token, "Sudden end of file");
            break;
        }
        if(token->type == '}') {
            advance();
            break;
        }
        std::string member_name="";
        token = gettok(&member_name);
        if(token->type != TOKEN_ID) {
            reporter->err(token, "Should be an identifier");
            return nullptr;
        }
        advance();

        token = gettok();
        if(token->type != ':') {
            reporter->err(token, "Should be a colon");
            return nullptr;
        }
        advance();
        
        token = gettok(); // needed when reporting error
        std::string type = parseType();
        if(type.empty()) {
            reporter->err(token, "Invalid syntax for type");
            return nullptr;
        }
        ASTStructure::Member member{};
        member.name = member_name;
        member.type = type;
        out->members.push_back(member);

        token = gettok();
        if(token->type == ',') {
            advance();
        } else if(token->type == '}') {
            continue; // break/exit logic handled at start of loop
        } else {
            reporter->err(token, "Bad token?");
            return nullptr;
        }
    }
    return out;
}

void ParseTokenStream(TokenStream* stream, AST* ast) {
    ParseContext context{};
    context.reporter = new Reporter();
    context.stream = stream;
    context.ast = ast;

    bool running = true;
    while(running) {
        auto token = context.gettok();
        if(token->type == TOKEN_EOF)
            break;

        switch(token->type) {
            case TOKEN_STRUCT: {
                auto ast_struct = context.parseStruct();
                if(!ast_struct)
                    running = false; // stop running, parsing failed
                context.ast->structures.push_back(ast_struct);
                break;
            }
            case TOKEN_FUNCTION: {
                auto ast_func = context.parseFunction();
                if(!ast_func)
                    running = false; // stop running, parsing failed
                context.ast->functions.push_back(ast_func);
                break;
            }
            case TOKEN_GLOBAL: {
                
                break;
            }
            case TOKEN_INCLUDE: {
                
                break;
            }
        }
    }
}