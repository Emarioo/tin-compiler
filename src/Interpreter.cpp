#include "Interpreter.h"

void Interpreter::init() {
    int max = 0x10000;
    stack = (u8*)malloc(max);
    Assert(stack);
    stack_max = max;
}

void Interpreter::execute() {
    code->apply_relocations();
    
    memset(registers,0,sizeof(registers));
    
    registers[REG_SP] = (i64)(stack + stack_max); // stack starts at the top and grows down
    
    int piece_index = 0;
    CodePiece* piece = code->pieces[piece_index];
    
        // printf("Interpreter: Stack overflow (sp: %ld, stack range: %d - %d)\n", (i64)registers[REG_SP], (int)stack_max, 0);
    #define CHECK_STACK if(registers[REG_SP] > (i64)stack + stack_max || registers[REG_SP] < (i64)stack) {\
        printf("\n");\
        log_color(Color::RED);\
        printf("Interpreter: Stack overflow (sp: %lld, stack range: %d - %d)\n", (i64)registers[REG_SP], (int)stack_max, 0);\
        log_color(Color::NO_COLOR);\
        return;\
    }
    auto mov=[&](int size, void* a, void* b) {
        switch(size){
        case 1: *(u8*)a = *(u8*)b; break;
        case 2: *(u16*)a = *(u16*)b; break;
        case 4: *(u32*)a = *(u32*)b; break;
        case 8: *(u64*)a = *(u64*)b; break;
        default: Assert(false);
        }
    };
    
    log_color(GOLD);
    printf("Interpreter:\n");
    log_color(NO_COLOR);
    bool running = true;
    while(running) {
        if(registers[REG_PC] >= piece->instructions.size()) {
            log_color(Color::RED);
            printf("INTERPRETER: PC out of bounds (pc: %d, piece instructions: %d\n", (int)registers[REG_PC], (int)piece->instructions.size());
            log_color(Color::NO_COLOR);
            break;
        }
        int prev_pc = registers[REG_PC];
        Instruction inst = piece->instructions[registers[REG_PC]];
        registers[REG_PC]++;
        
        int imm = 0;
        if(inst.opcode >= INST_IMMEDIATES) {
            imm = *(int*)&piece->instructions[registers[REG_PC]];
            registers[REG_PC]++;
        }
        
        // log_color(Color::GRAY);
        // printf(" %d:", prev_pc);
        // log_color(Color::PURPLE);
        // printf(" %s ",opcode_names[inst.opcode]);
        // log_color(Color::NO_COLOR);
        piece->print(prev_pc, prev_pc+1);
        
        switch(inst.opcode) {
        case INST_MOV_RR: {
            registers[inst.op0] = registers[inst.op1];
            break;
        }
        case INST_MOV_MR: {
            mov(inst.op2, (void*)registers[inst.op0], &registers[inst.op1]);
            break;
        }
        case INST_MOV_RM: {
            mov(inst.op2, &registers[inst.op0], (void*)registers[inst.op1]);
            break;
        }
        case INST_LI: {
            registers[inst.op0] = imm;
            break;
        }
        case INST_PUSH: {
            registers[REG_SP] -= 8;
            CHECK_STACK
            *(i64*)registers[REG_SP] = registers[inst.op0];
            break;
        }
        case INST_POP: {
            registers[inst.op0] = *(i64*)registers[REG_SP];
            registers[REG_SP] += 8;
            CHECK_STACK
            break;
        }
        case INST_CALL: {
            registers[REG_SP] -= 8;
            CHECK_STACK
            *(i64*)registers[REG_SP] = registers[REG_PC];
            
            registers[REG_SP] -= 8;
            CHECK_STACK
            *(i64*)registers[REG_SP] = piece_index;
            
            registers[REG_PC] = 0;
            piece_index = imm - 1;
            piece = code->pieces[piece_index];
            break;
        }
        case INST_RET: {
            if(registers[REG_SP] == (i64)stack + stack_max) {
                running = false;
                break;
            }
            
            piece_index = *(i64*)registers[REG_SP];
            registers[REG_SP] += 8;
            CHECK_STACK
            
            registers[REG_PC] = *(i64*)registers[REG_SP];
            registers[REG_SP] += 8;
            CHECK_STACK
            
            piece = code->pieces[piece_index];
            break;
        }
        case INST_JMP: {
            registers[REG_PC] += imm -1; // -1 because imm is relative to the immediates address and not the end of the jump instruction. See CodePiece::fix_jump_here for specifics.
            break;
        }
        case INST_JZ: {
            if(registers[inst.op0] == 0) {
                registers[REG_PC] += imm -1; // -1 because imm is relative to the immediates address and not the end of the jump instruction. See CodePiece::fix_jump_here for specifics.
            }
            break;
        }
        case INST_ADD: registers[inst.op0] = registers[inst.op0] + registers[inst.op1]; break;
        case INST_SUB: registers[inst.op0] = registers[inst.op0] - registers[inst.op1]; break;
        case INST_MUL: registers[inst.op0] = registers[inst.op0] * registers[inst.op1]; break;
        case INST_DIV: registers[inst.op0] = registers[inst.op0] / registers[inst.op1]; break;
        case INST_AND: registers[inst.op0] = registers[inst.op0] && registers[inst.op1]; break;
        case INST_OR:  registers[inst.op0] = registers[inst.op0] || registers[inst.op1]; break;
        case INST_NOT: registers[inst.op0] = !registers[inst.op1]; break;
        default: Assert(("Incomplete instruction",false));
        }
        
        if(inst.op0 != REG_INVALID) {
            if(inst.opcode == INST_MOV_MR) {
                i64 tmp = 0;
                mov((u8)inst.op2, &tmp, (void*)registers[inst.op0]);
                log_color(GRAY);
                printf("  *%s = %lld ", register_names[inst.op0], tmp);
                // printf("  *%s = %ld ", register_names[inst.op0], tmp);
                log_color(NO_COLOR);
            } else {
                log_color(GRAY);
                // printf("  %s = %ld ", register_names[inst.op0], registers[inst.op0]);
                printf("  %s = %lld ", register_names[inst.op0], registers[inst.op0]);
                log_color(NO_COLOR);
            }
        }
        
        printf("\n");
    }
    log_color(Color::GOLD);
    printf("Registers:\n");
    log_color(Color::NO_COLOR);
    for(int i=REG_A;i<=REG_F;i++) {
        log_color(Color::CYAN);
        printf(" %s", register_names[i]);
        log_color(Color::GRAY);
        printf(" = ");
        log_color(Color::GREEN);
        // printf("%ld\n", registers[i]);
        printf("%lld\n", registers[i]);
        log_color(Color::NO_COLOR);
    }
}