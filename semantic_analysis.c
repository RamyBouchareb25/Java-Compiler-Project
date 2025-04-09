#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token_defs.h"



// Complete symbol table entry structure
typedef struct symbol_table_entry {
    char* name;      // Symbol name
    char* type;      // Type name (e.g., "int", "String", etc.)
    int is_method;   // Flag for methods
    int scope_level; // For scope tracking
    // Add other fields as needed
    struct symbol_table_entry* next; // For linked list implementation
} symbol_table_entry;

// Forward declarations
extern symbol_table_entry* lookup_symbol(char* name);
extern int line_num;
extern int column_num;

// Structure to hold type information
typedef struct {
    int type_code;    // One of the TYPE_* constants
    char* class_name; // For object types, the class name
    int is_array;     // 1 if array, 0 otherwise
} type_info;

// Convert string type name to type code
int get_type_code(const char* type_name) {
    if (strcmp(type_name, "int") == 0) return TYPE_INT;
    if (strcmp(type_name, "float") == 0) return TYPE_FLOAT;
    if (strcmp(type_name, "double") == 0) return TYPE_DOUBLE;
    if (strcmp(type_name, "char") == 0) return TYPE_CHAR;
    if (strcmp(type_name, "boolean") == 0) return TYPE_BOOLEAN;
    if (strcmp(type_name, "String") == 0) return TYPE_STRING;
    if (strcmp(type_name, "void") == 0) return TYPE_VOID;
    if (strcmp(type_name, "null") == 0) return TYPE_NULL;
    return TYPE_OBJECT; // Assume it's a user-defined class
}

// Convert type code to string
const char* type_code_to_string(int type_code) {
    switch (type_code) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_CHAR: return "char";
        case TYPE_BOOLEAN: return "boolean";
        case TYPE_STRING: return "String";
        case TYPE_VOID: return "void";
        case TYPE_OBJECT: return "object";
        case TYPE_ERROR: return "error";
        case TYPE_NULL: return "null";
        default: return "unknown";
    }
}

// Get type information for an identifier
type_info get_identifier_type(char* id) {
    type_info result = {TYPE_ERROR, NULL, 0};
    
    symbol_table_entry* entry = lookup_symbol(id);
    if (entry == NULL) {
        printf("Semantic_Error, %d, %d, Undeclared identifier: %s\n", line_num, column_num, id);
        return result;
    }
    
    result.type_code = get_type_code(entry->type);
    if (result.type_code == TYPE_OBJECT) {
        result.class_name = strdup(entry->type);
    }
    
    // Check if it's an array (if type ends with [])
    int len = strlen(entry->type);
    if (len >= 2 && entry->type[len-2] == '[' && entry->type[len-1] == ']') {
        result.is_array = 1;
    }
    
    return result;
}

