#pragma once

enum InstructionType {
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
    
};