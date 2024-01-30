#include "Parser.h"

/* TODO:
    A function for every statement type, function, struct, expression.
*/

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
int GetPrecedence(ASTExpression::Type type) {
    switch(type) {
        case ASTExpression::Type::MUL:
        case ASTExpression::Type::DIV:
            return 10;
        case ASTExpression::Type::ADD:
        case ASTExpression::Type::SUB:
            return 5;
        case ASTExpression::Type::AND:
        case ASTExpression::Type::OR:
            return -5;
    }
    return 0;
}
ASTExpression::Type CharToExprType(char chr) {
    switch(chr) {
        case '+': return ASTExpression::Type::ADD;
        case '-': return ASTExpression::Type::SUB;
        case '*': return ASTExpression::Type::MUL;
        case '/': return ASTExpression::Type::DIV;
    }
    return ASTExpression::INVALID;
}
ASTExpression* ParseContext::parseExpression() {
    std::vector<ASTExpression*> expressions;
    std::vector<ASTExpression::Type> operations;

    bool is_operator = false;
    std::string name="";
    int number = 0;
    while(true) {
        Token* token = gettok(&name, &number);
        Token* token2 = gettok(1);

        bool ending = false;
        // TODO: Handle eof
        if(is_operator) {
            ASTExpression::Type operationType = CharToExprType((char)token->type);
            if(operationType != ASTExpression::INVALID) {
                advance();
                operations.push_back(operationType);
            } else if (token->type == '&' && token2->type == '&') {
                advance();
                // && is okay but & & is also considered an AND operation. Is that okay?
                operations.push_back(ASTExpression::AND);
            } else if (token->type == '|' && token2->type == '|') {
                advance();
                operations.push_back(ASTExpression::OR);
            } else {
                ending = true; // no valid operation, end of expression
            }
        } else {
            if(token->type == TOKEN_ID) {
                advance();
                token = gettok();
                if(token->type == '(') {
                    advance();
                    // function call
                    
                    ASTExpression* expr_call = ast->createExpression(ASTExpression::FUNCTION_CALL);
                    expr_call->name = name;

                    while(true) {
                        token = gettok();
                        if(token->type == TOKEN_EOF) {
                            reporter->err(token,"Sudden end of file");
                            return nullptr;
                        }
                        if(token->type == ')') {
                            advance();
                            break;
                        }
                        ASTExpression* expr = parseExpression();
                        if(!expr)
                            return nullptr;

                        token = gettok();
                        if(token->type == ',') {
                            advance();
                            continue;
                        } else if(token->type == ')') {
                            continue;
                        } else {
                            reporter->err(token, "Expected closing parentheses");
                            return nullptr;
                        }

                        expr_call->arguments.push_back(expr);
                    }

                    expressions.push_back(expr_call);
                } else {
                    // identifier
                    ASTExpression* expr = ast->createExpression(ASTExpression::IDENTIFIER);
                    expr->name = name;
                    expressions.push_back(expr);
                }
            } else if(token->type == TOKEN_LITERAL_INTEGER) {
                advance();
                ASTExpression* expr = ast->createExpression(ASTExpression::LITERAL_INT);
                expr->literal_integer = number;
                expressions.push_back(expr);
            } else if(token->type == TOKEN_LITERAL_STRING) {
                advance();
                ASTExpression* expr = ast->createExpression(ASTExpression::LITERAL_STR);
                expr->literal_string = name;
                expressions.push_back(expr);
            } else if(token->type == '(') {
                advance();
                ASTExpression* expr = parseExpression();
                if(!expr)
                    return nullptr;

                token = gettok();
                if(token->type == ')') {
                    advance();
                } else {
                    reporter->err(token, "Expected closing parentheses");
                    return nullptr;
                }

                expressions.push_back(expr);
            } else {
                reporter->err(token, "Invalid token");
                return nullptr;
            }
        }

        if(is_operator) {
            if(!ending) {
                while(true) {
                    if(operations.size() >= 2) {
                        auto left_op = operations[operations.size()-2];
                        auto right_op = operations[operations.size()-1];
                        if(GetPrecedence(left_op) >= GetPrecedence(right_op)) {
                            operations.erase(operations.begin() + operations.size() - 2);

                            ASTExpression* expr = ast->createExpression(left_op);
                            expr->right = expressions.back();
                            expressions.pop_back();
                            expr->left = expressions.back();
                            expressions.pop_back();
                            
                            expressions.push_back(expr);
                        }
                    }

                    break;
                }
            } else {
                while(operations.size() > 0) {
                    auto op = operations.back();
                    operations.pop_back();

                    ASTExpression* expr = ast->createExpression(op);
                    expr->right = expressions.back();
                    expressions.pop_back();
                    expr->left = expressions.back();
                    expressions.pop_back();
                    
                    expressions.push_back(expr);
                }
            }
            if(ending) {
                Assert(expressions.size() == 1 && operations.size() == 0);
                return expressions.back();
            }
        }
        is_operator = !is_operator;
    }
    return nullptr;
}
/*################
    STATEMENTS
##################*/
ASTStatement* ParseContext::parseIf() {
    // if expression { } else expression
    
    ASTStatement* out = ast->createStatement(ASTStatement::IF);

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
    ASTBody* out = ast->createBody();

    Token* token = gettok();
    if(token->type != '{') {
        reporter->err(token, "Should be a curly brace");
        return nullptr;
    }
    advance();

    while(true) {
        token = gettok();
        if(token->type == TOKEN_EOF)
            break;
        if(token->type == '}') {
            advance();
            break;
        }

        ASTStatement* stmt = nullptr;
        switch(token->type) {
            case TOKEN_IF: {
                stmt = parseIf();
                break;
            }
            case TOKEN_WHILE: {
                // stmt = parseWhile();
                break;
            }
            default: {
                ASTExpression* expr = parseExpression();
                if(!expr)
                    return nullptr;
                stmt = ast->createStatement(ASTStatement::EXPRESSION);
                stmt->expression = expr;

                // TODO: check semi colon
                break;
            }
        }
        if(!stmt)
            return nullptr;
        out->statements.push_back(stmt);
    }
    return out;
}
/*################
    FUNCTIONS AND STRUCTURES
##################*/
ASTFunction* ParseContext::parseFunction() {
    Token* token = gettok();
    Assert(token->type == TOKEN_FUNCTION);
    advance();

    ASTFunction* out = ast->createFunction();

    std::string name="";
    token = gettok(&name);
    if(token->type != TOKEN_ID) {
        reporter->err(token, "Should be an identifier");
        return nullptr;
    }
    advance();
    out->name = name;

    token = gettok();
    if(token->type != '(') {
        reporter->err(token, "Should be a opening parenthesses");
        return nullptr;
    }
    advance();

    while(true) {
        token = gettok();
        if(token->type == TOKEN_EOF) {
            reporter->err(token, "Sudden end of file");
            break;
        }
        if(token->type == ')') {
            advance();
            break;
        }
        std::string param_name="";
        token = gettok(&param_name);
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
        ASTFunction::Parameter param{};
        param.name = param_name;
        param.type = type;
        out->parameters.push_back(param);

        token = gettok();
        if(token->type == ',') {
            advance();
        } else if(token->type == ')') {
            continue; // break/exit logic handled at start of loop
        } else {
            reporter->err(token, "Bad token?");
            return nullptr;
        }
    }

    token = gettok();
    if(token->type == ':') {
        advance();
        std::string return_type = parseType();
         if(return_type.empty()) {
            reporter->err(token, "Invalid syntax for type");
            return nullptr;
        }
        out->returnType = return_type;
    }

    ASTBody* body = parseBody();
    if(!body)
        return nullptr;
    out->body = body;
    return out;
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