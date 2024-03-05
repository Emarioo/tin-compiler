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

    // int threadcount = 2;
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
    ZoneScopedC(tracy::Color::Gray12);
    int thread_id = atomic_add(&total_threads, 1);

    
    bool running = true;
    while(running) {
        ZoneNamedC(zone0, tracy::Color::Gray12, true);
        
        MUTEX_LOCK(tasks_lock);
        
        int task_index=-1;
        for(int i=0;i<tasks.size();i++) {
            auto& task = tasks[i];   

            if(task.imp && task.imp->deps_count != task.imp->deps_now) {
                printf("Not ready %d/%d: %s\n", task.imp->deps_now, task.imp->deps_count, task.name.c_str());
                continue; // not ready
            }

            task_index = i;
            break;
        }
        
        if(task_index == -1) {
            if(tasks.size() == 0) {
                // finished all tasks
                running = false;
                MUTEX_UNLOCK(tasks_lock);
                break;
            }
            if (threads_processing == 0) {
                log_color(RED);
                printf("processTasks: No thread is processing tasks and no task is ready for processing.");
                log_color(NO_COLOR);
                MUTEX_UNLOCK(tasks_lock);
                break;
            }
            continue;
        }
        
        Task task = tasks[task_index];
        tasks.erase(tasks.begin() + task_index);
        
        atomic_add(&threads_processing, 1);
        MUTEX_UNLOCK(tasks_lock);
        
        bool queue_task = false;
        
        switch(task.type) {
            case TASK_LEX_FILE: {
                // LOGC("Lexing: %s\n", task.name.c_str());
                TokenStream* stream = lex_file(task.name); // MEMORY LEAK, stream not destroyed
                MUTEX_LOCK(tasks_lock);
                streams.push_back(stream);
                stream_map[task.name] = stream;
                MUTEX_UNLOCK(tasks_lock);
                auto imp = ParseTokenStream(stream, task.imp, ast, reporter);
                task.imp = imp;

                MUTEX_LOCK(tasks_lock);
                for(auto& fix_imp : imp->fixups) {
                    fix_imp->deps_now++;
                    // TODO: Can we access shared_scopes like this, do we need mutex? is task_lock enough?
                    ast->getScope(fix_imp->body->scopeId)->shared_scopes.push_back(ast->getScope(imp->body->scopeId));
                }
                imp->fixups.clear();
                MUTEX_UNLOCK(tasks_lock);
                
                if(imp) {
                    LOGC("[%d]: ",thread_id);
                    log_color(GREEN); LOGC("Lexed: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    
                    imp->deps_now = 0;
                    imp->deps_count = 0;
                    for (auto& n : imp->dependencies) {
                        MUTEX_LOCK(ast->scopes_lock);
                        auto pair = ast->import_map.find(n);
                        if(pair == ast->import_map.end()) {
                            MUTEX_UNLOCK(ast->scopes_lock);
                            
                            Task t{};
                            t.type = TASK_LEX_FILE;
                            t.name = n;
                            t.imp = ast->createImport(t.name);
                            MUTEX_LOCK(tasks_lock);
                            t.imp->fixups.push_back(imp);
                            imp->deps_count++;
                            tasks.push_back(t);
                            MUTEX_UNLOCK(tasks_lock);
                        } else {
                            auto ptr = pair->second;
                            MUTEX_UNLOCK(ast->scopes_lock);
                            MUTEX_LOCK(tasks_lock);
                            if(ptr->body) {
                                // TODO: Is tasks_lock mutex enough for shared_scopes?
                                ast->getScope(imp->body->scopeId)->shared_scopes.push_back(ast->getScope(pair->second->body->scopeId));
                            } else {
                                pair->second->fixups.push_back(imp);
                                imp->deps_count++;
                            }
                            MUTEX_UNLOCK(tasks_lock);
                            // import is already a task
                        }
                    }
                    task.type = TASK_CHECK_STRUCTS;
                    task.imp = imp;
                    queue_task = true;
                } else {
                    LOGC("[%d]: ",thread_id);
                    log_color(RED); LOGC("Lexer failed: %s\n", task.name.c_str()); log_color(NO_COLOR);
                }
            } break;
            case TASK_CHECK_STRUCTS: {
                // LOGC("Checking structs: %s\n", task.name.c_str());

                // TODO: Bug if two threads check structs at same time. One thread may be fast and check structs twice where is it hoping a type was evaluated the second time. However, the second thread is so slow on evaluating the type so that the first one thinks there is an invalid type.

                bool ignore_errors = true;
                bool changed = false;
                bool success = CheckStructs(ast, task.imp, reporter, &changed, ignore_errors);

                if(!success) {
                    if(!changed) {
                        // printf("No struct change\n");
                        if(task.no_change) {
                            ignore_errors = false;
                            CheckStructs(ast, task.imp, reporter, &changed, ignore_errors); // print errors
                        }
                    }
                    task.no_change = !changed;
                    LOGC("[%d]: ",thread_id);
                    log_color(RED); LOGC("Checking structs failure: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    if(ignore_errors) {
                        queue_task = true;
                    } else {
                        // actual failure
                    }
                } else {
                    LOGC("[%d]: ",thread_id);
                    log_color(GREEN); LOGC("Checked structs: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    task.type = TASK_CHECK_FUNCTIONS;
                    queue_task = true;
                }
            } break;
            case TASK_CHECK_FUNCTIONS: {
                // LOGC("Checking functions: %s\n", task.name.c_str());
                for(auto func : task.imp->body->functions)
                    CheckFunction(ast, task.imp, func, reporter);
                
                if(reporter->errors == 0) {
                    task.type = TASK_GEN_FUNCTIONS;
                    queue_task = true;
                    LOGC("[%d]: ",thread_id);
                    log_color(GREEN); LOGC("Checked functions: %s\n", task.name.c_str()); log_color(NO_COLOR); 
                } else {
                    LOGC("[%d]: ",thread_id);
                    log_color(RED);  LOGC("Checking functions failure: %s\n", task.name.c_str()); log_color(NO_COLOR); 
                }
                
                CheckGlobals(ast, task.imp, code, reporter);
            } break;
            case TASK_GEN_FUNCTIONS: {
                LOGC("[%d]: ",thread_id);
                LOGC("Gen functions: %s\n", task.name.c_str());
                
                for(auto func : task.imp->body->functions)
                    GenerateFunction(ast, func, code, reporter);
                
                log_color(GREEN); LOGC("generated functions: %s\n", task.name.c_str()); log_color(NO_COLOR); 
            } break;
            default: Assert(false);
        }
        
        MUTEX_LOCK(tasks_lock);
        atomic_add(&threads_processing, -1);
        if(queue_task) {
            tasks.push_back(task);
        }
        MUTEX_UNLOCK(tasks_lock);
    }
    atomic_add(&total_threads, -1);
}

void Compiler::init() {
    ast = new AST();
    reporter = new Reporter();
    code = new Code();
    // TODO: Memory leak
}