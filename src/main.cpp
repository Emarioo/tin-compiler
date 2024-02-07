#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

int main() {
    // TokenStream* stream = lex_file("test.tin");
    // stream->print();
    
    // CompileFile("test.tin");
    // CompileFile("tests/structs.tin");
    // CompileFile("tests/expr.tin");
    // CompileFile("tests/stmts.tin");
    CompileFile("tests/fib.tin");
    // CompileFile("tests/loops.tin");

    printf("Finished\n");

    return 0;
}