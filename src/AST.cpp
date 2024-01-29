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