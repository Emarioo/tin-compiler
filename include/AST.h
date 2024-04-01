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
    TypeId(PrimitiveType type) : TypeId(Make(type)) { }
    static TypeId Make(u32 index) { TypeId t{}; t._index = index; return t; }
    static TypeId Make(u32 index, u32 level) { TypeId t{}; t._index = index; t._pointer_level = level; return t; }

    u32 index() const { return _index; }
    u32 pointer_level() const { return _pointer_level; }
    bool valid() const { return _index != 0 || _pointer_level != 0; }

    void set_pointer_level(u32 level) { _pointer_level = level; }

    TypeId base() const { return Make(_index); }

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
struct Identifier {
    enum Kind {
        LOCAL_ID,
        GLOBAL_ID,
        CONST_ID,
    };
    Identifier(Kind kind) : kind(kind) { }
    Kind kind;
    TypeId type;
    
    // union {
        int offset; // frame offset or global offset
        ASTStatement* statement = nullptr;
    // };
};
struct ScopeInfo {
    ScopeId scopeId;
    ScopeId parent;
    
    ASTBody* body = nullptr;
    
    std::unordered_map<std::string, Identifier*> identifiers;

    std::unordered_map<std::string, TypeInfo*> type_map;
    std::vector<ScopeInfo*> shared_scopes;
};

struct ASTNode {
    int nodeid;
};

struct ASTExpression : public ASTNode {
    enum Kind {
        INVALID = 0,
        IDENTIFIER,
        FUNCTION_CALL,
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
        ASSIGN,
        PRE_INCREMENT,
        POST_INCREMENT,
        PRE_DECREMENT,
        POST_DECREMENT,
        
        CONST_EXPR,
        SIZEOF = CONST_EXPR,
        LITERAL_INT,
        LITERAL_FLOAT,
        LITERAL_STR,
        LITERAL_NULL,
        LITERAL_TRUE,
        LITERAL_FALSE,
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
    bool isConst() const {
        return _kind >= ASTExpression::CONST_EXPR;
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

struct ASTStatement : public ASTNode {
    enum Kind {
        INVALID = 0,
        KIND_BEGIN,
        VAR_DECLARATION = KIND_BEGIN, // variable declaration
        GLOBAL_DECLARATION,
        CONST_DECLARATION,
        EXPRESSION,
        IF,
        WHILE,
        BREAK,
        CONTINUE,
        RETURN,
        KIND_END,
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

struct ASTBody : public ASTNode {
    ScopeId scopeId;
    std::vector<ASTStatement*> statements;
    std::vector<ASTFunction*> functions;
    std::vector<ASTStructure*> structures;
    
    void add(ASTFunction* f) { functions.push_back(f); }
    void add(ASTStructure* s) { structures.push_back(s); }
};

struct ASTFunction : public ASTNode {
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
    int return_offset = 0; // offset from base pointer inside the function

    int piece_code_index = -1;

    ASTBody* body = nullptr;
    
    bool is_native = false;
};
struct ASTStructure : public ASTNode {
    std::string name;
    
    SourceLocation location;
    TokenStream* origin_stream;
    
    struct Member {
        std::string name;
        std::string typeString;
        TypeId typeId;
        int offset;
        SourceLocation location;
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

    // ScopeInfo* global_scope = nullptr;
    ASTBody* global_body = nullptr;

    struct Import {
        std::string name;
        TokenStream* stream=nullptr;
        ASTBody* body;
        std::vector<std::string> dependencies; // other imports
        
        std::vector<Import*> fixups;
        int deps_count = 0; // locked behind tasks_lock
        int deps_now = 0; // locked behind tasks_lock
    };
    std::vector<Import*> imports;
    std::unordered_map<std::string, Import*> import_map;
    Import* createImport(const std::string& name) {
        auto i = new Import();
        i->name = name;
        MUTEX_LOCK(scopes_lock);
        imports.push_back(i);
        import_map[name] = i;
        MUTEX_UNLOCK(scopes_lock);
        return i;
    }

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
    MUTEX_DECL(types_lock);
    std::vector<TypeInfo*> typeInfos;
    MUTEX_DECL(scopes_lock);
    std::vector<ScopeInfo*> scopeInfos;

    ScopeInfo* getScope(ScopeId scopeId) {
        MUTEX_LOCK(scopes_lock);
        auto ptr = scopeInfos[scopeId];
        MUTEX_UNLOCK(scopes_lock);
        return ptr;
    }
    ScopeInfo* createScope(ScopeId parent) {
        auto ptr = new ScopeInfo();
        ptr->parent = parent;
        
        MUTEX_LOCK(scopes_lock);
        ptr->scopeId = scopeInfos.size();
        scopeInfos.push_back(ptr);
        MUTEX_UNLOCK(scopes_lock);
        return ptr;
    }

    TypeInfo* getType(TypeId typeId) {
        Assert(typeId.pointer_level() == 0);
        Assert(typeId.index() < typeInfos.size());
        return typeInfos[typeId.index()];
    }
    TypeInfo* findType(const std::string& str, ScopeId scopeId);
    TypeId convertFullType(const std::string& str, ScopeId scopeId);
    TypeInfo* createType(const std::string& str, ScopeId scopeId);
    int sizeOfType(TypeId typeId);
    std::string nameOfType(TypeId typeId);
    
    Identifier* addVariable(Identifier::Kind var_type, const std::string& name, ScopeId scopeId, TypeId type, int frame_offset);
    Identifier* findVariable(const std::string& name, ScopeId scopeId);

    struct ScopeIterator {
        std::vector<ScopeInfo*> searchScopes;
        int search_index = 0;
    };
    ScopeIterator createScopeIterator(ScopeId scopeId);
    ScopeInfo* iterate(ScopeIterator& iterator);

private:
    int next_nodeid() { return atomic_add(&_nodeid,1) - 1; }
    volatile int _nodeid = 0;
};