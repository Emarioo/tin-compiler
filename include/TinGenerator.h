#pragma once

struct Config {
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
    Config* config = nullptr;
    std::string output = "";
    int indent_level = 0;
    bool in_loop = false;

    void genProgram();
    void genStatements();
    void genExpression();

    void indent() {
        for(int i=0;i<indent_level;i++) output += "    ";
    }
};

void GenerateTin(Config* config);