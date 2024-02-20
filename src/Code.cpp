#include "Code.h"
#include "AST.h"

#define DEF_EMIT2(FN,OP) void CodePiece::emit_##FN(Register r0, Register r1) { emit({INST_##OP, r0, r1}); }
#define DEF_EMIT2F(FN,OP) void CodePiece::emit_##FN(Register r0, Register r1, bool is_float) { emit({INST_##OP, r0, r1, (Register)is_float}); }
#define DEF_EMIT1(FN,OP) void CodePiece::emit_##FN(Register r0) { emit({INST_##OP, r0}); }
#define DEF_EMIT0(FN,OP) void CodePiece::emit_##FN() { emit({INST_##OP}); }

void CodePiece::emit_li(Register reg, int imm) {
    emit({INST_LI,reg});
    emit_imm(imm);
}
void CodePiece::emit_push(Register r0) {
    emit({INST_PUSH, r0});
    virtual_sp -= 8;
}
void CodePiece::emit_pop(Register r0) {
    // This code will remove redundant push and pop.
    // "push a" followed by "pop a" can be removed as they are opposites of each other.
    virtual_sp += 8;
    if(index_of_non_immediates.size() > 0) {
        auto last_inst = get(index_of_non_immediates.back());
        if(last_inst->opcode == INST_PUSH && last_inst->op0 == r0) {
            pop_last_inst();
            return;
        }
    }
    
    emit({INST_POP, r0});
}
void CodePiece::emit_incr(Register reg, int imm) {
    // Assert((imm >> 16) == 0);
    // BUG HERE, 32 bits truncated to 16 bits
    emit({INST_INCR, reg, (Register)(imm&0xFF), (Register)(imm >> 8) }); // we cast to register but it's not actually registers, it's immediate values
    if(reg == REG_SP)
        virtual_sp += imm;
}
void CodePiece::emit_cast(Register reg, CastType castType) {
    emit({INST_CAST, reg, (Register)castType});
}
DEF_EMIT2(mov_rr,MOV_RR)

void CodePiece::emit_mov_mr(Register r0, Register r1, u8 size) {
    Assert(size != 0);
    emit({INST_MOV_MR, r0, r1, (Register)size});
}
void CodePiece::emit_mov_rm(Register r0, Register r1, u8 size) {
    Assert(size != 0);
    emit({INST_MOV_RM, r0, r1, (Register)size});
}
void CodePiece::emit_mov_mr_disp(Register to_reg, Register from_reg, u8 size, int offset) {
    Assert(size != 0);
    if(offset == 0) {
        emit_mov_mr(to_reg, from_reg, size);
    } else {
        emit({INST_MOV_MR_DISP, to_reg, from_reg, (Register)size});
        emit_imm(offset);
        // emit_li(REG_T0, offset);
        // emit_add(REG_T0, to_reg);
        // emit_mov_mr(REG_T0, from_reg, size);
    }
}
void CodePiece::emit_mov_rm_disp(Register to_reg, Register from_reg, u8 size, int offset) {
    Assert(size != 0);
    if(offset == 0) {
        emit_mov_rm(to_reg, from_reg, size);
    } else {
        emit({INST_MOV_RM_DISP, to_reg, from_reg, (Register)size});
        emit_imm(offset);
        // emit_li(REG_T0, offset);
        // emit_add(REG_T0, from_reg);
        // emit_mov_rm(to_reg, REG_T0, size);
    }
}

DEF_EMIT2F(add, ADD)
DEF_EMIT2F(sub, SUB)
DEF_EMIT2F(mul, MUL)
DEF_EMIT2F(div, DIV)
DEF_EMIT2(and, AND)
DEF_EMIT2(or , OR)
DEF_EMIT2(not, NOT)
DEF_EMIT2F(eq, EQUAL)
DEF_EMIT2F(neq, NOT_EQUAL)
DEF_EMIT2F(less, LESS)
DEF_EMIT2F(greater, GREATER)
DEF_EMIT2F(less_equal, LESS_EQUAL)
DEF_EMIT2F(greater_equal, GREATER_EQUAL)

void CodePiece::emit_jmp(int pc) {
    emit({INST_JMP});
    emit_imm(pc - get_pc());
}
void CodePiece::emit_jmp(int* out_index_of_imm) {
    emit({INST_JMP});
    *out_index_of_imm = get_pc();
    emit_imm(0);
}
void CodePiece::emit_jz(Register reg, int pc) {
    emit({INST_JZ, reg});
    emit_imm(pc - get_pc());
}
void CodePiece::emit_jz(Register reg, int* out_index_of_imm) {
    emit({INST_JZ, reg});
    *out_index_of_imm = get_pc();
    emit_imm(0);
}

void CodePiece::emit_call(int* out_index_of_imm) {
    emit({INST_CALL});
    *out_index_of_imm = get_pc();
    emit_imm(0);
}
DEF_EMIT0(ret,RET)
void CodePiece::emit_memzero(Register reg, Register reg_size) {
    emit({INST_MEMZERO, reg, reg_size});
}
void CodePiece::fix_jump_here(int imm_index) {
    *(int*)&instructions[imm_index] = get_pc() - imm_index;
}
// void CodePiece::fix_jump_to(int imm_index, int pc) {
//     *(int*)&instructions[imm_index] = pc - imm_index;
// }

