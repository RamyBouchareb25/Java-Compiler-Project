#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "quad.h"
#include "symbol_table.h"

// Définition de l'enum IdType pour la compatibilité avec le reste du code
typedef enum {
    TYPE_INTEGER,
    TYPE_FLOAT_NUM,
    TYPE_CHARACTER,
    TYPE_BOOL,
    TYPE_STRING_CHARACTER,
    TYPE_ARRAY_INT,
    TYPE_ARRAY_FLOAT,
    TYPE_ARRAY_CHAR,
    TYPE_ARRAY_BOOL
} IdType;

// Main function to generate assembly code
void generate_assembly_code(char* output_file) ;


// Helper functions
void generate_data_segment(FILE* asm_file);
void generate_code_segment(FILE* asm_file);
void generate_standard_procedures(FILE* asm_file);
void translate_quadruple(FILE* asm_file, int qdr_index);

// Specialized translation functions for different operations
void translate_assignment(FILE* asm_file, QUAD q, int qdr_index);
void translate_arithmetic(FILE* asm_file, QUAD q, int qdr_index);
void translate_comparison(FILE* asm_file, QUAD q, int qdr_index);
void translate_logical(FILE* asm_file, QUAD q, int qdr_index);
void translate_logical_not(FILE* asm_file, QUAD q, int qdr_index);
void translate_jump(FILE* asm_file, QUAD q, int qdr_index);
void translate_print(FILE* asm_file, QUAD q, int qdr_index);
void translate_read(FILE* asm_file, QUAD q, int qdr_index);
void translate_array_store(FILE* asm_file, QUAD q, int qdr_index);
void translate_array_load(FILE* asm_file, QUAD q, int qdr_index);

// Register allocation
char* allocate_register(char* operand);
void free_register(char* reg);
void initialize_registers();
char* generate_label();

// Utility functions
char* get_data_type_directive(symbol_table_entry* entry);
int get_data_type_size(IdType type);
int is_numeric(const char* str);

#endif 