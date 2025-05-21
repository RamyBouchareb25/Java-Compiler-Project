/* optimize.h - Header file for intermediate code optimization */
#ifndef OPTIMIZE_H
#define OPTIMIZE_H

/* Function prototypes for optimization techniques */
int is_constant(char *str);
int is_arithmetic_op(char *op);

void constant_folding();
void constant_propagation();
void copy_propagation();
void dead_code_elimination();
void common_subexpression_elimination();

/* Main optimization function */
void optimize_intermediate_code();

#endif 