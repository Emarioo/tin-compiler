#include "Compiler.h"

// #define LOGC(...) printf(__VA_ARGS__)
#define LOGC(...)
// #define LOGIT(X) X
#define LOGIT(X)

u32 ThreadProc(void* arg) {
    auto compiler = (Compiler*)arg;
    // SleepMS(100);
    compiler->processTasks();
    return 0;
}

bool CompileFile(CompilerOptions* options, Bytecode** out_bytecode) {
    Compiler compiler{};
    compiler.init();
    #ifdef ENABLE_MULTITHREADING
    if(options->thread_count == 1) {
        log_color(RED);
        printf("You are using one thread with multi-threading enabled. Disable multi-threading when compiling to get fair performance out of one thread.\n");
        log_color(NO_COLOR);
    }
    #endif

    auto pre_imp = compiler.ast->createImport("preload");
    pre_imp->body = compiler.ast->global_body;
    {
    // Process native functions added in constructor of AST
    Compiler::Task task{};
    task.name = "preload";
    task.imp = pre_imp;
    task.type = TASK_CHECK_STRUCTS;
    compiler.tasks.push_back(task);
    }
    {
    Compiler::Task task{};
    if(options->initial_file.empty()) {
        log_color(RED);
        printf("You did not specify a file to compiler");
        log_color(NO_COLOR);
        return false;
    }
    task.name = options->initial_file;
    task.type = TASK_LEX_FILE;
    compiler.tasks.push_back(task);
    }

    int threadcount = options->thread_count;

#ifndef ENABLE_MULTITHREADING
    if(threadcount > 1) {
        log_color(YELLOW);
        printf("Thread count of %d was specified while compiler wasn't built with ENABLE_MULTITHREADING. (compiler will use 1 thread unless you recompile the compiler)\n", threadcount);
        log_color(NO_COLOR);
        threadcount = 1;
    }
#endif

    auto start_time = StartMeasure();

    SEM_INIT(compiler.tasks_queue_lock, 1, 1)
#ifdef ENABLE_HIGH_PRIORITY_PROCESS
    SetHighProcessPriority();
#endif

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
    
    double time = StopMeasure(start_time);

    
    int total_lines = 0;
    int non_blank_lines = 0;
    u64 total_bytes = 0;
    for(int i=0;i<compiler.streams.size();i++) {
        auto& s = compiler.streams[i];
        total_lines += s->total_lines;
        non_blank_lines += s->non_blank_lines;
        total_bytes += s->processed_bytes;
    }
    
    options->compilation_time = time;
    options->processed_bytes = total_bytes;
    options->processed_lines = total_lines;
    
    u64 lines_per_sec = total_lines/time;

    // int insts = 0;
    // for(auto p : compiler.bytecode->pieces_unsafe().size();i++) {
    //     printf("%d\n",compiler.bytecode->);
    //     compiler
    // }

    if(compiler.reporter->errors){

        return false;
    } else if(!options->silent) {
        log_color(GOLD);
        printf("Compiled %d lines in ", total_lines);
        if(time >= 1.0)
            printf("%.3f sec", (float)time);
        else if(time >= 0.001)
            printf("%.3f ms", (float)(time*1000));
        else
            printf("%.3f us", (float)(time*1000'000));
            
        printf(" (%llu lines/s)\n", lines_per_sec);
        log_color(NO_COLOR);

         printf(" %d non-blank lines\n", non_blank_lines);
        printf(" %d files\n", (int)compiler.streams.size());
        printf(" %d threads\n", (int)threadcount);

        printf(" ");
        if(total_bytes < 1024)
            printf("%llu bytes", total_bytes);
        else if(total_bytes < 1024*1024)
            printf("%.2f KB", total_bytes / 1024.f);
        else
            printf("%.2f MB", total_bytes / 1024.f / 1024.f);
        printf(", ");
        u64 bytes_per_sec = (double)total_bytes / time;
        if(bytes_per_sec < 1024)
            printf("%llu bytes/s", bytes_per_sec);
        else if(bytes_per_sec < 1024*1024)
            printf("%.2f KB/s", bytes_per_sec / 1024.f);
        else
            printf("%.2f MB/s", bytes_per_sec / 1024.f / 1024);
        printf("\n");
    }
    
    if(options->run && !out_bytecode) {
        VirtualMachine* interpreter = new VirtualMachine();
        interpreter->bytecode = compiler.bytecode;
        interpreter->init();
        interpreter->execute();
        
        delete interpreter;
        return nullptr;
    }

    // printf("Scopes: %d\n", compiler.ast->scopes_used);
    
    if(out_bytecode) {
        // steal ast
        compiler.bytecode->owns_ast = true;
        compiler.bytecode->ast = compiler.ast;
        compiler.ast = nullptr;
        // steal bytecode
        *out_bytecode = compiler.bytecode;
        compiler.bytecode = nullptr;
    }
    return true;
}

void Compiler::processTasks() {
#ifdef ENABLE_HIGH_PRIORITY_PROCESS
    SetHighThreadPriority();
#endif
    // return;
    ZoneScopedC(tracy::Color::Gray12);
    
    int thread_id = atomic_add(&total_threads, 1);
    // printf("Start T\n");

    // SleepMS(100);
    
    bool running = true;
    while(running) {
        ZoneNamedNC(zone0, "try_task", tracy::Color::Gray15, true);
        
        SEM_WAIT(tasks_queue_lock)
        MUTEX_LOCK(tasks_lock)
        is_signaled = false;
        
        int task_index=-1;
        for(int i=0;i<tasks.size();i++) {
            auto& task = tasks[i];   

            if(task.imp && task.imp->deps_count != task.imp->deps_now) {
                // printf("Not ready %d/%d: %s\n", task.imp->deps_now, task.imp->deps_count, task.name.c_str());
                continue; // not ready
            }

            task_index = i;
            break;
        }
        
        if(task_index == -1) {
            if(tasks.size() == 0 && threads_processing == 0) {
                // finished all tasks
                running = false;

                if(!is_signaled) {
                    is_signaled = true;
                    SEM_SIGNAL(tasks_queue_lock)
                }
                
                MUTEX_UNLOCK(tasks_lock);
                break;
            }
            if (threads_processing == 0) {
                log_color(RED);
                printf("processTasks: No thread is processing tasks and no task is ready for processing.");
                log_color(NO_COLOR);

                if(!is_signaled) {
                    is_signaled = true;
                    SEM_SIGNAL(tasks_queue_lock)
                }

                MUTEX_UNLOCK(tasks_lock);
                break;
            }
            MUTEX_UNLOCK(tasks_lock);
            continue;
        }
        
        Task task = tasks[task_index];
        tasks.erase(tasks.begin() + task_index);
        
        atomic_add(&threads_processing, 1);

        if(tasks.size() && !is_signaled) {
            is_signaled = true;
            SEM_SIGNAL(tasks_queue_lock)
        }

        MUTEX_UNLOCK(tasks_lock);
        
        bool queue_task = false;
        
        switch(task.type) {
            case TASK_LEX_FILE: {
                // LOGC("Lexing: %s\n", task.name.c_str());
                TokenStream* stream = lex_file(reporter,task.name); // MEMORY LEAK, stream not destroyed
                if(!stream) {
                    // lex_file Reports errors, no need to do so here.
                    // log_color(RED);
                    // printf("File %s could not be found.\n", task.name.c_str());
                    // log_color(NO_COLOR);
                    // reporter->errors++;
                    break;   
                }
                MUTEX_LOCK(tasks_lock);
                streams.push_back(stream);
                stream_map[task.name] = stream;
                MUTEX_UNLOCK(tasks_lock);
                auto imp = ParseTokenStream(stream, task.imp, ast, reporter);
                task.imp = imp;

                // stream->print();
                // ast->print();

                // 
                MUTEX_LOCK(tasks_lock);
                for(auto& fix_imp : imp->fixups) {
                    fix_imp->deps_now++;
                    // TODO: Can we access shared_scopes like this, do we need mutex? is task_lock enough?
                    ast->getScope(fix_imp->body->scopeId)->shared_scopes.push_back(ast->getScope(imp->body->scopeId));
                }
                imp->fixups.clear();
                MUTEX_UNLOCK(tasks_lock);
                
                if(imp) {
                    LOGIT(
                        LOGC("[%d]: ",thread_id); log_color(GREEN); LOGC("Lexed: %s\n", task.name.c_str()); log_color(NO_COLOR);
                    )
                    
                    imp->deps_now = 0;
                    imp->deps_count = 0;
                    for (auto& n : imp->dependencies) {
                        // IMPORTANT TODO: find and create import as well as adding it as a task
                        // should be an atomic operation. This is currently a bug.
                        auto dep_imp = ast->findImport(n);
                        if(!dep_imp) {
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
                            // import is already a task
                            MUTEX_LOCK(tasks_lock);
                            if(dep_imp->body) {
                                // TODO: Is tasks_lock mutex enough for shared_scopes?
                                ScopeInfo* scope = ast->getScope(imp->body->scopeId);
                                scope->shared_scopes.push_back(ast->getScope(dep_imp->body->scopeId));
                            } else {
                                dep_imp->fixups.push_back(imp);
                                imp->deps_count++;
                            }
                            MUTEX_UNLOCK(tasks_lock);
                        }
                    }
                    task.type = TASK_CHECK_STRUCTS;
                    task.imp = imp;
                    queue_task = true;
                } else {
                    LOGIT(LOGC("[%d]: ",thread_id);
                    log_color(RED); LOGC("Lexer failed: %s\n", task.name.c_str()); log_color(NO_COLOR);)
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
                    LOGIT(LOGC("[%d]: ",thread_id);
                    log_color(RED); LOGC("Checking structs failure: %s\n", task.name.c_str()); log_color(NO_COLOR);)
                    if(ignore_errors) {
                        queue_task = true;
                    } else {
                        // actual failure
                    }
                } else {
                    LOGIT(LOGC("[%d]: ",thread_id);
                    log_color(GREEN); LOGC("Checked structs: %s\n", task.name.c_str()); log_color(NO_COLOR);)
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
                    LOGIT(LOGC("[%d]: ",thread_id);
                    log_color(GREEN); LOGC("Checked functions: %s\n", task.name.c_str()); log_color(NO_COLOR); )
                } else {
                    LOGIT(LOGC("[%d]: ",thread_id);
                    log_color(RED);  LOGC("Checking functions failure: %s\n", task.name.c_str()); log_color(NO_COLOR); )
                }
                
                CheckGlobals(ast, task.imp, bytecode, reporter);
            } break;
            case TASK_GEN_FUNCTIONS: {
                LOGIT(LOGC("[%d]: ",thread_id);
                LOGC("Gen functions: %s\n", task.name.c_str());)
                
                int prev_pieces = bytecode->pieces_unsafe().size();

                for(auto func : task.imp->body->functions)
                    GenerateFunction(ast, func, bytecode, reporter);
                
                #ifdef DEBUG_BYTECODE
                if(bytecode->pieces_unsafe().size() != prev_pieces) {
                    for(int i=prev_pieces;i<bytecode->pieces_unsafe().size();i++) {
                        auto p = bytecode->pieces_unsafe()[i];
                        log_color(GOLD);
                        printf("%s",p->name.c_str());
                        log_color(NO_COLOR);
                        p->print(bytecode);
                    }
                }
                #endif
                
                LOGIT(log_color(GREEN); LOGC("generated functions: %s\n", task.name.c_str()); log_color(NO_COLOR); )
            } break;
            default: Assert(false);
        }
        
        MUTEX_LOCK(tasks_lock);
        atomic_add(&threads_processing, -1);
        if(queue_task) {
            tasks.push_back(task);
            
        }
        // If one thread finished (even if we don't queued a task) something
        // new might have happened where new processing is possible.
        if(!is_signaled) {
            is_signaled = true;
            SEM_SIGNAL(tasks_queue_lock)
        }
        MUTEX_UNLOCK(tasks_lock);
    }
    atomic_add(&total_threads, -1);
    
    // printf("End T\n");
}

void Compiler::init() {
    ast = NEW(AST, HERE);
    reporter = NEW(Reporter, HERE);
    bytecode = NEW(Bytecode, HERE);
    // TODO: Memory leak
}
void Compiler::cleanup() {
    if (ast) {
        DELNEW(ast, AST, HERE);
    }
    if (reporter)   DELNEW(reporter, Reporter, HERE);
    if (bytecode)   DELNEW(bytecode, Bytecode, HERE);

    ast = nullptr;
    reporter = nullptr;
    bytecode = nullptr;

    for(auto it : streams) {
        DELNEW(it, TokenStream, HERE);
    }
    streams.clear();
    stream_map.clear();
}