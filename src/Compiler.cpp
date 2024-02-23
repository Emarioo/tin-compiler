#include "Compiler.h"

void CompileFile(const std::string& path) {
    
    Compiler compiler{};
    compiler.init();
   
    Compiler::Task task{};
    task.name = path;
    task.type = TASK_LEX_FILE;
    compiler.tasks.push_back(task);
   
    compiler.processTasks();

    // TokenStream* stream = lex_file(path);
    // ParseTokenStream(stream, ast, reporter);
    // stream->print();


    // ast->print();
    
    
    // if(reporter->errors == 0) {
    //     CheckStructs(ast, reporter);
        
    //     for(auto func : ast->functions)
    //         CheckFunction(ast, func, reporter);
    //     for(auto func : ast->functions)
    //         GenerateFunction(ast, func, code, reporter);
        
    //     // code->print();
    // }
    
    if(compiler.reporter->errors == 0) {
        Interpreter* interpreter = new Interpreter();
        interpreter->code = compiler.code;
        interpreter->init();
        interpreter->execute();
        
        delete interpreter;
    }
}

void Compiler::processTasks() {
    bool running = true;
    while(running) {
        
        int task_index=-1;
        for(int i=0;i<tasks.size();i++) {
            auto& task = tasks[i];   
            task_index = i;
            break;
        }
        
        if(task_index == -1) {
            // finished all tasks
            running = false;
            break;
        }
        
        Task task = tasks[task_index];
        tasks.erase(tasks.begin() + task_index);
        
        switch(task.type) {
            case TASK_LEX_FILE: {
                TokenStream* stream = lex_file(task.name);
                // MEMORY LEAK, stream not destroyed
                streams.push_back(stream);
                stream_map[task.name] = stream;
                auto body = ParseTokenStream(stream, ast, reporter);
                
                if(reporter->errors == 0) {
                    task.type = TASK_CHECK_STRUCTS;
                    task.body = body;
                    tasks.push_back(task);
                }
            } break;
            case TASK_CHECK_STRUCTS: {
                // Check structs when dependencies have been calculated.
                
                CheckStructs(ast, reporter);
                
                if(reporter->errors == 0) {
                    task.type = TASK_CHECK_FUNCTIONS;
                    tasks.push_back(task);
                }
            } break;
            case TASK_CHECK_FUNCTIONS: {
                for(auto func : ast->functions)
                    CheckFunction(ast, func, reporter);
                    
                if(reporter->errors == 0) {
                    task.type = TASK_GEN_FUNCTIONS;
                    tasks.push_back(task);
                }
            } break;
            case TASK_GEN_FUNCTIONS: {
                for(auto func : ast->functions)
                    GenerateFunction(ast, func, code, reporter);
            } break;
            default: Assert(false);
        }
    }
}

void Compiler::init() {
    ast = new AST();
    reporter = new Reporter();
    code = new Code();
    // TODO: Memory leak
}