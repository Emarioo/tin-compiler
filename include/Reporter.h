#pragma once

#include "Lexer.h"

/*
    Struct that handles error reporting
*/
struct Reporter {
    int errors = 0;
    void err(Token* token, const std::string& msg) {
        Assert(token);
        errors++;
        // TODO: Color
        const char* file = "?";
        if(token->file)
            file = token->file->c_str();
            
        log_color(Color::RED);
        printf("ERROR %s:%d:%d: ",file, token->line, token->column);
        log_color(Color::NO_COLOR);
        printf("%s\n", msg.c_str());
    }
    void err(TokenStream* stream, SourceLocation loc, const std::string& msg) {
        Assert(stream);
        errors++;
        
        auto token = stream->getToken(loc);
        
        // TODO: Color
        const char* file = "?";
        if(token->file)
            file = token->file->c_str();
            
        log_color(Color::RED);
        printf("ERROR %s:%d:%d: ",file, token->line, token->column);
        log_color(Color::NO_COLOR);
        printf("%s\n", msg.c_str());
    }
};