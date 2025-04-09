#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include <stdio.h>
#include "semantic_analysis.h"
extern int line_num, column_num;

void check_expression(ast_node* node) {
    if (node->type == AST_IDENTIFIER) {
        if (!lookup_symbol(node->data.identifier.name)) {
            printf("Semantic_Error, %d, %d, Undeclared variable: %s\n",
                   line_num, column_num, node->data.identifier.name);
        }
    }
    // Add more type checking for operations
}

void analyze_ast(ast_node* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_DECLARATION:
            add_symbol(node->data.declaration.name, node->data.declaration.type, line_num, 0);
            if (node->data.declaration.init_expr)
                check_expression(node->data.declaration.init_expr);
            break;
        case AST_ASSIGNMENT:
            check_expression(node->data.assignment.lhs);
            check_expression(node->data.assignment.rhs);
            break;
        // Add cases for other node types
    }
}
symbol_t* symbol_table = NULL;
int current_scope = 0;

void add_symbol(char* name, char* type, int line_num, int is_method) {
    symbol_t* sym = malloc(sizeof(symbol_t));
    sym->name = strdup(name);
    sym->type = strdup(type);
    sym->scope = current_scope;
    sym->is_method = is_method;
    sym->next = symbol_table;
    symbol_table = sym;
}

symbol_t* lookup_symbol(char* name) {
    symbol_t* current = symbol_table;
    while (current) {
        if (strcmp(current->name, name) == 0 && current->scope <= current_scope)
            return current;
        current = current->next;
    }
    return NULL;
}

void enter_scope() { current_scope++; }
void exit_scope() {
    symbol_t* current = symbol_table;
    while (current && current->scope == current_scope) {
        symbol_t* temp = current;
        current = current->next;
        free(temp->name);
        free(temp->type);
        free(temp);
    }
    symbol_table = current;
    current_scope--;
}

void free_symbol_table() {
    while (symbol_table) {
        symbol_t* temp = symbol_table;
        symbol_table = symbol_table->next;
        free(temp->name);
        free(temp->type);
        free(temp);
    }
}