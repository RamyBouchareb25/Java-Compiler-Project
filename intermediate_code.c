#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token_defs.h"
#include "parser.tab.h"
// TAC Operators - Now using the enum from token_defs.h
typedef int tac_op;

// TAC Operand Types
typedef enum {
    OPERAND_NONE,
    OPERAND_LITERAL,
    OPERAND_VARIABLE,
    OPERAND_TEMPORARY,
    OPERAND_LABEL
} operand_type;



// Three-Address Code Instruction
typedef struct tac_quad {
    tac_op op;             // Operation
    tac_operand result;    // Result
    tac_operand arg1;      // First argument
    tac_operand arg2;      // Second argument
    struct tac_quad* next; // Next instruction
} tac_quad;

// Global variables
tac_quad* tac_code_head = NULL;
tac_quad* tac_code_tail = NULL;
int temp_var_count = 0;     // For generating temporary variables
int label_count = 0;        // For generating unique labels

// Create a literal integer operand
tac_operand create_int_literal(int value) {
    tac_operand op;
    op.type = OPERAND_LITERAL;
    op.value.int_val = value;
    return op;
}

// Create a literal float operand
tac_operand create_float_literal(float value) {
    tac_operand op;
    op.type = OPERAND_LITERAL;
    op.value.float_val = value;
    return op;
}

// Create a variable operand
tac_operand create_variable(char* name) {
    tac_operand op;
    op.type = OPERAND_VARIABLE;
    op.value.str_val = strdup(name);
    return op;
}

// Create a temporary variable operand
tac_operand create_temporary() {
    tac_operand op;
    op.type = OPERAND_TEMPORARY;
    char temp_name[20];
    sprintf(temp_name, "t%d", temp_var_count++);
    op.value.str_val = strdup(temp_name);
    return op;
}

// Create a label operand
tac_operand create_label() {
    tac_operand op;
    op.type = OPERAND_LABEL;
    char label_name[20];
    sprintf(label_name, "L%d", label_count++);
    op.value.str_val = strdup(label_name);
    return op;
}

// Create a label with specific name
tac_operand create_named_label(char* name) {
    tac_operand op;
    op.type = OPERAND_LABEL;
    op.value.str_val = strdup(name);
    return op;
}

// Create an empty operand
tac_operand create_none() {
    tac_operand op;
    op.type = OPERAND_NONE;
    return op;
}

// Generate a new TAC instruction
tac_quad* generate_tac(tac_op op, tac_operand result, tac_operand arg1, tac_operand arg2) {
    tac_quad* quad = (tac_quad*)malloc(sizeof(tac_quad));
    quad->op = op;
    quad->result = result;
    quad->arg1 = arg1;
    quad->arg2 = arg2;
    quad->next = NULL;
    
    // Add to the linked list of TAC instructions
    if (tac_code_head == NULL) {
        tac_code_head = quad;
        tac_code_tail = quad;
    } else {
        tac_code_tail->next = quad;
        tac_code_tail = quad;
    }
    
    return quad;
}

// Convert operator token to TAC operator
tac_op token_to_tac_op(int token) {
    switch (token) {
        case '+': return OP_ADD;
        case '-': return OP_SUB;
        case '*': return OP_MUL;
        case '/': return OP_DIV;
        case '%': return OP_MOD;
        case '!': return OP_NOT;
        case AND: return OP_AND;
        case OR: return OP_OR;
        case '<': return OP_LT;
        case '>': return OP_GT;
        case LESS_EQUAL: return OP_LE;
        case GREATER_EQUAL: return OP_GE;
        case EQUAL: return OP_EQ;
        case NOT_EQUAL: return OP_NE;
        case '=': return OP_ASSIGN;
        default: return OP_ADD; // Default case
    }
}

// Generate TAC for a binary operation
tac_operand generate_binary_op(tac_op op, tac_operand left, tac_operand right) {
    tac_operand result = create_temporary();
    generate_tac(op, result, left, right);
    return result;
}

// Generate TAC for a unary operation
tac_operand generate_unary_op(tac_op op, tac_operand expr) {
    tac_operand result = create_temporary();
    generate_tac(op, result, expr, create_none());
    return result;
}

