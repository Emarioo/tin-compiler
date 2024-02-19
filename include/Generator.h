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
    
    int current_frameOffset = 0; // used for allocating local variables
    ScopeId current_scopeId;

    struct Variable {
        int frame_offset;
        std::string name;
        TypeId typeId;
    };
    // std::vector<Variable> local_variables;
    std::unordered_map<std::string, Variable*> local_variables;
    
    [[nodiscard]] bool generateStruct(ASTStructure* aststruct);
    [[nodiscard]] TypeId generateExpression(ASTExpression* expr);
    [[nodiscard]] TypeId generateReference(ASTExpression* expr);
    [[nodiscard]] bool generateBody(ASTBody* body);

    void generatePop(Register reg, int offset, TypeId type);
    void generatePush(Register reg, int offset, TypeId type);
    
    Variable* addVariable(const std::string& name, int frame_offset = 0);
    Variable* findVariable(const std::string& name);
};

void GenerateStructs(AST* ast, Reporter* reporter);
void GenerateFunction(AST* ast, ASTFunction* function, Code* code, Reporter* reporter);