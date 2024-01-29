#include <stdio.h>

#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"

int main() {
    // TokenStream* stream = lex_file("test.txt");
    // stream->print();
    
    CompileFile("test.txt");
}