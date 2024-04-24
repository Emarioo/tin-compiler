#pragma once

#include "AST.h"
#include "Bytecode.h"
#include "Reporter.h"

struct GeneratorContext {
    Reporter* reporter;
    AST* ast;
    ASTFunction* function;
    Bytecode* bytecode;
    BytecodePiece* piece;
    bool ignore_errors = false;

    TokenStream* current_stream=nullptr;
    
    int current_frameOffset = 0; // used for allocating local variables
    ScopeId current_scopeId;

    // We need to store continue and break offsets to the bytecode.
    // That is what loop scopes are. Each while statement creates a loop
    // scope.
    struct LoopScope {
        int continue_pc = 0;
        int frame_offset = 0;
        std::vector<int> breaks_to_resolve;
    };
    std::vector<LoopScope*> loopScopes;
    LoopScope* pushLoop(int continue_pc, int cur_frame_offset);
    void popLoop();

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
    
    // only casts if necessary
    bool performCast(TypeId ltype, TypeId rtype);

    void generatePop(Register reg, int offset, TypeId type);
    void generatePush(Register reg, int offset, TypeId type);
    
    // Variable* addVariable(const std::string& name, TypeId type, int frame_offset = 0);
    // Variable* findVariable(const std::string& name);
};

bool CheckStructs(AST* ast, AST::Import* imp, Reporter* reporter, bool* changed, bool ignore_errors);
bool CheckFunction(AST* ast, AST::Import* imp, ASTFunction* function, Reporter* reporter);
bool CheckGlobals(AST* ast, AST::Import* imp, Bytecode* bytecode, Reporter* reporter);
void GenerateFunction(AST* ast, ASTFunction* function, Bytecode* bytecode, Reporter* reporter);