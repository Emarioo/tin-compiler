#include "Generator.h"

#define REPORT(L, ...) reporter->err(function->origin_stream, L, __VA_ARGS__)

bool GeneratorContext::generateReference(ASTExpression* expr) {
    std::vector<ASTExpression*> exprs;
    
    while(expr) {
        if(expr->type == ASTExpression::IDENTIFIER) {
            exprs.push_back(expr);            
            expr = expr->left;
            continue;
        }
        // else if(expr->type == ASTExpression::MEMBER) {
        //     expr = expr->left;
        // }
        
        // expr type not allowed when referencing
        return false;
    }
    
    for(int i=exprs.size()-1;i>=0;i--) {
        auto e = exprs[i];
        
        if(e->type == ASTExpression::IDENTIFIER) {
            auto variable = findVariable(e->name);
            if(!variable) {
                REPORT(expr->location, std::string() + "Variable '" + e->name + "' does not exist.");
                return false;
            }
            int var_offset = variable->frame_offset;
            // get the address of the variable
            // a local variable exists on the stack
            // with an offset from the base pointer
            piece->emit_li(REG_B, var_offset);
            piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
            
            piece->emit_push(REG_B); // push pointer
            break;
        }
    }
    return true;
}
bool GeneratorContext::generateExpression(ASTExpression* expr) {
    Assert(expr);
    switch(expr->type) {
        case ASTExpression::LITERAL_INT: {
            piece->emit_li(REG_A, expr->literal_integer);
            piece->emit_push(REG_A);
            break;
        }
         case ASTExpression::LITERAL_STR: {
            // TODO: How do we do this?
            piece->emit_li(REG_A, 0);
            piece->emit_push(REG_A);
            break;
        }
        case ASTExpression::IDENTIFIER: {
            auto variable = findVariable(expr->name);
            if(!variable) {
                REPORT(expr->location, std::string() + "Variable '" + expr->name + "' does not exist.");
                break;
            }
            int var_offset = variable->frame_offset;
            // get the address of the variable
            // a local variable exists on the stack
            // with an offset from the base pointer
            piece->emit_li(REG_B, var_offset);
            piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
            
            // get the value of the variable
            piece->emit_mov_rm(REG_A, REG_B, 4); // reg_a = *reg_b
            piece->emit_push(REG_A);
            break;
        }
        case ASTExpression::FUNCTION_CALL: {
            for(int i=0;i<expr->arguments.size(); i++) {
                auto arg = expr->arguments[i];
                generateExpression(arg);
            }

            int relocation_index = 0;
            piece->emit_call(&relocation_index);
            
            // how about native calls
            piece->relocations.push_back({expr->name, relocation_index});
            
            for(int i=0;i<expr->arguments.size(); i++) {
                piece->emit_pop(REG_F); // throw away argument values
            }
            // NOTE: Optimize by popping all arguments by incrementing stack pointer directly
            // piece->emit_li(REG_F, expr->arguments.size() * 8);
            // piece->emit_add(REG_SP, REG_F);
            
            // TODO: Some functions do not return a value. Handle it.
            piece->emit_push(REG_A); // push return value
            break;
        }
        case ASTExpression::ADD:
        case ASTExpression::SUB:
        case ASTExpression::MUL:
        case ASTExpression::DIV:
        case ASTExpression::AND:
        case ASTExpression::OR:
        case ASTExpression::EQUAL:
        case ASTExpression::NOT_EQUAL:
        case ASTExpression::LESS:
        case ASTExpression::GREATER:
        case ASTExpression::LESS_EQUAL:
        case ASTExpression::GREATER_EQUAL: {
            generateExpression(expr->left);
            generateExpression(expr->right);
            
            piece->emit_pop(REG_D);
            piece->emit_pop(REG_A);
            
            switch(expr->type) {
                case ASTExpression::ADD: piece->emit_add(REG_A, REG_D); break;
                case ASTExpression::SUB: piece->emit_sub(REG_A, REG_D); break;
                case ASTExpression::MUL: piece->emit_mul(REG_A, REG_D); break;
                case ASTExpression::DIV: piece->emit_div(REG_A, REG_D); break;
                case ASTExpression::AND: piece->emit_and(REG_A, REG_D); break;
                case ASTExpression::OR:  piece->emit_or (REG_A, REG_D); break;
                case ASTExpression::EQUAL:          piece->emit_eq(REG_A, REG_D); break;
                case ASTExpression::NOT_EQUAL:      piece->emit_neq(REG_A, REG_D); break;
                case ASTExpression::LESS:           piece->emit_less(REG_A, REG_D); break;
                case ASTExpression::GREATER:        piece->emit_greater(REG_A, REG_D); break;
                case ASTExpression::LESS_EQUAL:     piece->emit_less_equal(REG_A, REG_D); break;
                case ASTExpression::GREATER_EQUAL:  piece->emit_greater_equal(REG_A, REG_D); break;
                default: Assert(false);
            }
            
            piece->emit_push(REG_A);
            break;
        }
        case ASTExpression::NOT: {
            generateExpression(expr->left);
            
            piece->emit_pop(REG_A);
            piece->emit_not(REG_A, REG_A);
            piece->emit_push(REG_A);
            break;   
        }
        case ASTExpression::REFER: {
            bool yes = generateReference(expr->left);
            break;   
        }
        case ASTExpression::DEREF: {
            bool yes = generateExpression(expr->left);
            
            piece->emit_pop(REG_B);
            piece->emit_mov_rm(REG_A, REG_B, 4);
            piece->emit_push(REG_A);
            break;   
        }
        default: Assert(false);
    }
    return true;
}
bool GeneratorContext::generateBody(ASTBody* body) {
    Assert(body);
    int prev_frame_offset = current_frame_offset;
    for(auto stmt : body->statements) {
        // debug line information
        std::string text = function->origin_stream->getline(stmt->location);
        int line = function->origin_stream->getToken(stmt->location)->line;
        piece->push_line(line, text);
        
        bool stop = false;
        switch(stmt->type){
        case ASTStatement::EXPRESSION: {
            generateExpression(stmt->expression);
            piece->emit_pop(REG_A); // we don't care about the value
            break;   
        }
        case ASTStatement::VAR_DECLARATION: {
            // make space on the stack frame for local variable
            
            Variable* variable = nullptr;
            bool declaration = false;
            if(stmt->declaration_type.empty()) {
                variable = findVariable(stmt->declaration_name);
                if(!variable) {
                    REPORT(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is not declared.");
                    break;
                }
            } else {
                declaration = true;
                variable = addVariable(stmt->declaration_name);
                if(!variable) {
                    REPORT(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is already declared.");
                    break;
                }
                // make space on stack for local variable
                // piece->emit_li(REG_B, 8);
                // piece->emit_sub(REG_SP, REG_B); // sp -= 8
                piece->emit_incr(REG_SP, -8);
            }
            
            int var_offset = variable->frame_offset;
            if(stmt->expression) {
                generateExpression(stmt->expression);
                piece->emit_pop(REG_A);
            } else {
                // zero initialize the variable if expression wasn't specified
                // TODO: Make this work with structs
                piece->emit_li(REG_A, 0);
            }
            
            // get the location (pointer) of the variable
            piece->emit_li(REG_B, var_offset);
            piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
            
            // set the value of the variable
            piece->emit_mov_mr(REG_B, REG_A, 4); // *reg_b = reg_a
            
            break;
        }
        case ASTStatement::WHILE: {
            int pc_loop = piece->get_pc();
            
            generateExpression(stmt->expression);
            piece->emit_pop(REG_A);
            
            int reloc_while = 0;
            piece->emit_jz(REG_A, &reloc_while);
            
            generateBody(stmt->body);
            
            piece->emit_jmp(pc_loop);
            
            piece->fix_jump_here(reloc_while);
            break;
        }
        case ASTStatement::IF: {
            generateExpression(stmt->expression);
            piece->emit_pop(REG_A);
            
            int reloc_if = 0;
            piece->emit_jz(REG_A, &reloc_if);
            
            generateBody(stmt->body);
            
            if(stmt->elseBody) {
                int reloc_if_end = 0;
                piece->emit_jmp(&reloc_if_end);
                piece->fix_jump_here(reloc_if);
                
                generateBody(stmt->elseBody);
                
                piece->fix_jump_here(reloc_if_end);
            } else {
                piece->fix_jump_here(reloc_if);
            }
            
            break;
        }
        case ASTStatement::RETURN: {
            if(stmt->expression) {
                generateExpression(stmt->expression);
                piece->emit_pop(REG_A);
            }
            if(current_frame_offset != prev_frame_offset) {
                // piece->emit_li(REG_B, current_frame_offset - prev_frame_offset);
                // piece->emit_sub(REG_SP, REG_B);
                piece->emit_incr(REG_SP, - (current_frame_offset - prev_frame_offset));
            }
            piece->emit_ret();
            
            stop = true; // return statement makes the rest of the statments useless
            
            if(stop)
                current_frame_offset = prev_frame_offset;
            break;
        }
        default: Assert(false);
        }
        
        if(stop)
            break;
    }
    if(current_frame_offset != prev_frame_offset) {
        // This also happens in the return statement
        // "free" local variables from stack
        // piece->emit_li(REG_B, current_frame_offset - prev_frame_offset);
        // piece->emit_sub(REG_SP, REG_B);
        piece->emit_incr(REG_SP, - (current_frame_offset - prev_frame_offset));
        current_frame_offset = prev_frame_offset;
    }
    
    return true;
}

void GenerateFunction(AST* ast, ASTFunction* function, Code* code, Reporter* reporter) {
    GeneratorContext context{};
    context.ast = ast;
    context.function = function;
    context.code = code;
    context.piece = code->createPiece();
    context.reporter = reporter;
    
    context.piece->name = function->name;
    
    // Setup the base/frame pointer (the call instruction handles this automatically)
    // context.piece->emit_push(REG_BP);
    // context.piece->emit_mov_rr(REG_BP, REG_SP);

    GeneratorContext::Variable* variable = nullptr;
    for(int i=0;i < function->parameters.size(); i++) {
        auto& param = function->parameters[i];
        int frame_offset = 16 // 16 bytes for the call frame (pc, bp)
            + (function->parameters.size()-1-i)*8; // param.size-1-i BECAUSE first argument is pushed first and is the furthest away

        auto variable = context.addVariable(param.name, frame_offset);
        Assert(("Parameter could not be created. Compiler bug!",variable));

        // TODO: Set type of variable
    }
    
    context.generateBody(function->body);
 
    // Emit ret instruction if the user forgot the return statement
    if(context.piece->instructions.back().opcode != INST_RET) {
        // context.piece->emit_pop(REG_BP); (the call instruction handles this automatically)
        context.piece->emit_ret();
    }
}

GeneratorContext::Variable* GeneratorContext::addVariable(const std::string& name, int frame_offset) {
    auto pair = local_variables.find(name);
    if(pair != local_variables.end())
        return nullptr; // variable already exists
        
    auto ptr = new Variable();
    ptr->name = name;
    local_variables[name] = ptr;
    if(frame_offset == 0) {
        current_frame_offset -= 8;
        ptr->frame_offset = current_frame_offset;
    } else {
        ptr->frame_offset = frame_offset;
    }
    return ptr;
}
GeneratorContext::Variable* GeneratorContext::findVariable(const std::string& name) {
    auto pair = local_variables.find(name);
    if(pair == local_variables.end())
        return nullptr;
    return pair->second;
}