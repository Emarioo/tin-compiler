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

    int struct_count = RandomInt(config->struct_frequency.min, config->struct_frequency.max);
    for(int i=0;i<struct_count;i++) {
        std::string name = "Type" + std::to_string(type_counter++);
        output += "struct ";
        output += name;
        output += " {\n";
        
        auto id = addType(name);

        indent_level++;
        int member_count = RandomInt(config->member_frequency.min, config->member_frequency.max);
        for(int j=0;j<member_count;j++) {
            std::string name = "mem"+std::to_string(j);
            std::string type = "int";
            indent();
            output += name;
            output += ": ";
            output += type;
            output += ",\n";
        }
        indent_level--;

        output += "}\n";
    }
    int fun_count = RandomInt(config->function_frequency.min, config->function_frequency.max);
    for(int i=0;i<fun_count;i++) {
        std::string name = "Func" + std::to_string(fun_counter++);
        output += "fun ";
        output += name;
        output += "(";

        auto id = addFunc(name);

        pushScope();

        int arg_count = RandomInt(config->argument_frequency.min, config->argument_frequency.max);
        for(int j=0;j<arg_count;j++) {
            if(j != 0)
                output += ", ";

            std::string name = "arg"+std::to_string(j);
            std::string type = "int";
            output += name;
            output += ": ";
            output += type;

            auto id = addVar(name, Identifier::LOCAL);
            id->typeString = type;
        }

        output += ")";

        output += ": ";
        std::string rettype = "int";
        output += rettype;

        output += " {\n";
        indent_level++;

        genStatements(false);

        popScope();

        indent();
        output += "return 0;\n";

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
                std::string type = "int";
                std::string name = "var"+std::to_string(var_counter++);
                output += name +": "+type+";\n";
                
                auto id = addVar(name, Identifier::LOCAL);
                id->typeString = type;
            } break;
            case ASTStatement::GLOBAL_DECLARATION: {
                indent();
                std::string type = "int";
                std::string name = "g_var"+std::to_string(var_counter++);
                output += "global "+ name + ": " + type + ";\n";
                
                auto id = addVar(name, Identifier::GLOBAL);
                id->typeString = type;
            } break;
            case ASTStatement::CONST_DECLARATION: {
                indent();
                
                std::string type = "int";
                std::string name = "CONST"+std::to_string(var_counter++);
                output += "const "+ name + ": "+type+";\n";
                
                auto id = addVar(name, Identifier::CONSTANT);
                id->typeString = type;
            } break;
            case ASTStatement::EXPRESSION: {
                indent();
                genExpression(0);
                output += ";\n";
            } break;
            case ASTStatement::IF: {
                indent();
                output += "if ";
                genExpression(0);

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
                genExpression(0);

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
                genExpression(0);
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
    ASTExpression::FUNCTION_CALL,
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
    ASTExpression::INDEX,
};
static const ASTExpression::Kind unary_kinds[]{
    ASTExpression::NOT,
    ASTExpression::REFER, // take reference
    ASTExpression::DEREF, // dereference
    ASTExpression::MEMBER,
    ASTExpression::CAST,
};
static const ASTExpression::Kind value_kinds[]{
    ASTExpression::PRE_INCREMENT,  // we generate thesee such that they refer to one identifier, in real code is just has to be a referable expression but that's annoying to generate
    ASTExpression::POST_INCREMENT,
    ASTExpression::PRE_DECREMENT,
    ASTExpression::POST_DECREMENT,

    ASTExpression::SIZEOF,
    ASTExpression::LITERAL_INT,
    ASTExpression::LITERAL_FLOAT,
    ASTExpression::LITERAL_STR,
    ASTExpression::LITERAL_NULL,
    ASTExpression::LITERAL_TRUE,
    ASTExpression::LITERAL_FALSE,
};
void TinContext::genExpression(int expr_depth) {
    expr_depth++;
    if(expr_depth == 0)
        expr_count++;
    
    float exponential_chance = (1.f-expr_count/10.f) * (1.f-expr_depth/5.f) * RandomFloat();

    ASTExpression::Kind kind = ASTExpression::INVALID;
    int kind_index = 0;
    if(exponential_chance > 0) {
        // expression with multiple values
        // we must be carefule to not cause infinite recursion
        kind_index = RandomInt(0, ARR_LEN(binary_kinds) - 1 + ARR_LEN(unary_kinds) - 1);
        if(kind_index<ARR_LEN(binary_kinds)) {
            kind = binary_kinds[kind_index];
        } else {
            kind = unary_kinds[kind_index - ARR_LEN(binary_kinds)];
        }
    } else {
        kind_index = RandomInt(0, ARR_LEN(value_kinds) - 1);
        kind = value_kinds[kind_index];
    }
    
    switch(kind) {
        case ASTExpression::FUNCTION_CALL: {
            auto id = requestFunction();
            if(id) {
                output += id->name;
                output += "(";
                int args = RandomInt(0,5);
                for(int i=0;i<args;i++){
                    if(i!=0) output+=", ";
                    genExpression(expr_depth);
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
            output += "(";
            genExpression(expr_depth);
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
            genExpression(expr_depth);
            output += ")";
        } break;
        case ASTExpression::INDEX: {
            auto id = requestVariable();
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
            auto id = requestVariable();
            if(id) {
                output += "!";
                genExpression(expr_depth);
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::REFER: {
            auto id = requestVariable();
            if(id) {
                output += "&"+id->name;
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::DEREF: {
            output += "*";
            genExpression(expr_depth);
        } break;
        case ASTExpression::MEMBER: {
            auto id = requestVariable();
            if(id) {
                output += id->name;
                output += ".member0";
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::CAST: {
            output += "cast int ";
            genExpression(expr_depth);
        } break;
        case ASTExpression::PRE_INCREMENT: {
            auto id = requestVariable();
            if(id) {
                output += "++";
                output += id->name;
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::POST_INCREMENT: {
            auto id = requestVariable();
            if(id) {
                output += id->name;
                output += "++";
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::PRE_DECREMENT: {
            auto id = requestVariable();
            if(id) {
                output += "--";
                output += id->name;
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::POST_DECREMENT: {
            auto id = requestVariable();
            if(id) {
                output += id->name;
                output += "--";
            } else {
                output += "null";
            }
        } break;
        case ASTExpression::SIZEOF: {
            output += "sizeof T";
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