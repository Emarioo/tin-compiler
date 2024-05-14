#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

#include "TinGenerator.h"

void print_help() {
    printf("Tin compiler usage:\n");
    printf(" tin <file> : Compile a file (and its imports)\n");
    printf(" tin <file> -run : Compile and execute a file\n");
    printf(" tin <file> -threads <thread_count> : Execute with one or more threads. Note that you should compile the compiler with multithreading disabled when using one thread.\n");
    printf(" tin <file> -gen-code : Generates procedural code in the 'generated' directory.\n");
    // printf(" tin <file> -debug : Compile, execute, and debug a file\n");
    // printf(" tin <file> -log : Log the execution\n");
}

void Measure(CompilerOptions* options = nullptr);

int main(int argc, const char** argv) {
    printf("Start\n");

    #define streq(X,Y) !strcmp(X,Y)

    CompilerOptions options{};

    bool dev_mode = false;
    bool gen_code = false;
    bool measure = false;
    // bool debug_mode = false;
    // bool logging = false;
    
    if(argc < 2) {
        print_help();
        return 0;
    }

    for(int i=1;i<argc;i++) { // i=1 <- skip first argument
        auto arg = argv[i];
        
        if (streq(arg, "-help") || streq(arg, "--help")) {
            print_help();
            return 0;
        } else  if (streq(arg, "-dev")) {
            dev_mode = true;
        } else if(streq(arg, "-run")) {
            options.run = true;
        } else if(streq(arg, "-silent")) {
            options.silent = true;
        } else if(streq(arg, "-gen-code")) {
            gen_code = true;
        } else if(streq(arg, "-measure")) {
            measure = true;
        // } else if(streq(arg, "-debug")) {
        //     debug_mode = true;
        // } else if(streq(arg, "-log")) {
        //     logging = true;
        } else if(streq(arg, "-t") || streq(arg, "-threads")) {
            i++;
            if(i < argc) {
                options.thread_count = atoi(argv[i]);
                // what if not integer?
            } else {
                printf("Missing thread count for %s\n", arg);
                return 0;
            }
        } else {
            if(options.initial_file.empty()) {
                options.initial_file = arg; 
            } else  {
                printf("Unexpected command line option '%s'\n", arg);
                return 1;
            }
        }
    }
    
    if(measure) {
        // config.struct_frequency = { 2, 4 };
        // config.member_frequency = { 5, 10 };
        // config.function_frequency = { 6, 8 };
        // config.argument_frequency = { 3, 5 };
        // config.statement_frequency = { 15, 20 };
        // config.file_count = { 80, 90 };
        // config.seed = 1713988173;
        Measure(&options);
    } else if(!dev_mode) {
        if(options.initial_file.empty()) {
            printf("Missing source file!\n");
            printf(" tin <path_to_source>\n");
            return 1;
        }
        if(gen_code) {
            TinConfig config{};
            config.struct_frequency = { 2, 4 };
            config.member_frequency = { 5, 10 };
            config.function_frequency = { 6, 8 };
            config.argument_frequency = { 3, 5 };
            config.statement_frequency = { 15, 20 };
            config.file_count = { 80, 90 };
            config.seed = 1713988173;

            GenerateTin(&config);
        }
        
        Bytecode* bytecode = nullptr;
        int start_mem = GetAllocatedMemory();
        bool yes = CompileFile(&options, &bytecode);

        if(yes && options.run) {
            Assert(bytecode);
            VirtualMachine* interpreter = new VirtualMachine();
            interpreter->bytecode = bytecode;
            interpreter->init();
            interpreter->execute();
            
            delete interpreter;
        }
        if(bytecode) {
            DELNEW(bytecode, Bytecode, HERE);
        }

        int end_mem = GetAllocatedMemory() - start_mem;
        if(end_mem != 0) {
            PrintMemoryUsage(start_mem);
        }
    } else if(dev_mode) {
        TinConfig config{};
        // IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // This settings were used when measuring for the first time (pessimistic results)
        // DO NOT CHANGE THIS SO THAT WE CAN TEST WITH THE SAME SOURCE FILES WITH AN IMPROVED COMPILER
        config.struct_frequency = { 2, 4 };
        config.member_frequency = { 5, 10 };
        config.function_frequency = { 6, 8 };
        config.argument_frequency = { 3, 5 };
        config.statement_frequency = { 15, 20 };
        config.file_count = { 80, 90 };
        config.seed = 1713988173;

        // More files is worse.
        // config.struct_frequency = { 0, 0 };
        // config.member_frequency = { 5, 10 };
        // config.function_frequency = { 38, 40 };
        // config.argument_frequency = { 3, 5 };
        // config.statement_frequency = { 7, 10 };
        // config.file_count = { 0, 1 };
        // config.seed = 1713988173;
        if(gen_code) {
            // do not comment this out, we allow the user to specify
            // whether to generate code or not
            GenerateTin(&config);
        } else {
            // you can comment this code out, either you always gen code
            // or you comment this out depending on what you need
            // when developing.
            GenerateTin(&config);
        }

        options.initial_file = "generated/main.tin";
        if(options.thread_count == 0)
            // options.thread_count = 8;
            options.thread_count = 1;

        // options.run = true;
        // options.initial_file = "main.tin";
        // options.initial_file = "tests/feature_set.tin";
        // options.initial_file = "sample.tin";
        // options.initial_file = "test.tin";
        // options.initial_file = "tests/file.tin";
        // options.initial_file = "tests/expr.tin";
        // options.initial_file = "tests/structs.tin";
        // options.initial_file = "tests/expr.tin";
        // options.initial_file = "tests/stmts.tin";
        // options.initial_file = "tests/fib.tin";
        // options.initial_file = "tests/loops.tin";
        // options.initial_file = "tests/ptrs.tin";
        // options.initial_file = "tests/scoping.tin";

        int start_mem = GetAllocatedMemory();
        CompileFile(&options);
        int end_mem = GetAllocatedMemory() - start_mem;
        if(end_mem != 0) {
            PrintMemoryUsage(start_mem);
        }

        // Measure();
    }
    printf("Finished\n");
    SleepMS(500); // TCP connection to tracy will close to fast unless we sleep a bit.
    return 0;
}

