#include "Generator.h"

#define LOCATION log_color(GRAY); printf("%s:%d\n",__FILE__,__LINE__); log_color(NO_COLOR);
#define REPORT(L, ...) LOCATION reporter->err(current_stream, L, __VA_ARGS__)


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
    ZoneScopedC(tracy::Color::Blue2);
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
            case ASTExpression::INDEX: {
                exprs.push_back(expr);
                expr = expr->left;
            } break;
            default: {
                REPORT(expr->location, "Invalid expression/operation in reference.");
                return TYPE_VOID;
                // Assert(false);
            }
        }
    }
    enum PtrType {
        INVALID,
        LOCAL,
        VALUE,
    };
    PtrType ptrType = INVALID;
    TypeId type = TYPE_VOID;
    int offset = 0;
    for(int i=exprs.size()-1;i>=0;i--) {
        auto expr = exprs[i];
        
        switch(expr->kind()) {
            case ASTExpression::IDENTIFIER: {
                Assert(ptrType == INVALID);
                auto variable = ast->findVariable(expr->name, current_scopeId);
                if(!variable) {
                    REPORT(expr->location, std::string() + "Variable '" + expr->name + "' does not exist.");
                    return TYPE_VOID;
                }
                switch(variable->kind) {
                    case Identifier::LOCAL_ID: {
                        offset = variable->offset;
                        type = variable->type;
                        ptrType = LOCAL;
                    } break;
                    case Identifier::GLOBAL_ID: {
                        // emit push pointer to global data section
                        offset = 0;
                        type = variable->type;
                        ptrType = VALUE;
                        piece->emit_dataptr(REG_B, variable->offset);
                    } break;
                    case Identifier::CONST_ID: {
                        REPORT(expr->location, "Cannot reference constants.");
                        return TYPE_VOID;
                    } break;
                    default: Assert(false);
                }
            } break;
            case ASTExpression::MEMBER: {
                Assert(ptrType != INVALID);
                auto prev_expr = exprs[i+1];
                if(type.pointer_level() == 0) {
                    auto info = ast->getType(type);
                    if(!info->ast_struct) {
                        REPORT(expr->location, std::string() + "Member access is only allowed on structure types. '"+prev_expr->name+"' is '"+ast->nameOfType(type) + "'.");
                        return TYPE_VOID;
                    }
                    auto mem = info->ast_struct->findMember(expr->name);
                    if(!mem) {
                        REPORT(expr->location, std::string() + "'"+ expr->name+"' is not a member of '"+ast->nameOfType(type) + "'.");
                        return TYPE_VOID;
                    }
                    offset += mem->offset;
                    type = mem->typeId;
                } else if(type.pointer_level() == 1) {
                    // implicit dereference
                    auto info = ast->getType(type.base());
                    if(!info->ast_struct) {
                        REPORT(expr->location, std::string() + "Member access is only allowed on structure types. '"+prev_expr->name+"' is '"+ast->nameOfType(type) + "'.");
                        return TYPE_VOID;
                    }
                    auto mem = info->ast_struct->findMember(expr->name);
                    if(!mem) {
                        REPORT(expr->location, std::string() + "'"+ expr->name+"' is not a member of '"+ast->nameOfType(type) + "'.");
                        return TYPE_VOID;
                    }
                    if(ptrType == LOCAL) {
                        piece->emit_mov_rm_disp(REG_B, REG_BP, 8, offset);
                    } else {
                        piece->emit_mov_rm_disp(REG_B, REG_B, 8, offset);
                    }
                    offset = mem->offset;
                    type = mem->typeId;
                    ptrType = VALUE;
                } else {
                    REPORT(expr->location, std::string() + "Member access does not work with pointer levels above 1. '"+prev_expr->name+"' is '"+ast->nameOfType(type) + "'.");
                    return TYPE_VOID;
                }
            } break;
            case ASTExpression::INDEX: {
                Assert(ptrType != INVALID);
                auto prev_expr = exprs[i+1];
                if(type.pointer_level() > 0) {
                    if(ptrType == LOCAL) {
                        piece->emit_mov_rm_disp(REG_B, REG_BP, 8, offset);
                    } else {
                        piece->emit_mov_rm_disp(REG_B, REG_B, 8, offset);
                    }

                    piece->emit_push(REG_B);

                    auto rtype = generateExpression(expr->right);
                    if(rtype != TYPE_INT) {
                        REPORT(expr->location, std::string() + "Expression within brackets of indexing operator must be an integer. '"+ast->nameOfType(type) + "' is not an integer.");
                        return TYPE_VOID;
                    }
                    TypeId elem_type = type;
                    elem_type.set_pointer_level(elem_type.pointer_level() - 1);
                    piece->emit_pop(REG_A);
                    piece->emit_pop(REG_B);
                    piece->emit_li(REG_C, ast->sizeOfType(elem_type));
                    piece->emit_mul(REG_A, REG_C);
                    piece->emit_add(REG_B, REG_A);

                    offset = 0;
                    type = elem_type;
                    ptrType = VALUE;
                } else {
                    REPORT(expr->location, std::string() + "Indexing is only allowed on pointer types. '"+ast->nameOfType(type) + "' is not a pointer.");
                    return TYPE_VOID;
                }
            } break;
            default: Assert(false);
        }
    }

    if(ptrType == LOCAL) {
        if(offset) {
            piece->emit_li(REG_B, offset);
            piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
        } else {
            piece->emit_mov_rr(REG_B, REG_BP); // reg_b = reg_bp
        }
    } else {
        if(offset) {
            piece->emit_li(REG_A, offset);
            piece->emit_add(REG_B, REG_A); // reg_b = reg_b + var_offset
        }
    }
    piece->emit_push(REG_B); // push pointer
    return type;
}
TypeId GeneratorContext::generateExpression(ASTExpression* expr) {
    ZoneScopedC(tracy::Color::Blue2);
    Assert(expr);
    switch(expr->kind()) {
        case ASTExpression::LITERAL_INT: {
            piece->emit_li(REG_A, expr->literal_integer);
            piece->emit_push(REG_A);
            return TYPE_INT;
        } break;
        case ASTExpression::LITERAL_FLOAT: {
            piece->emit_li(REG_A, *(int*)&expr->literal_float);
            piece->emit_push(REG_A);
            return TYPE_FLOAT;
        } break;
        case ASTExpression::LITERAL_STR: {
            int off = code->appendString(expr->literal_string);
            piece->emit_dataptr(REG_A, off);
            piece->emit_push(REG_A);
            
            TypeId type = TYPE_CHAR;
            type.set_pointer_level(1);
            return type;
        } break;
        case ASTExpression::IDENTIFIER: {
            auto variable = ast->findVariable(expr->name, current_scopeId);
            if(!variable) {
                REPORT(expr->location, std::string() + "Variable '" + expr->name + "' does not exist.");
                break;
            }
            switch(variable->kind) {
                case Identifier::LOCAL_ID: {
                    int var_offset = variable->offset;
                    // get the address of the variable
                    // a local variable exists on the stack
                    // with an offset from the base pointer
                    // piece->emit_li(REG_B, var_offset);
                    // piece->emit_add(REG_B, REG_BP); // reg_b = reg_bp + var_offset
                    
                    // // get the value of the variable
                    // piece->emit_mov_rm(REG_A, REG_B, 4); // reg_a = *reg_b
                    // piece->emit_push(REG_A);
                    
                    // piece->emit_mov_rr(REG_B, REG_BP);
                    generatePush(REG_BP, var_offset, variable->type);
                    return variable->type;
                } break;
                case Identifier::GLOBAL_ID: {
                    piece->emit_dataptr(REG_B, variable->offset);
                    generatePush(REG_B, 0, variable->type);
                    return variable->type;
                } break;
                case Identifier::CONST_ID: {
                    Assert(variable->statement && variable->statement->expression);
                    auto e = variable->statement->expression;
                    if(!e->isConst()) {
                        REPORT(e->location, "Constant is not a valid const expression. Only literals are allowed in constants (no operations at all)");
                        return TYPE_VOID;
                    }
                    auto type = generateExpression(e);
                    return type;
                } break;
                default: Assert(false);
            }
            return TYPE_VOID;
       
        } break;
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
            auto pvoid = TypeId::Make(TYPE_VOID, 1);
            for(int i=0;i<argument_types.size();i++) {
                auto& ltype = fun->parameters[i].typeId;
                auto& rtype = argument_types[i];
                if(ltype == rtype) {

                } else if((ltype == pvoid && rtype.pointer_level() == 1) || (ltype.pointer_level() == 1 && rtype == pvoid) ) {

                } else {
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

            // piece->emit_mov_rm_disp(REG_A, REG_B, 4, 16);
            // piece->emit_mov_rm_disp(REG_A, REG_B, 8, 24);
            // piece->emit_mov_rm_disp(REG_A, REG_B, 8, 32);

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
            
            if(ltype == rtype) {
            
            } else if((ltype == TYPE_INT || ltype == TYPE_CHAR || ltype.pointer_level()>0) && (rtype == TYPE_INT || rtype == TYPE_CHAR || rtype.pointer_level()>0)) {

            } else if((ltype == TYPE_INT || ltype == TYPE_FLOAT) && (rtype == TYPE_INT || rtype == TYPE_FLOAT)) {
                
            } else {
                if(reporter->errors == 0 || (ltype != TYPE_VOID && rtype != TYPE_VOID)) {
                    REPORT(expr->location, "Cannot perform operation on the types '"+ast->nameOfType(ltype)+"', '"+ast->nameOfType(rtype)+"'.");
                }
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
            type.set_pointer_level(type.pointer_level() + 1);
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
            generatePush(REG_B, 0, type);

            type.set_pointer_level(type.pointer_level()-1);
            return type;
            break;   
        }
        case ASTExpression::MEMBER: {
            TypeId type = generateReference(expr);
            if(!type.valid())
                return TYPE_VOID;
            piece->emit_pop(REG_B);

            generatePush(REG_B, 0, type);
            return type;
            break;   
        }
        case ASTExpression::INDEX: {
            TypeId rtype = generateExpression(expr->right);
            TypeId ltype = generateExpression(expr->left);

            if(ltype.pointer_level() == 0) {
                REPORT(expr->left->location, "Index operator only works on pointers. '"+ast->nameOfType(ltype)+"' is not a pointer.");
                return TYPE_VOID;
            }
            if(rtype != TYPE_INT) {
                REPORT(expr->left->location, "Value in brackets of index operator must be an integer. '"+ast->nameOfType(rtype)+"' is not an integer.");
                return TYPE_VOID;
            }
            
            TypeId elem_type = ltype;
            elem_type.set_pointer_level(elem_type.pointer_level()-1);

            piece->emit_pop(REG_B);
            piece->emit_pop(REG_A);
            piece->emit_li(REG_C, ast->sizeOfType(elem_type));
            piece->emit_mul(REG_A, REG_C);
            piece->emit_add(REG_B, REG_A);
            
            generatePush(REG_B, 0, elem_type);
            return elem_type;
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
            auto pvoid = TypeId::Make(TYPE_VOID,1);
            if(rtype == ltype) {

            } else if((ltype == pvoid && rtype.pointer_level() == 1) || (ltype.pointer_level() == 1 && rtype == pvoid) ) {

            } else {

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
            auto type = generateReference(expr->left);
            if(!type.valid())
                return TYPE_VOID;
            // if(expr->left->kind() != ASTExpression::IDENTIFIER) {
            //     REPORT(expr->left->location, "Increment/decrement operation is only allowed on references (identifiers, members, dereference pointers).");
            //     return TYPE_VOID;
            // }

            // auto variable = findVariable(expr->left->name);
            // if(!variable) {
            //     REPORT(expr->left->location, std::string() + "Variable '" + expr->left->name + "' does not exist.");
            //     return TYPE_VOID;
            // }

            if(type != TYPE_INT) {
                REPORT(expr->left->location, std::string() + "Increment/decrement is limited to integer types. Type was '" + ast->nameOfType(type) + "'.");
                return TYPE_VOID;
            }

            piece->emit_pop(REG_B);
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

            return type;
        } break;
        case ASTExpression::LITERAL_TRUE: {
            piece->emit_li(REG_A, 1);
            piece->emit_push(REG_A);
            return TYPE_BOOL;
        } break;
        case ASTExpression::LITERAL_FALSE: {
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

GeneratorContext::LoopScope* GeneratorContext::pushLoop(int continue_pc, int cur_frame_offset){
    auto ptr = new LoopScope();
    ptr->continue_pc = continue_pc;
    ptr->frame_offset = cur_frame_offset;
    loopScopes.push_back(ptr);
    return ptr;
}
void GeneratorContext::popLoop(){
    loopScopes.pop_back();
}
bool GeneratorContext::generateBody(ASTBody* body) {
    ZoneScopedC(tracy::Color::Blue2);
    auto prev_scope = current_scopeId;
    current_scopeId = body->scopeId;
    defer {
        current_scopeId = prev_scope;
    };

    Assert(body);
    int prev_frame_offset = current_frameOffset;
    for(auto stmt : body->statements) {
        // debug line information
        std::string text = current_stream->getline(stmt->location);
        int line = current_stream->getToken(stmt->location)->line;
        if(piece)
            piece->push_line(line, text);
        
        bool stop = false;
        switch(stmt->kind()){
        case ASTStatement::EXPRESSION: {
            auto type = generateExpression(stmt->expression);
            generatePop(REG_INVALID, 0, type);
        } break;   
        case ASTStatement::GLOBAL_DECLARATION: {
            // handled elsewhere
        //     auto type = ast->convertFullType(stmt->declaration_type, current_scopeId);
        //     if(!type.valid()) {
        //         REPORT(stmt->location, "Type in declaration is not a valid type.");
        //         return false;
        //     }
        //     int size = ast->sizeOfType(type);
        //     int frame_offset = code->appendData(size);
            
        //     Identifier* variable = ast->addVariable(Identifier::GLOBAL_ID, stmt->declaration_name, current_scopeId, type, frame_offset);
        //     if(!variable) {
        //         REPORT(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is already declared.");
        //         break;
        //     }
        } break;
        case ASTStatement::CONST_DECLARATION: {
            auto type = ast->convertFullType(stmt->declaration_type, current_scopeId);
            if(!type.valid()) {
                REPORT(stmt->location, "Type in declaration is not a valid type.");
                return false;
            }
            int frame_offset = 0;
            Identifier* variable = ast->addVariable(Identifier::CONST_ID, stmt->declaration_name, current_scopeId, type, frame_offset);
            if(!variable) {
                REPORT(stmt->location, std::string() + "Constant '" + stmt->declaration_name + "' is already declared as a variable or constant.");
                break;
            }
            variable->statement = stmt;
        } break;
        case ASTStatement::VAR_DECLARATION: {
            auto type = ast->convertFullType(stmt->declaration_type, current_scopeId);
            if(!type.valid()) {
                REPORT(stmt->location, "Invalid type in declaration.");
                return false;
            }
            int size = ast->sizeOfType(type);
            size = (size + 7) & ~7;
            piece->emit_incr(REG_SP, -size);
            current_frameOffset -= size;
            int frame_offset = current_frameOffset;
            
            Identifier* variable = ast->addVariable(Identifier::LOCAL_ID, stmt->declaration_name, current_scopeId, type, frame_offset);
            if(!variable) {
                REPORT(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is already declared.");
                break;
            }
            int var_offset = variable->offset;
            if(stmt->expression) {
                TypeId type = generateExpression(stmt->expression);
                if(!type.valid()) {
                    return false; // error should be reported already
                }
                auto ltype = type;
                auto rtype = variable->type;
                if(type == variable->type) {

                } else if(ltype.pointer_level() == rtype.pointer_level()) {

                } else {
                    REPORT(stmt->location, "The expression and variable has mismatching types.");
                    // TODO: Implicit casting
                    return false;
                }
                generatePop(REG_BP, var_offset, type);
            } else {
                int size = ast->sizeOfType(variable->type);
                piece->emit_li(REG_B, var_offset);
                piece->emit_add(REG_B, REG_BP);
                piece->emit_li(REG_A, size);
                piece->emit_memzero(REG_B, REG_A);
            }
        } break;
        case ASTStatement::WHILE: {
            auto loop = pushLoop(piece->get_pc(), current_frameOffset);
            
            TypeId type = generateExpression(stmt->expression);
            piece->emit_pop(REG_A);
            
            int reloc_while = 0;
            piece->emit_jz(REG_A, &reloc_while);
            
            generateBody(stmt->body);
            
            piece->emit_jmp(loop->continue_pc);
            
            piece->fix_jump_here(reloc_while);
            for(auto imm_offset : loop->breaks_to_resolve) {
                piece->fix_jump_here(imm_offset);
            }
            popLoop();
        } break;
        case ASTStatement::CONTINUE: {
            if(loopScopes.size() == 0) {
                REPORT(stmt->location, "Continue statements are only allowed in loops.");
                return false;
            }
            auto loop = loopScopes.back();

            if(current_frameOffset != loop->frame_offset) {
                piece->emit_incr(REG_SP, loop->frame_offset - (current_frameOffset));
            }
            
            piece->emit_jmp(loop->continue_pc);
            
            stop = true; // continue statement makes the rest of the statments useless
            if(stop)
                current_frameOffset = prev_frame_offset;
        } break;
        case ASTStatement::BREAK: {
            if(loopScopes.size() == 0) {
                REPORT(stmt->location, "Break statements are only allowed in loops.");
                return false;
            }
            auto loop = loopScopes.back();

            if(current_frameOffset != loop->frame_offset) {
                piece->emit_incr(REG_SP, loop->frame_offset - (current_frameOffset));
            }
            
            int reloc;
            piece->emit_jmp(&reloc);
            loop->breaks_to_resolve.push_back(reloc);
            
            stop = true; // continue statement makes the rest of the statments useless
            if(stop)
                current_frameOffset = prev_frame_offset;
        } break;
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
        } break;
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
                
                // piece->emit_mov_rr(REG_B, REG_BP);
                generatePop(REG_BP, function->return_offset, type);
            }
            if(current_frameOffset != 0) {
                piece->emit_incr(REG_SP, - (current_frameOffset));
            }
            piece->emit_ret();
            
            stop = true; // return statement makes the rest of the statments useless
            if(stop)
                current_frameOffset = prev_frame_offset;
        } break;
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
            if(!ignore_errors) {
                REPORT(mem.location, "Invalid type '"+mem.typeString+"'.");
            }
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
bool CheckStructs(AST* ast, AST::Import* imp, Reporter* reporter, bool* changed, bool ignore_errors) {
    ZoneScopedC(tracy::Color::Green3);
    GeneratorContext context{};
    context.ast = ast;
    context.reporter = reporter;
    context.ignore_errors = ignore_errors;
    
    // three cases can occur
    // we check structs and all types are valid, success
    // we check but some types don't exist, failure print errors
    // we check but some types don't exist yet, try again later

    bool failure = false;
    *changed = false;

    for(int i=0;i<imp->body->structures.size();i++) {
        auto st = imp->body->structures[i];
        if(st->complete)
            continue;

        context.current_stream = imp->stream;
        bool yes = context.generateStruct(st);
        if(yes) {
            *changed = true;
        } else {
            failure = true;
        }
    }

    return !failure;
}

bool CheckFunction(AST* ast, AST::Import* imp, ASTFunction* function, Reporter* reporter) {
    ZoneScopedC(tracy::Color::Green3);
    GeneratorContext context{};
    context.ast = ast;
    context.function = function;
    context.reporter = reporter;
    context.current_scopeId = imp->body->scopeId;
    context.current_stream = function->origin_stream;

    auto current_stream = context.current_stream;
    
    int param_offset = 16;
    // for(int i=function->parameters.size()-1; i>=0; i--) {
    for(int i=0;i<function->parameters.size(); i++) {
        auto& param = function->parameters[i];
        param.typeId = ast->convertFullType(param.typeString, context.current_scopeId);
        if(!param.typeId.valid()) {
            REPORT(function->location, "One of the parameters has an invalid type.");
        }
        
        int size = ast->sizeOfType(param.typeId);

        int alignment = (size <= 1 ? 1 : size <= 2 ? 2 : size <= 4 ? 4 : 8) - 1;
        param_offset = (param_offset + alignment) & ~alignment;
        
        if(function->body) {
            auto variable = ast->addVariable(Identifier::LOCAL_ID, param.name, function->body->scopeId, param.typeId, param_offset);
            if(!variable) {
                REPORT(function->location, "Function paramaters must have unique names. At least two are named '"+param.name+"'.");
            }
            Assert(("Parameter could not be created. Compiler bug!",variable));
        }

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
    return true;
}
bool CheckGlobals(AST* ast, AST::Import* imp, Bytecode* code, Reporter* reporter) {
    ZoneScopedC(tracy::Color::Green3);
    GeneratorContext context{};
    context.ast = ast;
    context.code = code;
    context.reporter = reporter;
    context.current_scopeId = imp->body->scopeId;
    context.current_stream = imp->stream;
    
    // bool failure = context.generateBody(imp->body);

    bool failure = false;
    auto current_stream = imp->stream;
    std::vector<ASTBody*> check_bodies;
    check_bodies.push_back(imp->body);
    while(check_bodies.size() > 0) {
        ASTBody* body = check_bodies.back();
        check_bodies.pop_back();
        auto current_scopeId = body->scopeId;
        for(auto f : body->functions) {
            if(f->body)
                check_bodies.push_back(f->body);
        }
        for(auto stmt : body->statements) {
            if(stmt->body) {
                check_bodies.push_back(stmt->body);
            }
            if(stmt->elseBody) {
                check_bodies.push_back(stmt->elseBody);
            }
            if(stmt->kind() != ASTStatement::GLOBAL_DECLARATION)
                continue;
            // switch(stmt->kind()) {
            //     case ASTStatement::GLOBAL_DECLARATION: {
                    auto type = ast->convertFullType(stmt->declaration_type, current_scopeId);
                    if(!type.valid()) {
                        REPORT(stmt->location, "Invalid type '"+stmt->declaration_type+"'.");
                        failure = true;
                        break;
                    }
                    
                    int size = ast->sizeOfType(type);
                    
                    int global_offset = code->appendData(size);
                    auto id = ast->addVariable(Identifier::GLOBAL_ID, stmt->declaration_name, current_scopeId, type, global_offset);
                    if(!id) {
                        // A good error message would tell the user where the variable was declared.
                        REPORT(stmt->location, "Global variable '"+stmt->declaration_name+"' already exists.");
                        failure = true;
                        break;
                    }
                    // log_color(YELLOW);
                    // printf("CheckGlobals: Expression of global variables are ignored at the moment. TODO: Evaluate in the start of 'main'.\n");
                    // log_color(NO_COLOR);
            //     } break;
            //     // case ASTStatement::CONST_DECLARATION: {
            //     //     Identifier* variable = nullptr;
            //     //     auto type = ast->convertFullType(stmt->declaration_type, current_scopeId);
            //     //     if(!type.valid()) {
            //     //         REPORT(stmt->location, "Type in declaration is not a valid type.");
            //     //         failure = true;
            //     //         break;
            //     //     }
            //     //     int frame_offset = 0;
            //     //     variable = ast->addVariable(Identifier::CONST_ID, stmt->declaration_name, current_scopeId, type, frame_offset);
            //     //     if(!variable) {
            //     //         REPORT(stmt->location, std::string() + "Variable '" + stmt->declaration_name + "' is already declared.");
            //     //         failure = true;
            //     //         break;
            //     //     }
            //     //     variable->statement = stmt;
            //     // } break;
            //     default: Assert(false);
            // }
        }
    }
    return !failure;
}
void GenerateFunction(AST* ast, ASTFunction* function, Bytecode* code, Reporter* reporter) {
    ZoneScopedC(tracy::Color::Blue2);
    if(function->is_native)
        return; // native funcs can't be generated
    
    GeneratorContext context{};
    context.ast = ast;
    context.function = function;
    context.code = code;
    context.piece = code->createPiece();
    context.reporter = reporter;
    context.current_scopeId = AST::GLOBAL_SCOPE;
    function->piece_code_index = context.piece->piece_index;
    
    context.piece->name = function->name;
    context.piece->virtual_sp = 0;
    context.current_stream = function->origin_stream;
    auto current_stream = context.current_stream;

    if(function->name == "main") {
        int prev_scope = context.current_scopeId;
        context.current_scopeId = function->body->scopeId;
        defer { context.current_scopeId = prev_scope; };
        
        context.piece->push_line(0, "<set-globals>");
        
        // Find all globals in all imports and set their default value at the start of the main function
        for(auto imp : ast->imports) {
            std::vector<ASTBody*> check_bodies;
            check_bodies.push_back(imp->body);
            while(check_bodies.size() > 0) {
                ASTBody* body = check_bodies.back();
                check_bodies.pop_back();
                context.current_scopeId = body->scopeId;
                for(auto f : body->functions) {
                    if(f->body)
                        check_bodies.push_back(f->body);
                }
                for(auto stmt : body->statements) {
                    if(stmt->body)
                        check_bodies.push_back(stmt->body);
                    if(stmt->elseBody)
                        check_bodies.push_back(stmt->elseBody);

                    if(stmt->kind() != ASTStatement::GLOBAL_DECLARATION)
                        continue;
                    
                    auto id = ast->findVariable(stmt->declaration_name, context.current_scopeId);
                    if(!id) {
                        Assert(reporter->errors != 0);
                        continue;
                    }
                    
                    if(!stmt->expression) {
                        context.piece->emit_dataptr(REG_B, id->offset);
                        int size = ast->sizeOfType(id->type);
                        context.piece->emit_li(REG_A, size);
                        context.piece->emit_memzero(REG_B, REG_A);
                    } else {
                        TypeId type = context.generateExpression(stmt->expression);
                        if(!type.valid())
                            continue;
                        
                        if(id->type != type) {
                            REPORT(stmt->expression->location, "Type in expression does not match type in declaration. '"+ast->nameOfType(id->type)+"' != '"+ast->nameOfType(type)+"'");
                            continue;
                        }
                    
                        context.piece->emit_dataptr(REG_B, id->offset);
                        context.generatePop(REG_B, 0, type);
                    }
                }
            }
        }
    }
    
    if(function->return_type.valid()) {
        int size = ast->sizeOfType(function->return_type);
        size = (size + 7) & ~7;
        context.piece->emit_incr(REG_SP, -size);
        context.current_frameOffset -= size;
        // function->return_offset = -size;
    }
    
    context.generateBody(function->body);
 
    // Emit ret instruction if the user forgot the return statement
    if(context.piece->instructions.size() > 0 && context.piece->instructions.back().opcode != INST_RET) {
        if(function->return_type != TYPE_VOID) {
            REPORT(function->location, "Missing return statement. They are mandatory if the function has a return type.");
        }
        if(context.current_frameOffset != 0) {
            context.piece->emit_incr(REG_SP, -(context.current_frameOffset));
        }
        context.piece->emit_ret();
    }
}

#undef LOCATION
#undef REPORT