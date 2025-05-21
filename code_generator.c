#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "code_generator.h"
#include "quad.h"
#include "symbol_table.h"  // Ajout de l'en-tête de la table des symboles
#include "token_defs.h"


// Fonction pour trouver un symbole dans la table des symboles globale
symbol_table_entry* find_symbol_any_block(char* name) {
    return lookup_symbol(name);
}


// Fonction pour vérifier si un type est un type tableau
int is_array_type(char* type) {
    return (strncmp(type, "array_", 6) == 0);
}

// Fonction pour obtenir le type d'élément d'un tableau
IdType get_element_type(char* type) {
    if (strcmp(type, "array_int") == 0) return TYPE_INT;
    if (strcmp(type, "array_float") == 0) return TYPE_FLOAT;
    if (strcmp(type, "array_char") == 0) return TYPE_CHAR;
    if (strcmp(type, "array_bool") == 0) return TYPE_BOOL;
    return TYPE_INT; // Type par défaut
}

// Register tracking for 8086
#define MAX_REGISTERS 4
char* registers[MAX_REGISTERS] = {"ax", "bx", "cx", "dx"};
int register_used[MAX_REGISTERS] = {0, 0, 0, 0};
char* register_content[MAX_REGISTERS] = {NULL, NULL, NULL, NULL};

// Label counter for unique label generation
int label_counter = 0;

// Temporary variables tracking
#define MAX_TEMP_VARS 100
char* temp_vars[MAX_TEMP_VARS] = {NULL};
int temp_var_count = 0;

// Initialize register allocation system
void initialize_registers() {
    for (int i = 0; i < MAX_REGISTERS; i++) {
        register_used[i] = 0;
        if (register_content[i] != NULL) {
            free(register_content[i]);
            register_content[i] = NULL;
        }
    }
}

// Find register containing a value or allocate a new one
char* allocate_register(char* operand) {
    // Check if operand is already in a register
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (register_used[i] && register_content[i] != NULL && 
            strcmp(register_content[i], operand) == 0) {
            return registers[i];
        }
    }
    
    // Find a free register
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (!register_used[i]) {
            register_used[i] = 1;
            register_content[i] = strdup(operand);
            return registers[i];
        }
    }
    
    // If no free register, use least recently used strategy
    // For simplicity, we'll just take the first register
    free(register_content[0]);
    register_content[0] = strdup(operand);
    return registers[0];
}

// Free a register
void free_register(char* reg) {
    for (int i = 0; i < MAX_REGISTERS; i++) {
        if (strcmp(registers[i], reg) == 0) {
            register_used[i] = 0;
            if (register_content[i] != NULL) {
                free(register_content[i]);
                register_content[i] = NULL;
            }
            return;
        }
    }
}

// Generate a unique label
char* generate_label() {
    char* label = (char*)malloc(20);
    sprintf(label, "L%d", label_counter++);
    return label;
}

