/* optimize.c - Improved implementation of intermediate code optimization */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "quad.h"
#include "optimise.h"

/* Helper function to check if a string is a constant */
int is_constant(char *str) {
    if (!str || strlen(str) == 0) return 0;
    
    // Check if the string is a number (can be negative)
    int i = 0;
    if (str[0] == '-') i = 1; // Skip leading minus
    
    for (; str[i] != '\0'; i++) {
        if (!isdigit(str[i]) && str[i] != '.') {
            return 0;
        }
    }
    return 1;
}

/* Helper function to check if an operation is arithmetic */
int is_arithmetic_op(char *op) {
    return (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || 
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0);
}

/* Helper function to check if an operation is a control flow operation */
int is_control_flow_op(char *op) {
    return (strcmp(op, "BR") == 0 || strcmp(op, "GOTO") == 0 || 
            strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || 
            strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 || 
            strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
            strcmp(op, "CALL") == 0 || strcmp(op, "PRINT") == 0 ||
            isdigit(op[0])); // Line numbers/labels like "10"
}

/* Helper function to check if a variable is used in control flow */
int is_used_in_control_flow(char *var) {
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue;
        
        if (is_control_flow_op(optimizedQuad[i].oper)) {
            if ((strcmp(optimizedQuad[i].op1, var) == 0) ||
                (strcmp(optimizedQuad[i].op2, var) == 0) ||
                (strcmp(optimizedQuad[i].res, var) == 0)) {
                return 1;
            }
        }
    }
    return 0;
}

/* Improved constant folding - evaluate constant expressions at compile time */
void constant_folding() {
    int changes_made = 1;
    
    // Keep applying constant folding until no more changes are made
    while (changes_made) {
        changes_made = 0;
        
        for (int i = 0; i < optimizedQc; i++) {
            // Skip if the operation is not arithmetic or if the quadruple is marked for removal
            if (!is_arithmetic_op(optimizedQuad[i].oper) || strlen(optimizedQuad[i].oper) == 0) {
                continue;
            }
            
            // Check if both operands are constants
            if (is_constant(optimizedQuad[i].op1) && is_constant(optimizedQuad[i].op2)) {
                double val1 = atof(optimizedQuad[i].op1);
                double val2 = atof(optimizedQuad[i].op2);
                double result = 0;
                int valid_operation = 1;
                
                // Perform the operation
                if (strcmp(optimizedQuad[i].oper, "+") == 0) {
                    result = val1 + val2;
                } else if (strcmp(optimizedQuad[i].oper, "-") == 0) {
                    result = val1 - val2;
                } else if (strcmp(optimizedQuad[i].oper, "*") == 0) {
                    result = val1 * val2;
                } else if (strcmp(optimizedQuad[i].oper, "/") == 0) {
                    if (val2 == 0) {
                        valid_operation = 0; // Skip division by zero
                    } else {
                        result = val1 / val2;
                    }
                }
                
                if (valid_operation) {
                    // Convert result back to string
                    char result_str[MAX_LENGTH];
                    if (result == (int)result) {
                        // Integer result
                        sprintf(result_str, "%d", (int)result);
                    } else {
                        // Float result (remove trailing zeros)
                        sprintf(result_str, "%.6f", result);
                        // Remove trailing zeros
                        int len = strlen(result_str);
                        while (len > 1 && result_str[len-1] == '0' && result_str[len-2] != '.') {
                            result_str[--len] = '\0';
                        }
                    }
                    
                    // Replace the operation with a direct assignment
                    strcpy(optimizedQuad[i].oper, "=");
                    strcpy(optimizedQuad[i].op1, result_str);
                    optimizedQuad[i].op2[0] = '\0';
                    changes_made = 1;
                }
            }
        }
    }
}