// Check type compatibility for binary operations
int check_binary_operation_type(int op, type_info left, type_info right) {
    // Error cases
    if (left.type_code == TYPE_ERROR || right.type_code == TYPE_ERROR) {
        return TYPE_ERROR;
    }
    
    // Arrays are not compatible with binary operations
    if (left.is_array || right.is_array) {
        printf("Semantic_Error, %d, %d, Cannot apply binary operator to arrays\n", line_num, column_num);
        return TYPE_ERROR;
    }
    
    // String concatenation with +
    if (op == '+' && (left.type_code == TYPE_STRING || right.type_code == TYPE_STRING)) {
        return TYPE_STRING;
    }
    
    // Arithmetic operations
    if (op == '+' || op == '-' || op == '*' || op == '/' || op == '%') {
        // Check if both operands are numeric
        if ((left.type_code >= TYPE_INT && left.type_code <= TYPE_DOUBLE) &&
            (right.type_code >= TYPE_INT && right.type_code <= TYPE_DOUBLE)) {
            
            // Type promotion rules
            if (left.type_code == TYPE_DOUBLE || right.type_code == TYPE_DOUBLE) {
                return TYPE_DOUBLE;
            } else if (left.type_code == TYPE_FLOAT || right.type_code == TYPE_FLOAT) {
                return TYPE_FLOAT;
            } else {
                return TYPE_INT;
            }
        } else {
            printf("Semantic_Error, %d, %d, Invalid operands for arithmetic operation: %s and %s\n",
                   line_num, column_num, 
                   type_code_to_string(left.type_code), 
                   type_code_to_string(right.type_code));
            return TYPE_ERROR;
        }
    }
    
    // Comparison operations
    if (op == '>' || op == '<' || op == GREATER_EQUAL || op == LESS_EQUAL || 
        op == EQUAL || op == NOT_EQUAL) {
        
        // Numeric comparisons
        if ((left.type_code >= TYPE_INT && left.type_code <= TYPE_DOUBLE) &&
            (right.type_code >= TYPE_INT && right.type_code <= TYPE_DOUBLE)) {
            return TYPE_BOOLEAN;
        }
        
        // String comparison
        if (left.type_code == TYPE_STRING && right.type_code == TYPE_STRING) {
            return TYPE_BOOLEAN;
        }
        
        // Boolean comparison (only == and !=)
        if (left.type_code == TYPE_BOOLEAN && right.type_code == TYPE_BOOLEAN &&
            (op == EQUAL || op == NOT_EQUAL)) {
            return TYPE_BOOLEAN;
        }
        
        // Object comparison (only == and !=)
        if (left.type_code == TYPE_OBJECT && right.type_code == TYPE_OBJECT &&
            (op == EQUAL || op == NOT_EQUAL)) {
            // For object comparison, we should check if types are compatible
            // (e.g., same class or subclass), but we'll simplify for now
            return TYPE_BOOLEAN;
        }
        
        printf("Semantic_Error, %d, %d, Invalid operands for comparison: %s and %s\n",
               line_num, column_num, 
               type_code_to_string(left.type_code), 
               type_code_to_string(right.type_code));
        return TYPE_ERROR;
    }
    
    // Logical operations
    if (op == AND || op == OR) {
        if (left.type_code == TYPE_BOOLEAN && right.type_code == TYPE_BOOLEAN) {
            return TYPE_BOOLEAN;
        } else {
            printf("Semantic_Error, %d, %d, Logical operators require boolean operands\n", 
                   line_num, column_num);
            return TYPE_ERROR;
        }
    }
    
    // Default case - shouldn't reach here
    printf("Semantic_Error, %d, %d, Unsupported binary operation\n", line_num, column_num);
    return TYPE_ERROR;
}

// Check type compatibility for assignment
int check_assignment_compatibility(type_info target, type_info value) {
    // Error cases
    if (target.type_code == TYPE_ERROR || value.type_code == TYPE_ERROR) {
        return 0;
    }
    
    // Arrays must match in type and dimensions
    if (target.is_array != value.is_array) {
        return 0;
    }
    
    // Exact type match
    if (target.type_code == value.type_code) {
        // For objects, check class compatibility
        if (target.type_code == TYPE_OBJECT) {
            // Simple approach: require exact class match
            // A more advanced version would check inheritance
            return (strcmp(target.class_name, value.class_name) == 0);
        }
        return 1;
    }
    
    // Widening primitive conversions (allowed in Java)
    if (target.type_code == TYPE_DOUBLE && 
        (value.type_code == TYPE_FLOAT || value.type_code == TYPE_INT)) {
        return 1;
    }
    
    if (target.type_code == TYPE_FLOAT && value.type_code == TYPE_INT) {
        return 1;
    }
    
    // Null can be assigned to any object type
    if (target.type_code == TYPE_OBJECT && value.type_code == TYPE_NULL) {
        return 1;
    }
    
    // Types are not compatible
    return 0;
}

// Check if an expression is a valid condition (for if, while, etc.)
int check_condition(type_info condition) {
    if (condition.type_code == TYPE_BOOLEAN) {
        return 1;
    }
    
    printf("Semantic_Error, %d, %d, Condition must be of boolean type, got %s\n",
        line_num, column_num, type_code_to_string(condition.type_code));
    return 0;
}

