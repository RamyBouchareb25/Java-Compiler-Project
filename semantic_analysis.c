#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token_defs.h"
#include "colors.h" // Ajout de l'inclusion pour les couleurs

// Définitions des variables et fonctions externes
extern int line_num;
extern int column_num;

// Ne pas utiliser 'extern symbol_table_entry* symbol_table' car le type n'est pas défini complètement ici
// Au lieu de cela, incluons la définition complète de la structure
// Définition complète de symbol_table_entry, identique à celle de symbol_table.c
typedef struct symbol_table_entry {
    char* name;        // Variable/function name
    char* type;        // Data type
    int scope;         // Scope level
    int line_defined;  // Line where it was defined
    int is_method;     // 1 if method, 0 if variable
    int is_param;      // 1 if parameter, 0 otherwise
    struct symbol_table_entry* next;
} symbol_table_entry;

// Déclarations externes
extern symbol_table_entry* symbol_table;
extern int current_scope;

// Déclarations de fonctions externes
extern symbol_table_entry* lookup_symbol(char* name);
extern symbol_table_entry* lookup_symbol_in_scope(char* name, int scope);
extern void add_symbol(char* name, char* type, int line_num, int is_method);
extern void enter_scope();
extern void exit_scope();

// Structure to hold type information
typedef struct {
    int type_code;    // One of the TYPE_* constants
    char* class_name; // For object types, the class name
    int is_array;     // 1 if array, 0 otherwise
} type_info;

// Create a type_info structure for a basic type
type_info create_basic_type(int type_code) {
    type_info result;
    result.type_code = type_code;
    result.class_name = NULL;
    result.is_array = 0;
    return result;
}

// Create a type_info structure for an object type
type_info create_object_type(char* class_name) {
    type_info result;
    result.type_code = TYPE_OBJECT;
    result.class_name = strdup(class_name);
    result.is_array = 0;
    return result;
}

// Create a type_info structure for an array type
type_info create_array_type(type_info base_type) {
    type_info result = base_type;
    result.is_array = 1;
    return result;
}

// Parse a type string into a type_info structure
type_info parse_type(char* type_str) {
    if (strstr(type_str, "[]") != NULL) {
        // Array type
        char base_type[256];
        strncpy(base_type, type_str, strlen(type_str) - 2);
        base_type[strlen(type_str) - 2] = '\0';
        
        type_info base = parse_type(base_type);
        return create_array_type(base);
    } else if (strcmp(type_str, "int") == 0) {
        return create_basic_type(TYPE_INT);
    } else if (strcmp(type_str, "boolean") == 0) {
        return create_basic_type(TYPE_BOOLEAN);
    } else if (strcmp(type_str, "void") == 0) {
        return create_basic_type(TYPE_VOID);
    } else {
        // Assume it's a class name
        return create_object_type(type_str);
    }
}

// Check if a variable is already declared in the current scope
int is_variable_declared_in_current_scope(char* name) {
    return lookup_symbol_in_scope(name, current_scope) != NULL;
}

// Check if a variable is declared in any accessible scope
int is_variable_declared(char* name) {
    return lookup_symbol(name) != NULL;
}

// Check if two types are compatible (for assignment, parameter passing, etc.)
int are_types_compatible(type_info t1, type_info t2) {
    // If both are arrays, check if element types are compatible
    if (t1.is_array && t2.is_array) {
        t1.is_array = 0;
        t2.is_array = 0;
        return are_types_compatible(t1, t2);
    }
    
    // If both are primitive types
    if (t1.type_code != TYPE_OBJECT && t2.type_code != TYPE_OBJECT) {
        return t1.type_code == t2.type_code;
    }
    
    // If both are object types, check if same class or subclass
    if (t1.type_code == TYPE_OBJECT && t2.type_code == TYPE_OBJECT) {
        // For now, just check if they're the same class
        return strcmp(t1.class_name, t2.class_name) == 0;
    }
    
    // Otherwise, not compatible
    return 0;
}

// Function to check variable declarations
void check_variable_declaration(char* name, char* type, int line) {
    // We'll use the add_symbol function which already checks for redeclarations
    // If there's a redeclaration, it will print an error message
    add_symbol(name, type, line, 0); // 0 means not a method
}

// Check expression type
type_info check_expression_type(char* expr, int line) {
    // For now, just check if it's a variable and return its type
    if (is_variable_declared(expr)) {
        symbol_table_entry* entry = lookup_symbol(expr);
        return parse_type(entry->type);
    }
    
    // Default to ERROR type if expression can't be recognized
    return create_basic_type(TYPE_ERROR);
}

// Check assignment compatibility
void check_assignment(char* left, char* right, int line) {
    type_info left_type = check_expression_type(left, line);
    type_info right_type = check_expression_type(right, line);
    
    if (!are_types_compatible(left_type, right_type)) {
        printf("%sSemantic_Error, %d, %d, Incompatible types in assignment%s\n", ANSI_YELLOW, line, 0, ANSI_RESET);
    }
}

// Enter a new semantic scope
void semantic_enter_scope() {
    enter_scope();
}

// Exit the current semantic scope
void semantic_exit_scope() {
    exit_scope();
}

// Print semantic error about redeclaration
void report_redeclaration_error(char* name, int line) {
    printf("%sSemantic_Error, %d, %d, Redeclaration of '%s' in the same scope%s\n", 
           ANSI_YELLOW, line, 0, name, ANSI_RESET);
}

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

