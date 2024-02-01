#include "Code.h"

#define DEF_EMIT2(FN,OP) void CodePiece::emit_##FN(Register r0, Register r1) { emit({INST_##OP, r0, r1}); }
#define DEF_EMIT1(FN,OP) void CodePiece::emit_##FN(Register r0) { emit({INST_##OP, r0}); }
#define DEF_EMIT0(FN,OP) void CodePiece::emit_##FN() { emit({INST_##OP}); }

void CodePiece::emit_li(Register reg, int imm) {
    emit({INST_LI,reg});
    emit_imm(imm);
}
void CodePiece::emit_push(Register r0) {
    emit({INST_PUSH, r0});
}
void CodePiece::emit_pop(Register r0) {
    // This code will remove redundant push and pop.
    // "push a" followed by "pop a" can be removed as they are opposites of each other.
    if(index_of_non_immediates.size() > 0) {
        auto last_inst = get(index_of_non_immediates.back());
        if(last_inst->opcode == INST_PUSH && last_inst->op0 == r0) {
            pop_last_inst();
            return;
        }
    }
    
    emit({INST_POP, r0});
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

DEF_EMIT2(add, ADD)
DEF_EMIT2(sub, SUB)
DEF_EMIT2(mul, MUL)
DEF_EMIT2(div, DIV)
DEF_EMIT2(and, AND)
DEF_EMIT2(or , OR)
DEF_EMIT2(not, NOT)

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

void CodePiece::fix_jump_here(int imm_index) {
    *(int*)&instructions[imm_index] = get_pc() - imm_index;
}
// void CodePiece::fix_jump_to(int imm_index, int pc) {
//     *(int*)&instructions[imm_index] = pc - imm_index;
// }

const char* opcode_names[] {
    "halt", // INST_HALT
    "nop", // INST_NOP
    
    "mov_rr", // INST_MOV_RR
    "mov_mr", // INST_MOV_MR
    "mov_rm", // INST_MOV_RM
    
    "push", // INST_PUSH
    "pop", // INST_POP
    
    "add", // INST_ADD
    "sub", // INST_SUB
    "mul", // INST_MUL
    "div", // INST_DIV
    "and", // INST_AND
    "or", // INST_OR
    "not", // INST_NOT
    
    "ret", // INST_RET
    
    "li", // INST_LI
    "jmp", // INST_JMP
    "jz", // INST_JZ
    "call", // INST_CALL
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
};

void CodePiece::print(int low_index, int high_index) {
    if(high_index == -1 || high_index > instructions.size())
        high_index = instructions.size();
        
    for(int i=low_index;i<high_index;i++) {
        Instruction inst = instructions[i];
        log_color(Color::GRAY);
        printf(" %3d:", i);
        log_color(Color::PURPLE);
        printf(" %s", opcode_names[inst.opcode]);
        log_color(Color::NO_COLOR);
        
        if(inst.op0) printf(" %s", register_names[inst.op0]);
        if(inst.op1) printf(", %s", register_names[inst.op1]);
        if(inst.op2 && (inst.opcode == INST_MOV_MR || inst.opcode == INST_MOV_RM)) {
            if(inst.op2 == 1) printf(", byte");
            if(inst.op2 == 2) printf(", word");
            if(inst.op2 == 4) printf(", dword");
            if(inst.op2 == 8) printf(", qword");
        }
        
        if(inst.opcode == INST_LI || inst.opcode == INST_JMP || inst.opcode == INST_JZ || inst.opcode == INST_CALL) {
            i++;
            int imm = *(int*)&instructions[i];
            printf(", ");
            log_color(Color::GREEN);
            if(inst.opcode == INST_JMP || inst.opcode == INST_JZ || inst.opcode == INST_CALL) {
                int addr = imm + i;
                log_color(Color::GRAY);
                printf("%d:", addr);
            } else {
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
            int piece_index = -1;
            for(int i=0;i<pieces.size();i++) {
                if(rel.func_name == pieces[i]->name) {
                    piece_index = i;
                    break;   
                }
            }
            if(piece_index != -1) {
                *(int*)&p->instructions[rel.index_of_immediate] = piece_index +1; // +1 because 0 is seen as invalid
            } else {
                // Probably caused by an error in which case we don't need
                //  to print a message here since it's probably already covered
                // printf("Function name for relocation not found\n");
            }
        }
    }
}