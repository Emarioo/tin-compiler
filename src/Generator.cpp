#include "Generator.h"

#define REPORT(L, ...) reporter->err(function->origin_stream, L, __VA_ARGS__)


void GeneratorContext::generatePop(Register reg, int offset, TypeId type) {
    if(type == TYPE_VOID) return;
    int size = ast->sizeOfType(type);
    TypeInfo* info = nullptr;
    if(type.pointer_level() > 0 || (info = ast->getType(type), !info->ast_struct)) {
        piece->emit_pop(REG_A);
        if(reg != REG_INVALID)
            piece->emit_mov_mr_disp(reg, REG_A, size, offset);
    } else {
        for(int i=info->ast_struct->members.size()-1;i>=0;i--) {
            auto& mem = info->ast_struct->members[i];
            generatePop(reg, offset + mem.offset, mem.typeId);
        }
    }
}
void GeneratorContext::generatePush(Register reg, int offset, TypeId type) {
    int size = ast->sizeOfType(type);
    TypeInfo* info = nullptr;
    if(type.pointer_level() > 0 || (info = ast->getType(type), !info->ast_struct)) {
        piece->emit_mov_rm_disp(REG_A, reg, size, offset);
        piece->emit_push(REG_A);
    } else {
        for(int i=0;i<info->ast_struct->members.size();i++) {
            auto& mem = info->ast_struct->members[i];
            generatePush(reg, offset + mem.offset, mem.typeId);
        }
    }
}

