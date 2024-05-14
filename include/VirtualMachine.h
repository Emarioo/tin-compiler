#pragma once

#include "Bytecode.h"


struct VirtualMachine {
    ~VirtualMachine() {
        DELNEW_ARRAY(stack, u8, stack_max, HERE);
        stack = nullptr;
        stack_max = 0;
        DELNEW_ARRAY(global_data, u8, global_data_max, HERE);
        global_data = nullptr;
        global_data_max = 0;
        bytecode = nullptr;
        piece = nullptr;
    }
    Bytecode* bytecode=nullptr;
    
    u8* global_data = nullptr;
    int global_data_max = 0;
    
    u8* stack = nullptr;
    int stack_max = 0;
    
    i64 registers[REG_COUNT];
    
    int piece_index = -1;
    BytecodePiece* piece = nullptr;
    
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