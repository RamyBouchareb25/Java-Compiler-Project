#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token_defs.h"
#include "colors.h"
#include "symbol_table.h"


// Global symbol table
symbol_table_entry* symbol_table = NULL;
int current_scope = 0;

// Create a new symbol table entry
symbol_table_entry* create_symbol(char* name, char* type, int line_defined, int is_method) {
    symbol_table_entry* entry = (symbol_table_entry*)malloc(sizeof(symbol_table_entry));
    entry->name = strdup(name);
    entry->type = strdup(type);
    entry->scope = current_scope;
    entry->line_defined = line_defined;
    entry->is_method = is_method;
    entry->next = NULL;
    return entry;
}

// Add a symbol to the symbol table
void add_symbol(char* name, char* type, int line_defined, int is_method) {
    // Check if symbol already exists in current scope only (not parent scopes)
    symbol_table_entry* current = symbol_table;
    while (current != NULL) {
        if (current->scope == current_scope && strcmp(current->name, name) == 0) {
            // For method parameters, allow shadowing of class fields
            if (current_scope > 0 && !current->is_method) {
                // This is a parameter shadowing a field - allow it
                break;
            }
            printf("%sSemantic_Error, %d, %d, Redeclaration of '%s' in the same scope%s\n", 
                   ANSI_YELLOW, line_defined, 0, name, ANSI_RESET);
            return;
        }
        current = current->next;
    }
    
    // Add new symbol to the table
    symbol_table_entry* new_entry = create_symbol(name, type, line_defined, is_method);
    new_entry->next = symbol_table;
    symbol_table = new_entry;
    
    printf("Added symbol: %s (type: %s) in scope %d\n", name, type, current_scope);
}

symbol_table_entry* lookup_symbol_in_scope(char* name, int scope) {
    symbol_table_entry* current = symbol_table;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0 && current->scope == scope) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Look up a symbol in the symbol table - checks current scope and all parent scopes
symbol_table_entry* lookup_symbol(char* name) {
    symbol_table_entry* current = symbol_table;
    symbol_table_entry* found = NULL;
    int deepest_scope = -1;
    
    // Find the symbol in the deepest (most local) scope
    while (current != NULL) {
        if (strcmp(current->name, name) == 0 && current->scope <= current_scope && current->scope > deepest_scope) {
            found = current;
            deepest_scope = current->scope;
        }
        current = current->next;
    }
    
    return found;
}

// Enter a new scope
void enter_scope() {
    current_scope++;
    printf("Entering scope %d\n", current_scope);
}

// Exit the current scope
void exit_scope() {
    // We don't actually remove symbols from the table
    // We just decrement the scope level
    printf("Exiting scope %d\n", current_scope);
    current_scope--;
}

// Print all symbols in the symbol table
void print_symbol_table() {
    printf("\n--- Symbol Table ---\n");
    printf("Name\t\tType\t\tScope\tLine\tIs Method\n");
    printf("------------------------------------------------\n");
    
    symbol_table_entry* current = symbol_table;
    while (current != NULL) {
        printf("%s\t\t%s\t\t%d\t%d\t%d\n", 
               current->name, current->type, current->scope, 
               current->line_defined, current->is_method);
        current = current->next;
    }
    printf("------------------------------------------------\n");
}

// Free all memory used by the symbol table
void free_symbol_table() {
    symbol_table_entry* current = symbol_table;
    while (current != NULL) {
        symbol_table_entry* temp = current;
        current = current->next;
        free(temp->name);
        free(temp->type);
        free(temp);
    }
    symbol_table = NULL;
}




