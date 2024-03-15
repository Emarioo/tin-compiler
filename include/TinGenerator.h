#pragma once

struct TinConfig {
    struct Range {
        int min, max;
    };
    Range struct_frequency;
    Range member_frequency;

    Range function_frequency;
    Range argument_frequency;
    Range statement_frequency;

    int seed;
};

struct TinContext {
    TinConfig* config = nullptr;
    std::string output = "";
    int indent_level = 0;
    bool in_loop = false;

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
        std::string typeString;
    };

    std::unordered_map<std::string, Identifier*> types;
    std::unordered_map<std::string, Identifier*> functions;
    std::unordered_map<std::string, Identifier*> variables;

    struct Scope {
        std::vector<std::string> types;
        std::vector<std::string> functions;
        std::vector<std::string> variables;
    };
    std::vector<Scope*> scopes;

    void pushScope() {
        scopes.push_back(new Scope());
    }
    void popScope() {
        auto scope = scopes.back();
        scopes.erase(scopes.begin() + scopes.size()-1);

        for(auto& s : scope->types) {
            auto pair = types.find(s);
            if(pair == types.end()) continue;

            delete pair->second;
            types.erase(pair);
        }
        for(auto& s : scope->functions) {
            auto pair = functions.find(s);
            if(pair == functions.end()) continue;

            delete pair->second;
            functions.erase(pair);
        }
        for(auto& s : scope->variables) {
            auto pair = variables.find(s);
            if(pair == variables.end()) continue;

            delete pair->second;
            variables.erase(pair);
        }

        delete scope;
    }

    Identifier* addType(const std::string& name) {
        auto id = new Identifier(Identifier::TYPE);
        types[name] = id;
        scopes.back()->types.push_back(name);
        return id;
    }
    Identifier* addFunc(const std::string& name) {
        auto id = new Identifier(Identifier::FUNC);
        functions[name] = id;
        scopes.back()->functions.push_back(name);
        return id;
    }
    Identifier* addVar(const std::string& name, Identifier::Kind kind) {
        auto id = new Identifier(kind);
        variables[name] = id;
        scopes.back()->variables.push_back(name);
        return id;
    }

    // Identifier* createType(std::string qq);

    void genProgram();
    void genStatements(bool inherit_scopes = true);
    void genExpression();

    void indent() {
        for(int i=0;i<indent_level;i++) output += "    ";
    }
};

void GenerateTin(TinConfig* config);