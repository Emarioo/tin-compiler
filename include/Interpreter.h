#pragma once

#include "Code.h"


struct Interpreter {
    Code* code=nullptr;
    
    u8* stack=nullptr;
    int stack_max=0;
    
    i64 registers[REG_COUNT];
    
    void init();
    void execute();
};