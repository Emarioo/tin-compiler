#pragma once

#include "Util.h"
#include "Lexer.h"

struct ASTExpression;
struct ASTStatement;
struct ASTBody;
struct ASTFunction;
struct ASTStructure;

enum PrimitiveType {
    TYPE_VOID = 0,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_FLOAT,
};
extern const char* primitive_names[];
struct TypeId {
    TypeId() : _index(0), _pointer_level(0) { }
    TypeId(PrimitiveType type) : TypeId((u32)type) { }
    TypeId(u32 index) : _index(index), _pointer_level(0) { }

    u32 index() const { return _index; }
    u32 pointer_level() const { return _pointer_level; }
    bool valid() const { return _index != 0 || _pointer_level != 0; }

    void set_pointer_level(u32 level) { _pointer_level = level; }

    bool operator ==(TypeId type) const {
        return _index == type._index && _pointer_level == type._pointer_level;
    }
    bool operator !=(TypeId type) const {
        return !(*this == type);
    }

private:
    u32 _index : 24;
    u8 _pointer_level : 8;
};

struct TypeInfo {
    TypeId typeId;
    std::string name="";
    int size=0;
    ASTStructure* ast_struct = nullptr;
};
typedef u32 ScopeId;
struct ScopeInfo {
    ScopeId scopeId;
    ScopeId parent;

    std::unordered_map<std::string, TypeInfo*> type_map;
};

struct ASTExpression {
    enum Kind {
        INVALID,
        IDENTIFIER,
        FUNCTION_CALL,
        LITERAL_INT,
        LITERAL_FLOAT,
        LITERAL_STR,
        ADD,
        SUB,
        DIV,
        MUL,
        AND,
        OR,
        NOT,
        EQUAL,
        NOT_EQUAL,
        LESS,
        GREATER,
        LESS_EQUAL,
        GREATER_EQUAL,
        REFER, // take reference
        DEREF, // dereference
        INDEX,
        MEMBER,
        CAST,
        SIZEOF,
        ASSIGN,
        PRE_INCREMENT,
        POST_INCREMENT,
        PRE_DECREMENT,
        POST_DECREMENT,
        TRUE,
        FALSE,
        LITERAL_NULL,
    };
    ASTExpression(Kind kind) : _kind(kind) {
        // switch(_kind){
        //     case FUNCTION_CALL:
        //         new(&arguments)std::vector<ASTExpression*>();
        //     case IDENTIFIER:
        //     case SIZEOF:
        //     case MEMBER:
        //     case CAST:
        //         new(&name)std::string();
        //         break;
        //     case LITERAL_STR:
        //         new(&literal_string)std::string();
        //         break;
        //     default:
        //         left = nullptr;
        //         right = nullptr;
        //         break;
        // }
    }
    // ~ASTExpression() {
    //     switch(_kind){
    //         case FUNCTION_CALL:
    //             arguments.~vector<ASTExpression*>();
    //         case IDENTIFIER:
    //         case SIZEOF:
    //         case MEMBER:
    //         case CAST:
    //             name.~basic_string();
    //             break;
    //         case LITERAL_STR:
    //             literal_string.~basic_string();
    //             break;
    //         default: break;
    //     }
    // }

    // ASTExpression(const ASTExpression&) = delete;
    // ASTExpression(ASTExpression&) = delete;
    // ASTExpression& operator=(const ASTExpression&) = delete;
    // ASTExpression& operator=(ASTExpression&) = delete;

    Kind kind() const {
        return _kind;
    }

    // union {
    //     // struct {
    //     // };
    //     struct {
    //     };
    // };
    std::string literal_string;
    int literal_integer;
    float literal_float;
    std::string name; // used with IDENTIFIER, FUNCTION_CALL, SIZEOF, MEMBER, CAST
    std::vector<ASTExpression*> arguments; // used with FUNCTION_CALL
    ASTExpression* left = nullptr;
    ASTExpression* right = nullptr;
    
    SourceLocation location;

private:
    Kind _kind;
};

struct ASTStatement {
    enum Kind {
        INVALID,
        VAR_DECLARATION, // variable declaration
        EXPRESSION,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
        RETURN,
    };
    ASTStatement(Kind kind) : _kind(kind) {}
    Kind kind() const { return _kind; }

    std::string declaration_type; // used by VAR_DECLARATION
    std::string declaration_name; // used by VAR_DECLARATION
    
    ASTExpression* expression = nullptr; // used by RETURN, EXPRESSION, IF, WHILE, VAR_DECLARATION
    ASTBody* body = nullptr; // used by IF, WHILE
    ASTBody* elseBody = nullptr; // used by IF sometimes
    
    SourceLocation location;

private:
    Kind _kind;
};

struct ASTBody {
    ScopeId scopeId;
    std::vector<ASTStatement*> statements;
};

struct ASTFunction {
    std::string name;
    
    SourceLocation location;
    TokenStream* origin_stream;
    
    struct Parameter {
        std::string name;
        std::string typeString;
        TypeId typeId;
        int offset;
    };
    std::vector<Parameter> parameters;
    int parameters_size=0;
    
    std::string return_typeString;
    TypeId return_type;

    ASTBody* body;
};
struct ASTStructure {
    std::string name;
    
    SourceLocation location;
    TokenStream* origin_stream;
    
    struct Member {
        std::string name;
        std::string typeString;
        TypeId typeId;
        int offset;
    };
    std::vector<Member> members;

    Member* findMember(const std::string& name) {
        for(int i=0;i<members.size();i++) {
            auto& mem = members[i];
            if(mem.name == name) {
                return &mem;
            }
        }
        return nullptr;
    }
    
    // size of struct?
    bool complete = false;
    bool created_type = false;
    TypeInfo* typeInfo = nullptr;
};

struct AST {
    AST();
    static const ScopeId GLOBAL_SCOPE = 0;

    ScopeInfo* global_scope = nullptr;
    std::vector<ASTFunction*> functions;
    std::vector<ASTStructure*> structures;
    // TODO: Global variables

    ASTExpression* createExpression(ASTExpression::Kind kind);
    ASTStatement* createStatement(ASTStatement::Kind kind);
    ASTBody* createBody(ScopeId parent);
    ASTBody* createBodyWithSharedScope(ScopeId scopeToShare);
    ASTFunction* createFunction();
    ASTStructure* createStructure();

    ASTFunction* findFunction(const std::string& name, ScopeId scopeId);

    void print(ASTExpression* expr, int depth = 0);
    void print(ASTBody* body, int depth = 0);
    void print();

    // Type functionality
    std::vector<TypeInfo*> typeInfos;
    std::vector<ScopeInfo*> scopeInfos;

    ScopeInfo* getScope(ScopeId scopeId) {
        return scopeInfos[scopeId];
    }
    ScopeInfo* createScope(ScopeId parent) {
        auto ptr = new ScopeInfo();
        ptr->scopeId = scopeInfos.size();
        ptr->parent = parent;
        scopeInfos.push_back(ptr);
        return ptr;
    }

    TypeInfo* getType(TypeId typeId) {
        Assert(typeId.pointer_level() == 0);
        return typeInfos[typeId.index()];
    }
    TypeInfo* findType(const std::string& str, ScopeId scopeId);
    TypeId convertFullType(const std::string& str, ScopeId scopeId);
    TypeInfo* createType(const std::string& str, ScopeId scopeId);
    int sizeOfType(TypeId typeId);
    std::string nameOfType(TypeId typeId);
};