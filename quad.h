/* quad.h - Header file for quadruple generation */
#ifndef QUAD_H
#define QUAD_H

#define MAX_QUAD 500
#define MAX_LENGTH 50

/* Quadruple structure */
typedef struct {
    char oper[MAX_LENGTH];  // Operation
    char op1[MAX_LENGTH];   // First operand
    char op2[MAX_LENGTH];   // Second operand
    char res[MAX_LENGTH];   // Result
} QUAD;

/* Global variables */
extern QUAD tableQuad[MAX_QUAD];  // Array of quadruples
extern QUAD optimizedQuad[MAX_QUAD];  // Array of optimized quadruples
extern int qc;      // Quadruple counter
extern int optimizedQc;  // Optimized quadruple counter
extern int tempCount;  // Counter for temporary variables

/* Function prototypes */
void quadr(char *op, char *arg1, char *arg2, char *res);
void ajour_quad(int pos, int col, char *val);
char* tempvar();
void afficher_qdr();
void copy_quads_for_optimization();
void afficher_qdr_optimized();

#endif /* QUAD_H */