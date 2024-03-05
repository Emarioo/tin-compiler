#include "TinGenerator.h"
#include "AST.h"
#include "Util.h"

void GenerateTin(Config* config) {
    TinContext context{};
    context.config = config;
    context.output = "";

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
    int struct_count = RandomInt(config->struct_frequency.min, config->struct_frequency.max);
    for(int i=0;i<struct_count;i++) {
        std::string name = "Type"+std::to_string(i);
        output += "struct ";
        output += name;
        output += " {\n";

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
        std::string name = "proc"+std::to_string(i);
        output += "fun ";
        output += name;
        output += "(";

        int arg_count = RandomInt(config->argument_frequency.min, config->argument_frequency.max);
        for(int j=0;j<arg_count;j++) {
            if(j != 0)
                output += ", ";

            std::string name = "arg"+std::to_string(j);
            std::string type = "int";
            output += name;
            output += ": ";
            output += type;
        }

        output += ")";

        output += ": ";
        std::string rettype = "int";
        output += rettype;

        output += " {\n";
        indent_level++;

        genStatements();

        indent();
        output += "return 0;\n";

        indent_level--;
        output += "}\n";
    }
}
void TinContext::genStatements() {
    // printf("gen stmt %d\n",indent_level);
    int stmt_count = RandomInt(config->statement_frequency.min, config->statement_frequency.max);
    for(int i=0;i<stmt_count;i++) {
        ASTStatement::Kind kind = (ASTStatement::Kind)RandomInt(ASTStatement::KIND_BEGIN, ASTStatement::KIND_END);

        if(indent_level > 5) {
            if(kind == ASTStatement::IF || kind == ASTStatement::WHILE)
                continue;
        }

        switch(kind) {
            case ASTStatement::VAR_DECLARATION: {
                indent();
                output += "var: int;\n";
            } break;
            case ASTStatement::GLOBAL_DECLARATION: {
                indent();
                output += "global var: int;\n";
            } break;
            case ASTStatement::CONST_DECLARATION: {
                indent();
                output += "const var: int;\n";
            } break;
            case ASTStatement::EXPRESSION: {
                
            } break;
            case ASTStatement::IF: {
                indent();
                output += "if ";
                genExpression();

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
                genExpression();

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
                output += "return 2;\n";
            } break;
            default: break;
        }
    }
}
void TinContext::genExpression() {
    // TODO: Generate random expressions, numbers, strings, function calls, math whatever you want. You can use these random functions:
    //  RandomInt(0, 10);
    //  RandomFloat(); // returns float between 0.0 - 1.0
    output += "false";

    int rand = RandomInt(0,2);
    if( rand == 0)
        output += "1";
    else
        output += "func(23,5)"; // for function call or variable expressions we need to make sure they exist first, perhaps an unordered_map of identifiers?
}