TypeId GeneratorContext::generateReference(ASTExpression* expr) {
    if(expr->kind() == ASTExpression::DEREF) {
        TypeId type = generateExpression(expr->left);
        type.set_pointer_level(type.pointer_level() - 1);
        return type;
    }
    
    std::vector<ASTExpression*> exprs;
    
    while(expr) {
        switch(expr->kind()) {
            case ASTExpression::IDENTIFIER: {
                exprs.push_back(expr);            
                expr = expr->left;
            } break;
            case ASTExpression::MEMBER: {
                exprs.push_back(expr);            
                expr = expr->left;
            } break;
            default: {
                REPORT(expr->location, "Invalid expression as a reference.");
                return TYPE_VOID;
                // Assert(false);
            }
        }
    }
    TypeId out = TYPE_VOID;
    int frame_offset = 0;
    for(int i=exprs.size()-1;i>=0;i--) {
        auto expr = exprs[i];
        
        switch(expr->kind()) {
            case ASTExpression::IDENTIFIER: {
                auto variable = findVariable(expr->name);
                if(!variable) {
                    REPORT(expr->location, std::string() + "Variable '" + expr->name + "' does not exist.");
                    return TYPE_VOID;
                }
                frame_offset = variable->frame_offset;
                out = variable->typeId;
            } break;
            case ASTExpression::MEMBER: {
                auto prev_expr = exprs[i+1];
                auto info = ast->getType(out);
                if(!info->ast_struct) {
                    REPORT(expr->location, std::string() + "Member access is only allowed on structure types. '"+prev_expr->name+"' is '"+ast->nameOfType(out) + "'.");
                    return TYPE_VOID;
                }
                auto mem = info->ast_struct->findMember(expr->name);
                if(!mem) {
                    REPORT(expr->location, std::string() + "'"+ expr->name+"' is not a member of '"+ast->nameOfType(out) + "'.");
                    return TYPE_VOID;
                }
                frame_offset += mem->offset;
                out = mem->typeId;
            } break;
            default: Assert(false);
        }
    }
    piece->emit_li(REG_B, frame_offset);
    piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
    
    piece->emit_push(REG_B); // push pointer
    return out;
}
TypeId GeneratorContext::generateExpression(ASTExpression* expr) {
    Assert(expr);
    switch(expr->kind()) {
        case ASTExpression::LITERAL_INT: {
            piece->emit_li(REG_A, expr->literal_integer);
            piece->emit_push(REG_A);
            return TYPE_INT;
            break;
        }
        case ASTExpression::LITERAL_FLOAT: {
            piece->emit_li(REG_A, *(int*)&expr->literal_float);
            piece->emit_push(REG_A);
            return TYPE_FLOAT;
            break;
        }
        case ASTExpression::LITERAL_STR: {
            // TODO: How do we do this?
            piece->emit_li(REG_A, 0);
            piece->emit_push(REG_A);
            TypeId type = TYPE_CHAR;
            type.set_pointer_level(1);
            return type;
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
            // piece->emit_li(REG_B, var_offset);
            // piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
            
            // // get the value of the variable
            // piece->emit_mov_rm(REG_A, REG_B, 4); // reg_a = *reg_b
            // piece->emit_push(REG_A);
            
            piece->emit_mov_rr(REG_B, REG_BP);
            generatePush(REG_B, var_offset, variable->typeId);
            return variable->typeId;
            break;
        }
        case ASTExpression::FUNCTION_CALL: {
            auto fun = ast->findFunction(expr->name, current_scopeId);
            if(!fun) {
                REPORT(expr->location, std::string() + "Function '"+expr->name+"' does not exist.");
                return TYPE_VOID;
            }

            piece->emit_incr(REG_SP, -fun->parameters_size);
            int arg_offset = piece->virtual_sp;

            std::vector<TypeId> argument_types;
            for(int i=0;i<expr->arguments.size(); i++) {
                auto arg = expr->arguments[i];
                auto type = generateExpression(arg);
                argument_types.push_back(type);
            }

            if(fun->parameters.size() != argument_types.size()) {
                REPORT(expr->location, std::string() + "The function call has " + std::to_string(argument_types.size()) + " arguments but the function '"+expr->name+"' requires " + std::to_string(fun->parameters.size()) + ".");
                return TYPE_VOID;
            }
            bool fail=false;
            for(int i=0;i<argument_types.size();i++) {
                if(fun->parameters[i].typeId != argument_types[i]) {
                    fail = true;
                    REPORT(expr->location, std::string() + "The "+std::to_string(i)+" argument is of type '"+ast->nameOfType(argument_types[i])+"' but the parameter '" + fun->parameters[i].name + "' requires '" + ast->nameOfType(fun->parameters[i].typeId) + "'.");
                }
            }
            if(fail)
                return TYPE_VOID;

            int arg_diff = piece->virtual_sp - arg_offset;

            piece->emit_mov_rr(REG_B, REG_SP);
            for(int i=fun->parameters.size()-1;i>=0; i--) {
                auto& param = fun->parameters[i];
                
                generatePop(REG_B, param.offset - 16 - arg_diff, param.typeId);
            }

            int relocation_index = 0;
            piece->emit_call(&relocation_index);

            // how about native calls
            // piece->relocations.push_back({expr->name, relocation_index});
            piece->addRelocation(fun, relocation_index);
            
            
            if(fun->return_type.valid()) {
                piece->emit_mov_rr(REG_B, REG_SP);
                piece->emit_incr(REG_SP, fun->parameters_size);
                generatePush(REG_B, fun->return_offset - 16, fun->return_type);
            } else {
                piece->emit_incr(REG_SP, fun->parameters_size);
            }
            
            
            return fun->return_type;
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
            TypeId ltype = generateExpression(expr->left);
            TypeId rtype = generateExpression(expr->right);
            
            if(ltype != rtype) {
                REPORT(expr->location, "Cannot perform operation on the types '"+ast->nameOfType(ltype)+"', '"+ast->nameOfType(rtype)+"'.");
                return TYPE_VOID;   
            }
            
            bool is_float = false;
            if(ltype == TYPE_FLOAT || rtype == TYPE_FLOAT) {
                is_float = true;
            }

            piece->emit_pop(REG_D);
            piece->emit_pop(REG_A);
            
            switch(expr->kind()) {
                case ASTExpression::ADD: piece->emit_add(REG_A, REG_D, is_float); break;
                case ASTExpression::SUB: piece->emit_sub(REG_A, REG_D, is_float); break;
                case ASTExpression::MUL: piece->emit_mul(REG_A, REG_D, is_float); break;
                case ASTExpression::DIV: piece->emit_div(REG_A, REG_D, is_float); break;
                case ASTExpression::AND: piece->emit_and(REG_A, REG_D); break;
                case ASTExpression::OR:  piece->emit_or (REG_A, REG_D); break;
                case ASTExpression::EQUAL:          piece->emit_eq(REG_A, REG_D, is_float); break;
                case ASTExpression::NOT_EQUAL:      piece->emit_neq(REG_A, REG_D, is_float); break;
                case ASTExpression::LESS:           piece->emit_less(REG_A, REG_D, is_float); break;
                case ASTExpression::GREATER:        piece->emit_greater(REG_A, REG_D, is_float); break;
                case ASTExpression::LESS_EQUAL:     piece->emit_less_equal(REG_A, REG_D, is_float); break;
                case ASTExpression::GREATER_EQUAL:  piece->emit_greater_equal(REG_A, REG_D, is_float); break;
                default: Assert(false);
            }
            
            piece->emit_push(REG_A);
            return ltype;
            break;
        }
        case ASTExpression::NOT: {
            auto type = generateExpression(expr->left);
            
            piece->emit_pop(REG_A);
            piece->emit_not(REG_A, REG_A);
            piece->emit_push(REG_A);
            return type;
            break;   
        }
        case ASTExpression::REFER: {
            TypeId type = generateReference(expr->left);
            return type;
            break;   
        }
        case ASTExpression::DEREF: {
            TypeId type = generateExpression(expr->left);
            if(type.pointer_level() == 0) {
                REPORT(expr->left->location, "Cannot dereference non-pointer types.");
                return TYPE_VOID;
            }

            piece->emit_pop(REG_B);
            piece->emit_mov_rm(REG_A, REG_B, 4);
            piece->emit_push(REG_A);

            type.set_pointer_level(type.pointer_level()-1);
            return type;
            break;   
        }
        case ASTExpression::MEMBER: {
            TypeId type = generateReference(expr);

            piece->emit_pop(REG_B);
            piece->emit_mov_rm(REG_A, REG_B, 4);
            piece->emit_push(REG_A);
            return type;
            break;   
        }
        case ASTExpression::INDEX: {
            Assert(false);
            // TypeId type = generateExpression(expr->left);
            
            // piece->emit_pop(REG_B);
            // piece->emit_mov_rm(REG_A, REG_B, 4);
            // piece->emit_push(REG_A);
            break;   
        }
        case ASTExpression::CAST: {
            TypeId type = generateExpression(expr->left);
            TypeId castType = ast->convertFullType(expr->name, current_scopeId);
            
            if(type == castType) {
                return castType; // casting to the same type is okay, unnecessary, but okay
            }
            
            // casting pointers is okay
            if(type.pointer_level() > 0 && castType.pointer_level() > 0) {
                return castType;
            }
            
            if((type == TYPE_INT || type == TYPE_BOOL || type == TYPE_CHAR)
            && (castType == TYPE_INT || castType == TYPE_BOOL || castType == TYPE_CHAR)) {
                return castType;
            }
            
            if((type == TYPE_INT || type == TYPE_FLOAT) && (castType == TYPE_INT || castType == TYPE_FLOAT)) {
                if(type == TYPE_INT) {
                    piece->emit_pop(REG_A);
                    piece->emit_cast(REG_A, CAST_INT_FLOAT);
                    piece->emit_push(REG_A);
                } else {
                    piece->emit_pop(REG_A);
                    piece->emit_cast(REG_A, CAST_FLOAT_INT);
                    piece->emit_push(REG_A);
                }
                return castType;
            }
            
            REPORT(expr->location, "You cannot cast '"+ast->nameOfType(type)+"' to '"+ast->nameOfType(castType)+"'.");
            return TYPE_VOID;
            break;   
        }
        case ASTExpression::SIZEOF: {
            TypeId type = ast->convertFullType(expr->name, current_scopeId);
            if(!type.valid()) {
                Assert(false); // find variable name instead of type?
            }
            int size = ast->sizeOfType(type);
            piece->emit_li(REG_A, size);
            piece->emit_push(REG_A);
            return TYPE_INT;
        } break;   
        case ASTExpression::ASSIGN: {
            TypeId rtype = generateExpression(expr->right);
            TypeId ltype = generateReference(expr->left);
            if(!ltype.valid() || !rtype.valid()) {
                return {}; // error should be reported already
            }

            if(rtype != ltype) {
                REPORT(expr->location, "The expression and reference has mismatching types.");
                // TODO: Implicit casting
                return {};
            }
            
            piece->emit_pop(REG_B); // pointer to reference/value/variable
            
            generatePop(REG_B, 0, ltype);
            
            generatePush(REG_B, 0, ltype);
            return ltype;
        } break;   
        case ASTExpression::PRE_INCREMENT:
        case ASTExpression::POST_INCREMENT:
        case ASTExpression::PRE_DECREMENT:
        case ASTExpression::POST_DECREMENT: {
            if(expr->left->kind() != ASTExpression::IDENTIFIER) {
                REPORT(expr->left->location, "Increment/decrement operation is only allowed on identifiers.");
                return TYPE_VOID;
            }

            auto variable = findVariable(expr->left->name);
            if(!variable) {
                REPORT(expr->left->location, std::string() + "Variable '" + expr->left->name + "' does not exist.");
                return TYPE_VOID;
            }

            if(variable->typeId != TYPE_INT) {
                REPORT(expr->left->location, std::string() + "Increment/decrement is limited to integer types. Variable '" + expr->left->name + "' is of type '" + ast->nameOfType(variable->typeId) + "'.");
                return TYPE_VOID;
            }

            int var_offset = variable->frame_offset;
            // get the address of the variable
            // a local variable exists on the stack
            // with an offset from the base pointer
            piece->emit_li(REG_B, var_offset);
            piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
            
            // get the value of the variable
            piece->emit_mov_rm(REG_A, REG_B, 4); // reg_a = *reg_b

            switch(expr->kind()){
            case ASTExpression::PRE_INCREMENT:
                piece->emit_incr(REG_A, 1);
                piece->emit_push(REG_A);
                break;
            case ASTExpression::PRE_DECREMENT:
                piece->emit_incr(REG_A, 1);
                piece->emit_push(REG_A);
                break;
            case ASTExpression::POST_INCREMENT:
                piece->emit_push(REG_A);
                piece->emit_incr(REG_A, 1);
                break;
            case ASTExpression::POST_DECREMENT:
                piece->emit_push(REG_A);
                piece->emit_incr(REG_A, -1);
                break;
            default: Assert(false);
            }
            piece->emit_mov_mr(REG_B, REG_A, 4); // *reg_b = reg_a

            return variable->typeId;
        } break;
        case ASTExpression::TRUE: {
            piece->emit_li(REG_A, 1);
            piece->emit_push(REG_A);
            return TYPE_BOOL;
        } break;
        case ASTExpression::FALSE: {
            piece->emit_li(REG_A, 0);
            piece->emit_push(REG_A);
            return TYPE_BOOL;
        } break;
        case ASTExpression::LITERAL_NULL: {
            piece->emit_li(REG_A, 0);
            piece->emit_push(REG_A);
            TypeId p = TYPE_VOID;
            p.set_pointer_level(1);
            return p;
        } break;
        default: Assert(false);
    }
    return TYPE_VOID;
}
bool GeneratorContext::generateBody(ASTBody* body) {
    auto prev_scope = current_scopeId;
    current_scopeId = body->scopeId;
    defer {
        current_scopeId = prev_scope;
    };

    Assert(body);
    int prev_frame_offset = current_frameOffset;
    for(auto stmt : body->statements) {
        // debug line information
        std::string text = function->origin_stream->getline(stmt->location);
        int line = function->origin_stream->getToken(stmt->location)->line;
        piece->push_line(line, text);
        
        bool stop = false;
        switch(stmt->kind()){
        case ASTStatement::EXPRESSION: {
            auto type = generateExpression(stmt->expression);
            generatePop(REG_INVALID, 0, type);
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
                auto type = ast->convertFullType(stmt->declaration_type, current_scopeId);
                if(!type.valid()) {
                    REPORT(stmt->location, "Type in declaration is not a valid type.");
                    return false;
                }
                declaration = true;
                variable = addVariable(stmt->declaration_name, type);
                if(!variable) {
                    REPORT(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is already declared.");
                    break;
                }
            }
            
            int var_offset = variable->frame_offset;
            if(stmt->expression) {
                TypeId type = generateExpression(stmt->expression);
                if(!type.valid()) {
                    return false; // error should be reported already
                }
                if(type != variable->typeId) {
                    REPORT(stmt->location, "The expression and variable has mismatching types.");
                    // TODO: Implicit casting
                    return false;
                }
                piece->emit_mov_rr(REG_B, REG_BP);
                generatePop(REG_B, var_offset, type);
            } else {
                int size = ast->sizeOfType(variable->typeId);
                piece->emit_li(REG_B, var_offset);
                piece->emit_add(REG_B, REG_BP);
                piece->emit_li(REG_A, size);
                piece->emit_memzero(REG_B, REG_A);
            }
            break;
        }
        case ASTStatement::WHILE: {
            int pc_loop = piece->get_pc();
            
            TypeId type = generateExpression(stmt->expression);
            piece->emit_pop(REG_A);
            
            int reloc_while = 0;
            piece->emit_jz(REG_A, &reloc_while);
            
            bool yes = generateBody(stmt->body);
            
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
                if(!function->return_type.valid()) {
                    REPORT(stmt->location, "Function does not have a return value but you specified one here.");
                    return false;
                }
                auto type = generateExpression(stmt->expression);
                if(type != function->return_type) {
                    REPORT(stmt->location, "Type '"+ast->nameOfType(type)+"' in return statement does not match return type '"+ast->nameOfType(function->return_type)+"' of the function.");
                    return false;
                }
                
                piece->emit_mov_rr(REG_B, REG_BP);
                generatePop(REG_B, function->return_offset, type);
            }
            if(current_frameOffset != 0) {
                piece->emit_incr(REG_SP, - (current_frameOffset));
            }
            piece->emit_ret();
            
            stop = true; // return statement makes the rest of the statments useless
            
            if(stop)
                current_frameOffset = prev_frame_offset;
            break;
        }
        default: Assert(false);
        }
        
        if(stop)
            break;
    }
    if(current_frameOffset != prev_frame_offset) {
        // This also happens in the return statement
        // "free" local variables from stack
        // piece->emit_li(REG_B, current_frameOffset - prev_frame_offset);
        // piece->emit_sub(REG_SP, REG_B);
        piece->emit_incr(REG_SP, - (current_frameOffset - prev_frame_offset));
        current_frameOffset = prev_frame_offset;
    }
    
    return true;
}
bool GeneratorContext::generateStruct(ASTStructure* astStruct) {
    /* BUG SCENARIO
        struct A { }
        fun main() {
            struct B {
                a: A;
            }
            struct A {}
        }
    Description:
        Type A in B will use the global A when it should use the local A (language is order-independent and looks at types in current scope first).
        What the code does however is create global types first then go through the local ones one by one. The local A won't have been created before B searches for A.
    Solution:
        We need a first pass to create all struct types.
        Then a second pass to evaluate all types.

        This will be especially hard in a global scope where threads adds types here and there as they are parsed and generated.
    */
    if(astStruct->complete)
        return true;

    if(!astStruct->created_type) {
        astStruct->created_type = true;
        TypeInfo* type = ast->createType(astStruct->name, current_scopeId);
        type->ast_struct = astStruct;
        astStruct->typeInfo = type;
    }
    int offset = 0;
    int alignment = 1;
    for(int j=0;j<astStruct->members.size();j++) {
        auto& mem = astStruct->members[j];
        TypeId type = ast->convertFullType(mem.typeString, current_scopeId);
        if(!type.valid()) {
            Assert(false);
            // error, type doesn't exist.
            return false;
        }
        mem.typeId = type;
        auto size = ast->sizeOfType(type);
        Assert(size != 0); // throw error instead?

        int aligned_size = 1;
        if(size == 2) aligned_size = 2;
        else if(size <= 4) aligned_size = 4;
        else aligned_size = 8;
        if(alignment < aligned_size)
            alignment = aligned_size;
        offset = (offset + (aligned_size-1)) & ~(aligned_size-1);

        mem.offset = offset;
        offset += size;
    }

    offset = (offset + (alignment-1)) & ~(alignment-1);

    astStruct->typeInfo->size = offset;
    astStruct->complete = true;
    return true;
}
void CheckStructs(AST* ast, Reporter* reporter) {
    GeneratorContext context{};
    context.ast = ast;
    context.reporter = reporter;
    
    for(int i=0;i<ast->structures.size();i++) {
        auto st = ast->structures[i];
        bool yes = context.generateStruct(st);
    }
}

