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

    ScopeId current_scopeId;

    Token* gettok(int off = 0) {
        return stream->getToken(head + off, nullptr, nullptr, nullptr);
    }
    Token* gettok(std::string* str, int off = 0) {
        return stream->getToken(head + off, str, nullptr, nullptr);
    }
    Token* gettok(std::string* str, int* num, float* dec, int off = 0) {
        return stream->getToken(head + off, str, num, dec);
    }
    void advance(int n = 1) {
        head += n;
    }
    SourceLocation getloc(int off = 0) {
        return { head + off };
    }
    
    ASTStatement* parseIf();
    ASTStatement* parseWhile();
    ASTStatement* parseVarDeclaration();

    ASTExpression* parseExpression();
    ASTBody* parseBody();

    ASTStructure* parseStruct();
    ASTFunction* parseFunction();

    std::string parseType();
};

AST::Import* ParseTokenStream(TokenStream* stream, AST* ast, Reporter* reporter);