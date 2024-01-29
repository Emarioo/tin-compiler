#include "Compiler.h"


void CompileFile(const std::string& path) {
    TokenStream* stream = lex_file("test.txt");
    stream->print();

    ParseTokenStream(stream);

    
    
    
    TokenStream::Destroy(stream);
}