void CheckFunction(AST* ast, ASTFunction* function, Reporter* reporter) {
    GeneratorContext context{};
    context.ast = ast;
    context.function = function;
    context.reporter = reporter;
    context.current_scopeId = AST::GLOBAL_SCOPE;
    
    
    int param_offset = 16;
    GeneratorContext::Variable* variable = nullptr;
    for(int i=function->parameters.size()-1; i>=0; i--) {
        auto& param = function->parameters[i];
        param.typeId = ast->convertFullType(param.typeString, context.current_scopeId);
        if(!param.typeId.valid()) {
            REPORT(function->location, "One of the parameters has an invalid type.");
        }
        
        int size = ast->sizeOfType(param.typeId);

        int alignment = (size <= 1 ? 1 : size <= 2 ? 2 : size <= 4 ? 4 : 8) - 1;
        param_offset = (param_offset + alignment) & ~alignment;

        auto variable = context.addVariable(param.name, param.typeId, param_offset);
        if(!variable) {
            REPORT(function->location, "Function paramaters must have unique names. At least two are named '"+param.name+"'.");
        }
        Assert(("Parameter could not be created. Compiler bug!",variable));

        param.offset = param_offset;
        param_offset += size;
    }
    param_offset = (param_offset + 7) & ~7;
    function->parameters_size = param_offset - 16;
    
    if(function->return_typeString.size()!=0) {
        function->return_type = ast->convertFullType(function->return_typeString, context.current_scopeId);
        if(!function->return_type.valid()) {
            REPORT(function->location, "'"+function->return_typeString+"' is not a known type.");
        } else {
            int size = ast->sizeOfType(function->return_type);
            size = (size + 7) & ~7;
            function->return_offset = -size;
        }
    }
}
void GenerateFunction(AST* ast, ASTFunction* function, Code* code, Reporter* reporter) {
    if(function->is_native)
        return; // native funcs can't be generated
    
    GeneratorContext context{};
    context.ast = ast;
    context.function = function;
    context.code = code;
    context.piece = code->createPiece();
    context.reporter = reporter;
    context.current_scopeId = AST::GLOBAL_SCOPE;
    function->piece_code_index = context.piece->code_index;
    
    context.piece->name = function->name;
    context.piece->virtual_sp = 0;
    
    // Setup the base/frame pointer (the call instruction handles this automatically)
    // context.piece->emit_push(REG_BP);
    // context.piece->emit_mov_rr(REG_BP, REG_SP);

    int param_offset = 16;
    GeneratorContext::Variable* variable = nullptr;
    for(int i=function->parameters.size()-1; i>=0; i--) {
        auto& param = function->parameters[i];
        
        // int size = ast->sizeOfType(param.typeId);

        // int alignment = (size <= 1 ? 1 : size <= 2 ? 2 : size <= 4 ? 4 : 8) - 1;
        // param_offset = (param_offset + alignment) & ~alignment;

        // param.offset = param_offset;
        // param_offset += size;
        auto variable = context.addVariable(param.name, param.typeId, param.offset);
        if(!variable) {
            REPORT(function->location, "Function paramaters must have unique names. At least two are named '"+param.name+"'.");
        }
        Assert(("Parameter could not be created. Compiler bug!",variable));

    }
    // param_offset = (param_offset + 7) & ~7;
    // function->parameters_size = param_offset - 16;
    
    if(function->return_type.valid()) {
        int size = ast->sizeOfType(function->return_type);
        size = (size + 7) & ~7;
        context.piece->emit_incr(REG_SP, -size);
        context.current_frameOffset -= size;
        // function->return_offset = -size;
    }
    
    context.generateBody(function->body);
 
    // Emit ret instruction if the user forgot the return statement
    if(context.piece->instructions.back().opcode != INST_RET) {
        if(context.current_frameOffset != 0) {
            context.piece->emit_incr(REG_SP, -(context.current_frameOffset));
        }
        context.piece->emit_ret();
    }
}

GeneratorContext::Variable* GeneratorContext::addVariable(const std::string& name, TypeId type, int frame_offset) {
    auto pair = local_variables.find(name);
    if(pair != local_variables.end())
        return nullptr; // variable already exists
        
    auto ptr = new Variable();
    ptr->name = name;
    ptr->typeId = type;
    local_variables[name] = ptr;
    if(frame_offset == 0) {
        int size = ast->sizeOfType(type);
        size = (size + 7) & ~7;
        piece->emit_incr(REG_SP, -size);
        current_frameOffset -= size;
        ptr->frame_offset = current_frameOffset;
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