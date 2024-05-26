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
    int thread_count = 16;
    #else
    int thread_count = 1;
    #endif

    double compilation_time = 0;
    int processed_bytes = 0;
    int processed_lines = 0;

    bool silent = false; // won't silence errors
};
struct Compiler {
    ~Compiler() {
        cleanup();
    }
    void cleanup();
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

bool CompileFile(CompilerOptions* options, Bytecode** out_bytecode = nullptr);