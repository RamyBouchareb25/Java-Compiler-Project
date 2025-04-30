#ifndef TOKEN_DEFS_H
#define TOKEN_DEFS_H

// Opérateurs de base pour les opérations TAC
#define OP_ADD         1
#define OP_SUB         2
#define OP_MUL         3
#define OP_DIV         4
#define OP_MOD         5
#define OP_AND         6
#define OP_OR          7
#define OP_EQ          8
#define OP_NE          9
#define OP_LT          10
#define OP_LE          11
#define OP_GT          12
#define OP_GE          13
#define OP_NOT         14
#define OP_NEG         15
#define OP_INC         16
#define OP_DEC         17
#define OP_POST_INC    18
#define OP_POST_DEC    19
#define OP_ASSIGN      20
#define OP_GOTO        21
#define OP_IF          22
#define OP_LABEL       23
#define OP_CALL        24
#define OP_RETURN      25
#define OP_NEW         26

// Ajout des opérateurs manquants
#define OP_IFNOT       27
#define OP_PARAM       28
#define OP_ARRAY_LOAD  29
#define OP_ARRAY_STORE 30
#define OP_FIELD_LOAD  31
#define OP_FIELD_STORE 32
#define OP_NEW_ARRAY   33

// Token constants pour le switch - attention aux conflits avec Bison
// Ne les utilisez pas dans la phase sémantique, mais seulement dans
// les fonctions de traduction comme token_to_tac_op
#define TOKEN_AND          1001
#define TOKEN_OR           1002
#define TOKEN_LESS_EQUAL   1003
#define TOKEN_GREATER_EQUAL 1004
#define TOKEN_EQUAL        1005
#define TOKEN_NOT_EQUAL    1006

#endif /* TOKEN_DEFS_H */