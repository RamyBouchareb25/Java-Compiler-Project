// semantic_analysis.h
#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H
#include "ast.h"
#include "symbol_table.h"
#include "token_defs.h"

void analyze_ast(ast_node* node);


// Structure to hold type information
typedef struct {
    int type_code;    // One of the TYPE_* constants from token_defs.h
    char* class_name; // For object types, the class name
    int is_array;     // 1 if array, 0 otherwise
} type_info;

// Symbol table entry (simplified version for semantic analysis)
typedef struct symbol_table_entry {
    char* name;      // Symbol name
    char* type;      // Type name (e.g., "int", "String", etc.)
    int is_method;   // Flag for methods
    int scope_level; // For scope tracking
    struct symbol_table_entry* next; // For linked list implementation
} symbol_table_entry;

// Function declarations
int get_type_code(const char* type_name);
const char* type_code_to_string(int type_code);
type_info get_identifier_type(char* id);
int check_binary_operation_type(int op, type_info left, type_info right);
int check_assignment_compatibility(type_info target, type_info value);
int check_condition(type_info condition);
int check_return_type(type_info return_value, type_info expected_type);
int check_method_arguments(char* method_name, type_info* arg_types, int arg_count);
int check_array_access(type_info array, type_info index);
int check_throw_statement(type_info exception);
int check_try_catch(type_info catch_type);
void check_initialization(char* var_name, type_info init_value);
void semantic_cleanup(void);

// External symbol table functions (from symbol_table.c)
extern symbol_table_entry* lookup_symbol(char* name);

#endif /* SEMANTIC_ANALYSIS_H */