#pragma once

enum Opcode : u8 {
    INST_HALT,
    INST_NOP,

    INST_MOV_RR, // reg <- reg
    INST_MOV_MR, // memory <- reg
    INST_MOV_RM, // reg <- memory

    INST_PUSH,
    INST_POP,
    INST_LI,

    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,

    INST_JMP,
    INST_CMP,
    INST_JE,
};

struct Code {
    // instructions bytes
    std::vector<u8> bytes;

    void emit_li(int reg, int imm);
    void emit_mov_rr(int to_reg, int from_reg);

    void emit_add(int to_reg, int from_reg);
    void emit_sub(int to_reg, int from_reg);
    void emit_mul(int to_reg, int from_reg);
    void emit_div(int to_reg, int from_reg);

    
};