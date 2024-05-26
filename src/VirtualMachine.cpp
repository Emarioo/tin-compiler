#include "VirtualMachine.h"

#ifdef OS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include "Windows.h"
    #include <intrin.h>
#endif

#define LOG(X) if(enable_logging || interactive) { X; }
// #define LOG(X)

void VirtualMachine::init() {
    int max = 0x10000;
    stack = NEW_ARRAY(u8, max, HERE);
    Assert(stack);
    stack_max = max;
}

void VirtualMachine::execute() {
    bool interactive = false;
    bool enable_logging = false;
    // interactive = true;
    // enable_logging = true;
    
    bytecode->apply_relocations();

    global_data = bytecode->copyGlobalData(&global_data_max);

    // printf("global size %d\n",global_data_max);
    
    memset(registers,0,sizeof(registers));
    
    registers[REG_SP] = (i64)(stack + stack_max); // stack starts at the top and grows down
    
    bool found_main = false;
    auto unsafe_pieces = bytecode->pieces_unsafe();
    for(int i=0;i<unsafe_pieces.size(); i++) {
        if(unsafe_pieces[i]->name == "main") {
            found_main = true;
            piece_index = i;
            piece = unsafe_pieces[i];
            break;
        }
    }
    if(!found_main) {
        log_color(RED);
        printf("VM: main was not found.\n");
        log_color(NO_COLOR);
        return;
    }
    if(piece->instructions.size() == 0) {
        log_color(YELLOW);
        printf("VM: '%s' has no instructions.\n", piece->name.c_str());
        log_color(NO_COLOR);
        return;
    }
    
    // printf("VirtualMachine: Stack overflow (sp: %ld, stack range: %d - %d)\n", (i64)registers[REG_SP], (int)stack_max, 0);
    #define CHECK_STACK if(registers[REG_SP] > (i64)stack + stack_max || registers[REG_SP] < (i64)stack) {\
        printf("\n");\
        log_color(Color::RED);\
        printf("VM: Stack overflow (d_sp: %lld, max: %d)\n", (i64)registers[REG_SP] - (i64)stack, (int)stack_max);\
        log_color(Color::NO_COLOR);\
        return;\
    }
    
    bool memory_check = false;
    auto can_access_memory=[this](void* ptr, int size) {
        u64 diff = (u64)ptr - (u64)stack;
        if((diff >= 0 && diff + size <= stack_max)) {
            goto valid_access;
        }
        diff = (u64)ptr - (u64)global_data;
        if((diff >= 0 && diff + size <= global_data_max)) {
            goto valid_access;
        }
        
        for(auto pair : allocations) {
            u64 diff = (u64)ptr - (u64)pair.first;
            if((diff >= 0 && diff + size <= pair.second.size)) {
                goto valid_access;
            }
        }
        
        log_color(RED);
        printf("VM: Access violation at 0x%p + %d\n", ptr, size);
        log_color(NO_COLOR);
        return false;
    valid_access:
        return true;
    };
    
    registers[REG_BP] = registers[REG_SP];
    
    auto mov=[&](int size, void* dst, void* src) {
        switch(size){
        case 1: *( u8*)dst = *( u8*)src; break;
        case 2: *(u16*)dst = *(u16*)src; break;
        case 4: *(u32*)dst = *(u32*)src; break;
        case 8: *(u64*)dst = *(u64*)src; break;
        default: Assert(false);
        }
    };
    
    log_color(GREEN);
    printf("VM: Started in '%s'\n", piece->name.c_str());
    log_color(NO_COLOR);
    
    int debug_last_piece = -1;
    int debug_last_line = -1;
    
    bool running = true;
    while(running) {
        if(interactive) {
            printf("> ");
            std::string line;
            std::getline(std::cin, line);
            
            // Remove what the user just typed  (it bloats the screen)
            #ifdef OS_WINDOWS
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(h, &info);
            COORD pos = info.dwCursorPosition;
            pos.X = 0;
            pos.Y--;
            SetConsoleCursorPosition(h, pos);
            DWORD garb;
            FillConsoleOutputCharacter(h, ' ', info.dwSize.X, pos, &garb);
            #elif defined(OS_UNIX)
            int new_y_pos = ?;
            printf("\033[0,%d", new_y_pos);
            #endif
            
            if(line.empty()) {
                // nothing
            } else if(line == "l") {
                print_registers(true);
            } else if(line == "f") {
                print_frame(4,4);
            } else if(line == "s") {
                print_stack();
            } else if(line == "c") {
                interactive = false;
            } else if(line == "help") {
                printf("log registers\n");
            }
        }
        
        if(registers[REG_PC] >= piece->instructions.size()) {
            log_color(Color::RED);
            printf("INTERPRETER: PC out of bounds (pc: %d, piece instructions: %d\n", (int)registers[REG_PC], (int)piece->instructions.size());
            log_color(Color::NO_COLOR);
            break;
        }
        int prev_pc = registers[REG_PC];
        Instruction inst = piece->instructions[registers[REG_PC]];
        registers[REG_PC]++;

        ControlFlags control = (ControlFlags)inst.op2;
        
        int imm = 0;
        if(inst.opcode >= INST_IMMEDIATES) {
            imm = *(int*)&piece->instructions[registers[REG_PC]];
            registers[REG_PC]++;
        }
        
        bool printed_newline = false;
        
        // log_color(Color::GRAY);
        // printf("(bp: %lld, sp: %lld)\n", registers[REG_BP], registers[REG_SP]);
        // log_color(Color::NO_COLOR);
        
        #ifndef DISABLE_DEBUG_LINES
        LOG(
            if(piece->line_of_instruction.size() > prev_pc) {
                int line_index = piece->line_of_instruction[prev_pc];
                if(line_index != -1 && (debug_last_piece != piece_index || debug_last_line != line_index)) {
                    BytecodePiece::Line& line = piece->lines[line_index];
                    log_color(Color::AQUA);
                    printf(" %d: %s\n", line.line_number, line.text.c_str());
                    log_color(Color::NO_COLOR);
                    debug_last_piece = piece_index;
                    debug_last_line = line_index;
                }
            }
            piece->print(bytecode, false, prev_pc, prev_pc+1);
        )
        #else
        LOG(
            piece->print(bytecode, false, prev_pc, prev_pc+1);
        )
        #endif
        
        switch(inst.opcode) {
        case INST_CAST: {
            CastType type = (CastType)inst.op1;
            if(type == CAST_INT_FLOAT) {
                *(float*)&registers[inst.op0] = *(int*)&registers[inst.op0];
            } else if(type == CAST_FLOAT_INT) {
                *(int*)&registers[inst.op0] = *(float*)&registers[inst.op0];
            }
            break;
        }
        case INST_MOV_RR: {
            registers[inst.op0] = registers[inst.op1];
            break;
        }
        case INST_MOV_MR:
        case INST_MOV_MR_DISP: {
            void* ptr = (void*)(registers[inst.op0] + imm);
            int size = inst.op2;
            if(!can_access_memory(ptr, size)) {
                piece->print(bytecode, true, prev_pc, prev_pc+1);
                printf("\n");
                running = false;
            } else {
                mov(size, ptr, &registers[inst.op1]);
            }
        } break;
        case INST_MOV_RM:
        case INST_MOV_RM_DISP: {
            void* ptr = (void*)(registers[inst.op1] + imm);
            int size = inst.op2;
            registers[inst.op0] = 0; // reset register
            if(!can_access_memory(ptr, size)) {
                piece->print(bytecode, true, prev_pc, prev_pc+1);
                printf("\n");
                running = false;
            } else {
                mov(size, &registers[inst.op0], ptr);
            }
        } break;
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
        case INST_INCR: {
            int imm = ((i8)inst.op1) | ((i8)inst.op2 << 8);
            registers[inst.op0] += imm;
            if(inst.op0 == REG_SP) {
                CHECK_STACK
            }
            break;
        }
        case INST_DATAPTR: {
            registers[inst.op0] = (i64)(global_data + imm);
        } break;
        case INST_CALL: {
            if(imm < 0) { // special native call
                NativeCalls type = (NativeCalls)(imm + NATIVE_MAX);
                bool will_print = type == NATIVE_printc || type == NATIVE_printf || type == NATIVE_printi || type == NATIVE_prints;
                if(will_print) {
                    LOG(printf("\n");)
                }
                run_native_call(type);
                if(will_print) {
                    printed_newline = true;
                    LOG(printf("\n");)
                }
                break;
            }
            
            // push pc
            registers[REG_SP] -= 4;
            CHECK_STACK
            *(int*)registers[REG_SP] = registers[REG_PC];
            Assert(registers[REG_PC] >> 32 == 0); // make sure we don't overflow, probably never will but just in case
            
            // push piece_index
            registers[REG_SP] -= 4;
            CHECK_STACK
            *(int*)registers[REG_SP] = piece_index;
            
            // set program counter to start of the function
            registers[REG_PC] = 0;
            piece_index = imm - 1;
            piece = bytecode->getPiece(piece_index);

            // push bp
            registers[REG_SP] -= 8;
            CHECK_STACK
            *(i64*)registers[REG_SP] = registers[REG_BP];

            // mov bp, sp
            registers[REG_BP] = registers[REG_SP];

            break;
        }
        case INST_RET: {
            if(registers[REG_SP] == (i64)stack + stack_max) {
                running = false;
                break;
            }

            if(registers[REG_BP] != registers[REG_SP]) {
                printf("\n");
                log_color(Color::RED);
                printf("INTERPRETER: Stack pointer and base pointer mismatch on ret instruction (bp: %lld, sp %lld\n", registers[REG_BP], registers[REG_SP]);
                log_color(Color::NO_COLOR);
                return;
            }


            // pop bp
            registers[REG_BP] = *(i64*)registers[REG_SP];
            registers[REG_SP] += 8;
            CHECK_STACK

            piece_index = *(int*)registers[REG_SP];
            registers[REG_SP] += 4;
            CHECK_STACK
            
            registers[REG_PC] = *(int*)registers[REG_SP];
            registers[REG_SP] += 4;
            CHECK_STACK

            
            piece = bytecode->getPiece(piece_index);
            break;
        }
        case INST_MEMZERO: {
            void* ptr = (void*)(registers[inst.op0]);
            int size = registers[inst.op1];
            if(!can_access_memory(ptr, size)) {
                piece->print(bytecode, true, prev_pc, prev_pc+1);
                printf("\n");
                running = false;
            } else {
                memset((void*)registers[inst.op0],0, registers[inst.op1]);
            }
            break;
        }
        case INST_JMP: {
            registers[REG_PC] += imm -1; // -1 because imm is relative to the immediates address and not the end of the jump instruction. See BytecodePiece::fix_jump_here for specifics.
            break;
        }
        case INST_JZ: {
            if(registers[inst.op0] == 0) {
                registers[REG_PC] += imm -1; // -1 because imm is relative to the immediates address and not the end of the jump instruction. See BytecodePiece::fix_jump_here for specifics.
            }
            break;
        }
        #define CASEC(T,OP) case T: if (control & CONTROL_FLOAT) registers[inst.op0] = *(float*)&registers[inst.op0] OP *(float*)&registers[inst.op1]; else if(control & CONTROL_1B) registers[inst.op0] = *(i8*)&registers[inst.op0] OP *(i8*)&registers[inst.op1]; else registers[inst.op0] = registers[inst.op0] OP registers[inst.op1]; break;
        #define CASEF(T,OP) case T: if (control & CONTROL_FLOAT) *(float*)&registers[inst.op0] = *(float*)&registers[inst.op0] OP *(float*)&registers[inst.op1]; else registers[inst.op0] = registers[inst.op0] OP registers[inst.op1]; break;
        CASEF(INST_ADD, +)
        CASEF(INST_SUB, -)
        CASEF(INST_MUL, *)
        CASEF(INST_DIV, /)
        case INST_AND: registers[inst.op0] = registers[inst.op0] && registers[inst.op1]; break;
        case INST_OR:  registers[inst.op0] = registers[inst.op0] || registers[inst.op1]; break;
        case INST_NOT: registers[inst.op0] = !registers[inst.op1]; break;
        CASEC(INST_EQUAL, ==)
        CASEC(INST_NOT_EQUAL, !=)
        CASEC(INST_LESS, <)
        CASEC(INST_LESS_EQUAL, <=)
        CASEC(INST_GREATER, >)
        CASEC(INST_GREATER_EQUAL, >=)
        default: Assert(("Incomplete instruction",false));
        }
        
        if(inst.op0 != REG_INVALID) {
            if(inst.opcode == INST_MOV_MR || inst.opcode == INST_MOV_MR_DISP) {
                i64 tmp = 0;
                mov((u8)inst.op2, &tmp, (void*)(registers[inst.op0] + imm));
                LOG(
                    log_color(GRAY);
                    printf("  *%s = %lld ", register_names[inst.op0], tmp);
                    // printf("  *%s = %ld ", register_names[inst.op0], tmp);
                    log_color(NO_COLOR);
                )
            } else {
                LOG(
                    log_color(GRAY);
                    // printf("  %s = %ld ", register_names[inst.op0], registers[inst.op0]);
                    printf("  %s = %lld ", register_names[inst.op0], registers[inst.op0]);
                    log_color(NO_COLOR);
                )
            }
        }
        
        if(!printed_newline)
            LOG(printf("\n");)
    }
    if(allocations.size() > 0) {
        log_color(RED);
        printf("VM: Finished with %d unfreed allocations.\n", (int)allocations.size());
        log_color(NO_COLOR);
        for(auto& pair : allocations) {
            Free(pair.first, pair.second.size, HERE, typeid(u8));
        }
        allocations.clear();
    } else {
        log_color(GREEN);
        printf("VM: Finished\n");
        log_color(NO_COLOR);
    }
    // print_registers();
}
void VirtualMachine::run_native_call(NativeCalls callType) {
    // auto inst_pop = [&](Register reg) {
    //     Assert(reg >= REG_T0 && reg <= REG_T1);
    //     registers[reg] = *(i64*)registers[REG_SP];
    //     registers[REG_SP] += 8;
    //     CHECK_STACK
    // };
    // auto inst_push = [&](Register reg) {
    //     Assert(reg >= REG_T0 && reg <= REG_T1);
    //     registers[REG_SP] -= 8;
    //     CHECK_STACK
    //     *(i64*)registers[REG_SP] = registers[reg];
    // };
    switch(callType) {
    case NATIVE_printi: {
        int arg0 = *(int*)(registers[REG_SP] + 0);
        
        printf("%d", arg0);
    } break;
    case NATIVE_printf: {
        float arg0 = *(float*)(registers[REG_SP] + 0);
        
        printf("%f", arg0);
    } break;
     case NATIVE_printc: {
        char arg0 = *(char*)(registers[REG_SP] + 0);
        
        printf("%c", arg0);
    } break;
     case NATIVE_prints: {
        char* arg0 = *(char**)(registers[REG_SP] + 0);
        
        printf("%s", arg0);
    } break;
    case NATIVE_malloc: {
        int arg0 = *(int*)(registers[REG_SP] + 0);
        // void* arg1 = *(void**)(registers[REG_SP] + 8);
        // int arg2 = *(int*)(registers[REG_SP] + 12);
        void*& ret = *(void**)(registers[REG_SP] - 16 - 8);
        
        ret = Alloc(arg0, HERE, typeid(u8), 1);
        if(ret) {
            allocations[ret] = {arg0};
        }
    } break;
     case NATIVE_mfree: {
        void* arg0 = *(void**)(registers[REG_SP] + 0);
        // void*& ret = *(void**)(registers[REG_SP] - 16 - 8);
        
        if(arg0) {
            auto pair = allocations.find(arg0);
            if(pair == allocations.end()) {
                log_color(Color::RED);
                printf("INTERPRETER: mfree was called with a pointer that wasn't allocated\n");
                log_color(Color::NO_COLOR);
            } else {
                // TODO: Not decreasing memory usage
                Free(arg0, pair->second.size, HERE, typeid(u8));
                allocations.erase(arg0);
            }
        }
    } break;
    case NATIVE_memcpy: {
        void* arg0 = *(void**)(registers[REG_SP] + 0);
        void* arg1 = *(void**)(registers[REG_SP] + 8);
        int arg2 = *(int*)(registers[REG_SP] + 16);
        // void*& ret = *(void**)(registers[REG_SP] - 16 - 8);
        
        memmove(arg0, arg1, arg2);
    } break;
    case NATIVE_pow: {
        float arg0 = *(float*)(registers[REG_SP] + 0);
        float arg1 = *(float*)(registers[REG_SP] + 4);
        float& ret = *(float*)(registers[REG_SP] - 16 - 8);
        
        ret = powf(arg0, arg1);
    } break;
    case NATIVE_sqrt: {
        float arg0 = *(float*)(registers[REG_SP] + 0);
        float& ret = *(float*)(registers[REG_SP] - 16 - 8);
        
        ret = sqrtf(arg0);
    } break;
    case NATIVE_read_file: {
        char* path = *(char**)(registers[REG_SP] + 0);
        char** out_data = *(char***)(registers[REG_SP] + 8);
        int* out_size = *(int**)(registers[REG_SP] + 16);
        bool& ret = *(bool*)(registers[REG_SP] - 16 - 8);
        
        Assert(path);
        
        std::ifstream file(path, std::ifstream::binary);
        if(!file.is_open()) {
            ret = false;
            break;
        }
        ret = true;

        file.seekg(0, file.end);
        int filesize = file.tellg();
        file.seekg(0, file.beg);
        
        if(out_size)
            *out_size = filesize;
        
        if(out_data) {
            char* text = (char*)NEW_ARRAY(u8, filesize, HERE); // we use u8 because free assumes u8, if we used char then types would be incompatible and assert would fire in the tracker when we allocate the ptr using one type and free it with another
            Assert(text);
            
            allocations[text] = {filesize};
            file.read(text, filesize);
            
            *out_data = text;
        }
        file.close();
    } break;
    case NATIVE_write_file: {
        char* path = *(char**)(registers[REG_SP] + 0);
        char* out_data = *(char**)(registers[REG_SP] + 8);
        int out_size = *(int*)(registers[REG_SP] + 16);
        bool& ret = *(bool*)(registers[REG_SP] - 16 - 8);
        
        Assert(path);
        
        std::ofstream file(path, std::ofstream::binary);
        if(!file.is_open()) {
            ret = false;
            break;
        }
        ret = true;
        if(out_size < 0) {
            Assert(out_data);
            file.write(out_data, out_size);
        }
        file.close();
    } break;
    default: Assert(false);   
    }
}
void VirtualMachine::print_registers(bool subtle) {
    log_color(Color::GOLD);
    if(!subtle)
        printf("Registers:\n");
    log_color(Color::NO_COLOR);
    for(int i=REG_A;i<=REG_BP;i++) {
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
void VirtualMachine::print_frame(int high, int low) {
    for(int i=0;i<high;i++) {
        int off = 16 + (high-1 - i) * 8;
        if(registers[REG_BP] + off >= (i64)stack + stack_max)
            break;
        i64 val = *(i64*)(registers[REG_BP] + off);
        
        log_color(Color::CYAN);
        printf(" %s+%d", register_names[REG_BP], off);
        log_color(Color::GRAY);
        printf(" = ");
        log_color(Color::GREEN);
        printf("%lld\n", val);
        log_color(Color::NO_COLOR);
    }
    for(int i=0;i<low;i++) {
        int off = -(i+1) * 8;
        if(registers[REG_BP] + off < (i64)stack)
            break;
        i64 val = *(i64*)(registers[REG_BP] + off);
        
        log_color(Color::CYAN);
        printf(" %s-%d", register_names[REG_BP], -off);
        log_color(Color::GRAY);
        printf(" = ");
        log_color(Color::GREEN);
        printf("%lld\n", val);
        log_color(Color::NO_COLOR);
    }
}
void VirtualMachine::print_stack() {
    i64 off = registers[REG_BP];
    while (true) {
        off -= 8;
        if(registers[REG_SP] > off)
            break;
        i64 val = *(i64*)(off);
        
        log_color(Color::CYAN);
        printf(" %s+%lld", register_names[REG_SP], off - registers[REG_SP]);
        log_color(Color::GRAY);
        printf(" = ");
        log_color(Color::GREEN);
        printf("%lld\n", val);
        log_color(Color::NO_COLOR);
    }
}