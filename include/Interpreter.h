#pragma once

#include "Code.h"


struct Interpreter {
    Code* code=nullptr;
    
    u8* stack=nullptr;
    int stack_max=0;
    
    i64 registers[REG_COUNT];
    
    int piece_index = -1;
    CodePiece* piece = nullptr;
    
    void init();
    void execute();
    
    void print_registers(bool subtle = false);
    void print_stack();
    void print_frame(int high, int low);
    
private:
    void run_native_call(NativeCalls callType);
    
    struct Allocation {
        int size;  
    };
    std::unordered_map<void*, Allocation> allocations;
};