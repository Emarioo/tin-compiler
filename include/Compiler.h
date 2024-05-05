#pragma once

#include "AST.h"
#include "Parser.h"
#include "Lexer.h"
#include "Generator.h"
#include "VirtualMachine.h"

enum TaskType {
    TASK_LEX_FILE,
    TASK_CHECK_STRUCTS,
    TASK_CHECK_FUNCTIONS,
    TASK_GEN_FUNCTIONS,
};
struct CompilerOptions {
    std::string initial_file;
    bool run = false;
    #ifdef ENABLE_MULTITHREADING
    int thread_count = 0;
    #else
    int thread_count = 1;
    #endif
};
struct Compiler {
    AST* ast = nullptr;
    Reporter* reporter = nullptr;
    Bytecode* bytecode = nullptr;
    
    std::vector<TokenStream*> streams;
    std::unordered_map<std::string, TokenStream*> stream_map;
    
    struct Task {
        TaskType type;
        std::string name;
        AST::Import* imp = nullptr;
        bool no_change = false;
    };
    std::vector<Task> tasks;
    
    volatile int threads_processing = 0;
    volatile int total_threads = 0;
    MUTEX_DECL(tasks_lock)

    bool is_signaled = false;
    SEM_DECL(tasks_queue_lock)
    
    void init();
    void processTasks();
};

Bytecode* CompileFile(CompilerOptions* options);