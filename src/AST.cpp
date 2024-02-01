#include "AST.h"

ASTExpression* AST::createExpression(ASTExpression::Type type) {
    auto ptr = new ASTExpression();
    ptr->type = type;
    return ptr;
}
ASTStatement* AST::createStatement(ASTStatement::Type type) {
    auto ptr = new ASTStatement();
    ptr->type = type;
    return ptr;
}
ASTBody* AST::createBody() {
    return new ASTBody();
}
ASTFunction* AST::createFunction() {
    return new ASTFunction();
}
ASTStructure* AST::createStructure() {
    return new ASTStructure();
}

const char* expr_type_table[]{
    "invalid", // INVALID
    "id", // IDENTIFIER
    "call", // FUNCTION_CALL
    "lit_int", // LITERAL_INT
    "lit_str", // LITERAL_STR
    "add", // ADD
    "sub", // SUB
    "div", // DIV
    "mul", // MUL
    "and", // AND
    "or",  // OR
    "not", // NOT
};
void AST::print(ASTExpression* expr, int depth) {
    Assert(expr);
    for(int i=0;i<depth;i++) printf(" ");
    if(expr->type == ASTExpression::FUNCTION_CALL) {
        printf("%s()\n",expr->name.c_str());
        for(const auto& arg : expr->arguments) {
            print(arg, depth + 1);
        }
    } else if(expr->type == ASTExpression::IDENTIFIER) {
        printf("%s\n",expr->name.c_str());
    } else if(expr->type == ASTExpression::LITERAL_INT) {
        printf("%d\n",expr->literal_integer);
    } else if(expr->type == ASTExpression::LITERAL_STR) {
        printf("%s\n",expr->literal_string.c_str());
    } else {
        printf("OP %s\n",expr_type_table[expr->type]);
        if(expr->left)
            print(expr->left, depth + 1);
        if(expr->right)
            print(expr->right, depth + 1);
    }
}
void AST::print(ASTBody* body, int depth) {
    Assert(body);
    for(const auto& s : body->statements) {
        switch(s->type){
            case ASTStatement::EXPRESSION: {
                print(s->expression, depth);
                break;
            }
            case ASTStatement::BREAK: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("break\n");
                break;
            }
            case ASTStatement::CONTINUE: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("continue\n");
                break;
            }
            case ASTStatement::RETURN: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("return\n");
                break;
            }
            case ASTStatement::IF: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("if\n");
                print(s->expression, depth+1);
                print(s->body, depth+1);
                if(s->elseBody) {
                    printf("else\n");
                    print(s->elseBody, depth+1);
                }
                break;
            }
            case ASTStatement::VAR_DECLARATION: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("var_decl %s: %s\n", s->declaration_name.c_str(), s->declaration_type.c_str());
                print(s->expression, depth+1);
                break;
            }
            default: Assert(false);
        }
    }
}
void AST::print() {
    log_color(Color::GOLD);
    printf("PRINTING AST:\n");
    log_color(Color::NO_COLOR);
    if(structures.size() == 0)
        printf(" No structures\n");
    if(functions.size() == 0)
        printf(" No functions\n");
    for(const auto& st : structures) {
        Assert(st);
        printf("%s {\n", st->name.c_str());
        for(const auto& member : st->members) {
            printf(" %s: %s,\n", member.name.c_str(), member.type.c_str());
        }
        printf("}\n");
    }
    for(const auto& fn : functions) {
        Assert(fn);
        printf("%s(", fn->name.c_str());
        for(const auto& param : fn->parameters) {
            printf("%s: %s, ", param.name.c_str(), param.type.c_str());
        }
        if(fn->returnType.empty())
            printf(")\n");
        else
            printf("): %s\n", fn->returnType.c_str());

        print(fn->body, 1);
    }
}