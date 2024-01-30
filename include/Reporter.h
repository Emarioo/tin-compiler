#pragma once

#include "Lexer.h"

/*
    Struct that handles error reporting
*/
struct Reporter {
    void err(Token* token, const char* msg) {
        Assert(token);
        // TODO: Color
        const char* file = "?";
        if(token->file)
            file = token->file->c_str();
        printf("ERROR %s:%d:%d: %s\n",file, token->line, token->column, msg);
    }
};