// Check if a return statement matches the expected return type
int check_return_type(type_info return_value, type_info expected_type) {
    // Error cases
    if (return_value.type_code == TYPE_ERROR || expected_type.type_code == TYPE_ERROR) {
        return 0;
    }
    
    // Void methods should not return a value
    if (expected_type.type_code == TYPE_VOID) {
        if (return_value.type_code != TYPE_VOID) {
            printf("Semantic_Error, %d, %d, Void method cannot return a value\n",
                   line_num, column_num);
            return 0;
        }
        return 1;
    }
    
    // Non-void methods must return a compatible value
    if (return_value.type_code == TYPE_VOID) {
        printf("Semantic_Error, %d, %d, Missing return value for non-void method\n",
               line_num, column_num);
        return 0;
    }
    
    // Check if the return type is compatible with the expected type
    if (!check_assignment_compatibility(expected_type, return_value)) {
        printf("Semantic_Error, %d, %d, Incompatible return type: expected %s, got %s\n",
               line_num, column_num, 
               type_code_to_string(expected_type.type_code),
               type_code_to_string(return_value.type_code));
        return 0;
    }
    
    return 1;
}

// Check method call arguments against parameter types
int check_method_arguments(char* method_name, type_info* arg_types, int arg_count) {
    // Find method in symbol table
    symbol_table_entry* method = lookup_symbol(method_name);
    if (method == NULL || !method->is_method) {
        printf("Semantic_Error, %d, %d, Undefined method: %s\n", 
               line_num, column_num, method_name);
        return 0;
    }
    
    // In a real compiler, we would store parameter types with the method
    // For simplicity, we'll just assume the arguments are valid
    // A complete implementation would check each argument against parameter type
    
    printf("Method call to %s with %d arguments\n", method_name, arg_count);
    return 1;
}

// Check array access
int check_array_access(type_info array, type_info index) {
    // Error cases
    if (array.type_code == TYPE_ERROR || index.type_code == TYPE_ERROR) {
        return TYPE_ERROR;
    }
    
    // Must be accessing an array
    if (!array.is_array) {
        printf("Semantic_Error, %d, %d, Cannot perform array access on non-array type\n",
               line_num, column_num);
        return TYPE_ERROR;
    }
    
    // Index must be int
    if (index.type_code != TYPE_INT) {
        printf("Semantic_Error, %d, %d, Array index must be an integer\n",
               line_num, column_num);
        return TYPE_ERROR;
    }
    
    // Return the element type (remove the array property)
    type_info result = array;
    result.is_array = 0;
    return result.type_code;
}

// Check a throw statement
int check_throw_statement(type_info exception) {
    // Exception must be an object type (ideally a subclass of Exception)
    if (exception.type_code != TYPE_OBJECT) {
        printf("Semantic_Error, %d, %d, Thrown exception must be an object type\n",
               line_num, column_num);
        return 0;
    }
    
    // In a real compiler, we would check that the exception type
    // extends Throwable or Exception
    
    return 1;
}

// Check a try-catch block
int check_try_catch(type_info catch_type) {
    // Catch type must be an object type (ideally a subclass of Exception)
    if (catch_type.type_code != TYPE_OBJECT) {
        printf("Semantic_Error, %d, %d, Catch parameter must be an exception type\n",
               line_num, column_num);
        return 0;
    }
    
    return 1;
}

// Check variable initialization
void check_initialization(char* var_name, type_info init_value) {
    symbol_table_entry* var = lookup_symbol(var_name);
    if (var == NULL) {
        // This shouldn't happen if declaration was processed correctly
        printf("Semantic_Error, %d, %d, Internal error: Variable %s not found\n",
               line_num, column_num, var_name);
        return;
    }
    
    type_info var_type = {get_type_code(var->type), NULL, 0};
    if (var_type.type_code == TYPE_OBJECT) {
        var_type.class_name = strdup(var->type);
    }
    
    if (!check_assignment_compatibility(var_type, init_value)) {
        printf("Semantic_Error, %d, %d, Incompatible types in initialization: %s = %s\n",
               line_num, column_num, 
               var->type, type_code_to_string(init_value.type_code));
    }
}

// Clean up resources
void semantic_cleanup() {
    // Free any allocated memory in type_info structures
    // (in a real compiler, we would track these allocations)
}