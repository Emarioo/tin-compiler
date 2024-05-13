#pragma once

#include "Lexer.h"

/*
    Struct that handles error reporting
*/
struct Reporter {
    volatile int errors = 0;
    // MUTEX_DECL(print_lock);
    void err(Token* token, const std::string& msg) {
        Assert(token);
        atomic_add(&errors,1);
        // TODO: Color
        const char* file = "?";
        if(token->file)
            file = token->file->c_str();
        // MUTEX_LOCK(print_lock);
        log_color(Color::RED);
        printf("ERROR %s:%d:%d: ",file, token->line, token->column);
        log_color(Color::NO_COLOR);
        printf("%s\n", msg.c_str());
        // fflush(stdout);
        // MUTEX_UNLOCK(print_lock);
    }
    void err(const std::string& path, int line, int column, const std::string& msg) {
        Assert(token);
        atomic_add(&errors,1);
        // TODO: Color
        // MUTEX_LOCK(print_lock);
        log_color(Color::RED);
        printf("ERROR %s:%d:%d: ",path.c_str(), line, column);
        log_color(Color::NO_COLOR);
        printf("%s\n", msg.c_str());
        // fflush(stdout);
        // MUTEX_UNLOCK(print_lock);
    }
    void err(TokenStream* stream, SourceLocation loc, const std::string& msg) {
        Assert(stream);
        atomic_add(&errors,1);
        
        auto token = stream->getToken(loc);
        
        // TODO: Color
        const char* file = "?";
        if(token->file)
            file = token->file->c_str();
            
        // MUTEX_LOCK(print_lock);
        log_color(Color::RED);
        printf("ERROR %s:%d:%d: ",file, token->line, token->column);
        log_color(Color::NO_COLOR);
        printf("%s\n", msg.c_str());
        // fflush(stdout);
        // MUTEX_UNLOCK(print_lock);
    }
};