#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

#include "TinGenerator.h"

// #include "Windows.h"

int main(int argc, const char** argv) {
    printf("Start\n");

    TinConfig config{};
    config.struct_frequency = { 1, 1 };
    config.member_frequency = { 1, 1 };
    config.function_frequency = { 1, 1 };
    config.argument_frequency = { 1, 3 };
    config.statement_frequency = { 2, 10 };
    GenerateTin(&config);

    // if(argc == 2 && !strcmp(argv[1], "-enable-logging")) {

    // }

    // CompileFile("test.tin");
    // CompileFile("tests/file.tin");
    // // CompileFile("tests/expr.tin");
    // // CompileFile("tests/structs.tin");
    // // CompileFile("tests/expr.tin");
    // // CompileFile("tests/stmts.tin");
    // // CompileFile("tests/fib.tin");
    // // CompileFile("tests/loops.tin");
    // // CompileFile("tests/ptrs.tin");
    // // CompileFile("tests/scoping.tin");

    printf("Finished\n");
    
    SleepMS(500); // TCP connection to tracy will close to fast unless we sleep a bit.
    // Sleep(500); // TCP connection to tracy will close to fast unless we sleep a bit.
    
    return 0;
}