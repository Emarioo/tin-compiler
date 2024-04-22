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
    Bytecode* code = nullptr;
    
    struct Task {
        TaskType type;
        std::string name;
        AST::Import* imp = nullptr;

        bool no_change = false;

        // ASTBody* body = nullptr; // only used if there is no import (preload for example)
    };
    std::vector<Task> tasks;
    
    std::vector<TokenStream*> streams;
    std::unordered_map<std::string, TokenStream*> stream_map;
    
    volatile int threads_processing = 0;
    volatile int total_threads = 0;
    MUTEX_DECL(tasks_lock);
    SEM_DECL(tasks_queue_lock);
    
    void init();
    void processTasks();
};

void CompileFile(const std::string& path);