// Generate TAC for an assignment
void generate_assignment(tac_operand target, tac_operand value) {
    generate_tac(OP_ASSIGN, target, value, create_none());
}

// Generate TAC for a goto statement
void generate_goto(tac_operand label) {
    generate_tac(OP_GOTO, create_none(), label, create_none());
}

// Generate TAC for an if statement
void generate_if(tac_operand condition, tac_operand true_label) {
    generate_tac(OP_IF, create_none(), condition, true_label);
}

// Generate TAC for an ifnot statement
void generate_ifnot(tac_operand condition, tac_operand false_label) {
    generate_tac(OP_IFNOT, create_none(), condition, false_label);
}

// Generate TAC for a label
void generate_label_tac(tac_operand label) {
    generate_tac(OP_LABEL, label, create_none(), create_none());
}

// Generate TAC for a function call
tac_operand generate_call(char* func_name, int arg_count) {
    tac_operand result = create_temporary();
    tac_operand func = create_variable(func_name);
    tac_operand count = create_int_literal(arg_count);
    generate_tac(OP_CALL, result, func, count);
    return result;
}

// Generate TAC for a function parameter
void generate_param(tac_operand param) {
    generate_tac(OP_PARAM, create_none(), param, create_none());
}

// Generate TAC for a return statement
void generate_return(tac_operand value) {
    generate_tac(OP_RETURN, create_none(), value, create_none());
}

// Generate TAC for array indexing
tac_operand generate_array_load(tac_operand array, tac_operand index) {
    tac_operand result = create_temporary();
    generate_tac(OP_ARRAY_LOAD, result, array, index);
    return result;
}

// Generate TAC for array assignment
void generate_array_store(tac_operand array, tac_operand index, tac_operand value) {
    generate_tac(OP_ARRAY_STORE, array, index, value);
}

// Generate TAC for field access
tac_operand generate_field_load(tac_operand object, char* field_name) {
    tac_operand result = create_temporary();
    tac_operand field = create_variable(field_name);
    generate_tac(OP_FIELD_LOAD, result, object, field);
    return result;
}

// Generate TAC for field assignment
void generate_field_store(tac_operand object, char* field_name, tac_operand value) {
    tac_operand field = create_variable(field_name);
    generate_tac(OP_FIELD_STORE, object, field, value);
}

// Generate TAC for object creation
tac_operand generate_new_object(char* class_name) {
    tac_operand result = create_temporary();
    tac_operand class_op = create_variable(class_name);
    generate_tac(OP_NEW, result, class_op, create_none());
    return result;
}

// Generate TAC for array creation
tac_operand generate_new_array(char* type_name, tac_operand size) {
    tac_operand result = create_temporary();
    tac_operand type_op = create_variable(type_name);
    generate_tac(OP_NEW_ARRAY, result, type_op, size);
    return result;
}

// Print a TAC operand
void print_tac_operand(tac_operand op) {
    switch (op.type) {
        case OPERAND_NONE:
            printf("_");
            break;
        case OPERAND_LITERAL:
            printf("%d", op.value.int_val);  // Simplified - only printing integers
            break;
        case OPERAND_VARIABLE:
        case OPERAND_TEMPORARY:
        case OPERAND_LABEL:
            printf("%s", op.value.str_val);
            break;
    }
}

// Print the TAC operator
void print_tac_op(tac_op op) {
    switch (op) {
        case OP_ADD: printf("ADD"); break;
        case OP_SUB: printf("SUB"); break;
        case OP_MUL: printf("MUL"); break;
        case OP_DIV: printf("DIV"); break;
        case OP_MOD: printf("MOD"); break;
        case OP_NEG: printf("NEG"); break;
        case OP_NOT: printf("NOT"); break;
        case OP_AND: printf("AND"); break;
        case OP_OR: printf("OR"); break;
        case OP_LT: printf("LT"); break;
        case OP_LE: printf("LE"); break;
        case OP_GT: printf("GT"); break;
        case OP_GE: printf("GE"); break;
        case OP_EQ: printf("EQ"); break;
        case OP_NE: printf("NE"); break;
        case OP_ASSIGN: printf("ASSIGN"); break;
        case OP_GOTO: printf("GOTO"); break;
        case OP_IF: printf("IF"); break;
        case OP_IFNOT: printf("IFNOT"); break;
        case OP_PARAM: printf("PARAM"); break;
        case OP_CALL: printf("CALL"); break;
        case OP_RETURN: printf("RETURN"); break;
        case OP_LABEL: printf("LABEL"); break;
        case OP_ARRAY_STORE: printf("ASTORE"); break;
        case OP_ARRAY_LOAD: printf("ALOAD"); break;
        case OP_FIELD_STORE: printf("FSTORE"); break;
        case OP_FIELD_LOAD: printf("FLOAD"); break;
        case OP_NEW: printf("NEW"); break;
        case OP_NEW_ARRAY: printf("NEWARRAY"); break;
    }
}

