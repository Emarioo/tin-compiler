#pragma once

#include "Config.h"

struct TinConfig {
    struct Range {
        int min, max;
    };
    Range struct_frequency;
    Range member_frequency;

    Range function_frequency;
    Range argument_frequency;
    Range statement_frequency;

    int seed = 0;
};

struct TinContext {
    TinConfig* config = nullptr;
    std::string output = "";
    int indent_level = 0;
    bool in_loop = false;

    int expr_count = 0;

    int var_counter = 0;
    int fun_counter = 0;
    int type_counter = 0;

    struct Type;
    struct ComplexType {
        Type* type = nullptr;
        int pointer_level = 0;
        
        std::string to_string() const {
            std::string out = type->name;
            for(int i=0;i<pointer_level;i++) out += "*";
            return out;
        }
        
        bool operator==(ComplexType& t) {
            return type == t.type && pointer_level == t.pointer_level;
        }
        bool operator!=(ComplexType& t) {
            return !(*this == t);
        }
        // valid/empty type
        bool valid() const { return type; }
    };
    struct Type {
        std::string name;
        struct Member {
            Member(const std::string& name, ComplexType type) : name(name), type(type) {}
            std::string name;
            ComplexType type;
        };
        std::vector<Member> members;
    };
    struct Function {
        std::string name;
        struct Arg {
            Arg(const std::string& name, ComplexType type) : name(name), type(type) {}
            std::string name;
            ComplexType type;
        };
        std::vector<Arg> args;
        ComplexType return_type;
    };
    struct Identifier {
        enum Kind {
            GLOBAL,
            LOCAL,
            CONSTANT,
        };
        Identifier(Kind k) : kind(k) {}
        Kind kind;
        std::string name;
        ComplexType type;
    };

    // types, functions and variables are handled separately in the compiler
    // std::unordered_map<std::string, Identifier*> types;
    // std::unordered_map<std::string, Identifier*> functions;
    // std::unordered_map<std::string, Identifier*> variables;

    Function* current_function = nullptr;
    struct Scope {
        std::vector<Type*> types;
        std::vector<Function*> functions;
        std::vector<Identifier*> variables;
    };
    std::vector<Scope*> scopes;

    void pushScope() {
        scopes.push_back(new Scope());
    }
    void popScope() {
        auto scope = scopes.back();
        scopes.erase(scopes.begin() + scopes.size()-1);

        for(auto& s : scope->types) {
            // auto pair = types.find(s->name);
            // if(pair != types.end())
            // types.erase(pair);

            delete s;
        }
        for(auto& s : scope->functions) {
            // auto pair = functions.find(s->name);
            // if(pair != functions.end())
            // functions.erase(pair);

            delete s;
        }
        for(auto& s : scope->variables) {
            // auto pair = variables.find(s->name);
            // if(pair != variables.end())
            //     variables.erase(pair);
                
            delete s;
        }

        delete scope;
    }

    Type* createType(const std::string& name) {
        auto id = new Type();
        id->name = name;
        return id;
    }
    void addType(Type* t) {
        scopes.back()->types.push_back(t);
    }
    Function* addFunc(const std::string& name) {
        auto id = new Function();
        id->name = name;
        scopes.back()->functions.push_back(id);
        return id;
    }
    Identifier* addVar(const std::string& name, Identifier::Kind kind) {
        auto id = new Identifier(kind);
        id->name = name;
        scopes.back()->variables.push_back(id);
        return id;
    }
    Identifier* requestVariable(ComplexType expected_type) {
        if(scopes.size() == 0) return nullptr;
        for(int i=scopes.size()-1;i>=0;i--) {
            auto s = scopes[i];
            if(s->variables.size() == 0)
                continue;
            int start = RandomInt(0,s->variables.size()-1);
            int j = start;
            while(true) {
                auto& v = s->variables[j];
                if(!expected_type.valid() || v->type == expected_type)
                    return v;
                
                j = (j + 1) % s->variables.size();
                if(j == start) {
                    break;
                }
            }
        }
        return nullptr;
    }
    Function* requestFunction(ComplexType expected_type) {
        if(scopes.size() == 0) return nullptr;
        for(int i=scopes.size()-1;i>=0;i--) {
            auto s = scopes[i];
            if(s->functions.size() == 0)
                continue;
            int start = RandomInt(0,s->functions.size()-1);
            int j = start;
            while(true) {
                auto& v = s->functions[j];
                if(!expected_type.valid() || v->return_type == expected_type)
                    return v;
                
                j = (j + 1) % s->functions.size();
                if(j == start) {
                    break;
                }
            }
        }
        return nullptr;
    }
    ComplexType requestType() {
        if(scopes.size() == 0) return {};
        for(int i=scopes.size()-1;i>=0;i--) {
            auto s = scopes[i];
            if(s->types.size() == 0)
                continue;
            int j = RandomInt(0,s->types.size()-1);
            return { s->types[j] };
        }
        return {};
    }
    ComplexType requestPrimitive() {
        int j = RandomInt(0,3);
        if(j==0) return {type_int};
        else if(j==1) return {type_float};
        else if(j==2) return {type_bool};
        else return {type_char};
    }
    Type* type_int      = nullptr;
    Type* type_float    = nullptr;
    Type* type_char     = nullptr;
    Type* type_bool     = nullptr;

    void genProgram();
    void genStatements(bool inherit_scopes = false);
    void genExpression(ComplexType expected_type, int expr_depth);

    void indent() {
        for(int i=0;i<indent_level;i++) output += "    ";
    }
};

void GenerateTin(TinConfig* config);