// intermediate_code.h
#ifndef INTERMEDIATE_CODE_H
#define INTERMEDIATE_CODE_H

#include "token_defs.h"

// TAC Operators - Using enum from token_defs.h
typedef int tac_op;

// TAC Operand Types
typedef enum {
    OPERAND_NONE,
    OPERAND_LITERAL,
    OPERAND_VARIABLE,
    OPERAND_TEMPORARY,
    OPERAND_LABEL
} operand_type;

// TAC Operand
typedef struct {
    operand_type type;
    union {
        int int_val;       // For integer literals
        float float_val;   // For float literals
        char* string_val;  // For variable names, label names, string literals
    } value;
} tac_operand;

// Three-Address Code Instruction
typedef struct tac_quad {
    tac_op op;             // Operation
    tac_operand result;    // Result
    tac_operand arg1;      // First argument
    tac_operand arg2;      // Second argument
    struct tac_quad* next; // Next instruction
} tac_quad;

// Function declarations
tac_operand create_int_literal(int value);
tac_operand create_float_literal(float value);
tac_operand create_variable(char* name);
tac_operand create_temporary(void);
tac_operand create_label(void);
tac_operand create_named_label(char* name);
tac_operand create_none(void);
tac_quad* generate_tac(tac_op op, tac_operand result, tac_operand arg1, tac_operand arg2);
tac_op token_to_tac_op(int token);
tac_operand generate_binary_op(tac_op op, tac_operand left, tac_operand right);
tac_operand generate_unary_op(tac_op op, tac_operand expr);
void generate_assignment(tac_operand target, tac_operand value);
void generate_goto(tac_operand label);
void generate_if(tac_operand condition, tac_operand true_label);
void generate_ifnot(tac_operand condition, tac_operand false_label);
void generate_label_tac(tac_operand label);
tac_operand generate_call(char* func_name, int arg_count);
void generate_param(tac_operand param);
void generate_return(tac_operand value);
tac_operand generate_array_load(tac_operand array, tac_operand index);
void generate_array_store(tac_operand array, tac_operand index, tac_operand value);
tac_operand generate_field_load(tac_operand object, char* field_name);
void generate_field_store(tac_operand object, char* field_name, tac_operand value);
tac_operand generate_new_object(char* class_name);
tac_operand generate_new_array(char* type_name, tac_operand size);
void print_tac_code(void);
void free_tac_code(void);

// External global variables
extern tac_quad* tac_code_head;
extern tac_quad* tac_code_tail;

#endif /* INTERMEDIATE_CODE_H */