#include "AST.h"

ASTExpression* AST::createExpression(ASTExpression::Type type) {
    return new ASTExpression();
}
ASTStatement* AST::createStatement(ASTStatement::Type type) {
    return new ASTStatement();
}
ASTBody* AST::createBody() {
    return new ASTBody();
}
ASTFunction* AST::createFunction() {
    return new ASTFunction();
}
ASTStructure* AST::createStructure() {
    return new ASTStructure();
}

void AST::print() {
    printf("PRINTING AST:\n");
    for(const auto& st : structures) {
        Assert(st);
        printf("%s {\n", st->name.c_str());
        for(const auto& member : st->members) {
            printf(" %s: %s,\n", member.name.c_str(), member.type.c_str());
        }
        printf("}\n");
    }
    for(const auto& fn : functions) {
        Assert(fn);
        printf("%s(", fn->name.c_str());
        for(const auto& param : fn->parameters) {
            printf("%s: %s, ", param.name.c_str(), param.type.c_str());
        }
        printf(")\n");
    }
}