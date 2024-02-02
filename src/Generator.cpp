#include "Generator.h"

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
                reporter->err(expr->location, std::string() + "Variable '" + expr->name + "' does not exist.");
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
            // TODO: Push arguments
            
            int relocation_index = 0;
            piece->emit_call(&relocation_index);
            
            piece->relocations.push_back({expr->name, relocation_index});
            
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
        default: Assert(false);
    }
    return true;
}
bool GeneratorContext::generateStatement(ASTStatement* stmt) {
    Assert(stmt);
    switch(stmt->type){
        case ASTStatement::EXPRESSION: {
            generateExpression(stmt->expression);
            piece->emit_pop(REG_A); // we don't care about the value
            break;   
        }
        case ASTStatement::VAR_DECLARATION: {
            // make space on the stack frame for local variable
            
            Variable* variable = nullptr;
            if(stmt->declaration_type.empty()) {
                variable = findVariable(stmt->declaration_name);
                if(!variable) {
                    reporter->err(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is not declared.");
                    break;
                }
            } else {
                variable = addVariable(stmt->declaration_name);
                if(!variable) {
                    reporter->err(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is already declared.");
                    break;
                }
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
        default: Assert(false);
    }
    return true;
}
bool GeneratorContext::generateBody(ASTBody* body) {
    Assert(body);
    for(auto stmt : body->statements) {
        auto res = generateStatement(stmt);
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
    
    context.generateBody(function->body);
    
    // Emit ret instruction if the user forgot the return statement
    if(context.piece->instructions.back().opcode != INST_RET) {
        // context.piece->emit_pop(REG_BP); (the call instruction handles this automatically)
        context.piece->emit_ret();
    }
}

GeneratorContext::Variable* GeneratorContext::addVariable(const std::string& name) {
    auto pair = local_variables.find(name);
    if(pair != local_variables.end())
        return nullptr; // variable already exists
        
    auto ptr = new Variable();
    ptr->name = name;
    current_frame_offset -= 4;
    ptr->frame_offset = current_frame_offset;
    local_variables[name] = ptr;
    return ptr;
}
GeneratorContext::Variable* GeneratorContext::findVariable(const std::string& name) {
    auto pair = local_variables.find(name);
    if(pair == local_variables.end())
        return nullptr;
    return pair->second;
}