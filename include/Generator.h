#pragma once

#include "AST.h"
#include "Code.h"
#include "Reporter.h"

struct GeneratorContext {
    Reporter* reporter;
    AST* ast;
    ASTFunction* function;
    Code* code;
    CodePiece* piece;
    
    int current_frame_offset = 0; // used for allocating local variables
    
    struct Variable {
        int frame_offset;
        std::string name;
        // TODO: Type
    };
    // std::vector<Variable> local_variables;
    std::unordered_map<std::string, Variable*> local_variables;
    
    bool generateExpression(ASTExpression* expr);
    bool generateReference(ASTExpression* expr);
    bool generateBody(ASTBody* body);
    
    Variable* addVariable(const std::string& name, int frame_offset = 0);
    Variable* findVariable(const std::string& name);
};

void GenerateFunction(AST* ast, ASTFunction* function, Code* code, Reporter* reporter);