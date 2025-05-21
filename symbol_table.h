#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef struct symbol_table_entry {
    char* name;        // Variable/function name
    char* type;        // Data type
    int scope;         // Scope level
    int line_defined;  // Line where it was defined
    int is_method;     // 1 if method, 0 if variable
    struct symbol_table_entry* next;
} symbol_table_entry;

// Déclaration des variables globales (extern)
extern symbol_table_entry* symbol_table;
extern int current_scope;

// Déclaration des fonctions
symbol_table_entry* create_symbol(char* name, char* type, int line_defined, int is_method);
void add_symbol(char* name, char* type, int line_defined, int is_method);
symbol_table_entry* lookup_symbol_in_scope(char* name, int scope);
symbol_table_entry* lookup_symbol(char* name);
void enter_scope();
void exit_scope();
void print_symbol_table();
void free_symbol_table();

#endif // SYMBOL_TABLE_H