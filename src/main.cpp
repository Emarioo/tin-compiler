#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

int main() {
    // TokenStream* stream = lex_file("test.txt");
    // stream->print();
    
    // CompileFile("test.txt");
    // CompileFile("tests/structs.txt");
    // CompileFile("tests/expr.txt");
    // CompileFile("tests/stmts.txt");
    CompileFile("tests/func.txt");
    // CompileFile("tests/loops.txt");

    printf("Finished\n");

    return 0;
}