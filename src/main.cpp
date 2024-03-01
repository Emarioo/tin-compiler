#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

#include "Windows.h"

int main() {
    // TokenStream* stream = lex_file("test.tin");
    // stream->print();
    
    // CompileFile("test.tin");
    // CompileFile("tests/expr.tin");
    CompileFile("tests/file.tin");
    // CompileFile("tests/structs.tin");
    // CompileFile("tests/expr.tin");
    // CompileFile("tests/stmts.tin");
    // CompileFile("tests/fib.tin");
    // CompileFile("tests/loops.tin");
    // CompileFile("tests/ptrs.tin");
    // CompileFile("tests/scoping.tin");

    printf("Finished\n");
    
    Sleep(500); // TCP connection to tracy will close to fast without sleeping a bit.
    
    return 0;
}