// optimize_tac_code.c
#include "intermediate_code.h"

void optimize_tac_code() {
    tac_quad* current = tac_code_head;
    tac_quad* prev = NULL;
    
    while (current != NULL) {
        // Constant folding for arithmetic operations
        if (current->op >= OP_ADD && current->op <= OP_MOD && 
            current->arg1.type == OPERAND_LITERAL && 
            current->arg2.type == OPERAND_LITERAL) {
            int result;
            switch (current->op) {
                case OP_ADD: result = current->arg1.value.int_val + current->arg2.value.int_val; break;
                case OP_SUB: result = current->arg1.value.int_val - current->arg2.value.int_val; break;
                case OP_MUL: result = current->arg1.value.int_val * current->arg2.value.int_val; break;
                case OP_DIV: 
                    if (current->arg2.value.int_val != 0) 
                        result = current->arg1.value.int_val / current->arg2.value.int_val; 
                    else continue; 
                    break;
                case OP_MOD: 
                    if (current->arg2.value.int_val != 0) 
                        result = current->arg1.value.int_val % current->arg2.value.int_val; 
                    else continue; 
                    break;
                default: continue;
            }
            current->op = OP_ASSIGN;
            current->arg1 = create_int_literal(result);
            current->arg2 = create_none();
        }
        
        // Remove redundant assignments (t1 = t2; t2 = t1)
        if (prev != NULL && prev->op == OP_ASSIGN && current->op == OP_ASSIGN &&
            prev->result.type == OPERAND_TEMPORARY && current->result.type == OPERAND_TEMPORARY &&
            strcmp(prev->result.value.string_val, current->arg1.value.string_val) == 0 &&
            strcmp(current->result.value.string_val, prev->arg1.value.string_val) == 0) {
            prev->next = current->next;
            if (current == tac_code_tail) tac_code_tail = prev;
            free(current->result.value.string_val);
            free(current->arg1.value.string_val);
            free(current);
            current = prev->next;
            continue;
        }
        
        prev = current;
        current = current->next;
    }
}