/* Improved constant propagation - replace variables with their constant values */
void constant_propagation() {
    // Map of variables to their constant values
    struct {
        char var[MAX_LENGTH];
        char val[MAX_LENGTH];
        int line; // Track where the constant was defined
    } constants[MAX_QUAD];
    int const_count = 0;
    
    // First pass: identify constant assignments
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue; // Skip removed quadruples
        
        if (strcmp(optimizedQuad[i].oper, "=") == 0 && is_constant(optimizedQuad[i].op1) && 
            strlen(optimizedQuad[i].op2) == 0) {
            // This is an assignment of a constant to a variable
            // Check if this variable is already in our constants list
            int found = 0;
            for (int j = 0; j < const_count; j++) {
                if (strcmp(constants[j].var, optimizedQuad[i].res) == 0) {
                    // Update the constant value (later assignment overrides)
                    strcpy(constants[j].val, optimizedQuad[i].op1);
                    constants[j].line = i;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(constants[const_count].var, optimizedQuad[i].res);
                strcpy(constants[const_count].val, optimizedQuad[i].op1);
                constants[const_count].line = i;
                const_count++;
            }
        }
    }
    
    // Second pass: propagate constants (only forward propagation)
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue; // Skip removed quadruples
        
        // Replace op1 if it's a constant variable (and the constant was defined before this line)
        for (int j = 0; j < const_count; j++) {
            if (strcmp(optimizedQuad[i].op1, constants[j].var) == 0 && constants[j].line < i) {
                strcpy(optimizedQuad[i].op1, constants[j].val);
            }
        }
        
        // Replace op2 if it's a constant variable (and the constant was defined before this line)
        for (int j = 0; j < const_count; j++) {
            if (strcmp(optimizedQuad[i].op2, constants[j].var) == 0 && constants[j].line < i) {
                strcpy(optimizedQuad[i].op2, constants[j].val);
            }
        }
    }
}

/* Copy propagation - replace variables with their copy source */
void copy_propagation() {
    // Map of copy relationships
    struct {
        char dest[MAX_LENGTH];
        char src[MAX_LENGTH];
        int line; // Track where the copy was defined
    } copies[MAX_QUAD];
    int copy_count = 0;
    
    // First pass: identify copy assignments
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue; // Skip removed quadruples
        
        if (strcmp(optimizedQuad[i].oper, "=") == 0 && !is_constant(optimizedQuad[i].op1) && 
            strlen(optimizedQuad[i].op2) == 0) {
            // This is a copy from one variable to another
            strcpy(copies[copy_count].dest, optimizedQuad[i].res);
            strcpy(copies[copy_count].src, optimizedQuad[i].op1);
            copies[copy_count].line = i;
            copy_count++;
        }
    }
    
    // Second pass: propagate copies (only forward propagation and avoid control flow variables)
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue; // Skip removed quadruples
        
        // Skip assignment statements to avoid changing the copy itself
        if (strcmp(optimizedQuad[i].oper, "=") == 0) continue;
        
        // Replace op1 if it's a copy destination
        for (int j = 0; j < copy_count; j++) {
            if (strcmp(optimizedQuad[i].op1, copies[j].dest) == 0 && copies[j].line < i) {
                strcpy(optimizedQuad[i].op1, copies[j].src);
            }
        }
        
        // Replace op2 if it's a copy destination
        for (int j = 0; j < copy_count; j++) {
            if (strcmp(optimizedQuad[i].op2, copies[j].dest) == 0 && copies[j].line < i) {
                strcpy(optimizedQuad[i].op2, copies[j].src);
            }
        }
    }
}

