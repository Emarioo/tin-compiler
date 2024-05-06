#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

#include "TinGenerator.h"

void print_help() {
    printf("Tin compiler usage:\n");
    printf(" tin <file> : Compile a file (and its imports)\n");
    printf(" tin <file> -run : Compile and execute a file\n");
    printf(" tin <file> -threads <thread_count> : Execute with one or more threads. Note that you should compile the compiler with multithreading disabled when using one thread.\n");
    // printf(" tin <file> -debug : Compile, execute, and debug a file\n");
    // printf(" tin <file> -log : Log the execution\n");
}

int main(int argc, const char** argv) {
    printf("Start\n");

    #define streq(X,Y) !strcmp(X,Y)

    CompilerOptions options{};

    bool dev_mode = false;
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
    
    if(!dev_mode) {
        if(options.initial_file.empty()) {
            printf("Missing source file!\n");
            printf(" tin <path_to_source>\n");
            return 1;
        }
        
        auto program = CompileFile(&options);
        if(program) {
            VirtualMachine* interpreter = new VirtualMachine();
            interpreter->bytecode = program;
            interpreter->init();
            interpreter->execute();
            
            delete interpreter;
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
        // config.struct_frequency = { 2, 4 };
        // config.member_frequency = { 5, 10 };
        // config.function_frequency = { 3, 4 };
        // config.argument_frequency = { 3, 5 };
        // config.statement_frequency = { 5, 5 };
        // config.file_count = { 490, 500 };
        // config.seed = 1713988173;
        
        GenerateTin(&config);

        // options.run = true;
        options.initial_file = "generated/main.tin";
        if(options.thread_count == 0)
            options.thread_count = 5;

        // 1: 1090 ms
        // 2: 600 ms
        // 4: 358 ms
        // 8: 238 ms
        // 16: 230 ms

        // options.initial_file = "main.tin";
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

        CompileFile(&options);
    }
    printf("Finished\n");
    SleepMS(500); // TCP connection to tracy will close to fast unless we sleep a bit.
    return 0;
}