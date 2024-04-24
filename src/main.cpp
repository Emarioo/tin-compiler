#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

#include "TinGenerator.h"

// #include "Windows.h"

void print_help() {
    printf("Tin compiler usage:\n");
    printf(" tin <file> : Compile a file (and its imports)\n");
    printf(" tin <file> -run : Compile and execute a file\n");
    printf(" tin <file> -debug : Compile, execute, and debug a file\n");
    printf(" tin <file> -log : Log the execution\n");
}

int main(int argc, const char** argv) {
    printf("Start\n");

    #define streq(X,Y) !strcmp(X,Y)

    bool dev_mode = false;
    bool run_vm = false;
    bool debug_mode = false;
    bool logging = false;
    
    const char* source_file = nullptr;

    if(argc < 2) {
        print_help();
        return 0;
    }

    for(int i=0;i<argc;i++) {
        auto arg = argv[i];
        
        if (streq(arg, "-help") || streq(arg, "--help")) {
            print_help();
            return 0;
        } else  if (streq(arg, "-dev")) {
            dev_mode = true;
        } else if(streq(arg, "-run")) {
            run_vm = true;
        } else if(streq(arg, "-debug")) {
            debug_mode = true;
        } else if(streq(arg, "-log")) {
            logging = true;
        } else {
            if(!source_file) {
                source_file = arg; 
            } else  {
                printf("Unexpected command line option '%s'\n", arg);
                return 1;
            }
        }
    }
    
    if(!dev_mode) {
        if(!source_file) {
            printf("Missing source file!\n");
            printf(" tin <path_to_source>\n");
            return 1;
        }
        
        auto program = CompileFile(source_file);
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
        config.struct_frequency = { 1, 1 };
        config.member_frequency = { 1, 1 };
        config.function_frequency = { 1, 1 };
        config.argument_frequency = { 1, 3 };
        config.statement_frequency = { 10, 20 };
        config.file_count = { 15, 50 };
        config.seed = 1713988173;
        
        GenerateTin(&config);

        // if(argc == 2 && !strcmp(argv[1], "-enable-logging")) {

        // }

        // CompileFile("main.tin", false);
        CompileFile("generated/main.tin", false);
        // CompileFile("sample.tin", false);
        // CompileFile("test.tin", true);
        // CompileFile("tests/file.tin", true);
        // CompileFile("tests/expr.tin", true);
        // CompileFile("tests/structs.tin", true);
        // CompileFile("tests/expr.tin", true);
        // CompileFile("tests/stmts.tin", true);
        // CompileFile("tests/fib.tin", true);
        // CompileFile("tests/loops.tin", true);
        // CompileFile("tests/ptrs.tin", true);
        // CompileFile("tests/scoping.tin", true);

        printf("Finished\n");
        
    }
    SleepMS(500); // TCP connection to tracy will close to fast unless we sleep a bit.
        // Sleep(500); // TCP connection to tracy will close to fast unless we sleep a bit.
    return 0;
}