// For field access expressions like this.field
type_info check_field_access(char* object_name, char* field_name) {
    type_info result = {TYPE_ERROR, NULL, 0};
    
    // Special case for 'this'
    if (strcmp(object_name, "this") == 0) {
        // Look up the field in the class scope (scope 0)
        // In a real compiler, you'd do more sophisticated handling here
        symbol_table_entry* entry = lookup_symbol_in_scope(field_name, 0);
        if (entry) {
            result.type_code = get_type_code(entry->type);
            return result;
        }
    }
    
    // Handle regular object field access
    // ...
    
    return result;
}

// Get type information for an identifier
type_info get_identifier_type(char* id) {
    type_info result = {TYPE_ERROR, NULL, 0};
    
    symbol_table_entry* entry = lookup_symbol(id);
    if (entry == NULL) {
        printf("%sSemantic_Error, %d, %d, Undeclared identifier: %s%s\n", ANSI_YELLOW, line_num, column_num, id, ANSI_RESET);
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
        printf("%sSemantic_Error, %d, %d, Cannot apply binary operator to arrays%s\n", ANSI_YELLOW, line_num, column_num, ANSI_RESET);
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
            printf("%sSemantic_Error, %d, %d, Invalid operands for arithmetic operation: %s and %s%s\n",
                   ANSI_YELLOW, line_num, column_num, 
                   type_code_to_string(left.type_code), 
                   type_code_to_string(right.type_code),
                   ANSI_RESET);
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
        
        printf("%sSemantic_Error, %d, %d, Invalid operands for comparison: %s and %s%s\n",
               ANSI_YELLOW, line_num, column_num, 
               type_code_to_string(left.type_code), 
               type_code_to_string(right.type_code),
               ANSI_RESET);
        return TYPE_ERROR;
    }
    
    // Logical operations
    if (op == AND || op == OR) {
        if (left.type_code == TYPE_BOOLEAN && right.type_code == TYPE_BOOLEAN) {
            return TYPE_BOOLEAN;
        } else {
            printf("%sSemantic_Error, %d, %d, Logical operators require boolean operands%s\n", 
                   ANSI_YELLOW, line_num, column_num, ANSI_RESET);
            return TYPE_ERROR;
        }
    }
    
    // Default case - shouldn't reach here
    printf("%sSemantic_Error, %d, %d, Unsupported binary operation%s\n", ANSI_YELLOW, line_num, column_num, ANSI_RESET);
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
    
    printf("%sSemantic_Error, %d, %d, Condition must be of boolean type, got %s%s\n",
        ANSI_YELLOW, line_num, column_num, type_code_to_string(condition.type_code), ANSI_RESET);
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
            printf("%sSemantic_Error, %d, %d, Void method cannot return a value%s\n",
                   ANSI_YELLOW, line_num, column_num, ANSI_RESET);
            return 0;
        }
        return 1;
    }
    
    // Non-void methods must return a compatible value
    if (return_value.type_code == TYPE_VOID) {
        printf("%sSemantic_Error, %d, %d, Missing return value for non-void method%s\n",
               ANSI_YELLOW, line_num, column_num, ANSI_RESET);
        return 0;
    }
    
    // Check if the return type is compatible with the expected type
    if (!check_assignment_compatibility(expected_type, return_value)) {
        printf("%sSemantic_Error, %d, %d, Incompatible return type: expected %s, got %s%s\n",
               ANSI_YELLOW, line_num, column_num, 
               type_code_to_string(expected_type.type_code),
               type_code_to_string(return_value.type_code),
               ANSI_RESET);
        return 0;
    }
    
    return 1;
}

// Check method call arguments against parameter types
int check_method_arguments(char* method_name, type_info* arg_types, int arg_count) {
    // Find method in symbol table
    symbol_table_entry* method = lookup_symbol(method_name);
    if (method == NULL || !method->is_method) {
        printf("%sSemantic_Error, %d, %d, Undefined method: %s%s\n", 
               ANSI_YELLOW, line_num, column_num, method_name, ANSI_RESET);
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
        printf("%sSemantic_Error, %d, %d, Cannot perform array access on non-array type%s\n",
               ANSI_YELLOW, line_num, column_num, ANSI_RESET);
        return TYPE_ERROR;
    }
    
    // Index must be int
    if (index.type_code != TYPE_INT) {
        printf("%sSemantic_Error, %d, %d, Array index must be an integer%s\n",
               ANSI_YELLOW, line_num, column_num, ANSI_RESET);
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
        printf("%sSemantic_Error, %d, %d, Thrown exception must be an object type%s\n",
               ANSI_YELLOW, line_num, column_num, ANSI_RESET);
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
        printf("%sSemantic_Error, %d, %d, Catch parameter must be an exception type%s\n",
               ANSI_YELLOW, line_num, column_num, ANSI_RESET);
        return 0;
    }
    
    return 1;
}

// Check variable initialization
void check_initialization(char* var_name, type_info init_value) {
    symbol_table_entry* var = lookup_symbol(var_name);
    if (var == NULL) {
        // This shouldn't happen if declaration was processed correctly
        printf("%sSemantic_Error, %d, %d, Internal error: Variable %s not found%s\n",
               ANSI_YELLOW, line_num, column_num, var_name, ANSI_RESET);
        return;
    }
    
    type_info var_type = {get_type_code(var->type), NULL, 0};
    if (var_type.type_code == TYPE_OBJECT) {
        var_type.class_name = strdup(var->type);
    }
    
    if (!check_assignment_compatibility(var_type, init_value)) {
        printf("%sSemantic_Error, %d, %d, Incompatible types in initialization: %s = %s%s\n",
               ANSI_YELLOW, line_num, column_num, 
               var->type, type_code_to_string(init_value.type_code), ANSI_RESET);
    }
}

// Clean up resources
void semantic_cleanup() {
    // Free any allocated memory in type_info structures
    // (in a real compiler, we would track these allocations)
}