// Print the entire TAC code
void print_tac_code() {
    tac_quad* current = tac_code_head;
    int line = 1;
    
    printf("\n--- Three-Address Code ---\n");
    while (current != NULL) {
        printf("%3d: ", line++);
        
        // Special case for labels
        if (current->op == OP_LABEL) {
            print_tac_operand(current->result);
            printf(": ");
        } else {
            // Print the operation (result := arg1 op arg2)
            if (current->result.type != OPERAND_NONE) {
                print_tac_operand(current->result);
                printf(" := ");
            }
            
            if (current->op == OP_GOTO) {
                printf("goto ");
                print_tac_operand(current->arg1);
            } else if (current->op == OP_IF) {
                printf("if ");
                print_tac_operand(current->arg1);
                printf(" goto ");
                print_tac_operand(current->arg2);
            } else if (current->op == OP_IFNOT) {
                printf("ifnot ");
                print_tac_operand(current->arg1);
                printf(" goto ");
                print_tac_operand(current->arg2);
            } else if (current->op == OP_CALL) {
                print_tac_operand(current->arg1);
                printf("(");
                if (current->arg2.type == OPERAND_LITERAL) {
                    printf("%d args", current->arg2.value.int_val);
                }
                printf(")");
            } else if (current->op == OP_PARAM) {
                printf("param ");
                print_tac_operand(current->arg1);
            } else if (current->op == OP_RETURN) {
                printf("return ");
                if (current->arg1.type != OPERAND_NONE) {
                    print_tac_operand(current->arg1);
                }
            } else if (current->op == OP_ARRAY_LOAD) {
                print_tac_operand(current->arg1);
                printf("[");
                print_tac_operand(current->arg2);
                printf("]");
            } else if (current->op == OP_ARRAY_STORE) {
                print_tac_operand(current->result);
                printf("[");
                print_tac_operand(current->arg1);
                printf("] := ");
                print_tac_operand(current->arg2);
            } else if (current->op == OP_FIELD_LOAD) {
                print_tac_operand(current->arg1);
                printf(".");
                print_tac_operand(current->arg2);
            } else if (current->op == OP_FIELD_STORE) {
                print_tac_operand(current->result);
                printf(".");
                print_tac_operand(current->arg1);
                printf(" := ");
                print_tac_operand(current->arg2);
            } else if (current->op == OP_NEW) {
                printf("new ");
                print_tac_operand(current->arg1);
                printf("()");
            } else if (current->op == OP_NEW_ARRAY) {
                printf("new ");
                print_tac_operand(current->arg1);
                printf("[");
                print_tac_operand(current->arg2);
                printf("]");
            } else {
                // Standard binary operation
                print_tac_operand(current->arg1);
                printf(" ");
                print_tac_op(current->op);
                printf(" ");
                print_tac_operand(current->arg2);
            }
        }
        
        printf("\n");
        current = current->next;
    }
    printf("------------------------\n");
}

// Free all allocated memory for TAC code
void free_tac_code() {
    tac_quad* current = tac_code_head;
    while (current != NULL) {
        tac_quad* temp = current;
        current = current->next;
        
        // Free string values in operands
        if (temp->result.type >= OPERAND_VARIABLE) {
            free(temp->result.value.str_val);
        }
        if (temp->arg1.type >= OPERAND_VARIABLE) {
            free(temp->arg1.value.str_val);
        }
        if (temp->arg2.type >= OPERAND_VARIABLE) {
            free(temp->arg2.value.str_val);
        }
        
        free(temp);
    }
    tac_code_head = NULL;
    tac_code_tail = NULL;
}