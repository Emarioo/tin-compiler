#include "AST.h"
#include "Code.h"

AST::AST() {
    // global_scope = createScope(GLOBAL_SCOPE);
    global_body = createBody(GLOBAL_SCOPE);

    TypeInfo* type;
    #define ADD(T,S) type = createType(primitive_names[(int)T], GLOBAL_SCOPE); type->size = S;
    ADD(TYPE_VOID, 0)
    ADD(TYPE_INT, 4)
    ADD(TYPE_CHAR, 1)
    ADD(TYPE_BOOL, 1)
    ADD(TYPE_FLOAT, 4)
    #undef ADD
    Assert(type->typeId.index() == (int)TYPE_FLOAT);
    
    {
        auto f = createFunction();
        f->is_native = true;
        f->name = "printi";
        f->parameters.push_back({});
        f->parameters.back().name = "v";
        f->parameters.back().typeString = "int";
        f->piece_code_index = NATIVE_printi - NATIVE_MAX - 1;
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->name = "printf";
        f->parameters.push_back({});
        f->parameters.back().name = "v";
        f->parameters.back().typeString = "float";
        f->piece_code_index = NATIVE_printf - NATIVE_MAX - 1;
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->name = "malloc";
        f->parameters.push_back({});
        f->parameters.back().name = "size";
        f->parameters.back().typeString = "int";
        f->piece_code_index = NATIVE_malloc - NATIVE_MAX - 1;
        f->return_typeString = "void*";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->name = "mfree";
        f->parameters.push_back({});
        f->parameters.back().name = "ptr";
        f->parameters.back().typeString = "void*";
        f->piece_code_index = NATIVE_mfree - NATIVE_MAX - 1;
        global_body->add(f);
    }
}
ASTExpression* AST::createExpression(ASTExpression::Kind kind) {
    auto ptr = new ASTExpression(kind);
    return ptr;
}
ASTStatement* AST::createStatement(ASTStatement::Kind kind) {
    auto ptr = new ASTStatement(kind);
    return ptr;
}
ASTBody* AST::createBody(ScopeId parent) {
    auto ptr = new ASTBody();
    auto scope = createScope(parent);
    ptr->scopeId = scope->scopeId;
    return ptr;
}
ASTBody* AST::createBodyWithSharedScope(ScopeId scopeToShare) {
    auto ptr = new ASTBody();
    ptr->scopeId = scopeToShare;
    return ptr;
}
ASTFunction* AST::createFunction() {
    return new ASTFunction();
}
ASTStructure* AST::createStructure() {
    return new ASTStructure();
}
ASTFunction* AST::findFunction(const std::string& name, ScopeId scopeId) {
    ScopeInfo* info = getScope(scopeId);
    while(info) {
        auto& fs = info->body->functions;
        
        for(int i=0;i<fs.size();i++) {
            if(fs[i]->name == name) {
                return fs[i];
            }
        }
        
        if(info->scopeId == info->parent)
            break;
        info = getScope(info->parent);
        
        // TODO: info->shared_scopes
    }
    
    return nullptr;
}
const char* expr_type_table[] {
    "invalid", // INVALID
    "id", // IDENTIFIER
    "call", // FUNCTION_CALL
    "lit_int", // LITERAL_INT
    "lit_float", // LITERAL_FLOAT
    "lit_str", // LITERAL_STR
    "add", // ADD
    "sub", // SUB
    "div", // DIV
    "mul", // MUL
    "and", // AND
    "or",  // OR
    "not", // NOT
    "equal", // EQUAL
    "not_equal", // NOT_EQUAL
    "less", // LESS
    "greater", // GREATER
    "less_equal", // LESS_EQUAL
    "greater_equal", // GREATER_EQUAL
    "refer", // REFER
    "deref", // DEREF
    "index", // INDEX
    "member", // MEMBER
    "cast", // CAST
    "sizeof", // SIZEOF
    "assign",
    "pre_increment",
    "post_increment",
    "pre_decrement",
    "post_decrement",
    "true",
    "false",
    "null",
};
void AST::print(ASTExpression* expr, int depth) {
    Assert(expr);
    for(int i=0;i<depth;i++)
        printf(" ");
    switch(expr->kind()) {
        case ASTExpression::FUNCTION_CALL: {
            printf("%s()\n",expr->name.c_str());
            for(const auto& arg : expr->arguments) {
                print(arg, depth + 1);
            }
        } break;
        case ASTExpression::IDENTIFIER: {
            printf("%s\n",expr->name.c_str());
        } break;
        case ASTExpression::LITERAL_INT: {
            printf("%d\n",expr->literal_integer);
        } break;
        case ASTExpression::LITERAL_FLOAT: {
            printf("%f\n",expr->literal_float);
        } break;
        case ASTExpression::LITERAL_STR: {
            printf("\"%s\"\n",expr->literal_string.c_str());
        } break;
        case ASTExpression::SIZEOF: {
            printf("sizeof %s\n",expr->name.c_str());
        } break;
        default: {
            printf("%s\n",expr_type_table[expr->kind()]);
            if(expr->left)
                print(expr->left, depth + 1);
            if(expr->right)
                print(expr->right, depth + 1);
        } break;
    }
}
void AST::print(ASTBody* body, int depth) {
    Assert(body);
    
    for(const auto& st : body->structures) {
        Assert(st);
        printf("%s {\n", st->name.c_str());
        for(const auto& member : st->members) {
            printf(" %s: %s,\n", member.name.c_str(), member.typeString.c_str());
        }
        printf("}\n");
    }
    for(const auto& fn : body->functions) {
        Assert(fn);
        printf("%s(", fn->name.c_str());
        for(const auto& param : fn->parameters) {
            printf("%s: %s, ", param.name.c_str(), param.typeString.c_str());
        }
        if(fn->return_typeString.empty())
            printf(")\n");
        else
            printf("): %s\n", fn->return_typeString.c_str());

        if(fn->body)
            print(fn->body, 1);
    }
    
    for(const auto& s : body->statements) {
        switch(s->kind()){
            case ASTStatement::EXPRESSION: {
                print(s->expression, depth);
            } break;
            case ASTStatement::BREAK: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("break\n");
            } break;
            case ASTStatement::CONTINUE: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("continue\n");
            } break;
            case ASTStatement::RETURN: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("return\n");
                print(s->expression, depth+1);
            } break;
            case ASTStatement::IF: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("if\n");
                print(s->expression, depth+1);
                print(s->body, depth+1);
                if(s->elseBody) {
                    printf("else\n");
                    print(s->elseBody, depth+1);
                }
            } break;
            case ASTStatement::WHILE: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("while\n");
                print(s->expression, depth+1);
                print(s->body, depth+1);
            } break;
            case ASTStatement::VAR_DECLARATION: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("var_decl %s: %s\n", s->declaration_name.c_str(), s->declaration_type.c_str());
                if(s->expression)
                    print(s->expression, depth+1);
            } break;
            default: Assert(false);
        }
    }
}
void AST::print() {
    log_color(Color::GOLD);
    printf("Printing AST:\n");
    log_color(Color::NO_COLOR);
    
    print(global_body);
    
    // if(structures.size() == 0)
    //     printf(" No structures\n");
    // if(functions.size() == 0)
    //     printf(" No functions\n");
    
    // for(const auto& st : structures) {
    //     Assert(st);
    //     printf("%s {\n", st->name.c_str());
    //     for(const auto& member : st->members) {
    //         printf(" %s: %s,\n", member.name.c_str(), member.typeString.c_str());
    //     }
    //     printf("}\n");
    // }
    // for(const auto& fn : functions) {
    //     Assert(fn);
    //     printf("%s(", fn->name.c_str());
    //     for(const auto& param : fn->parameters) {
    //         printf("%s: %s, ", param.name.c_str(), param.typeString.c_str());
    //     }
    //     if(fn->return_typeString.empty())
    //         printf(")\n");
    //     else
    //         printf("): %s\n", fn->return_typeString.c_str());

    //     if(fn->body)
    //         print(fn->body, 1);
    // }
}
TypeInfo* AST::findType(const std::string& str, ScopeId scopeId) {
    ScopeInfo* scope = getScope(scopeId);
    while(scope) {
        auto pair = scope->type_map.find(str);
        if(pair != scope->type_map.end()) {
            return pair->second;
        }
        if(scope->scopeId == scope->parent) {
            return nullptr; // global scope
        }
        scope = getScope(scope->parent);
    }
    Assert(("bug?", false));
    return nullptr;
}
TypeId AST::convertFullType(const std::string& str, ScopeId scopeId) {
    std::string base = str;
    int pointer_level = 0;
    for(int i=str.length()-1; i>=0 ;i--)  {
        if(str[i] != '*') {
            base = str.substr(0,i+1);
            pointer_level = str.length() - i - 1;
            break;
        }
    }
    auto info = findType(base, scopeId);
    if(!info)
        return {}; // void
    auto type = info->typeId;
    type.set_pointer_level(pointer_level);
    return type;
}
TypeInfo* AST::createType(const std::string& str, ScopeId scopeId) {
    ScopeInfo* scope = getScope(scopeId);
    auto type = new TypeInfo();
    type->name = str;
    type->typeId = TypeId::Make(typeInfos.size());
    typeInfos.push_back(type);

    scope->type_map[str] = type;
    return type;
}
int AST::sizeOfType(TypeId typeId) {
    if(typeId.pointer_level() != 0)
        return 8; // pointers have fixed size
    auto type = getType(typeId);
    if(!type)
        return 0;
    return type->size;
}
std::string AST::nameOfType(TypeId typeId) {
    std::string out="";
    TypeId base = typeId;
    base.set_pointer_level(0);
    auto type = getType(base);
    Assert(type);
    out += type->name;
    for(int i=0;i<typeId.pointer_level();i++)
        out += "*";
    return out;
}

const char* primitive_names[] {
    "void",
    "int",
    "char",
    "bool",
    "float",
};