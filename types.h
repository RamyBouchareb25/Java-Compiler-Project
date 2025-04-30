#ifndef TYPES_H
#define TYPES_H

// Type codes constants
#define TYPE_ERROR    0
#define TYPE_INT      1
#define TYPE_FLOAT    2
#define TYPE_DOUBLE   3
#define TYPE_CHAR     4
#define TYPE_BOOLEAN  5
#define TYPE_STRING   6
#define TYPE_VOID     7
#define TYPE_OBJECT   8
#define TYPE_NULL     9

// Pour éviter les conflits avec les macros des tokens
// définis par Bison dans parser.tab.h
#define TYPE_OP_EQUAL         1000
#define TYPE_OP_NOT_EQUAL     1001
#define TYPE_OP_GREATER_EQUAL 1002
#define TYPE_OP_LESS_EQUAL    1003
#define TYPE_OP_AND           1004
#define TYPE_OP_OR            1005

#endif /* TYPES_H */