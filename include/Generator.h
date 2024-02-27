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
    bool ignore_errors = false;

    TokenStream* current_stream=nullptr;
    
    int current_frameOffset = 0; // used for allocating local variables
    ScopeId current_scopeId;

    // struct Variable {
    //     int frame_offset;
    //     std::string name;
    //     TypeId typeId;
    // };
    // // std::vector<Variable> local_variables;
    // std::unordered_map<std::string, Variable*> local_variables;
    
    bool generateStruct(ASTStructure* aststruct);
    TypeId generateExpression(ASTExpression* expr);
    TypeId generateReference(ASTExpression* expr);
    bool generateBody(ASTBody* body);

    void generatePop(Register reg, int offset, TypeId type);
    void generatePush(Register reg, int offset, TypeId type);
    
    // Variable* addVariable(const std::string& name, TypeId type, int frame_offset = 0);
    // Variable* findVariable(const std::string& name);
};

bool CheckStructs(AST* ast, AST::Import* imp, Reporter* reporter, bool* changed, bool ignore_errors);
bool CheckFunction(AST* ast, AST::Import* imp, ASTFunction* function, Reporter* reporter);
bool CheckGlobals(AST* ast, AST::Import* imp, Code* code, Reporter* reporter);
void GenerateFunction(AST* ast, ASTFunction* function, Code* code, Reporter* reporter);