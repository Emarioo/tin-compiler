#include "TinGenerator.h"
#include "AST.h"
#include "Util.h"

void GenerateTin(TinConfig* config) {
    TinContext context{};
    context.config = config;
    context.output = "";

    if(config->seed == 0) {
        config->seed = (int)time(NULL);
        printf("Seed: %d\n", config->seed);
    }
    SetRandomSeed(config->seed);

    // printf("start gen\n");

    context.genProgram();

    // printf("gen done\n");

    std::string path = "sample.tin";
    std::ofstream file(path, std::ofstream::binary);
    if(!file.is_open())
        return;

    file.write(context.output.data(), context.output.length());
    file.close();
}

void TinContext::genProgram() {
    pushScope();
    
    {
        type_int = createType("int");
        addType(type_int);
    }
    {
        type_float = createType("float");
        addType(type_float);
    }
    {
        type_char = createType("char");
        addType(type_char);
    }
    {
        type_bool = createType("bool");
        addType(type_bool);
    }

    int struct_count = RandomInt(config->struct_frequency.min, config->struct_frequency.max);
    for(int i=0;i<struct_count;i++) {
        std::string name = "Type" + std::to_string(type_counter++);
        output += "struct ";
        output += name;
        output += " {\n";
        
        auto id = createType(name);

        indent_level++;
        int member_count = RandomInt(config->member_frequency.min, config->member_frequency.max);
        for(int j=0;j<member_count;j++) {
            if(j!=0) output +=", ";
            std::string name = "mem"+std::to_string(j);
            auto type = requestType();
            // std::string type = "int";
            indent();
            output += name;
            output += ": ";
            output += type.to_string();
            output += "\n";
            id->members.push_back({name, type});
        }
        addType(id);
        indent_level--;

        output += "}\n";
    }
    int fun_count = RandomInt(config->function_frequency.min, config->function_frequency.max);
    for(int i=0;i<fun_count;i++) {
        std::string name = "Func" + std::to_string(fun_counter++);
        output += "fun ";
        output += name;
        output += "(";

        auto func = addFunc(name);

        pushScope();

        int arg_count = RandomInt(config->argument_frequency.min, config->argument_frequency.max);
        for(int j=0;j<arg_count;j++) {
            if(j != 0)
                output += ", ";

            std::string name = "arg"+std::to_string(j);
            auto t = requestType();
            output += name;
            output += ": ";
            output += t.to_string();

            auto id = addVar(name, Identifier::LOCAL);
            id->type = t;
            
            func->args.push_back({name, t});
        }

        output += ")";

        output += ": ";
        auto ret_type = requestPrimitive();
        func->return_type = ret_type;
        output += ret_type.to_string();

        output += " {\n";
        indent_level++;

        current_function = func;
        genStatements(true);

        popScope();

        indent();
        output += "return ";
        genExpression(func->return_type, 0);
        output += ";\n";

        indent_level--;
        output += "}\n";
    }
    popScope();

}
void TinContext::genStatements(bool inherit_scope) {
    if(!inherit_scope)
        pushScope();
    // printf("gen stmt %d\n",indent_level);
    int stmt_count = RandomInt(config->statement_frequency.min, config->statement_frequency.max);
    for(int i=0;i<stmt_count;i++) {
        ASTStatement::Kind kind = (ASTStatement::Kind)RandomInt(ASTStatement::KIND_BEGIN, ASTStatement::KIND_END-1);

        if(indent_level > 5) {
            if(kind == ASTStatement::IF || kind == ASTStatement::WHILE)
                continue;
        }

        switch(kind) {
            case ASTStatement::VAR_DECLARATION: {
                indent();
                auto type = requestType();
                std::string name = "var"+std::to_string(var_counter++);
                output += name +": "+type.to_string()+";\n";
                
                auto id = addVar(name, Identifier::LOCAL);
                id->type = type;
            } break;
            case ASTStatement::GLOBAL_DECLARATION: {
                indent();
                auto type = requestType();
                std::string name = "g_var"+std::to_string(var_counter++);
                output += "global "+ name + ": " + type.to_string() + ";\n";
                
                auto id = addVar(name, Identifier::GLOBAL);
                id->type = type;
            } break;
            case ASTStatement::CONST_DECLARATION: {
                indent();
                
                auto type = type_int;
                std::string name = "CONST"+std::to_string(var_counter++);
                int num = RandomInt(-1000,1000);
                output += "const "+ name + ": "+type->name+" = "+ std::to_string(num)+";\n";
                
                auto id = addVar(name, Identifier::CONSTANT);
                id->type = {type};
            } break;
            case ASTStatement::EXPRESSION: {
                indent();
                genExpression({}, 0);
                output += ";\n";
            } break;
            case ASTStatement::IF: {
                indent();
                output += "if ";
                genExpression({type_bool},0);

                output += " {\n";
                indent_level++;
                
                genStatements();

                indent_level--;
                indent();
                output += "}\n";

                indent();
                output += "else {\n";
                indent_level++;

                genStatements();

                indent_level--;
                indent();
                output += "}\n";

            } break;
            case ASTStatement::WHILE: {
                indent();
                output += "while ";
                genExpression({type_bool}, 0);

                output += " {\n";
                indent_level++;
                
                bool prev_in_loop = in_loop;
                in_loop = true;
                genStatements();
                in_loop = prev_in_loop;

                indent_level--;
                indent();
                output += "}\n";
            } break;
            case ASTStatement::BREAK: {
                if(!in_loop)
                    break;
                indent();
                output += "break;\n";
            } break;
            case ASTStatement::CONTINUE: {
                if(!in_loop)
                    break;
                indent();
                output += "continue;\n";
            } break;
            case ASTStatement::RETURN: {
                indent();
                output += "return ";
                genExpression(current_function->return_type, 0);
                output += ";\n";

                i = stmt_count; // quit
            } break;
            default: Assert(false);
        }
    }
    if(!inherit_scope)
        popScope();
}
#define ARR_LEN(A) (sizeof(A)/sizeof(*A))
static const ASTExpression::Kind binary_kinds[]{
    // ASTExpression::FUNCTION_CALL,
    ASTExpression::ADD,
    ASTExpression::SUB,
    ASTExpression::DIV,
    ASTExpression::MUL,
    ASTExpression::AND,
    ASTExpression::OR,
    ASTExpression::EQUAL,
    ASTExpression::NOT_EQUAL,
    ASTExpression::LESS,
    ASTExpression::GREATER,
    ASTExpression::LESS_EQUAL,
    ASTExpression::GREATER_EQUAL,
    // ASTExpression::INDEX,
    ASTExpression::ASSIGN,
    ASTExpression::NOT,
    // ASTExpression::REFER, // take reference
    // ASTExpression::DEREF, // dereference
    // ASTExpression::MEMBER,
    // ASTExpression::CAST,
};
static const ASTExpression::Kind binary_kinds_int[]{
    // ASTExpression::FUNCTION_CALL,
    ASTExpression::ADD,
    ASTExpression::SUB,
    ASTExpression::DIV,
    ASTExpression::MUL,
    // ASTExpression::INDEX,
    // ASTExpression::ASSIGN,
    // ASTExpression::DEREF, // dereference
    // ASTExpression::MEMBER,
};
static const ASTExpression::Kind binary_kinds_bool[]{
    ASTExpression::AND,
    ASTExpression::OR,
    ASTExpression::NOT,
    ASTExpression::EQUAL,
    ASTExpression::NOT_EQUAL,
    ASTExpression::LESS,
    ASTExpression::GREATER,
    ASTExpression::LESS_EQUAL,
    ASTExpression::GREATER_EQUAL,
};
static const ASTExpression::Kind value_kinds[]{
    ASTExpression::IDENTIFIER,
    // ASTExpression::PRE_INCREMENT,
    // ASTExpression::POST_INCREMENT,
    // ASTExpression::PRE_DECREMENT,
    // ASTExpression::POST_DECREMENT,

    ASTExpression::SIZEOF,
    ASTExpression::LITERAL_INT,
    ASTExpression::LITERAL_FLOAT,
    ASTExpression::LITERAL_STR,
    ASTExpression::LITERAL_NULL,
    ASTExpression::LITERAL_TRUE,
    ASTExpression::LITERAL_FALSE,
};
static const ASTExpression::Kind value_kinds_int[]{
    ASTExpression::IDENTIFIER,
    // ASTExpression::PRE_INCREMENT,
    // ASTExpression::POST_INCREMENT,
    // ASTExpression::PRE_DECREMENT,
    // ASTExpression::POST_DECREMENT,

    ASTExpression::SIZEOF,
    ASTExpression::LITERAL_INT,
};
static const ASTExpression::Kind value_kinds_bool[]{
    ASTExpression::IDENTIFIER,
    ASTExpression::LITERAL_TRUE,
    ASTExpression::LITERAL_FALSE,
};
void TinContext::genExpression(ComplexType expected_type, int expr_depth) {
    // auto id = requestVariable();
    // if(id) {
    //     output += id->name;
    // } else 
    //     output += "0";
    // return;
    expr_depth++;
    if(expr_depth == 0)
        expr_count++;
    
    float exponential_chance = (1.f-expr_count/10.f) * (1.f-expr_depth/5.f) * RandomFloat();

    ASTExpression::Kind kind = ASTExpression::INVALID;
    int kind_index = 0;
    if(exponential_chance > 0) {
        if(!expected_type.valid()) {
            kind_index = RandomInt(0, ARR_LEN(binary_kinds) - 1);
            kind = binary_kinds[kind_index];
        // } else if(expected_type.type == type_char && expected_type.pointer_level > 0) {
        //     kind_index = RandomInt(0, 1);
        //     if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
        //     else kind = ASTExpression::LITERAL_STR;
        // } else if(expected_type.pointer_level > 0) {
        //     kind_index = RandomInt(0, 1);
        //     if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
        //     else kind = ASTExpression::LITERAL_NULL;
        } else if(expected_type.type == type_int) {
            kind_index = RandomInt(0, ARR_LEN(binary_kinds_int) - 1);
            kind = binary_kinds_int[kind_index];
        } else if(expected_type.type == type_float) {
            kind_index = RandomInt(0, ARR_LEN(binary_kinds_int) - 1);
            kind = binary_kinds_int[kind_index];
        } else if(expected_type.type == type_bool) {
            kind_index = RandomInt(0, ARR_LEN(binary_kinds_bool) - 1);
            kind = binary_kinds_bool[kind_index];
        // } else if(expected_type.type == type_char) {
        //     kind_index = RandomInt(0, 1);
        //     if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
        //     else kind = ASTExpression::LITERAL_CHAR;
        // } else if(expected_type.type == type_float) {
        //     kind_index = RandomInt(0, 1);
        //     if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
        //     else kind = ASTExpression::LITERAL_FLOAT;
        } else {
            kind = ASTExpression::IDENTIFIER;
        }
    }
    if(kind == ASTExpression::INVALID) {
        if(!expected_type.valid()) {
            kind_index = RandomInt(0, ARR_LEN(value_kinds) - 1);
            kind = value_kinds[kind_index];
        } else if(expected_type.type == type_char && expected_type.pointer_level > 0) {
            kind_index = RandomInt(0, 1);
            if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
            else kind = ASTExpression::LITERAL_STR;
        } else if(expected_type.pointer_level > 0) {
            kind_index = RandomInt(0, 1);
            if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
            else kind = ASTExpression::LITERAL_NULL;
        } else if(expected_type.type == type_int) {
            kind_index = RandomInt(0, ARR_LEN(value_kinds_int) - 1);
            kind = value_kinds_int[kind_index];
        } else if(expected_type.type == type_bool) {
            kind_index = RandomInt(0, 2);
            if(kind_index == 0) ASTExpression::IDENTIFIER;
            else if(kind_index == 1) ASTExpression::LITERAL_TRUE;
            else if(kind_index == 2) ASTExpression::LITERAL_FALSE;
        } else if(expected_type.type == type_char) {
            kind_index = RandomInt(0, 1);
            if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
            else kind = ASTExpression::LITERAL_CHAR;
        } else if(expected_type.type == type_float) {
            kind_index = RandomInt(0, 1);
            if(kind_index == 0) kind = ASTExpression::IDENTIFIER;
            else kind = ASTExpression::LITERAL_FLOAT;
        } else {
            kind = ASTExpression::IDENTIFIER;
        }
    }
    
    switch(kind) {
        case ASTExpression::IDENTIFIER: {
            auto id = requestVariable(expected_type);
            if(id) {
                output += id->name;
            } else {
                if(expected_type.pointer_level) {
                    output += "null";
                } else if(expected_type.type == type_bool) {
                    output += "true";
                } else if(expected_type.type == type_int) {
                    output += "5";
                } else if(expected_type.type == type_char) {
                    output += "'j'";
                } else if(expected_type.type == type_float) {
                    output += "0.2";
                } else {
                    output += "null";
                }
            }
        } break;
        case ASTExpression::FUNCTION_CALL: {
            auto id = requestFunction(expected_type);
            if(id) {
                output += id->name;
                output += "(";
                for(int i=0;i<id->args.size();i++){
                    if(i!=0) output+=", ";
                    genExpression(id->args[i].type, expr_depth);
                }
                output += ")";
            } else output += "null";
        } break;
        case ASTExpression::ADD:
        case ASTExpression::SUB:
        case ASTExpression::DIV:
        case ASTExpression::MUL:
        case ASTExpression::AND:
        case ASTExpression::OR:
        case ASTExpression::EQUAL:
        case ASTExpression::NOT_EQUAL:
        case ASTExpression::LESS:
        case ASTExpression::GREATER:
        case ASTExpression::LESS_EQUAL:
        case ASTExpression::GREATER_EQUAL: {
            // output += "(";
            ComplexType t = {type_int};
            if(RandomInt(0,1) == 0) t = {type_float};
            genExpression(t, expr_depth);
            switch(kind) {
                case ASTExpression::ADD:            output += " + "; break;
                case ASTExpression::SUB:            output += " - "; break;
                case ASTExpression::DIV:            output += " / "; break;
                case ASTExpression::MUL:            output += " * "; break;
                case ASTExpression::AND:            output += " && "; break;
                case ASTExpression::OR:             output += " || "; break;
                case ASTExpression::EQUAL:          output += " == "; break;
                case ASTExpression::NOT_EQUAL:      output += " != "; break;
                case ASTExpression::LESS:           output += " < "; break;
                case ASTExpression::GREATER:        output += " > "; break;
                case ASTExpression::LESS_EQUAL:     output += " <= "; break;
                case ASTExpression::GREATER_EQUAL:  output += " >= "; break;
                default: Assert(false);
            }
            genExpression(t, expr_depth);
            // output += ")";
        } break;
        case ASTExpression::INDEX: {
            auto id = requestVariable(expected_type);
            if(id) {
                output += id->name;
                output += "[";
                output += std::to_string(RandomInt(0,50));
                output += "]";
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::NOT: {
            auto id = requestVariable(expected_type);
            if(id) {
                output += "!";
                genExpression({type_bool}, expr_depth);
            } else {
                output += "false";
            }
        } break;
        case ASTExpression::REFER: {
            auto id = requestVariable(expected_type);
            if(id) {
                output += "&"+id->name;
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::DEREF: {
            output += "*";
            genExpression({expected_type.type,expected_type.pointer_level + 1}, expr_depth);
        } break;
        case ASTExpression::ASSIGN: {
            auto id = requestVariable(expected_type);
            if(id) {
                output += id->name + " = ";
                genExpression(id->type, expr_depth);
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::MEMBER: {
            auto id = requestVariable(expected_type);
            if(id) {
                output += id->name;
                output += ".member0";
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::CAST: {
            output += "cast int ";
            genExpression({type_float}, expr_depth);
        } break;
        case ASTExpression::PRE_INCREMENT: {
            auto id = requestVariable({type_int});
            if(id) {
                output += "++";
                output += id->name;
            } else {
                output += "0";
            }
        } break;
        case ASTExpression::POST_INCREMENT: {
            auto id = requestVariable({type_int});
            if(id) {
                output += id->name;
                output += "++";
            } else {
                output += "0";
            }
        } break;
        case ASTExpression::PRE_DECREMENT: {
            auto id = requestVariable({type_int});
            if(id) {
                output += "--";
                output += id->name;
            } else {
                output += "0";
            }
        } break;
        case ASTExpression::POST_DECREMENT: {
            auto id = requestVariable({type_int});
            if(id) {
                output += id->name;
                output += "--";
            } else {
                output += "0";
            }
        } break;
        case ASTExpression::SIZEOF: {
            auto type = requestType();
            if(type.valid()) {
                output += "sizeof ";
                output += type.to_string();
            } else {
                output += "1";   
            }
        } break;
        case ASTExpression::LITERAL_INT: {
            output += std::to_string(RandomInt(-1000,1000));
        } break;
        case ASTExpression::LITERAL_FLOAT: {
            output += std::to_string(RandomFloat() * 200 - 100);
        } break;
        case ASTExpression::LITERAL_STR: {
            output += "\"My string\"";
        } break;
        case ASTExpression::LITERAL_CHAR: {
            output += "'c'";
        } break;
        case ASTExpression::LITERAL_NULL: {
            output += "null";
        } break;
        case ASTExpression::LITERAL_TRUE: {
            output += "true";
        } break;
        case ASTExpression::LITERAL_FALSE: {
            output += "false";
        } break;
        default: Assert(false);
    }
}