/* Improved dead code elimination - remove unused variable assignments */
void dead_code_elimination() {
    // Array to track which variables are used
    char used_vars[MAX_QUAD][MAX_LENGTH];
    int used_count = 0;

    // First pass: collect all variables that are used in expressions and control flow
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue;

        // For non-assignment operations, mark op1 and op2 as used
        if (strcmp(optimizedQuad[i].oper, "=") != 0) {
            // Check op1
            if (!is_constant(optimizedQuad[i].op1) && strlen(optimizedQuad[i].op1) > 0) {
                int found = 0;
                for (int j = 0; j < used_count; j++) {
                    if (strcmp(used_vars[j], optimizedQuad[i].op1) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(used_vars[used_count], optimizedQuad[i].op1);
                    used_count++;
                }
            }

            // Check op2
            if (!is_constant(optimizedQuad[i].op2) && strlen(optimizedQuad[i].op2) > 0) {
                int found = 0;
                for (int j = 0; j < used_count; j++) {
                    if (strcmp(used_vars[j], optimizedQuad[i].op2) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(used_vars[used_count], optimizedQuad[i].op2);
                    used_count++;
                }
            }

            // For control flow operations, also mark the result as used
            if (is_control_flow_op(optimizedQuad[i].oper)) {
                if (!is_constant(optimizedQuad[i].res) && strlen(optimizedQuad[i].res) > 0) {
                    int found = 0;
                    for (int j = 0; j < used_count; j++) {
                        if (strcmp(used_vars[j], optimizedQuad[i].res) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        strcpy(used_vars[used_count], optimizedQuad[i].res);
                        used_count++;
                    }
                }
            }
        }
    }

    // Second pass: find variables that are used later in the code
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0) continue;

        if (strcmp(optimizedQuad[i].oper, "=") == 0) {
            int used = 0;

            // Check if result is in used_vars (from first pass)
            for (int j = 0; j < used_count; j++) {
                if (strcmp(optimizedQuad[i].res, used_vars[j]) == 0) {
                    used = 1;
                    break;
                }
            }

            // Also check if result is used later in the code
            for (int j = i + 1; j < optimizedQc && !used; j++) {
                if (strlen(optimizedQuad[j].oper) == 0) continue; // Skip removed quadruples
                if ((strlen(optimizedQuad[j].op1) > 0 && strcmp(optimizedQuad[j].op1, optimizedQuad[i].res) == 0) ||
                    (strlen(optimizedQuad[j].op2) > 0 && strcmp(optimizedQuad[j].op2, optimizedQuad[i].res) == 0)) {
                    used = 1;
                    break;
                }
            }

            // Check if this variable is used in control flow
            if (!used && is_used_in_control_flow(optimizedQuad[i].res)) {
                used = 1;
            }

            // Don't remove assignments to class attributes (variables that were assigned constants initially)
            // This preserves the original constant assignments like maxage = 3, car = 213
            for (int k = 0; k < optimizedQc; k++) {
                if (k != i && strcmp(optimizedQuad[k].oper, "=") == 0 && 
                    strcmp(optimizedQuad[k].res, optimizedQuad[i].res) == 0 &&
                    is_constant(optimizedQuad[k].op1)) {
                    used = 1; // This variable had a constant assignment, keep it
                    break;
                }
            }

            // Mark for deletion if not used
            if (!used) {
                optimizedQuad[i].oper[0] = '\0'; // Mark as removed
            }
        }
    }
}

/* Common subexpression elimination */
void common_subexpression_elimination() {
    for (int i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) == 0 || !is_arithmetic_op(optimizedQuad[i].oper)) {
            continue; // Skip removed or non-arithmetic quadruples
        }
        
        // Look for identical expressions earlier in the code
        for (int j = 0; j < i; j++) {
            if (strlen(optimizedQuad[j].oper) == 0) continue; // Skip removed quadruples
            
            // Check if this is the same operation with the same operands
            if (strcmp(optimizedQuad[i].oper, optimizedQuad[j].oper) == 0 &&
                strcmp(optimizedQuad[i].op1, optimizedQuad[j].op1) == 0 &&
                strcmp(optimizedQuad[i].op2, optimizedQuad[j].op2) == 0) {
                
                // Make sure the result of the earlier computation is still valid
                // (hasn't been reassigned between j and i)
                int result_modified = 0;
                for (int k = j + 1; k < i; k++) {
                    if (strlen(optimizedQuad[k].oper) == 0) continue;
                    if (strcmp(optimizedQuad[k].oper, "=") == 0 && 
                        strcmp(optimizedQuad[k].res, optimizedQuad[j].res) == 0) {
                        result_modified = 1;
                        break;
                    }
                }
                
                if (!result_modified) {
                    // Replace this calculation with a copy of the earlier result
                    strcpy(optimizedQuad[i].oper, "=");
                    strcpy(optimizedQuad[i].op1, optimizedQuad[j].res);
                    optimizedQuad[i].op2[0] = '\0';
                    break;
                }
            }
        }
    }
}

/* Compact the quadruple array by removing empty entries */
void compact_quadruples() {
    int write_idx = 0;
    
    for (int read_idx = 0; read_idx < optimizedQc; read_idx++) {
        if (strlen(optimizedQuad[read_idx].oper) > 0) {
            if (write_idx != read_idx) {
                optimizedQuad[write_idx] = optimizedQuad[read_idx];
            }
            write_idx++;
        }
    }
    
    optimizedQc = write_idx;
}

/* Main optimization function */
void optimize_intermediate_code() {
    // Copy original quadruples to optimization workspace
    copy_quads_for_optimization();
    
    // Apply optimization techniques in multiple passes
    
    // First pass: Basic optimizations
    constant_folding();
    constant_propagation();
    copy_propagation();
    
    // Second pass: Apply folding again after propagation
    constant_folding();
    
    // Third pass: Advanced optimizations
    common_subexpression_elimination();
    
    // Final pass: Remove dead code (be conservative)
    dead_code_elimination();
    
    // Compact the array to remove empty entries
    compact_quadruples();
}