#pragma once

#include "AST.h"
#include "Parser.h"
#include "Lexer.h"
#include "Generator.h"
#include "Interpreter.h"

enum TaskType {
    TASK_LEX_FILE,
    TASK_CHECK_STRUCTS,
    TASK_CHECK_FUNCTIONS,
    TASK_GEN_FUNCTIONS,
};
struct Compiler {
    AST* ast = nullptr;
    Reporter* reporter = nullptr;
    Code* code = nullptr;
    
    struct Task {
        TaskType type;
        std::string name;
        ASTBody* body;
    };
    std::vector<Task> tasks;
    
    std::vector<TokenStream*> streams;
    std::unordered_map<std::string, TokenStream*> stream_map;
    
    void init();
    
    void processTasks();
};

void CompileFile(const std::string& path);