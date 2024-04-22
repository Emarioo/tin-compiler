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

    struct Identifier {
        enum Kind {
            TYPE,
            GLOBAL,
            LOCAL,
            CONSTANT,
            FUNC,
        };
        Identifier(Kind k) : kind(k) {}
        Kind kind;
        std::string name;
        std::string typeString;
    };

    // types, functions and variables are handled separately in the compiler
    std::unordered_map<std::string, Identifier*> types;
    std::unordered_map<std::string, Identifier*> functions;
    std::unordered_map<std::string, Identifier*> variables;

    struct Scope {
        std::vector<Identifier*> types;
        std::vector<Identifier*> functions;
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
            auto pair = types.find(s->name);
            if(pair == types.end()) continue;

            delete pair->second;
            types.erase(pair);
        }
        for(auto& s : scope->functions) {
            auto pair = functions.find(s->name);
            if(pair == functions.end()) continue;

            delete pair->second;
            functions.erase(pair);
        }
        for(auto& s : scope->variables) {
            auto pair = variables.find(s->name);
            if(pair == variables.end()) continue;

            delete pair->second;
            variables.erase(pair);
        }

        delete scope;
    }

    Identifier* addType(const std::string& name) {
        auto id = new Identifier(Identifier::TYPE);
        id->name = name;
        types[name] = id;
        scopes.back()->types.push_back(id);
        return id;
    }
    Identifier* addFunc(const std::string& name) {
        auto id = new Identifier(Identifier::FUNC);
        id->name = name;
        functions[name] = id;
        scopes.back()->functions.push_back(id);
        return id;
    }
    Identifier* addVar(const std::string& name, Identifier::Kind kind) {
        auto id = new Identifier(kind);
        id->name = name;
        variables[name] = id;
        scopes.back()->variables.push_back(id);
        return id;
    }
    Identifier* requestVariable() {
        if(scopes.size() == 0) return nullptr;
        int i = RandomInt(0,scopes.size()-1);
        auto s = scopes[i];
        if(s->variables.size() == 0) return nullptr;
        int j = RandomInt(0,s->variables.size()-1);
        return s->variables[j];
    }
    Identifier* requestFunction() {
        if(scopes.size() == 0) return nullptr;
        int i = RandomInt(0,scopes.size()-1);
        auto s = scopes[i];
        if(s->functions.size() == 0) return nullptr;
        int j = RandomInt(0,s->functions.size()-1);
        return s->functions[j];
    }
    // void requestType()
    // void requestFunction()

    void genProgram();
    void genStatements(bool inherit_scopes = true);
    void genExpression(int expr_depth);

    void indent() {
        for(int i=0;i<indent_level;i++) output += "    ";
    }
};

void GenerateTin(TinConfig* config);