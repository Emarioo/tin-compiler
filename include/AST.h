
struct ASTExpression;
struct ASTStatement;
struct ASTBody;
struct ASTFunction;
struct ASTStructure;

struct ASTExpression {
    enum Type {
        IDENTIFIER,
        FUNCTION_CALL,
        // integer literal
        // add, sub...
    };
    Type type;
    
    std::string name; // used with IDENTIFIER, FUNCTION_CALL
    
    ASTExpression* left;
    ASTExpression* right;
    
    std::vector<ASTExpression*> arguments; // used with FUNCTION_CALL
};

struct ASTStatement {
    enum Type {
        VAR_DECLARATION, // variable declaration
        EXPRESSION,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
        RETURN,
    };
    Type type;
    
    std::string declaration_name; // used by VAR_DECLARATION
    
    ASTExpression* expression; // used by RETURN, EXPRESSION, IF, WHILE, VAR_DECLARATION
    ASTBody* body; // used by IF, WHILE
};

struct ASTBody {
    
    std::vector<ASTStatement*> statements;
};

struct ASTFunction {
    std::string name;
    
    struct Parameter {
        std::string name;
        // type
    };
    std::vector<Parameter> parameters;
    
    ASTBody* body;
};
struct ASTStructure {
    std::string name;
    
    struct Member {
        std::string name;
        // type, byte offset (in case of padding)
    };
    std::vector<Member> members;
    
    // size of struct?
};

struct AST {
    std::vector<ASTFunction*> functions;
    std::vector<ASTStructure*> structures;
    // TODO: Global variables
    
    ASTExpression* createExpression(ASTExpression::Type type);
    ASTStatement* createStatement(ASTStatement::Type type);
    ASTBody* createBody();
    ASTFunction* createFunction();
    ASTStructure* createStructure();
};