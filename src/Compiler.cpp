#include "Compiler.h"


void CompileFile(const std::string& path) {

    AST* ast = new AST();

    TokenStream* stream = lex_file(path);
    // stream->print();

    ParseTokenStream(stream, ast);

    ast->print();
    
    delete ast;
    TokenStream::Destroy(stream);
}