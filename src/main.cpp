#include <stdio.h>

#include "Lexer.h"

int main() {
    // printf("Hello world\n");

    auto tokens = lex_file("test.txt");

    tokens->print();
}