#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef struct symbol {
    char* name;
    char* type;
    int scope;
    int is_method;
    struct symbol* next;
} symbol_t;

extern symbol_t* symbol_table;
extern int current_scope;

void add_symbol(char* name, char* type, int line_num, int is_method);
symbol_t* lookup_symbol(char* name);
void enter_scope();
void exit_scope();
void free_symbol_table();

#endif