// Check if a string represents a number
int is_numeric(const char* str) {
    if (!str) return 0;
    
    // Check for negative numbers
    if (*str == '-' || *str == '+') str++;
    
    // Make sure there's at least one digit
    if (!*str) return 0;
    
    // Check each character is a digit
    while (*str) {
        if (*str == '.') {
            str++; // Skip decimal point
            // Must have at least one digit after decimal
            if (!*str || !isdigit(*str)) return 0;
        } else if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    
    return 1;
}

// Add a temporary variable to our list
void add_temp_var(const char* name) {
    // Check if already exists
    for (int i = 0; i < temp_var_count; i++) {
        if (temp_vars[i] && strcmp(temp_vars[i], name) == 0) {
            return; // Already tracked
        }
    }
    
    // Add if there's space
    if (temp_var_count < MAX_TEMP_VARS) {
        temp_vars[temp_var_count++] = strdup(name);
    }
}

// Scan all quadruples to identify and track all temporary variables
void identify_temp_vars() {
    // Clear existing temp vars
    for (int i = 0; i < temp_var_count; i++) {
        if (temp_vars[i]) {
            free(temp_vars[i]);
            temp_vars[i] = NULL;
        }
    }
    temp_var_count = 0;
    
    // Scan all quadruples
    for (int i = 0; i < optimizedQc; i++) {
        QUAD q = optimizedQuad[i];
        
        // Process result of quadruple if it starts with 't'
        if (q.res && q.res[0] == 't') {
            add_temp_var(q.res);
        }
        
        // Also check operands in case they're temporaries
        if (q.op1 && q.op1[0] == 't') {
            add_temp_var(q.op1);
        }
        
        if (q.op2 && q.op2[0] == 't') {
            add_temp_var(q.op2);
        }
    }
}

// Check for referenced labels in quadruples
int* identify_labels() {
    int* label_exists = (int*)calloc(10000, sizeof(int)); // Assuming labels are < 10000
    
    // First pass: identify labels used in jumps
    for (int i = 0; i < optimizedQc; i++) {
        QUAD q = optimizedQuad[i];
        
        // Check jump instructions
        if (strcmp(q.oper, "BR") == 0 && q.op1) {
            int label = atoi(q.op1);
            label_exists[label] = 1;
        } 
        else if ((strcmp(q.oper, "BZ") == 0 || strcmp(q.oper, "BNZ") == 0) && q.res) {
            int label = atoi(q.res);
            label_exists[label] = 1;
        }
    }
    
    return label_exists;
}

// Determine data type directive for a symbol
char* get_data_type_directive(symbol_table_entry* entry) {
    if (entry == NULL) return "dw"; // Default to word (16-bit)
    
    if (strcmp(entry->type, "int") == 0 || strcmp(entry->type, "bool") == 0 || 
        strcmp(entry->type, "boolean") == 0) {
        return "dw"; // 16-bit word
    } else if (strcmp(entry->type, "float") == 0 || strcmp(entry->type, "double") == 0) {
        return "dd"; // 32-bit double word
    } else if (strcmp(entry->type, "char") == 0) {
        return "db"; // 8-bit byte
    } else if (strcmp(entry->type, "string") == 0) {
        return "db"; // array of bytes
    } else if (is_array_type(entry->type)) {
        // Pour les tableaux, utilisez le type des éléments pour déterminer la directive
        IdType element_type = get_element_type(entry->type);
        switch (element_type) {
            case TYPE_INT:
            case TYPE_BOOL:
                return "dw";
            case TYPE_FLOAT:
                return "dd";
            case TYPE_CHAR:
                return "db";
            default:
                return "dw"; // Par défaut
        }
    }
    return "dw"; // Par défaut pour les autres types
}

// Get size of a data type in bytes
// Helper function to convert IdType to string
char* id_type_to_string(IdType type) {
    switch (type) {
        case TYPE_ARRAY_INT: return "array_int";
        case TYPE_ARRAY_FLOAT: return "array_float";
        case TYPE_ARRAY_CHAR: return "array_char";
        case TYPE_ARRAY_BOOL: return "array_bool";
        default: return "";
    }
}

int get_data_type_size(IdType type) {
    switch (type) {
        case TYPE_INTEGER:
        case TYPE_BOOL:
            return 2; // 16-bit word
        case TYPE_FLOAT_NUM:
            return 4; // 32-bit double word
        case TYPE_CHARACTER:
            return 1; // 8-bit byte
        case TYPE_STRING_CHARACTER:
            return 2; // Pointer (address) size
        default:
            if (is_array_type(id_type_to_string(type))) {
                // For arrays, get the base type's size
                IdType element_type = get_element_type(id_type_to_string(type));
                return get_data_type_size(element_type);
            }
            return 2; // Default
    }
}

int symbol_exists_in_data_segment(char* name) {
    static char* defined_symbols[1000] = {NULL};
    static int count = 0;
    
    // Check if symbol already exists in our tracking array
    for (int i = 0; i < count; i++) {
        if (defined_symbols[i] && strcmp(defined_symbols[i], name) == 0) {
            return 1; // Symbol already exists
        }
    }
    
    // Add symbol to our tracking array
    if (count < 1000) {
        defined_symbols[count++] = strdup(name);
    }
    
    return 0; // Symbol was not previously defined
}

// Updated function to generate the data segment of the assembly code
void generate_data_segment(FILE* asm_file) {
    // Process all identifiers in the table
    symbol_table_entry* current = symbol_table;
    while (current != NULL) {
        // Skip methods/functions
        if (current->is_method) {
            current = current->next;
            continue;
        }
        
        // Check if this symbol has already been defined
        if (symbol_exists_in_data_segment(current->name)) {
            current = current->next;
            continue;
        }
        
        // Get the appropriate data type directive
        char* directive = get_data_type_directive(current);
        
        // Handle different data types
        if (strcmp(current->type, "string") == 0) {
            // Strings are initialized with a terminator and need a buffer for input
            fprintf(asm_file, "%s %s '$'\n", current->name, directive);
            
            // Add an input buffer for string variables (for READ operation)
            fprintf(asm_file, "%s_buf db 255, 0, 255 dup(?)\n", current->name);
        } 
        else if (is_array_type(current->type)) {
            // Tableaux : utiliser une taille par défaut ou extraire la taille du type
            int array_size = 10; // Taille par défaut
            
            fprintf(asm_file, "%s %s %d dup(0)\n", current->name, directive, array_size);
        } 
        else {
            // Regular variables initialized to 0
            fprintf(asm_file, "%s %s 0\n", current->name, directive);
        }
        
        current = current->next;
    }
    
    // Add temporary variables
    for (int i = 0; i < temp_var_count; i++) {
        fprintf(asm_file, "%s dw 0\n", temp_vars[i]);
    }
    
    // Add string literals from the quadruples (for PRINT operations)
    for (int i = 0; i < optimizedQc; i++) {
        QUAD q = optimizedQuad[i];
        
        // Check for string literals in the quadruples
        if ((strcmp(q.oper, "PRINT") == 0 || strcmp(q.oper, "=") == 0) && 
            q.op1 && q.op1[0] == '"') {
            
            // Create a unique label for this string
            fprintf(asm_file, "str_%d db %s, 0Dh, 0Ah, '$'\n", i, q.op1);
        }
    }
    
    fprintf(asm_file, "\n");
}

// Handle array load operations
void translate_array_load(FILE* asm_file, QUAD q, int qdr_index) {
    symbol_table_entry* array_entry = find_symbol_any_block(q.op1);
    int array_size = 10; // Taille par défaut pour les tableaux
    IdType element_type = get_element_type(array_entry->type);
    int element_size = get_data_type_size(element_type);
    
    // Load index value
    if (is_numeric(q.op2)) {
        fprintf(asm_file, "    mov bx, %s\n", q.op2);
    } else {
        fprintf(asm_file, "    mov bx, [%s]\n", q.op2);
    }
    
    // Multiply by element size to get offset (in bytes)
    if (element_size > 1) {
        if (element_size == 2) {
            fprintf(asm_file, "    shl bx, 1     ; Multiply by 2\n");
        } else if (element_size == 4) {
            fprintf(asm_file, "    shl bx, 2     ; Multiply by 4\n");
        } else {
            fprintf(asm_file, "    mov ax, %d\n", element_size);
            fprintf(asm_file, "    mul bx\n");
            fprintf(asm_file, "    mov bx, ax\n");
        }
    }
    
    // Load value from array (no bounds checking)
    fprintf(asm_file, "    mov si, offset %s\n", q.op1);
    fprintf(asm_file, "    add si, bx\n");
    fprintf(asm_file, "    mov ax, [si]\n");
    fprintf(asm_file, "    mov [%s], ax\n", q.res);
}

// Handle assignment operations
void translate_assignment(FILE* asm_file, QUAD q, int qdr_index) {
    // Handle numeric constants
    if (is_numeric(q.op1)) {
        fprintf(asm_file, "    mov ax, %s\n", q.op1);
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    } 
    // Handle string literals
    else if (q.op1 && q.op1[0] == '"') {
        char str_label[50];
        sprintf(str_label, "str_%d", qdr_index);
        fprintf(asm_file, "    lea ax, %s\n", str_label);
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    }
    // Handle character literals
    else if (q.op1 && q.op1[0] == '\'') {
        fprintf(asm_file, "    mov al, %s\n", q.op1);
        fprintf(asm_file, "    mov [%s], al\n", q.res);
    }
    // Variable to variable assignment
    else if (q.op1 && q.op1[0] != '\0') {
        symbol_table_entry* entry = find_symbol_any_block(q.op1);
        
        // Check if we're dealing with a memory-to-memory move
        if (entry && strcmp(entry->type, "char") == 0) {
            fprintf(asm_file, "    mov al, [%s]\n", q.op1);
            fprintf(asm_file, "    mov [%s], al\n", q.res);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
            fprintf(asm_file, "    mov [%s], ax\n", q.res);
        }
    }
    // Empty initialization 
    else {
        fprintf(asm_file, "    xor ax, ax\n");
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    }
}
// Handle arithmetic operations
void translate_arithmetic(FILE* asm_file, QUAD q, int qdr_index) {
    // Addition
    if (strcmp(q.oper, "+") == 0) {
        // Load first operand
        if (is_numeric(q.op1)) {
            fprintf(asm_file, "    mov ax, %s\n", q.op1);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
        }
        
        // Add second operand
        if (is_numeric(q.op2)) {
            fprintf(asm_file, "    add ax, %s\n", q.op2);
        } else {
            fprintf(asm_file, "    add ax, [%s]\n", q.op2);
        }
        
        // Store result
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    } 
    // Subtraction
    else if (strcmp(q.oper, "-") == 0) {
        // Load first operand
        if (is_numeric(q.op1)) {
            fprintf(asm_file, "    mov ax, %s\n", q.op1);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
        }
        
        // Subtract second operand
        if (is_numeric(q.op2)) {
            fprintf(asm_file, "    sub ax, %s\n", q.op2);
        } else {
            fprintf(asm_file, "    sub ax, [%s]\n", q.op2);
        }
        
        // Store result
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    } 
    // Multiplication
    else if (strcmp(q.oper, "*") == 0) {
        // Load first operand
        if (is_numeric(q.op1)) {
            fprintf(asm_file, "    mov ax, %s\n", q.op1);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
        }
        
        // Load second operand
        if (is_numeric(q.op2)) {
            fprintf(asm_file, "    mov bx, %s\n", q.op2);
        } else {
            fprintf(asm_file, "    mov bx, [%s]\n", q.op2);
        }
        
        // Multiply
        fprintf(asm_file, "    mul bx\n");
        
        // Store result
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    } 
    // Division
    else if (strcmp(q.oper, "/") == 0) {
        // Load dividend
        if (is_numeric(q.op1)) {
            fprintf(asm_file, "    mov ax, %s\n", q.op1);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
        }
        
        // Clear DX for division
        fprintf(asm_file, "    xor dx, dx\n");
        
        // Load divisor
        if (is_numeric(q.op2)) {
            fprintf(asm_file, "    mov bx, %s\n", q.op2);
        } else {
            fprintf(asm_file, "    mov bx, [%s]\n", q.op2);
        }
        
        // Divide (no error checking)
        fprintf(asm_file, "    div bx\n");
        
        // Store result (quotient)
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    }
    // Modulo operation
    else if (strcmp(q.oper, "%%") == 0) {
        // Load dividend
        if (is_numeric(q.op1)) {
            fprintf(asm_file, "    mov ax, %s\n", q.op1);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
        }
        
        // Clear DX for division
        fprintf(asm_file, "    xor dx, dx\n");
        
        // Load divisor
        if (is_numeric(q.op2)) {
            fprintf(asm_file, "    mov bx, %s\n", q.op2);
        } else {
            fprintf(asm_file, "    mov bx, [%s]\n", q.op2);
        }
        
        // Divide (no error checking)
        fprintf(asm_file, "    div bx\n");
        
        // Store result (remainder)
        fprintf(asm_file, "    mov [%s], dx\n", q.res);
    }
}

// Handle comparison operations
void translate_comparison(FILE* asm_file, QUAD q, int qdr_index) {
    // Load first operand
    if (is_numeric(q.op1)) {
        fprintf(asm_file, "    mov ax, %s\n", q.op1);
    } else {
        fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
    }
    
    // Compare with second operand
    if (is_numeric(q.op2)) {
        fprintf(asm_file, "    cmp ax, %s\n", q.op2);
    } else {
        fprintf(asm_file, "    cmp ax, [%s]\n", q.op2);
    }
    
    // Generate comparison code and jump instructions
    char* jump_instruction = NULL;
    
    if (strcmp(q.oper, "<") == 0) {
        jump_instruction = "jl";
    } else if (strcmp(q.oper, ">") == 0) {
        jump_instruction = "jg";
    } else if (strcmp(q.oper, "<=") == 0) {
        jump_instruction = "jle";
    } else if (strcmp(q.oper, ">=") == 0) {
        jump_instruction = "jge";
    } else if (strcmp(q.oper, "==") == 0) {
        jump_instruction = "je";
    } else if (strcmp(q.oper, "!=") == 0) {
        jump_instruction = "jne";
    }
    
    if (jump_instruction) {
        // Set result to true (1) or false (0) based on comparison
        fprintf(asm_file, "    mov ax, 0      ; Assume false\n");
        fprintf(asm_file, "    %s comp_true_%d\n", jump_instruction, qdr_index);
        fprintf(asm_file, "    jmp comp_end_%d\n", qdr_index);
        fprintf(asm_file, "comp_true_%d:\n", qdr_index);
        fprintf(asm_file, "    mov ax, 1      ; Set to true\n");
        fprintf(asm_file, "comp_end_%d:\n", qdr_index);
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    }
}

// Handle logical operations
void translate_logical(FILE* asm_file, QUAD q, int qdr_index) {
    // Load first operand
    if (is_numeric(q.op1)) {
        fprintf(asm_file, "    mov ax, %s\n", q.op1);
    } else {
        fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
    }
    
    // Load second operand
    if (is_numeric(q.op2)) {
        fprintf(asm_file, "    mov bx, %s\n", q.op2);
    } else {
        fprintf(asm_file, "    mov bx, [%s]\n", q.op2);
    }
    
    if (strcmp(q.oper, "&&") == 0) {
        // Logical AND
        fprintf(asm_file, "    test ax, ax     ; Check if first operand is non-zero\n");
        fprintf(asm_file, "    jz logical_false_%d\n", qdr_index);
        fprintf(asm_file, "    test bx, bx     ; Check if second operand is non-zero\n");
        fprintf(asm_file, "    jz logical_false_%d\n", qdr_index);
        
        // Both operands are non-zero, result is true (1)
        fprintf(asm_file, "    mov ax, 1\n");
        fprintf(asm_file, "    jmp logical_end_%d\n", qdr_index);
        
        // At least one operand is zero, result is false (0)
        fprintf(asm_file, "logical_false_%d:\n", qdr_index);
        fprintf(asm_file, "    xor ax, ax\n");
        
        fprintf(asm_file, "logical_end_%d:\n", qdr_index);
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    } 
    else if (strcmp(q.oper, "||") == 0) {
        // Logical OR
        fprintf(asm_file, "    test ax, ax     ; Check if first operand is non-zero\n");
        fprintf(asm_file, "    jnz logical_true_%d\n", qdr_index);
        fprintf(asm_file, "    test bx, bx     ; Check if second operand is non-zero\n");
        fprintf(asm_file, "    jnz logical_true_%d\n", qdr_index);
        
        // Both operands are zero, result is false (0)
        fprintf(asm_file, "    xor ax, ax\n");
        fprintf(asm_file, "    jmp logical_end_%d\n", qdr_index);
        
        // At least one operand is non-zero, result is true (1)
        fprintf(asm_file, "logical_true_%d:\n", qdr_index);
        fprintf(asm_file, "    mov ax, 1\n");
        
        fprintf(asm_file, "logical_end_%d:\n", qdr_index);
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    }
}

// Handle logical NOT operation
void translate_logical_not(FILE* asm_file, QUAD q, int qdr_index) {
    // Load operand
    if (is_numeric(q.op1)) {
        fprintf(asm_file, "    mov ax, %s\n", q.op1);
    } else {
        fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
    }
    
    // Logical NOT: if ax is 0, set to 1; otherwise, set to 0
    fprintf(asm_file, "    test ax, ax\n");
    fprintf(asm_file, "    jz not_zero_%d\n", qdr_index);
    
    // Operand is non-zero, result is false (0)
    fprintf(asm_file, "    xor ax, ax\n");
    fprintf(asm_file, "    jmp not_end_%d\n", qdr_index);
    
    // Operand is zero, result is true (1)
    fprintf(asm_file, "not_zero_%d:\n", qdr_index);
    fprintf(asm_file, "    mov ax, 1\n");
    
    fprintf(asm_file, "not_end_%d:\n", qdr_index);
    fprintf(asm_file, "    mov [%s], ax\n", q.res);
}

// Handle jump operations
void translate_jump(FILE* asm_file, QUAD q, int qdr_index) {
    if (strcmp(q.oper, "BR") == 0) {
        // Unconditional branch
        if (q.op1 && *q.op1) {
            fprintf(asm_file, "    jmp L%s\n", q.op1);
        } else {
            fprintf(asm_file, "    ; Warning: Branch instruction with no target label\n");
            fprintf(asm_file, "    ; Skipping invalid jump\n");
        }
    } 
    else if (strcmp(q.oper, "BZ") == 0) {
        // Branch if zero (conditional)
        if (q.res && *q.res) {
            if (is_numeric(q.op1)) {
                fprintf(asm_file, "    mov ax, %s\n", q.op1);
            } else {
                fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
            }
            fprintf(asm_file, "    test ax, ax\n");
            fprintf(asm_file, "    jz L%s\n", q.res);
        } else {
            fprintf(asm_file, "    ; Warning: Conditional branch with no target label\n");
            fprintf(asm_file, "    ; Skipping invalid jump\n");
        }
    }
    else if (strcmp(q.oper, "BNZ") == 0) {
        // Branch if not zero (conditional)
        if (q.res && *q.res) {
            if (is_numeric(q.op1)) {
                fprintf(asm_file, "    mov ax, %s\n", q.op1);
            } else {
                fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
            }
            fprintf(asm_file, "    test ax, ax\n");
            fprintf(asm_file, "    jnz L%s\n", q.res);
        } else {
            fprintf(asm_file, "    ; Warning: Conditional branch with no target label\n");
            fprintf(asm_file, "    ; Skipping invalid jump\n");
        }
    }
}

// Handle print operations
void translate_print(FILE* asm_file, QUAD q, int qdr_index) {
    // Get type of operand to be printed
    symbol_table_entry* entry = find_symbol_any_block(q.op1);
    
    if (entry && entry->type == TYPE_STRING_CHARACTER) {
        // Print string using DOS function
        fprintf(asm_file, "    mov ah, 09h      ; DOS function: print string\n");
        fprintf(asm_file, "    lea dx, [%s]     ; Load address of string\n", q.op1);
        fprintf(asm_file, "    int 21h          ; Call DOS interrupt\n");
    } 
    else if (q.op1 && q.op1[0] == '"') {
        // Direct string literal to print
        char str_label[50];
        sprintf(str_label, "str_%d", qdr_index);
        
        fprintf(asm_file, "    mov ah, 09h      ; DOS function: print string\n");
        fprintf(asm_file, "    lea dx, [%s]     ; Load address of string\n", str_label);
        fprintf(asm_file, "    int 21h          ; Call DOS interrupt\n");
    }
    else {
        // Print numeric value - call our print_num procedure
        if (is_numeric(q.op1)) {
            fprintf(asm_file, "    mov ax, %s\n", q.op1);
        } else {
            fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
        }
        fprintf(asm_file, "    call print_num\n");
    }
    
    // Add newline after printing (except for string literals which should include their own)
    if (!(entry && entry->type == TYPE_STRING_CHARACTER) && !(q.op1 && q.op1[0] == '"')) {
        fprintf(asm_file, "    ; Print newline\n");
        fprintf(asm_file, "    mov ah, 02h      ; DOS function: print character\n");
        fprintf(asm_file, "    mov dl, 0Dh      ; Carriage return\n");
        fprintf(asm_file, "    int 21h\n");
        fprintf(asm_file, "    mov dl, 0Ah      ; Line feed\n");
        fprintf(asm_file, "    int 21h\n");
    }
}

// Function to create default label entries for all jump targets
void ensure_all_labels_exist(FILE* asm_file, int* label_used) {
    // Find highest referenced label
    int max_label = 0;
    
    // First pass: check all quadruples
    for (int i = 0; i < optimizedQc; i++) {
        QUAD q = optimizedQuad[i];
        
        // Check jumps - BR, BZ, BNZ
        if (strcmp(q.oper, "BR") == 0 && q.op1) {
            int label = atoi(q.op1);
            if (label > max_label) max_label = label;
            label_used[label] = 1;
        } 
        else if ((strcmp(q.oper, "BZ") == 0 || strcmp(q.oper, "BNZ") == 0) && q.res) {
            int label = atoi(q.res);
            if (label > max_label) max_label = label;
            label_used[label] = 1;
        }
    }
    
    // Mark all referenced labels
    for (int i = 0; i <= max_label; i++) {
        if (label_used[i] && i >= optimizedQc) {
            // This is a label that's referenced but beyond the quadruple count
            // Create a placeholder label at the end of the code
            fprintf(asm_file, "; Adding missing label L%d\n", i);
            fprintf(asm_file, "L%d:\n", i);
        }
    }
}

// Main function to generate assembly code from quadruples
void generate_assembly_code(char* output_file) {
    FILE* asm_file = fopen(output_file, "w");
    if (!asm_file) {
        fprintf(stderr, "Error: Could not open output file %s\n", output_file);
        return;
    }
    
    // Identify all temporary variables used in quadruples
    identify_temp_vars();
    
    // Identify labels used in jumps
    int* label_used = identify_labels();
    
    // Generate appropriate MASM directives for emu8086
    fprintf(asm_file, "; 8086 Assembly code generated for emu8086\n");
    fprintf(asm_file, ".model small\n");
    fprintf(asm_file, ".stack 100h\n\n");
    
    // Generate data segment - using emu8086 compatible syntax
    fprintf(asm_file, ".data\n");
    generate_data_segment(asm_file);
    
    // Generate code segment - emu8086 compatible syntax
    fprintf(asm_file, ".code\n");
    fprintf(asm_file, "main PROC\n");
    
    // Set up data segment
    fprintf(asm_file, "    ; Set data segment register\n");
    fprintf(asm_file, "    mov ax, @data\n");
    fprintf(asm_file, "    mov ds, ax\n\n");
    fprintf(asm_file, "    ; Program starts here\n\n");
    
    // Reset register tracking
    initialize_registers();
    
    // Process each quadruple
    for (int i = 0; i < optimizedQc; i++) {
        QUAD q = optimizedQuad[i];
        
        // Check if this line needs a label
        if (label_used[i]) {
            fprintf(asm_file, "L%d:\n", i);
        }
        
        // Emit comment showing the original quadruple
        fprintf(asm_file, "    ; Quad %d: (%s, %s, %s, %s)\n", 
                i, q.oper, q.op1 ? q.op1 : "", q.op2 ? q.op2 : "", q.res ? q.res : "");
        
        // Process based on operation type
        if (strcmp(q.oper, "=") == 0) {
            translate_assignment(asm_file, q, i);
        }
        else if (strcmp(q.oper, "+") == 0 || strcmp(q.oper, "-") == 0 || 
                 strcmp(q.oper, "*") == 0 || strcmp(q.oper, "/") == 0 ||
                 strcmp(q.oper, "%%") == 0) {
            translate_arithmetic(asm_file, q, i);
        }
        else if (strcmp(q.oper, "<") == 0 || strcmp(q.oper, ">") == 0 ||
                 strcmp(q.oper, "<=") == 0 || strcmp(q.oper, ">=") == 0 ||
                 strcmp(q.oper, "==") == 0 || strcmp(q.oper, "!=") == 0) {
            translate_comparison(asm_file, q, i);
        }
        else if (strcmp(q.oper, "&&") == 0 || strcmp(q.oper, "||") == 0) {
            translate_logical(asm_file, q, i);
        }
        else if (strcmp(q.oper, "!") == 0) {
            translate_logical_not(asm_file, q, i);
        }
        else if (strcmp(q.oper, "BR") == 0 || strcmp(q.oper, "BZ") == 0 || 
                 strcmp(q.oper, "BNZ") == 0) {
            translate_jump(asm_file, q, i);
        }
        else if (strcmp(q.oper, "PRINT") == 0) {
            translate_print(asm_file, q, i);
        }
        else if (strcmp(q.oper, "READ") == 0) {
            translate_read(asm_file, q, i);
        }
        else if (strcmp(q.oper, "ARR_LOAD") == 0) {
            translate_array_load(asm_file, q, i);
        }
        else if (strcmp(q.oper, "ARR_STORE") == 0) {
            translate_array_store(asm_file, q, i);
        }
        else {
            fprintf(asm_file, "    ; Unsupported quadruple operation: %s\n", q.oper);
            // Actually implement the operations that are currently unsupported
            if (is_numeric(q.op1) && q.res) {
                fprintf(asm_file, "    mov ax, %s\n", q.op1);
                fprintf(asm_file, "    mov [%s], ax\n", q.res);
            }
        }
        
        fprintf(asm_file, "\n");
    }
    
    // After processing quads, ensure all labels referenced in jumps actually exist
    ensure_all_labels_exist(asm_file, label_used);
    
    // Add program exit code
    fprintf(asm_file, "    ; Exit program\n");
    fprintf(asm_file, "    mov ah, 4Ch      ; DOS function: terminate program\n");
    fprintf(asm_file, "    mov al, 0        ; Return code = 0\n");
    fprintf(asm_file, "    int 21h          ; Call DOS interrupt\n\n");
    
    fprintf(asm_file, "main ENDP\n\n");
    
    // Generate standard procedures (numeric I/O, etc.)
    generate_standard_procedures(asm_file);
    
    fprintf(asm_file, "END main\n");
    
    // Clean up and close the file
    free(label_used);
    fclose(asm_file);
    
    printf("Assembly code generated successfully: %s\n", output_file);
}

// Handle read operations
void translate_read(FILE* asm_file, QUAD q, int qdr_index) {
    symbol_table_entry* entry = find_symbol_any_block(q.res);
    
    if (entry && entry->type == TYPE_CHARACTER) {
        // Read a character using DOS function
        fprintf(asm_file, "    mov ah, 01h      ; DOS function: read character\n");
        fprintf(asm_file, "    int 21h          ; Call DOS interrupt\n");
        fprintf(asm_file, "    mov [%s], al     ; Store character\n", q.res);
    } 
    else if (entry && (entry->type == TYPE_STRING_CHARACTER)) {
        // Read a string using DOS buffered input
        fprintf(asm_file, "    mov ah, 0Ah      ; DOS function: buffered input\n");
        fprintf(asm_file, "    lea dx, [%s_buf] ; Load address of buffer\n", q.res);
        fprintf(asm_file, "    int 21h          ; Call DOS interrupt\n");
        
        // Copy from buffer to string (skipping length bytes)
        fprintf(asm_file, "    mov si, %s_buf + 2 ; Source: buffer data\n", q.res);
        fprintf(asm_file, "    mov di, %s         ; Destination: string variable\n", q.res);
        fprintf(asm_file, "    mov cl, [%s_buf+1] ; Get length\n", q.res);
        fprintf(asm_file, "    xor ch, ch        ; Clear high byte\n");
        fprintf(asm_file, "    rep movsb         ; Copy bytes\n");
        fprintf(asm_file, "    mov byte [di], '$'; Add string terminator\n");
    }
    else {
        // Read numeric value - call our read_num procedure
        fprintf(asm_file, "    call read_num\n");
        fprintf(asm_file, "    mov [%s], ax\n", q.res);
    }
}

// Handle array store operations
void translate_array_store(FILE* asm_file, QUAD q, int qdr_index) {
    symbol_table_entry* array_entry = find_symbol_any_block(q.res);
    int element_size = get_data_type_size(get_element_type(array_entry->type));
    
    // Load index value
    if (is_numeric(q.op2)) {
        fprintf(asm_file, "    mov bx, %s\n", q.op2);
    } else {
        fprintf(asm_file, "    mov bx, [%s]\n", q.op2);
    }
    
    // Multiply by element size to get offset (in bytes)
    if (element_size > 1) {
        if (element_size == 2) {
            fprintf(asm_file, "    shl bx, 1     ; Multiply by 2\n");
        } else if (element_size == 4) {
            fprintf(asm_file, "    shl bx, 2     ; Multiply by 4\n");
        } else {
            fprintf(asm_file, "    mov ax, %d\n", element_size);
            fprintf(asm_file, "    mul bx\n");
            fprintf(asm_file, "    mov bx, ax\n");
        }
    }
    
    // Load value to store
    if (is_numeric(q.op1)) {
        fprintf(asm_file, "    mov ax, %s\n", q.op1);
    } else {
        fprintf(asm_file, "    mov ax, [%s]\n", q.op1);
    }
    
    // Store value in array (no bounds checking)
    fprintf(asm_file, "    mov si, %s\n", q.res);
    fprintf(asm_file, "    add si, bx\n");
    fprintf(asm_file, "    mov [si], ax\n");
}

// Generate standard library procedures for 8086 with NASM syntax
void generate_standard_procedures(FILE* asm_file) {
    // Print number procedure
    fprintf(asm_file, "; Procedure to print a number in AX\n");
    fprintf(asm_file, "print_num PROC\n");
    fprintf(asm_file, "    push ax\n");
    fprintf(asm_file, "    push bx\n");
    fprintf(asm_file, "    push cx\n");
    fprintf(asm_file, "    push dx\n");
    
    fprintf(asm_file, "    ; Check if number is negative\n");
    fprintf(asm_file, "    test ax, ax\n");
    fprintf(asm_file, "    jns positive_num\n");
    fprintf(asm_file, "    push ax\n");
    fprintf(asm_file, "    mov ah, 02h      ; DOS function: print character\n");
    fprintf(asm_file, "    mov dl, '-'      ; Print minus sign\n");
    fprintf(asm_file, "    int 21h\n");
    fprintf(asm_file, "    pop ax\n");
    fprintf(asm_file, "    neg ax           ; Make number positive\n");
    fprintf(asm_file, "positive_num:\n");
    
    fprintf(asm_file, "    ; Initialize for conversion\n");
    fprintf(asm_file, "    mov bx, 10       ; Divisor\n");
    fprintf(asm_file, "    mov cx, 0        ; Digit counter\n");
    
    fprintf(asm_file, "    ; Special case for zero\n");
    fprintf(asm_file, "    cmp ax, 0\n");
    fprintf(asm_file, "    jne not_zero\n");
    fprintf(asm_file, "    mov ah, 02h\n");
    fprintf(asm_file, "    mov dl, '0'\n");
    fprintf(asm_file, "    int 21h\n");
    fprintf(asm_file, "    jmp end_print\n");
    fprintf(asm_file, "not_zero:\n");
    
    fprintf(asm_file, "    ; Convert number to digits on stack\n");
    fprintf(asm_file, "convert_loop:\n");
    fprintf(asm_file, "    xor dx, dx       ; Clear DX for division\n");
    fprintf(asm_file, "    div bx           ; AX / 10, remainder in DX\n");
    fprintf(asm_file, "    push dx          ; Save remainder (digit)\n");
    fprintf(asm_file, "    inc cx           ; Increment digit count\n");
    fprintf(asm_file, "    test ax, ax      ; Check if quotient is zero\n");
    fprintf(asm_file, "    jnz convert_loop\n");
    
    fprintf(asm_file, "    ; Print digits from stack\n");
    fprintf(asm_file, "print_loop:\n");
    fprintf(asm_file, "    pop dx           ; Get digit\n");
    fprintf(asm_file, "    add dl, '0'      ; Convert to ASCII\n");
    fprintf(asm_file, "    mov ah, 02h      ; DOS function: print character\n");
    fprintf(asm_file, "    int 21h\n");
    fprintf(asm_file, "    loop print_loop  ; Decrement CX and loop if not zero\n");
    
    fprintf(asm_file, "end_print:\n");
    fprintf(asm_file, "    pop dx\n");
    fprintf(asm_file, "    pop cx\n");
    fprintf(asm_file, "    pop bx\n");
    fprintf(asm_file, "    pop ax\n");
    fprintf(asm_file, "    ret\n");
    fprintf(asm_file, "print_num ENDP\n\n");
    
    // Read number procedure
    fprintf(asm_file, "; Procedure to read a number into AX\n");
    fprintf(asm_file, "read_num PROC\n");
    fprintf(asm_file, "    push bx\n");
    fprintf(asm_file, "    push cx\n");
    fprintf(asm_file, "    push dx\n");
    
    fprintf(asm_file, "    ; Initialize\n");
    fprintf(asm_file, "    mov ax, 0        ; Result\n");
    fprintf(asm_file, "    mov cx, 10       ; Multiplier\n");
    fprintf(asm_file, "    mov bx, 0        ; Sign flag (0 = positive)\n");
    fprintf(asm_file, "    mov dx, 0        ; Initialize dx to 0 for accumulation\n");
    
    fprintf(asm_file, "    ; Read first character\n");
    fprintf(asm_file, "    mov ah, 01h      ; DOS function: read character\n");
    fprintf(asm_file, "    int 21h\n");
    
    fprintf(asm_file, "    ; Check for sign\n");
    fprintf(asm_file, "    cmp al, '-'\n");
    fprintf(asm_file, "    jne check_digit\n");
    fprintf(asm_file, "    mov bx, 1        ; Set sign flag\n");
    fprintf(asm_file, "    mov ah, 01h      ; Read next character\n");
    fprintf(asm_file, "    int 21h\n");
    
    fprintf(asm_file, "check_digit:\n");
    fprintf(asm_file, "    ; Check if character is valid digit\n");
    fprintf(asm_file, "    cmp al, '0'\n");
    fprintf(asm_file, "    jb apply_sign\n");
    fprintf(asm_file, "    cmp al, '9'\n");
    fprintf(asm_file, "    ja apply_sign\n");
    
    fprintf(asm_file, "read_loop:\n");
    fprintf(asm_file, "    ; Convert digit and add to result\n");
    fprintf(asm_file, "    sub al, '0'      ; Convert ASCII to digit\n");
    fprintf(asm_file, "    xor ah, ah       ; Clear high byte\n");
    fprintf(asm_file, "    push ax          ; Save digit\n");
    fprintf(asm_file, "    mov ax, dx       ; Load accumulated result\n");
    fprintf(asm_file, "    mul cx           ; AX = AX * 10\n");
    fprintf(asm_file, "    mov dx, ax       ; DX = result of multiplication\n");
    fprintf(asm_file, "    pop ax           ; Restore digit\n");
    fprintf(asm_file, "    add dx, ax       ; Add digit to result\n");
    
    fprintf(asm_file, "    ; Read next character\n");
    fprintf(asm_file, "    mov ah, 01h\n");
    fprintf(asm_file, "    int 21h\n");
    
    fprintf(asm_file, "    ; Check if character is valid digit\n");
    fprintf(asm_file, "    cmp al, '0'\n");
    fprintf(asm_file, "    jb end_read\n");
    fprintf(asm_file, "    cmp al, '9'\n");
    fprintf(asm_file, "    jbe read_loop\n");
    
    fprintf(asm_file, "apply_sign:\n");
    fprintf(asm_file, "    ; Apply sign if negative\n");
    fprintf(asm_file, "    test bx, bx\n");
    fprintf(asm_file, "    jz end_read\n");
    fprintf(asm_file, "    neg dx\n");
    
    fprintf(asm_file, "end_read:\n");
    fprintf(asm_file, "    mov ax, dx       ; Move result to AX\n");
    fprintf(asm_file, "    pop dx\n");
    fprintf(asm_file, "    pop cx\n");
    fprintf(asm_file, "    pop bx\n");
    fprintf(asm_file, "    ret\n");
    fprintf(asm_file, "read_num ENDP\n");
}