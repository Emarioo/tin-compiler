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
        f->piece_code_index = NATIVE_printi - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "v";
        f->parameters.back().typeString = "int";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->piece_code_index = NATIVE_printf - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "v";
        f->parameters.back().typeString = "float";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->piece_code_index = NATIVE_malloc - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "size";
        f->parameters.back().typeString = "int";
        f->return_typeString = "void*";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->piece_code_index = NATIVE_mfree - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "ptr";
        f->parameters.back().typeString = "void*";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->piece_code_index = NATIVE_memmove - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "dst";
        f->parameters.back().typeString = "void*";
        f->parameters.push_back({});
        f->parameters.back().name = "src";
        f->parameters.back().typeString = "void*";
        f->parameters.push_back({});
        f->parameters.back().name = "size";
        f->parameters.back().typeString = "int";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->piece_code_index = NATIVE_pow - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "x";
        f->parameters.back().typeString = "float";
        f->parameters.push_back({});
        f->parameters.back().name = "y";
        f->parameters.back().typeString = "float";
        f->return_typeString = "float";
        global_body->add(f);
    }
    {
        auto f = createFunction();
        f->is_native = true;
        f->piece_code_index = NATIVE_sqrt - NATIVE_MAX - 1;
        f->name = NAME_OF_NATIVE(f->piece_code_index + NATIVE_MAX + 1);
        f->parameters.push_back({});
        f->parameters.back().name = "x";
        f->parameters.back().typeString = "float";
        f->return_typeString = "float";
        global_body->add(f);
    }
}
ASTExpression* AST::createExpression(ASTExpression::Kind kind) {
    auto ptr = new ASTExpression(kind);
    ptr->nodeid = next_nodeid();
    
    return ptr;
}
ASTStatement* AST::createStatement(ASTStatement::Kind kind) {
    auto ptr = new ASTStatement(kind);
    return ptr;
}
ASTBody* AST::createBody(ScopeId parent) {
    auto ptr = new ASTBody();
    ptr->nodeid = next_nodeid();
    auto scope = createScope(parent);
    ptr->scopeId = scope->scopeId;
    scope->body = ptr;
    return ptr;
}
ASTBody* AST::createBodyWithSharedScope(ScopeId scopeToShare) {
    auto ptr = new ASTBody();
    ptr->nodeid = next_nodeid();
    ptr->scopeId = scopeToShare;
    return ptr;
}
ASTFunction* AST::createFunction() {
    auto ptr = new ASTFunction();
    ptr->nodeid = next_nodeid();
    return ptr;
}
ASTStructure* AST::createStructure() {
    auto ptr = new ASTStructure();
    ptr->nodeid = next_nodeid();
    return ptr;
}
ASTFunction* AST::findFunction(const std::string& name, ScopeId scopeId) {
    auto iterator = createScopeIterator(scopeId);
    ScopeInfo* scope=nullptr;
    while((scope = iterate(iterator))) {
        auto& fs = scope->body->functions;
        for(int i=0;i<fs.size();i++) {
            if(fs[i]->name == name) {
                return fs[i];
            }
        }
    }
    return nullptr;
}
const char* expr_type_table[] {
    "invalid", // INVALID
    "id", // IDENTIFIER
    "call", // FUNCTION_CALL
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
    "assign",
    "pre_increment",
    "post_increment",
    "pre_decrement",
    "post_decrement",
    
    "sizeof", // SIZEOF
    "lit_int", // LITERAL_INT
    "lit_float", // LITERAL_FLOAT
    "lit_str", // LITERAL_STR
    "null",
    "true",
    "false",
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
            case ASTStatement::GLOBAL_DECLARATION: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("global %s: %s\n", s->declaration_name.c_str(), s->declaration_type.c_str());
                if(s->expression)
                    print(s->expression, depth+1);
            } break;
            case ASTStatement::CONST_DECLARATION: {
                for(int i=0;i<depth;i++) printf(" ");
                printf("const %s: %s\n", s->declaration_name.c_str(), s->declaration_type.c_str());
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
    auto iterator = createScopeIterator(scopeId);
    ScopeInfo* scope=nullptr;
    while((scope = iterate(iterator))) {
        auto pair = scope->type_map.find(str);
        if(pair != scope->type_map.end()) {
            return pair->second;
        }
    }
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

Identifier* AST::addVariable(Identifier::Kind var_type, const std::string& name, ScopeId scopeId, TypeId type, int frame_offset) {
    auto scope = getScope(scopeId);
    
    auto pair = scope->identifiers.find(name);
    if(pair != scope->identifiers.end())
        return nullptr; // variable already exists
        
    auto ptr = new Identifier(var_type);
    // ptr->name = name;
    ptr->type = type;
    scope->identifiers[name] = ptr;
    if(var_type == Identifier::CONST_ID) {
        ptr->statement = nullptr;
    } else {
        ptr->offset = frame_offset;
    }
    return ptr;
}
Identifier* AST::findVariable(const std::string& name, ScopeId scopeId) {
    auto iterator = createScopeIterator(scopeId);
    ScopeInfo* scope=nullptr;
    while((scope = iterate(iterator))) {
        auto pair = scope->identifiers.find(name);
        if(pair != scope->identifiers.end())
            return pair->second;
    }
    return nullptr;
}

const char* primitive_names[] {
    "void",
    "int",
    "char",
    "bool",
    "float",
};

AST::ScopeIterator AST::createScopeIterator(ScopeId scopeId) {
    ScopeIterator iter{};
    iter.searchScopes.push_back(getScope(scopeId));
    return iter;
}
ScopeInfo* AST::iterate(AST::ScopeIterator& iterator) {
    auto add = [&](ScopeInfo* s) {
        // TODO: Optimize
        for (int i=0;i<iterator.search_index;i++) {
            if(iterator.searchScopes[i] == s) {
                return;
            }
        }
        iterator.searchScopes.push_back(s);
    };
    
    while(iterator.search_index < iterator.searchScopes.size()) {
        ScopeInfo* info = iterator.searchScopes[iterator.search_index];
        iterator.search_index++;
        
        for(auto s : info->shared_scopes) {
            add(s);
        }
        if(info->scopeId != info->parent) {
            auto s = getScope(info->parent);
            add(s);
        }
        
        return info;
    }
    return nullptr;
}