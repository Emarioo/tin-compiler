#include <stdio.h>

#include "Lexer.h"
#include "Parser.h"

int main() {
    // printf("Hello world\n");

    TokenStream* stream = lex_file("test.txt");

    ParseTokenStream(stream);

    stream->print();
}