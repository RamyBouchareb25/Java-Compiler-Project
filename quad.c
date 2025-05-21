/* quad.c - Quadruple generation and handling */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quad.h"

/* Global variables */
QUAD tableQuad[MAX_QUAD];
int qc = 0;  // Quadruple counter
int tempCount = 0;  // Counter for temporary variables

/* Create a new quadruple */
void quadr(char *op, char *arg1, char *arg2, char *res) {
    if (qc < MAX_QUAD) {
        strcpy(tableQuad[qc].oper, op);
        strcpy(tableQuad[qc].op1, arg1);
        strcpy(tableQuad[qc].op2, arg2);
        strcpy(tableQuad[qc].res, res);
        qc++;
    } else {
        printf("Error: Quadruple table overflow\n");
        exit(1);
    }
}

/* Update an existing quadruple */
void ajour_quad(int pos, int col, char *val) {
    if (pos < qc) {
        switch (col) {
            case 1: strcpy(tableQuad[pos].oper, val); break;
            case 2: strcpy(tableQuad[pos].op1, val); break;
            case 3: strcpy(tableQuad[pos].op2, val); break;
            case 4: strcpy(tableQuad[pos].res, val); break;
            default: printf("Error: Invalid column number for quadruple update\n");
        }
    } else {
        printf("Error: Invalid quadruple position for update\n");
    }
}

/* Generate a new temporary variable name */
char* tempvar() {
    char* temp = malloc(10);
    sprintf(temp, "t%d", tempCount++);
    return temp;
}

/* Display all quadruples */
void afficher_qdr() {
    int i;
    for (i = 0; i < qc; i++) {
        printf(" %d - ( %s  ,  %s  ,  %s  ,  %s )\n", i, 
               tableQuad[i].oper, tableQuad[i].op1, tableQuad[i].op2, tableQuad[i].res);
        printf("--------------------------------------------------------\n\n");
    }
}

/* Create a copy of the original quadruple table for optimization */
QUAD optimizedQuad[MAX_QUAD];
int optimizedQc = 0;

/* Copy original quadruples to optimization table */
void copy_quads_for_optimization() {
    optimizedQc = qc;
    memcpy(optimizedQuad, tableQuad, sizeof(QUAD) * qc);
}

/* Display optimized quadruples */
void afficher_qdr_optimized() {
    int i;
    int newIndex = 0;  // Counter for new indices starting from 0
    
    // printf("*********************Les Quadruplets Optimisés***********************\n\n");
    
    for (i = 0; i < optimizedQc; i++) {
        if (strlen(optimizedQuad[i].oper) > 0) { // Skip empty/removed quadruples
            printf(" %d - ( %s  ,  %s  ,  %s  ,  %s )\n", newIndex, 
                   optimizedQuad[i].oper, optimizedQuad[i].op1, optimizedQuad[i].op2, optimizedQuad[i].res);
            printf("--------------------------------------------------------\n\n");
            newIndex++;  // Increment the new index only for non-empty quadruples
        }
    }
}