// assembly_generator.c
#include <stdio.h>
#include "intermediate_code.h"

void generate_assembly_code(const char *output_file)
{
    FILE *fp = fopen(output_file, "w");
    if (!fp)
    {
        fprintf(stderr, "Error: Could not open output file %s\n", output_file);
        return;
    }

    fprintf(fp, "; 8086 Assembly Code\n");
    fprintf(fp, ".model small\n");
    fprintf(fp, ".stack 100h\n");
    fprintf(fp, ".data\n");

    // Declare variables (simplified)
    tac_quad *current = tac_code_head;
    while (current != NULL)
    {
        if (current->result.type == OPERAND_VARIABLE)
        {
            fprintf(fp, "%s DW ?\n", current->result.value.string_val);
        }
        current = current->next;
    }

    fprintf(fp, ".code\n");
    fprintf(fp, "main PROC\n");
    fprintf(fp, "    mov ax, @data\n");
    fprintf(fp, "    mov ds, ax\n");

    current = tac_code_head;
    while (current != NULL)
    {
        switch (current->op)
        {
        case OP_ASSIGN:
            fprintf(fp, "    mov ax, ");
            print_tac_operand_to_file(current->arg1, fp);
            fprintf(fp, "\n    mov ");
            print_tac_operand_to_file(current->result, fp);
            fprintf(fp, ", ax\n");
            break;
        case OP_ADD:
            fprintf(fp, "    mov ax, ");
            print_tac_operand_to_file(current->arg1, fp);
            fprintf(fp, "\n    add ax, ");
            print_tac_operand_to_file(current->arg2, fp);
            fprintf(fp, "\n    mov ");
            print_tac_operand_to_file(current->result, fp);
            fprintf(fp, ", ax\n");
            break;
        case OP_IF:
            fprintf(fp, "    mov ax, ");
            print_tac_operand_to_file(current->arg1, fp);
            fprintf(fp, "\n    cmp ax, 0\n");
            fprintf(fp, "    je %s\n", current->arg2.value.string_val);
            break;
        case OP_GOTO:
            fprintf(fp, "    jmp %s\n", current->arg1.value.string_val);
            break;
        case OP_LABEL:
            fprintf(fp, "%s:\n", current->result.value.string_val);
            break;
            // Add more cases for other operations
        }
        current = current->next;
    }

    fprintf(fp, "    mov ax, 4C00h\n");
    fprintf(fp, "    int 21h\n");
    fprintf(fp, "main ENDP\n");
    fprintf(fp, "END main\n");

    fclose(fp);
}

// Helper function to print operand to file
void print_tac_operand_to_file(tac_operand op, FILE *fp)
{
    switch (op.type)
    {
    case OPERAND_LITERAL:
        fprintf(fp, "%d", op.value.int_val);
        break;
    case OPERAND_VARIABLE:
    case OPERAND_TEMPORARY:
        fprintf(fp, "%s", op.value.string_val);
        break;
    default:
        fprintf(fp, "0");
        break;
    }
}