void Measure(CompilerOptions* user_options) {
    CompilerOptions options;
    if(user_options)
        options.silent = user_options->silent;
    options.initial_file = "generated/main.tin";
    struct Data {
        int threads;
        double time;
        int bytes;
    };
    std::vector<Data> datas;
    int thread_counts[] {
        // 2,4,6,8,10,12,14,16
        // 2,
        // 4,
        // 6,
        // 8,
        // 10,
        // 12,
        // 14,
        16
    };
    int thread_count_len = sizeof(thread_counts)/sizeof(*thread_counts);
    if(user_options) {
        thread_counts[0] = user_options->thread_count;
        thread_count_len = 1;
    }
#ifndef ENABLE_MULTITHREADING
    thread_counts[0] = 1;
    thread_count_len = 1;    
#endif

    for(int i=0;i<thread_count_len;i++) {
        options.thread_count = thread_counts[i];
        int run_count = 5;
        std::vector<double> times;
        for(int j=0;j<run_count;j++) {
            int start_mem = GetAllocatedMemory();
            // printf("%d\n",start_mem);
            CompileFile(&options);
            int end_mem = GetAllocatedMemory() - start_mem;
            if(end_mem != 0) {
                PrintMemoryUsage(start_mem);
            }
            times.push_back(options.compilation_time);
        }
        Data d{};
        d.threads = options.thread_count;
        for(int i=0;i<times.size();i++)
            for(int j=0;j<times.size()-1;j++)
                if(times[j] > times[j+1]) {
                    double tmp = times[j];
                    times[j] = times[j+1];
                    times[j+1] = tmp;
                }
        d.time = times[times.size()/2];
        d.bytes = options.processed_bytes;
        datas.push_back(d);
    }
    log_color(GOLD);
    printf("FINISHED, Summary:\n");
    log_color(NO_COLOR);
    int prev_bytes = -1;
    for(auto& d : datas) {
        float bytes_per_second = d.bytes/d.time;
        printf(" %3d, %5.3f ms, %3.3f MB/s", d.threads, (float)d.time * 1000.f, bytes_per_second / 1024.f / 1024.f);
        if(prev_bytes == -1 || prev_bytes != d.bytes) {
            printf(", (%f MB)", d.bytes / 1024.f / 1024.f);
            prev_bytes = d.bytes;
        } else {
            printf(", (...)");
        }
        printf("\n");
    }
}