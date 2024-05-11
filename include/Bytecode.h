#pragma once

#include "Util.h"

struct ASTFunction;

enum Opcode : u8 {
    INST_HALT=0,
    INST_NOP,

    INST_CAST,
    INST_MOV_RR, // reg <- reg
    INST_MOV_MR, // memory <- reg
    INST_MOV_RM, // reg <- memory

    INST_PUSH,
    INST_POP,
    INST_INCR,

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
    INST_MEMZERO,
    
    // instructions with immediates
    INST_IMMEDIATES,
    INST_LI = INST_IMMEDIATES, // load immediate
    INST_JMP, // jump
    INST_JZ, // conditional jump (if zero)
    INST_CALL, // function call
    
    INST_MOV_MR_DISP, // memory+disp <- reg
    INST_MOV_RM_DISP, // reg <- memory+disp
    
    INST_DATAPTR,
    // don't add non-immediate instructions here (inst.opcode >= INST_IMMEDIATES)
};
enum CastType {
    CAST_FLOAT_INT,
    CAST_INT_FLOAT,
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
struct Bytecode;
struct BytecodePiece {
    int piece_index=0;
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
    
    int virtual_sp=0;
    struct Relocation {
        // std::string func_name;
        ASTFunction* function = nullptr;
        int index_of_immediate; // immediate of INST_CALL instruction
    };
    std::vector<Relocation> relocations;
    void addRelocation(ASTFunction* func, int imm_index);

    void emit_li(Register reg, int imm);
    void emit_push(Register reg);
    void emit_pop(Register reg);
    void emit_incr(Register reg, int imm);
    
    void emit_cast(Register reg, CastType castType);
    void emit_mov_rr(Register to_reg, Register from_reg);
    void emit_mov_mr(Register to_reg, Register from_reg, u8 size);
    void emit_mov_rm(Register to_reg, Register from_reg, u8 size);
    void emit_mov_mr_disp(Register to_reg, Register from_reg, u8 size, int offset);
    void emit_mov_rm_disp(Register to_reg, Register from_reg, u8 size, int offset);

    void emit_add(Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_sub(Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_mul(Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_div(Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_and(Register to_from_reg, Register from_reg);
    void emit_or(Register to_from_reg, Register from_reg);
    void emit_not(Register to_reg, Register from_reg);

    void emit_eq            (Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_neq           (Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_less          (Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_greater       (Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_less_equal    (Register to_from_reg, Register from_reg, bool is_float = false);
    void emit_greater_equal (Register to_from_reg, Register from_reg, bool is_float = false);
    
    void emit_jmp(int pc);
    void emit_jmp(int* out_index_of_imm);
    void emit_jz(Register reg, int pc);
    void emit_jz(Register reg, int* out_index_of_imm);
    
    void emit_call(int* out_index_of_imm);
    void emit_ret();
    void emit_dataptr(Register reg, int offset);
    
    void emit_memzero(Register reg, Register reg_size);

    int get_pc() { return instructions.size(); }
    void fix_jump_here(int imm_index);
    // void fix_jump_to(int imm_index, int pc);
    
    Instruction* get(int index) { return &instructions[index]; }
    
    void print(Bytecode* bytecode, bool with_debug_info = true, int low_index = 0, int high_index = -1);
    
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

struct Bytecode {
    ~Bytecode() {
        cleanup();
    }
    void cleanup() {
        if(global_data) {
            DELNEW_ARRAY(global_data, u8, global_data_max, HERE);
        }
        global_data = nullptr;
        global_data_max = 0;
        global_data_size = 0;
        for(auto t : pieces) {
            DELNEW(t, BytecodePiece, HERE);
        }
        pieces.clear();
    }
    BytecodePiece* createPiece() {
        auto ptr = NEW(BytecodePiece, HERE);
        MUTEX_LOCK(general_lock);
        ptr->piece_index = pieces.size();
        pieces.push_back(ptr);
        MUTEX_UNLOCK(general_lock);
        return ptr;
    }
    BytecodePiece* getPiece(int index) {
        MUTEX_LOCK(general_lock);
        if(pieces.size() <= index) {
            MUTEX_UNLOCK(general_lock);
            return nullptr;
        }
        auto ptr = pieces[index];
        MUTEX_UNLOCK(general_lock);
        return ptr;
    }
    
    void apply_relocations();
    
    void print();
    
    // MUTEX_DECL(data_lock);
    MUTEX_DECL(general_lock);
    
    // returns offset of sub data into the global data
    int appendData(int size, void* data = nullptr);
    // Used by interpreter
    u8* copyGlobalData(int* size);
    
    int appendString(const std::string& str);
    std::unordered_map<std::string, int> string_map{};
    
    // unsafe because direct access without mutex
    std::vector<BytecodePiece*>& pieces_unsafe() {
        return pieces;
    }
    
private:
    u8* global_data = nullptr;
    int global_data_size = 0;
    int global_data_max = 0;
    
    std::vector<BytecodePiece*> pieces;
};

enum NativeCalls {
    NATIVE_printi = 0,
    NATIVE_printf,
    NATIVE_printc,
    NATIVE_prints,
    NATIVE_malloc,
    NATIVE_mfree,
    NATIVE_memcpy,
    NATIVE_pow,
    NATIVE_sqrt,
    NATIVE_read_file,
    NATIVE_write_file,
    NATIVE_MAX,
};
#define NAME_OF_NATIVE(X) native_names[X]
extern const char* native_names[];