#pragma once

#include "Lexer.h"
#include "AST.h"
#include "Reporter.h"

// This struct contains information
// about where we are in the stream and what
// we should parse next.
struct ParseContext {
    AST* ast = nullptr;
    Reporter* reporter = nullptr;

    int head = 0;
    TokenStream* stream = nullptr;

    Token* gettok(int off = 0) {
        return stream->getToken(head + off, nullptr, nullptr);
    }
    Token* gettok(std::string* str, int off = 0) {
        return stream->getToken(head + off, str, nullptr);
    }
    Token* gettok(std::string* str, int* num, int off = 0) {
        return stream->getToken(head + off, str, num);
    }
    void advance(int n = 1) {
        head++;
    }
    
    ASTStatement* parseIf();

    ASTExpression* parseExpression();
    ASTBody* parseBody();

    ASTStructure* parseStruct();
    ASTFunction* parseFunction();

    std::string parseType();
};

void ParseTokenStream(TokenStream* stream, AST* ast);