#include "Compiler.h"

#define LOGC(...) printf(__VA_ARGS__)
// #define LOGC(...)

u32 ThreadProc(void* arg) {
    auto compiler = (Compiler*)arg;
    compiler->processTasks();
    return 0;
}

void CompileFile(const std::string& path) {
    Compiler compiler{};
    compiler.init();

    auto pre_imp = compiler.ast->createImport("preload");
    pre_imp->body = compiler.ast->global_body;
    {
    Compiler::Task task{}; // process native function
    task.name = "preload";
    task.imp = pre_imp;
    task.type = TASK_CHECK_STRUCTS;
    compiler.tasks.push_back(task);
    }
    {
    Compiler::Task task{};
    task.name = path;
    task.type = TASK_LEX_FILE;
    compiler.tasks.push_back(task);
    }

    int threadcount = 1;

    std::vector<Thread*> threads;
    for(int i=0;i<threadcount - 1;i++) {
        auto t = new Thread();
        threads.push_back(t);
        t->init(ThreadProc, &compiler);
    }

    compiler.processTasks();

    for(int i=0;i<threadcount - 1;i++) {
        auto t = threads[i];
        if(t->joinable()) {
            t->join();
        }
        delete t;
    }
    threads.clear();

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

            if(task.imp && task.imp->deps_count != task.imp->deps_now) {
                printf("Not ready %d/%d: %s\n", task.imp->deps_now, task.imp->deps_count, task.name.c_str());
                continue; // not ready
            }

            // go through includes (scope_info.shared_scopes)
            // check that all structs are complete
            // UNLESS there is a circullar dependency
            // ast->getScope(task.imp->body->scopeId)->shared_scopes;
            // if() {

            // }

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
                // LOGC("Lexing: %s\n", task.name.c_str());
                TokenStream* stream = lex_file(task.name);
                // MEMORY LEAK, stream not destroyed
                streams.push_back(stream);
                stream_map[task.name] = stream;
                auto imp = ParseTokenStream(stream, task.imp, ast, reporter);
                task.imp = imp;

                // TODO: Mutex
                for(auto& fix_imp : imp->fixups) {
                    fix_imp->deps_now++;
                    ast->getScope(fix_imp->body->scopeId)->shared_scopes.push_back(ast->getScope(imp->body->scopeId));
                }
                imp->fixups.clear();
                
                if(imp) {
                    log_color(GREEN); LOGC("Lexed: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    
                    imp->deps_now = 0;
                    imp->deps_count = 0;
                    for (auto& n : imp->dependencies) {
                        auto pair = ast->import_map.find(n);
                        if(pair == ast->import_map.end()) {
                            Task t{};
                            t.type = TASK_LEX_FILE;
                            t.name = n;
                            t.imp = ast->createImport(t.name);
                            t.imp->fixups.push_back(imp);
                            imp->deps_count++;
                            tasks.push_back(t);
                        } else {
                            // TODO: Mutex
                            if(pair->second->body) {
                                ast->getScope(imp->body->scopeId)->shared_scopes.push_back(ast->getScope(pair->second->body->scopeId));
                            } else {
                                pair->second->fixups.push_back(imp);
                                imp->deps_count++;
                            }
                            // import is already a task
                        }
                    }
                    task.type = TASK_CHECK_STRUCTS;
                    task.imp = imp;
                    tasks.push_back(task);
                } else {
                    log_color(RED); LOGC("Lexer failed: %s\n", task.name.c_str()); log_color(NO_COLOR);
                }
            } break;
            case TASK_CHECK_STRUCTS: {
                // LOGC("Checking structs: %s\n", task.name.c_str());
                // Check structs when dependencies have been calculated.
                // check shared scopes

                bool ignore_errors = true;
                bool changed = false;
                bool success = CheckStructs(ast, task.imp, reporter, &changed, ignore_errors);

                if(!success) {
                    if(!changed) {
                        printf("No struct change\n");
                    }
                    log_color(RED); LOGC("Checking structs failure: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    if(ignore_errors) {
                        tasks.push_back(task);
                    } else {
                        // actual failure
                    }
                } else {
                    log_color(GREEN); LOGC("Checked structs: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    task.type = TASK_CHECK_FUNCTIONS;
                    tasks.push_back(task);
                }
            } break;
            case TASK_CHECK_FUNCTIONS: {
                // LOGC("Checking functions: %s\n", task.name.c_str());
                for(auto func : task.imp->body->functions)
                    CheckFunction(ast, task.imp, func, reporter);
                    
                if(reporter->errors == 0) {
                    task.type = TASK_GEN_FUNCTIONS;
                    tasks.push_back(task);
                    log_color(GREEN); LOGC("Checked functions: %s\n", task.name.c_str()); log_color(NO_COLOR); 
                } else {
                    log_color(RED);  LOGC("Checking functions failure: %s\n", task.name.c_str()); log_color(NO_COLOR); 
                }
            } break;
            case TASK_GEN_FUNCTIONS: {
                // LOGC("Gen functions: %s\n", task.name.c_str());
                for(auto func : task.imp->body->functions)
                    GenerateFunction(ast, func, code, reporter);
                log_color(GREEN); LOGC("generated functions: %s\n", task.name.c_str()); log_color(NO_COLOR); 
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