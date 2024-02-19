#include "Compiler.h"

void CompileFile(const std::string& path) {
    AST* ast = new AST();
    Reporter* reporter = new Reporter();
    Code* code = new Code();

    // enum TaskType {
    //     lex,
    //     parse,
    //     gen,

    // };
    // struct Task {
    //     TaskType ;
    //     TokenStream* ;
    // };

    // // thread pool

    // std::vector<Task> tasks;


    TokenStream* stream = lex_file(path);
    // stream->print();

    ParseTokenStream(stream, ast, reporter);

    ast->print();
    
    
    if(reporter->errors == 0) {
        GenerateStructs(ast, reporter);
        
        for(auto func : ast->functions)
            GenerateFunction(ast, func, code, reporter);
        
        code->print();
    }
    
    if(reporter->errors == 0) {
        Interpreter* interpreter = new Interpreter();
        interpreter->code = code;
        interpreter->init();
        interpreter->execute();
        
        delete interpreter;
    }
    
    delete code;
    delete ast;
    TokenStream::Destroy(stream);
}