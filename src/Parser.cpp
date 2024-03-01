#include "Parser.h"

/* TODO:
    A function for every statement type, function, struct, expression.
*/

#define LOCATION log_color(GRAY); printf("%s:%d\n",__FILE__,__LINE__); log_color(NO_COLOR);
#define REPORT(L, ...) LOCATION reporter->err(L, __VA_ARGS__)

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
        advance();
    }
    return out;
}
int GetPrecedence(ASTExpression::Kind kind) {
    switch(kind) {
        case ASTExpression::Kind::MEMBER:
        case ASTExpression::Kind::INDEX:
            return 15;
        case ASTExpression::Kind::MUL:
        case ASTExpression::Kind::DIV:
            return 10;
        case ASTExpression::Kind::ADD:
        case ASTExpression::Kind::SUB:
            return 5;
        case ASTExpression::Kind::EQUAL:
        case ASTExpression::Kind::NOT_EQUAL:
        case ASTExpression::Kind::LESS:
        case ASTExpression::Kind::LESS_EQUAL:
        case ASTExpression::Kind::GREATER:
        case ASTExpression::Kind::GREATER_EQUAL:
            return -4;
        case ASTExpression::Kind::AND:
        case ASTExpression::Kind::OR:
            return -5;
        case ASTExpression::Kind::ASSIGN:
            return -10;
        default: Assert(false);
    }
    return 0;
}
ASTExpression::Kind CharToExprType(char chr) {
    switch(chr) {
        case '+': return ASTExpression::Kind::ADD;
        case '-': return ASTExpression::Kind::SUB;
        case '*': return ASTExpression::Kind::MUL;
        case '/': return ASTExpression::Kind::DIV;
    }
    return ASTExpression::INVALID;
}
ASTExpression* ParseContext::parseExpression() {
    ZoneScopedC(tracy::Color::Red2);
    
    std::vector<ASTExpression*> expressions;
    std::vector<ASTExpression::Kind> operations;
    std::vector<std::string> cast_strings;
    std::vector<SourceLocation> saved_locations;

    bool is_operator = false;
    std::string name="";
    int number = 0;
    float decimal;
    while(true) {
        Token* token = gettok(&name, &number, &decimal);
        Token* token2 = gettok(1);

        bool ending = false;
        // TODO: Handle eof
        if(is_operator) {
            ASTExpression::Kind operationType = CharToExprType((char)token->type);
            if(token->type == '+' && token2->type == '+') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::POST_INCREMENT);
                is_operator = !is_operator;
            } else if(token->type == '-' && token2->type == '-') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::POST_DECREMENT);
                is_operator = !is_operator;
            } else if(operationType != ASTExpression::INVALID) {
                saved_locations.push_back(getloc());
                advance();
                operations.push_back(operationType);
            } else if (token->type == '&' && token2->type == '&') {
                saved_locations.push_back(getloc());
                advance(2);
                // && is okay but & & is also considered an AND operation. Is that okay?
                operations.push_back(ASTExpression::AND);
            } else if (token->type == '|' && token2->type == '|') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::OR);
            } else if (token->type == '=' && token2->type == '=') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::EQUAL);
            } else if (token->type == '!' && token2->type == '=') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::NOT_EQUAL);
            } else if (token->type == '<' && token2->type == '=') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::LESS_EQUAL);
            } else if (token->type == '>' && token2->type == '=') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::GREATER_EQUAL);
            } else if (token->type == '<') {
                saved_locations.push_back(getloc());
                advance();
                operations.push_back(ASTExpression::LESS);
            } else if (token->type == '>') {
                saved_locations.push_back(getloc());
                advance();
                operations.push_back(ASTExpression::GREATER);
            } else if (token->type == '|' && token2->type == '|') {
                saved_locations.push_back(getloc());
                advance(2);
                operations.push_back(ASTExpression::OR);
            } else if (token->type == '.') {
                advance();
                // operations.push_back(ASTExpression::MEMBER);
                ASTExpression* expr = ast->createExpression(ASTExpression::MEMBER);

                expr->location = getloc();
                token = gettok(&expr->name);
                if(token->type != TOKEN_ID) {
                    REPORT(token, "Member access operator expects and identifier after '.'.");
                    return nullptr;
                }
                advance();
                // can we just pop expression like this?
                expr->left = expressions.back();
                expressions.pop_back();

                expressions.push_back(expr);
                is_operator = !is_operator;
            } else if (token->type == '=') {
                saved_locations.push_back(getloc());
                advance();
                operations.push_back(ASTExpression::ASSIGN);
            } else if (token->type == '[') {
                saved_locations.push_back(getloc());
                advance();
                operations.push_back(ASTExpression::INDEX);
                
                auto index_expr = parseExpression();
                
                token = gettok();
                if(token->type != ']') {
                    REPORT(token,"Expected closing bracket for index operator.");
                    return nullptr;
                }
                advance();
                
                expressions.push_back(index_expr);
                is_operator = !is_operator;
            } else {
                ending = true; // no valid operation, end of expression
            }
        } else {
            if(token->type == '!') {
                saved_locations.push_back(getloc());
                operations.push_back(ASTExpression::NOT);
                advance();
                continue;
            }
            if(token->type == '&') {
                saved_locations.push_back(getloc());
                operations.push_back(ASTExpression::REFER);
                advance();
                continue;
            }
            if(token->type == '*') {
                saved_locations.push_back(getloc());
                operations.push_back(ASTExpression::DEREF);
                advance();
                continue;
            }
            if(token->type == TOKEN_CAST) {
                saved_locations.push_back(getloc());
                operations.push_back(ASTExpression::CAST);
                advance();
                
                std::string type = parseType();
                cast_strings.push_back(type);
                continue;
            }
            if(token->type == '+' && token2->type == '+') {
                saved_locations.push_back(getloc());
                operations.push_back(ASTExpression::PRE_INCREMENT);
                advance(2);
                continue;
            }
            if(token->type == '-' && token2->type == '-') {
                saved_locations.push_back(getloc());
                operations.push_back(ASTExpression::PRE_DECREMENT);
                advance(2);
                continue;
            }
            
            if(token->type == TOKEN_ID) {
                auto location = getloc();
                advance();
                token = gettok();
                if(token->type == '(') {
                    advance();
                    // function call
                    
                    ASTExpression* expr_call = ast->createExpression(ASTExpression::FUNCTION_CALL);
                    expr_call->name = name;
                    expr_call->location = location;

                    while(true) {
                        token = gettok();
                        if(token->type == TOKEN_EOF) {
                            REPORT(token,"Sudden end of file.");
                            return nullptr;
                        }
                        if(token->type == ')') {
                            advance();
                            break;
                        }
                        ASTExpression* expr = parseExpression();
                        if(!expr)
                            return nullptr;

                        expr_call->arguments.push_back(expr);

                        token = gettok();
                        if(token->type == ',') {
                            advance();
                            continue;
                        } else if(token->type == ')') {
                            continue;
                        } else {
                            REPORT(token, "Expected closing parentheses.");
                            return nullptr;
                        }
                    }

                    expressions.push_back(expr_call);
                } else {
                    // identifier
                    ASTExpression* expr = ast->createExpression(ASTExpression::IDENTIFIER);
                    expr->name = name;
                    expr->location = location;
                    expressions.push_back(expr);
                }
            } else if(token->type == TOKEN_TRUE) {
                ASTExpression* expr = ast->createExpression(ASTExpression::TRUE);
                expr->location = getloc();
                advance();
                expressions.push_back(expr);
            } else if(token->type == TOKEN_FALSE) {
                ASTExpression* expr = ast->createExpression(ASTExpression::FALSE);
                expr->location = getloc();
                advance();
                expressions.push_back(expr);
            } else if(token->type == TOKEN_NULL) {
                ASTExpression* expr = ast->createExpression(ASTExpression::LITERAL_NULL);
                expr->location = getloc();
                advance();
                expressions.push_back(expr);
            } else if(token->type == TOKEN_LITERAL_INTEGER) {
                ASTExpression* expr = ast->createExpression(ASTExpression::LITERAL_INT);
                expr->location = getloc();
                advance();
                expr->literal_integer = number;
                expressions.push_back(expr);
            } else if(token->type == TOKEN_LITERAL_FLOAT) {
                ASTExpression* expr = ast->createExpression(ASTExpression::LITERAL_FLOAT);
                expr->location = getloc();
                advance();
                expr->literal_float = decimal;
                expressions.push_back(expr);
            } else if(token->type == TOKEN_LITERAL_STRING) {
                ASTExpression* expr = ast->createExpression(ASTExpression::LITERAL_STR);
                expr->location = getloc();
                advance();
                expr->literal_string = name;
                expressions.push_back(expr);
            } else if(token->type == TOKEN_SIZEOF) {
                ASTExpression* expr = ast->createExpression(ASTExpression::SIZEOF);
                expr->location = getloc();
                advance();
                std::string type = parseType();
                expr->name = type;
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
                    REPORT(token, "Expected closing parentheses.");
                    return nullptr;
                }

                expressions.push_back(expr);
            } else {
                REPORT(token, "Invalid token.");
                return nullptr;
            }
        }

        if(!is_operator) {
            // fix unary operators (like NOT)
            while(operations.size() > 0) {
                auto op = operations.back();
                if(op != ASTExpression::NOT && op != ASTExpression::REFER && op != ASTExpression::DEREF && op != ASTExpression::CAST && op != ASTExpression::PRE_INCREMENT && op != ASTExpression::PRE_DECREMENT && op != ASTExpression::POST_INCREMENT && op != ASTExpression::POST_DECREMENT) {
                    break;
                }
                operations.pop_back();
                ASTExpression* expr = ast->createExpression(op);
                expr->left = expressions.back();
                expressions.pop_back();
                expressions.push_back(expr);
                
                if(op == ASTExpression::CAST) {
                    expr->name = cast_strings.back();
                    cast_strings.pop_back();
                }
                expr->location = saved_locations.back();
                saved_locations.pop_back();
            }
        } else {
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

                            // if(left_op == ASTExpression::ASSIGN || left_op == ASTExpression::INDEX) {
                                expr->location = saved_locations.back();
                                saved_locations.pop_back();
                            // }
                            
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

                    // if(op == ASTExpression::ASSIGN || left_op == ASTExpression::INDEX) {
                        expr->location = saved_locations.back();
                        saved_locations.pop_back();
                    // }
                    
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
ASTStatement * ParseContext::parseWhile(){
    ASTStatement* out = ast->createStatement(ASTStatement::Kind::WHILE);
    
    // out->location = getloc();
    Token* token = gettok();

    if (token->type != TOKEN_WHILE){
        return nullptr;
    }
    advance();
    ASTExpression * expr = parseExpression();
    if (!expr){
        return nullptr;
        // printf("Error in Expresion!");
    }
    out->expression = expr;

    ASTBody* body = parseBody();
    if(!body)
        return nullptr;
        
    out->body = body;
    return out;
}
ASTStatement* ParseContext::parseIf() {
    // if expression { } else expression
    
    ASTStatement* out = ast->createStatement(ASTStatement::Kind::IF);
    
    // out->location = getloc();
    
    Token* token = gettok();
    Assert(token->type == TOKEN_IF);
    advance();

    ASTExpression* expr = parseExpression();
    if(!expr) {
        return nullptr;
    }
    out->expression = expr;

    out->body = parseBody();
    if(!out->body) return nullptr;
    token = gettok();
    if(token->type != TOKEN_ELSE) {
        return out;
    }
    advance();
    
    // TODO: Don't handle else if recursively
    token = gettok();
    if(token->type == TOKEN_IF) {
        ASTBody* body = ast->createBodyWithSharedScope(current_scopeId);
        
        ASTStatement* stmt = parseIf();
        if(!stmt)
            return nullptr;
        body->statements.push_back(stmt);
        
        out->elseBody = body;
    } else {
        out->elseBody = parseBody();
    }

    return out;
}
ASTStatement* ParseContext::parseVarDeclaration(bool is_global, bool is_constant) {
    Assert(!is_global || !is_constant);
    ASTStatement* out = ast->createStatement(is_global ? ASTStatement::GLOBAL_DECLARATION : 
        is_constant ? ASTStatement::CONST_DECLARATION : ASTStatement::VAR_DECLARATION);

    out->location = getloc();

    std::string var_name;
    Token* token = gettok(&var_name);
    Assert(token->type == TOKEN_ID);
    advance();
    
    out->declaration_name = var_name;
    // out->location = token; // for error messages in generator
    
    token = gettok();
    if(token->type != ':') {
        REPORT(token, "Expected ':' for variable declarations.");
        return nullptr;
    }
    advance();
        
    std::string type = parseType();
    if(type.empty()) {
        REPORT(token, "Invalid syntax for type.");
        return nullptr;
    }
    out->declaration_type = type;
    
    token = gettok();
    if(token->type != '=') {
        if(token->type == ';') {
            return out;
        }
        REPORT(token, "Expected '=' or semi-colon after variable name and type.");
        return nullptr;
    }
    advance();

    ASTExpression* expr = parseExpression();
    if(!expr) {
        return nullptr;
    }
    
    out->expression = expr;
    
    return out;
}

ASTBody* ParseContext::parseBody() {
    ZoneScopedC(tracy::Color::Red2);
    ASTBody* out = ast->createBody(current_scopeId);

    auto prev_scope = current_scopeId;
    current_scopeId = out->scopeId;
    defer {
        current_scopeId = prev_scope;
    };

    Token* token = gettok();
    if(token->type != '{') {
        REPORT(token, "Should be a curly brace.");
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
        auto loc = getloc();
        switch(token->type) {
            case TOKEN_IF: {
                stmt = parseIf();
            } break;
            case TOKEN_WHILE: {
                stmt = parseWhile();
            } break;
            case TOKEN_RETURN: {
                advance();

                stmt = ast->createStatement(ASTStatement::RETURN);

                token = gettok();
                if(token->type != ';') {
                    stmt->expression = parseExpression();
                    if(!stmt->expression)
                        return nullptr;
                }

                token = gettok();
                if(token->type != ';') {
                    REPORT(token, "You forgot a semi-colon after the statement.");
                    return nullptr;
                }
                advance();
            } break;
            case TOKEN_CONST: {
                advance();
                loc = getloc();
                stmt = parseVarDeclaration(false, true);
                if(!stmt)
                    return nullptr;
                    
                token = gettok();
                if(token->type != ';') {
                    REPORT(token, "You forgot a semi-colon after the statement.");
                    return nullptr;
                }
                advance();
            } break;
            case TOKEN_GLOBAL: {
                REPORT(token, "Global declarations are not allowed within functions. Only allowed in the global scope.");
                return nullptr;
            } break;
            default: {
                auto token2 = gettok(1);
                // v.x.y = 23; is an assignment, checkin for id, colon, and equals tokens won't work
                if(token->type == TOKEN_ID && token2->type == ':')  {
                    stmt = parseVarDeclaration(false, false);
                    if(!stmt)
                        return nullptr;
                        
                    token = gettok();
                    if(token->type != ';') {
                        REPORT(token, "You forgot a semi-colon after the statement.");
                        return nullptr;
                    }
                    advance();
                } else {
                    token = gettok();
                    ASTExpression* expr = parseExpression();
                    if(!expr)
                        return nullptr;
                    stmt = ast->createStatement(ASTStatement::EXPRESSION);
                    stmt->expression = expr;
                    
                    token = gettok();
                    if(token->type != ';') {
                        REPORT(token, "You forgot a semi-colon after the statement.");
                        return nullptr;
                    }
                    advance();
                }
            } break;
        }
        if(!stmt)
            return nullptr;
        stmt->location = loc;
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

    out->location = getloc();
    std::string name="";
    token = gettok(&name);
    if(token->type != TOKEN_ID) {
        REPORT(token, "Should be an identifier.");
        return nullptr;
    }
    advance();
    out->name = name;
    
    out->origin_stream = stream;

    token = gettok();
    if(token->type != '(') {
        REPORT(token, "Should be a opening parenthesses.");
        return nullptr;
    }
    advance();

    while(true) {
        token = gettok();
        if(token->type == TOKEN_EOF) {
            REPORT(token, "Sudden end of file.");
            break;
        }
        if(token->type == ')') {
            advance();
            break;
        }
        std::string param_name="";
        token = gettok(&param_name);
        if(token->type != TOKEN_ID) {
            REPORT(token, "Should be an identifier.");
            return nullptr;
        }
        advance();

        token = gettok();
        if(token->type != ':') {
            REPORT(token, "Should be a colon.");
            return nullptr;
        }
        advance();
        
        token = gettok(); // needed when reporting error
        std::string type = parseType();
        if(type.empty()) {
            REPORT(token, "Invalid syntax for type.");
            return nullptr;
        }
        ASTFunction::Parameter param{};
        param.name = param_name;
        param.typeString = type;
        out->parameters.push_back(param);

        token = gettok();
        if(token->type == ',') {
            advance();
        } else if(token->type == ')') {
            continue; // break/exit logic handled at start of loop
        } else {
            REPORT(token, "Bad token?");
            return nullptr;
        }
    }

    token = gettok();
    if(token->type == ':') {
        advance();
        std::string return_type = parseType();
         if(return_type.empty()) {
            REPORT(token, "Invalid syntax for type.");
            return nullptr;
        }
        out->return_typeString = return_type;
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

    out->location = getloc();
    std::string name="";
    token = gettok(&name);
    if(token->type != TOKEN_ID) {
        REPORT(token, "Should be an identifier.");
        return nullptr;
    }
    advance();
    out->name = name;

    token = gettok();
    if(token->type != '{') {
        REPORT(token, "Should be a curly brace.");
        return nullptr;
    }
    advance();

    while(true) {
        token = gettok();
        if(token->type == TOKEN_EOF) {
            REPORT(token, "Sudden end of file.");
            break;
        }
        if(token->type == '}') {
            advance();
            break;
        }
        std::string member_name="";
        token = gettok(&member_name);
        auto name_loc = getloc();
        if(token->type != TOKEN_ID) {
            REPORT(token, "Should be an identifier.");
            return nullptr;
        }
        advance();

        token = gettok();
        if(token->type != ':') {
            REPORT(token, "Should be a colon.");
            return nullptr;
        }
        advance();
        
        token = gettok(); // needed when reporting error
        std::string type = parseType();
        if(type.empty()) {
            REPORT(token, "Invalid syntax for type.");
            return nullptr;
        }
        ASTStructure::Member member{};
        member.name = member_name;
        member.typeString = type;
        member.location = name_loc;
        out->members.push_back(member);

        token = gettok();
        if(token->type == ',') {
            advance();
        } else if(token->type == '}') {
            continue; // break/exit logic handled at start of loop
        } else {
            REPORT(token, "Bad token?");
            return nullptr;
        }
    }
    return out;
}

AST::Import* ParseTokenStream(TokenStream* stream, AST::Import* imp, AST* ast, Reporter* reporter) {
    ZoneScopedC(tracy::Color::Red2);
    ParseContext context{};
    context.reporter = reporter;
    context.stream = stream;
    context.ast = ast;
    
    // context.current_scopeId = AST::GLOBAL_SCOPE;
    auto body = ast->createBody(AST::GLOBAL_SCOPE);
    context.current_scopeId = body->scopeId;
    
    if(!imp) {
        imp = ast->createImport(stream->path);
    }
    imp->body = body; // TODO: Mutex, compiler checks body and modifies shared_scopes

    imp->stream = stream;

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
                else {
                    body->add(ast_struct);
                    // context.ast->structures.push_back(ast_struct);
                }
            } break;
            case TOKEN_FUNCTION: {
                auto ast_func = context.parseFunction();
                if(!ast_func)
                    running = false; // stop running, parsing failed
                else {
                    body->add(ast_func);
                    // context.ast->functions.push_back(ast_func);
                }
            } break;
            case TOKEN_GLOBAL:
            case TOKEN_CONST: {
                context.advance();
                
                ASTStatement* stmt = context.parseVarDeclaration(token->type == TOKEN_GLOBAL, token->type == TOKEN_CONST);
                if(!stmt)
                    return nullptr;
                    
                token = context.gettok();
                if(token->type != ';') {
                    REPORT(token, "You forgot a semi-colon after the statement.");
                    return nullptr;
                }
                context.advance();
                
                body->statements.push_back(stmt);
            } break;
            case TOKEN_IMPORT: {
                context.advance();
                
                std::string path{};
                auto token = context.gettok(&path);
                if(token->type == TOKEN_LITERAL_STRING) {
                    context.advance();
                    
                    bool found = false;
                    for(auto& d : imp->dependencies) {
                        if(d == path) {
                            found = true;
                            break; // ignore duplicates
                        }
                    }
                    if(!found)
                        imp->dependencies.push_back(path);
                } else {
                    REPORT(token, "A string literal is expected after 'import'.");
                    return imp;
                }
            } break;
            default: {
                REPORT(token, "Unexpected token.");
                return imp;
            } break;
        }
    }
    return imp;
}

#undef LOCATION
#undef REPORT