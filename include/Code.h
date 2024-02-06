#pragma once

#include "Util.h"

enum Opcode : u8 {
    INST_HALT=0,
    INST_NOP,

    INST_MOV_RR, // reg <- reg
    INST_MOV_MR, // memory <- reg
    INST_MOV_RM, // reg <- memory

    INST_PUSH,
    INST_POP,

    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_AND,
    INST_OR,
    INST_NOT,
    INST_EQUAL,
    INST_NOT_EQUAL,
    INST_LESS,
    INST_GREATER,
    INST_LESS_EQUAL,
    INST_GREATER_EQUAL,

    INST_RET, // return
    
    // instructions with immediates
    INST_IMMEDIATES,
    INST_LI = INST_IMMEDIATES, // load immediate
    INST_JMP, // jump
    INST_JZ, // conditional jump (if zero)
    INST_CALL, // function call
    
};
enum Register : u8 {
    REG_INVALID=0,
    REG_A, // accumulator
    REG_B, // pointer arithmetic
    REG_C,
    REG_D,
    REG_E,
    REG_F,
    
    REG_SP,
    REG_BP,
    REG_PC,
    
    REG_T0, // temporary internal registers
    REG_T1,
    
    REG_COUNT,
};
extern const char* opcode_names[];
extern const char* register_names[];
struct Instruction {
    Opcode opcode;
    union {
        struct {
            Register op0,op1,op2;
        };
        Register operands[3];
    };
};
struct Code;
struct CodePiece {
    std::string name;
    std::vector<Instruction> instructions;
    
    std::vector<int> line_of_instruction;
    struct Line {
        int line_number;
        std::string text;
    };
    std::vector<Line> lines;
    
    void push_line(int line, std::string text) {
        lines.push_back({line, text});
    }
    
    struct Relocation {
        std::string func_name;
        int index_of_immediate; // immediate of INST_CALL instruction
    };
    std::vector<Relocation> relocations;

    void emit_li(Register reg, int imm);
    void emit_push(Register reg);
    void emit_pop(Register reg);
    
    void emit_mov_rr(Register to_reg, Register from_reg);
    void emit_mov_mr(Register to_reg, Register from_reg, u8 size);
    void emit_mov_rm(Register to_reg, Register from_reg, u8 size);

    void emit_add(Register to_from_reg, Register from_reg);
    void emit_sub(Register to_from_reg, Register from_reg);
    void emit_mul(Register to_from_reg, Register from_reg);
    void emit_div(Register to_from_reg, Register from_reg);
    void emit_and(Register to_from_reg, Register from_reg);
    void emit_or(Register to_from_reg, Register from_reg);
    void emit_not(Register to_reg, Register from_reg);

    void emit_eq(Register to_from_reg, Register from_reg);
    void emit_neq(Register to_from_reg, Register from_reg);
    void emit_less(Register to_from_reg, Register from_reg);
    void emit_greater(Register to_from_reg, Register from_reg);
    void emit_less_equal(Register to_from_reg, Register from_reg);
    void emit_greater_equal(Register to_from_reg, Register from_reg);
    
    void emit_jmp(int pc);
    void emit_jmp(int* out_index_of_imm);
    void emit_jz(Register reg, int pc);
    void emit_jz(Register reg, int* out_index_of_imm);
    
    void emit_call(int* out_index_of_imm);
    void emit_ret();

    int get_pc() { return instructions.size(); }
    void fix_jump_here(int imm_index);
    // void fix_jump_to(int imm_index, int pc);
    
    Instruction* get(int index) { return &instructions[index]; }
    
    void print(int low_index = 0, int high_index = -1, Code* code = nullptr);
    
private:
    std::vector<int> index_of_non_immediates;
    
    void emit(Instruction inst) {
        index_of_non_immediates.push_back(instructions.size());
        instructions.push_back(inst);
        
        line_of_instruction.resize(instructions.size());
        line_of_instruction[instructions.size()-1] = lines.size()-1;
    }
    void emit_imm(int imm){
        instructions.push_back(*(Instruction*)&imm);
    }
    void pop_last_inst() {
        Assert(index_of_non_immediates.size() > 0);
        int index = index_of_non_immediates.back();
        index_of_non_immediates.pop_back();
        while(instructions.size() > index) {
            instructions.pop_back();
        }
    }
};

struct Code {
    std::vector<CodePiece*> pieces;
    
    CodePiece* createPiece() {
        auto ptr = new CodePiece();
        pieces.push_back(ptr);
        return ptr;
    }
    
    void apply_relocations();
    
    void print();
};

enum NativeCalls {
    NATIVE_START = -200,
    NATIVE_printi = NATIVE_START,
};
#define NAME_OF_NATIVE(X) native_names[X - NATIVE_START]
extern const char* native_names[];