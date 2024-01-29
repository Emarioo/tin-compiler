#pragma once

#include "Lexer.h"

// This struct contains information
// about where we are in the stream and what
// we should parse next.
struct ParseContext {

    int head = 0;
    TokenStream* stream = nullptr;

    Token* gettok(int off = 0) {
        return stream->getToken(head + off, nullptr, nullptr);
    }
    Token* gettok(int off, std::string** str, int** num) {
        return stream->getToken(head + off, str, num);
    }
    void advance(int n = 1) {
        head++;
    }
};

void ParseTokenStream(TokenStream* stream);