const char* opcode_names[] {
    "halt",             // INST_HALT
    "nop",              // INST_NOP
    
    "cast",
    "mov_rr",           // INST_MOV_RR
    "mov_mr",           // INST_MOV_MR
    "mov_rm",           // INST_MOV_RM
    
    "push",             // INST_PUSH
    "pop",              // INST_POP
    "incr",              // INST_INCR
    
    "add",              // INST_ADD
    "sub",              // INST_SUB
    "mul",              // INST_MUL
    "div",              // INST_DIV
    "and",              // INST_AND
    "or",               // INST_OR
    "not",              // INST_NOT
    
    "equal",            // INST_EQUAL,
    "not_equal",        // INST_NOT_EQUAL,
    "less",             // INST_LESS,
    "greater",          // INST_GREATER,
    "less_equal",       // INST_LESS_EQUAL,
    "greater_equal",    // INST_GREATER_EQUAL,
    
    "ret",              // INST_RET
    "memzero",             // INST_CALL
    
    // INSTRUCTIONS WITH IMMEDIATES
    "li",               // INST_LI
    "jmp",              // INST_JMP
    "jz",               // INST_JZ
    "call",             // INST_CALL
    "mov_mr_disp",           // INST_MOV_MR_DISP
    "mov_rm_disp",           // INST_MOV_RM_DISP
    
};
const char* register_names[] {
    "invalid", // REG_INVALID
    "a", // REG_A
    "b", // REG_B
    "c", // REG_C
    "d", // REG_D
    "e", // REG_E
    "f", // REG_F
    "sp", // REG_SP
    "bp", // REG_BP
    "pc", // REG_PC
    "t0", // REG_T0
    "t1", // REG_T1
};

void CodePiece::print(int low_index, int high_index, Code* code) {
    if(high_index == -1 || high_index > instructions.size())
        high_index = instructions.size();
        
    for(int i=low_index;i<high_index;i++) {
        Instruction inst = instructions[i];
        log_color(Color::GRAY);
        printf(" %3d:", i);
        log_color(Color::PURPLE);
        printf(" %s", opcode_names[inst.opcode]);
        log_color(Color::NO_COLOR);
        
        if(inst.opcode == INST_INCR) {
            int imm = ((i8)inst.op1) | ((i8)inst.op2<<8);
            printf(" %s, %d", register_names[inst.op0], imm);
        } else {
            if(inst.op0) printf(" %s", register_names[inst.op0]);
            if(inst.op1) printf(", %s", register_names[inst.op1]);
            if(inst.op2 && (inst.opcode == INST_MOV_MR || inst.opcode == INST_MOV_RM || inst.opcode == INST_MOV_MR_DISP || inst.opcode == INST_MOV_RM_DISP)) {
                if(inst.op2 == 1) printf(", byte");
                if(inst.op2 == 2) printf(", word");
                if(inst.op2 == 4) printf(", dword");
                if(inst.op2 == 8) printf(", qword");
            }
        }
        
        if(inst.opcode == INST_LI || inst.opcode == INST_JMP || inst.opcode == INST_JZ || inst.opcode == INST_CALL || inst.opcode == INST_MOV_MR_DISP || inst.opcode == INST_MOV_RM_DISP) {
            i++;
            int imm = *(int*)&instructions[i];
            if(code && inst.opcode == INST_CALL) {
                log_color(Color::GREEN);
                if(imm < 0) {
                    printf(" %s", NAME_OF_NATIVE(imm + NATIVE_MAX));
                } else {
                    printf(" %s", code->pieces[imm-1]->name.c_str());
                }
            } else if(inst.opcode == INST_JMP || inst.opcode == INST_JZ) {
                printf(", ");
                int addr = imm + i;
                log_color(Color::GRAY);
                printf("%d:", addr);
            } else {
                printf(", ");
                log_color(Color::GREEN);
                printf("%d",imm);
            }
            log_color(Color::NO_COLOR);
        }
        if(high_index - low_index > 1)
            printf("\n");
    }
}
void Code::print() {
    log_color(Color::GOLD);
    printf("Printing Code:\n");
    log_color(Color::NO_COLOR);
    if(pieces.size() == 0)
    printf(" No pieces (generated functions)\n");
    for(int i=0;i<pieces.size();i++) {
        auto p = pieces[i];
        log_color(Color::GOLD);
        printf("%s[%d]:\n", p->name.c_str(), (int)p->instructions.size());
        log_color(Color::NO_COLOR);
        p->print();
    }
}
void Code::apply_relocations() {
    for(auto p : pieces) {
        for(const auto& rel : p->relocations) {
            if(rel.function) {
                *(int*)&p->instructions[rel.index_of_immediate] = rel.function->piece_code_index + 1; // +1 because 0 is seen as invalid
            } else {
                printf("Function was null when applying relocations\n");
            }
            
            // int piece_index = -1;
            // for(int i=0;i<pieces.size();i++) {
            //     if(rel.func_name == pieces[i]->name) {
            //         piece_index = i;
            //         break;
            //     }
            // }
            // if(piece_index != -1) {
            //     *(int*)&p->instructions[rel.index_of_immediate] = piece_index + 1; // +1 because 0 is seen as invalid
            // } else {
            //     int found = -1;
            //     for(int j=0;j<NATIVE_MAX;j++) {
            //         if(!strcmp(native_names[j], rel.func_name.c_str())) {
            //             found = j;
            //             break;
            //         }
            //     }
            //     if(found != -1) {
            //         *(int*)&p->instructions[rel.index_of_immediate] = found - NATIVE_MAX;
            //         continue;
            //     }
            //     // Probably caused by an error in which case we don't need
            //     //  to print a message here since it's probably already covered
            //     printf("'%s' is not a function and relocations to it cannot be applied\n", rel.func_name.c_str());
            // }
        }
    }
}
const char* native_names[] {
    "printi", // NATIVE_printi
    "printf", // NATIVE_printf
    "malloc",
    "mfree",
};