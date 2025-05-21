#ifndef TOKEN_DEFS_H
#define TOKEN_DEFS_H

// Token definitions
// #define GREATER_EQUAL 256
// #define LESS_EQUAL    257
// #define EQUAL         258
// #define NOT_EQUAL     259
// #define AND           260
// #define OR            261

// Type definitions
#define TYPE_INT     1
#define TYPE_FLOAT   2
#define TYPE_DOUBLE  3
#define TYPE_CHAR    4
#define TYPE_BOOLEAN 5
#define TYPE_STRING  6
#define TYPE_OBJECT  7
#define TYPE_VOID    8
#define TYPE_ERROR   9
#define TYPE_NULL    10

// Three-address code operation codes
enum {
    OP_ADD = 1,     // +
    OP_SUB,         // -
    OP_MUL,         // *
    OP_DIV,         // /
    OP_MOD,         // %
    OP_NEG,         // unary -
    OP_NOT,         // logical not
    OP_AND,         // logical and
    OP_OR,          // logical or
    OP_LT,          // 
    OP_LE,          // <=
    OP_GT,          // >
    OP_GE,          // >=
    OP_EQ,          // ==
    OP_NE,          // !=
    OP_ASSIGN,      // =
    OP_GOTO,        // goto
    OP_IF,          // if
    OP_IFNOT,       // ifnot
    OP_PARAM,       // parameter
    OP_CALL,        // function call
    OP_RETURN,      // return
    OP_LABEL,       // label
    OP_ARRAY_STORE, // array store
    OP_ARRAY_LOAD,  // array load
    OP_FIELD_STORE, // field store
    OP_FIELD_LOAD,  // field load
    OP_NEW,         // new object
    OP_NEW_ARRAY    // new array
};

#endif /* TOKEN_DEFS_H */