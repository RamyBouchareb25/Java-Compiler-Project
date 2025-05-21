%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "quad.h"  // Include the quad.h header
#include "quad.c"  // Include the quad.h header
#include "colors.h" // Ajout de l'inclusion pour les couleurs
#include "parser_helper.h" // Ajout de l'inclusion pour les fonctions d'aide
#include "parser_helper.c" // Ajout de l'inclusion pour les fonctions d'aide
#include "code_generator.h"

void enhanced_syntax_error(const char* msg, const char* filename);
extern int line_num;
extern int column_num;
extern char* yytext;
extern FILE* yyin;
char current_filename[256] = {0}; // Buffer pour stocker le nom du fichier courant

void yyerror(const char* s);
int yylex();



extern symbol_table_entry* symbol_table;
extern int current_scope;

// Function prototypes for symbol table operations
void add_symbol(char* name, char* type, int line_num, int is_method);
symbol_table_entry* lookup_symbol(char* name);
void enter_scope();
void exit_scope();

// Stack for handling loops and conditionals
#define MAX_STACK 100
int stack[MAX_STACK];
int stack_ptr = -1;

void push(int value) {
    if (stack_ptr < MAX_STACK - 1) {
        stack[++stack_ptr] = value;
    } else {
        printf("Stack overflow\n");
        exit(1);
    }
}

int pop() {
    if (stack_ptr >= 0) {
        return stack[stack_ptr--];
    } else {
        printf("Stack underflow\n");
        exit(1);
    }
}

int top() {
    if (stack_ptr >= 0) {
        return stack[stack_ptr];
    } else {
        printf("Empty stack\n");
        exit(1);
    }
}

// Helper functions for code generation
char* int_to_string(int value) {
    char* result = malloc(20);  // Enough for any integer
    sprintf(result, "%d", value);
    return result;
}

char* get_expr_result(char* expr) {
    // If expr is a temporary variable, return it
    if (expr[0] == 't') {
        return expr;
    }
    // Otherwise create a new temporary and generate a copy assignment
    char* temp = tempvar();
    quadr("=", expr, "", temp);
    return temp;
}
%}

%union {
    int int_val;
    float float_val;
    char char_val;
    char* string_val;
    char* id;
    struct {
        char* code;    // Stores a temporary variable or a constant
        char* type;    // Type information
        int true_list; // For boolean expressions - list of quads to patch for true condition
        int false_list; // For boolean expressions - list of quads to patch for false condition
        int next_list; // For control flow - list of quads to patch for next statement
    } expr;
}

/* Token definitions */
%token CLASS PUBLIC PRIVATE PROTECTED STATIC VOID MAIN
%token INT FLOAT DOUBLE CHAR BOOLEAN
%token IF ELSE SWITCH CASE DEFAULT FOR WHILE DO BREAK CONTINUE RETURN
%token TRY CATCH FINALLY THROW
%token TRUE_VAL FALSE_VAL NULL_VAL
%token PRINT
%token THIS

%token PLUS MINUS MULTIPLY DIVIDE MODULO
%token INCREMENT DECREMENT
%token ASSIGN PLUS_ASSIGN MINUS_ASSIGN MULTIPLY_ASSIGN DIVIDE_ASSIGN MODULO_ASSIGN
%token EQUAL NOT_EQUAL GREATER LESS GREATER_EQUAL LESS_EQUAL
%token AND OR NOT

%token LEFT_PAREN RIGHT_PAREN LEFT_BRACE RIGHT_BRACE LEFT_BRACKET RIGHT_BRACKET
%token SEMICOLON COMMA DOT COLON

%token <int_val> INTEGER_LITERAL
%token <float_val> FLOAT_LITERAL
%token <char_val> CHAR_LITERAL
%token <string_val> STRING_LITERAL
%token <id> IDENTIFIER

%type <id> type
%type <expr> expression assignment_expression conditional_expression
%type <int_val> assignment_operator
%type <expr> logical_or_expression logical_and_expression equality_expression
%type <expr> relational_expression additive_expression multiplicative_expression
%type <expr> unary_expression postfix_expression primary_expression
%type <expr> expression_opt literal
%type <int_val> marker  /* Marker for handling control flow */

/* Precedence rules */
%left OR
%left AND
%left EQUAL NOT_EQUAL
%left GREATER LESS GREATER_EQUAL LESS_EQUAL
%left PLUS MINUS
%left MULTIPLY DIVIDE MODULO
%right NOT INCREMENT DECREMENT
%left DOT LEFT_BRACKET

%%

/* Grammar Rules */

program
    : class_declaration
    | program class_declaration
    ;

class_declaration
    : CLASS IDENTIFIER LEFT_BRACE class_body RIGHT_BRACE
        { printf("Class declared: %s\n", $2); }
    ;

class_body
    : /* empty */
    | class_member_declaration
    | class_body class_member_declaration
    ;

class_member_declaration
    : field_declaration
    | method_declaration
    ;

field_declaration
    : type IDENTIFIER SEMICOLON
        { 
            printf("Field declared: %s of type %s\n", $2, $1);
            add_symbol($2, $1, line_num, 0);
        }
    | type IDENTIFIER ASSIGN expression SEMICOLON
        { 
            printf("Field declared with initialization: %s of type %s\n", $2, $1);
            add_symbol($2, $1, line_num, 0);
            // Generate quadruple for field initialization
            quadr("=", $4.code, "", $2);
        }
    ;

method_declaration
    : type IDENTIFIER LEFT_PAREN
            { enter_scope(); } /* Enter scope before parameters */
        parameter_list_opt RIGHT_PAREN LEFT_BRACE
            { 
                // Generate method entry quadruple
                quadr("PROC", $2, "", "");
            }
        statement_list 
        RIGHT_BRACE
            { 
                printf("Method declared: %s returning %s\n", $2, $1);
                // Generate method exit quadruple
                quadr("ENDPROC", $2, "", "");
                exit_scope();
            }
    | VOID IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
            { 
                enter_scope();
                // Generate method entry quadruple
                quadr("PROC", $2, "", "");
            }
      statement_list 
      RIGHT_BRACE
            { 
                printf("Void method declared: %s\n", $2);
                // Generate method exit quadruple
                quadr("ENDPROC", $2, "", "");
                exit_scope();
            }
    | IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
            { 
                enter_scope();
                // Generate constructor entry quadruple
                quadr("CONSTRUCTOR", $1, "", "");
                printf("Constructor declared for class: %s\n", $1);
            }
      statement_list 
      RIGHT_BRACE
            { 
                // Generate constructor exit quadruple
                quadr("ENDCONSTRUCTOR", $1, "", "");
                exit_scope();
            }
    | PUBLIC STATIC VOID MAIN LEFT_PAREN IDENTIFIER LEFT_BRACKET RIGHT_BRACKET IDENTIFIER RIGHT_PAREN LEFT_BRACE
            { 
                enter_scope();
                // Generate main method entry quadruple
                quadr("MAIN", "", "", "");
            }
      statement_list 
      RIGHT_BRACE
            { 
                printf("Main method declared\n");
                // Generate main method exit quadruple
                quadr("ENDMAIN", "", "", "");
                exit_scope();
            }
    ;

parameter_list_opt
    : /* empty */
    | parameter_list
    ;

parameter_list
    : parameter
    | parameter_list COMMA parameter
    ;

parameter
    : type IDENTIFIER
        { 
            printf("Parameter: %s of type %s\n", $2, $1);
            add_symbol($2, $1, line_num, 0);
            // Generate parameter quadruple
            quadr("PARAM", $1, "", $2);
        }
    | type LEFT_BRACKET RIGHT_BRACKET IDENTIFIER
        {
            char array_type[64];
            sprintf(array_type, "%s[]", $1);
            printf("Parameter: %s of type %s\n", $4, array_type);
            add_symbol($4, array_type, line_num, 0);
            // Generate array parameter quadruple
            quadr("PARAM", array_type, "", $4);
        }
    ;

type
    : INT { $$ = strdup("int"); }
    | FLOAT { $$ = strdup("float"); }
    | DOUBLE { $$ = strdup("double"); }
    | CHAR { $$ = strdup("char"); }
    | BOOLEAN { $$ = strdup("boolean"); }
    | IDENTIFIER { $$ = $1; } // For class types
    | type LEFT_BRACKET RIGHT_BRACKET {
        char* array_type = malloc(strlen($1) + 3);
        sprintf(array_type, "%s[]", $1);
        $$ = array_type;
    }
    ;

statement_list
    : /* empty */
    | statement_list statement
    ;

statement
    : expression_statement
    | declaration_statement
    | if_statement
    | for_statement
    | while_statement
    | do_while_statement
    | switch_statement
    | return_statement
    | try_catch_statement
    | block
    ;

block
    : LEFT_BRACE 
        { enter_scope(); }
      statement_list 
      RIGHT_BRACE
        { exit_scope(); }
    ;

expression_statement
    : expression SEMICOLON
    | SEMICOLON
    ;

declaration_statement
    : type IDENTIFIER SEMICOLON
        { 
            printf("Variable declared: %s of type %s\n", $2, $1);
            add_symbol($2, $1, line_num, 0);
            // Generate variable declaration quadruple
            quadr("DECL", $1, "", $2);
        }
    | type IDENTIFIER ASSIGN expression SEMICOLON
        { 
            printf("Variable declared with initialization: %s of type %s\n", $2, $1);
            add_symbol($2, $1, line_num, 0);
            // Generate variable declaration and initialization quadruples
            quadr("DECL", $1, "", $2);
            quadr("=", $4.code, "", $2);
        }
    ;

/* Control flow markers */
marker
    : /* empty */ { $$ = qc; } /* Current quadruple index */
    ;

if_statement
    : IF LEFT_PAREN expression RIGHT_PAREN 
        {
            // Generate conditional jump
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("JZ", $3.code, "", temp_label);  // Jump if expression is zero (false)
            push(qc-1);  // Push position to backpatch
        }
      statement
        {
            // Jump past the else part
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("GOTO", "", "", temp_label);
            
            // Backpatch the conditional jump
            int false_jump = pop();
            sprintf(temp_label, "L%d", qc);
            ajour_quad(false_jump, 4, temp_label);
            
            // Push current position for end of if-else
            push(qc-1);
        }
    | IF LEFT_PAREN expression RIGHT_PAREN
        {
            // Generate conditional jump
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("JZ", $3.code, "", temp_label);  // Jump if expression is zero (false)
            push(qc-1);  // Push position to backpatch
        }
      statement ELSE
        {
            // Jump past the else part
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("GOTO", "", "", temp_label);
            
            // Backpatch the conditional jump
            int false_jump = pop();
            sprintf(temp_label, "L%d", qc);
            ajour_quad(false_jump, 4, temp_label);
            
            // Push current position for end of if-else
            push(qc-1);
        }
      statement
        {
            // Backpatch the jump at the end of the 'if' part
            int end_if_jump = pop();
            char temp_label[20];
            sprintf(temp_label, "L%d", qc);
            ajour_quad(end_if_jump, 4, temp_label);
        }
    ;

for_statement
    : FOR LEFT_PAREN expression_opt SEMICOLON
        {
            quadr("LABEL", "", "", int_to_string(qc));  // Label for condition
            push(qc-1);  // Store start of condition
        }
      expression_opt SEMICOLON
        {
            // Generate conditional jump
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            if ($6.code && strlen($6.code) > 0) {
                quadr("JZ", $6.code, "", temp_label);  // Jump if condition is false
            } else {
                quadr("JZ", "1", "", temp_label);  // Always true if no condition
            }
            push(qc-1);  // Store position of conditional jump
            
            // Jump to loop body
            sprintf(temp_label, "L%d", qc+2);
            quadr("GOTO", "", "", temp_label);
            
            // Label for increment
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
            push(qc-1);  // Store position of increment label
        }
      expression_opt RIGHT_PAREN
        {
            // After increment, jump back to condition
            int cond_start = stack[stack_ptr-2];
            char temp_label[20];
            sprintf(temp_label, "L%d", cond_start+1);
            quadr("GOTO", "", "", temp_label);
            
            // Label for loop body
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
        }
      statement
        {
            // Jump to increment
            int incr_pos = pop();
            char temp_label[20];
            sprintf(temp_label, "L%d", incr_pos+1);
            quadr("GOTO", "", "", temp_label);
            
            // Label for loop exit
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
            
            // Backpatch conditional jump
            int cond_jump = pop();
            sprintf(temp_label, "L%d", qc-1);
            ajour_quad(cond_jump, 4, temp_label);
            
            // Remove condition start position
            pop();
        }
        | FOR LEFT_PAREN type IDENTIFIER COLON expression RIGHT_PAREN
        {
            // Initialiser les variables pour la boucle for-each
            char* array_name = $6.code;
            char* element_type = $3;
            char* element_var = $4;
            
            // Ajouter le symbole de variable d'itération
            add_symbol(element_var, element_type, line_num, 0);
            
            // Générer des variables temporaires pour l'itération
            char* index_var = tempvar();
            char* array_length = tempvar();
            
            // Initialiser l'index à 0
            quadr("=", "0", "", index_var);
            
            // Obtenir la longueur du tableau
            quadr("LENGTH", array_name, "", array_length);
            
            // Étiquette pour le début de la boucle
            char label_start[20];
            sprintf(label_start, "L%d", qc);
            quadr("LABEL", "", "", label_start);
            
            // Vérifier si l'index est < longueur
            char* cond_var = tempvar();
            quadr("<", index_var, array_length, cond_var);
            
            // Sauter à la fin si la condition est fausse
            char label_end[20];
            sprintf(label_end, "L%d", qc+1);
            quadr("JZ", cond_var, "", label_end);
            push(qc-1);  // Stocker la position du saut conditionnel
            
            // Assigner array[index] à la variable d'élément
            char access_code[100];
            sprintf(access_code, "%s[%s]", array_name, index_var);
            quadr("=", access_code, "", element_var);
        }
      statement
        {
            // Récupérer l'index temporaire (supposons qu'il est stocké dans une variable connue)
            char* index_var = "t0"; // Récupérer la variable d'index correcte
            
            // Incrémenter l'index
            quadr("++", index_var, "", index_var);
            
            // Retourner au début de la boucle
            char label_start[20];
            sprintf(label_start, "L%d", qc-2);
            quadr("GOTO", "", "", label_start);
            
            // Étiquette pour la fin de la boucle
            char label_end[20];
            sprintf(label_end, "L%d", qc);
            quadr("LABEL", "", "", label_end);
            
            // Backpatch du saut conditionnel
            int cond_jump = pop();
            sprintf(label_end, "L%d", qc-1);
            ajour_quad(cond_jump, 4, label_end);
        }
    ;

while_statement
    : WHILE 
        {
            // Label for loop condition
            char temp_label[20];
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
            push(qc-1);  // Store position of condition label
        }
      LEFT_PAREN expression RIGHT_PAREN
        {
            // Generate conditional jump
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("JZ", $4.code, "", temp_label);  // Jump if condition is false
            push(qc-1);  // Store position of conditional jump
        }
      statement
        {
            // Jump back to condition
            int cond_label = pop();
            int start_label = pop();
            char temp_label[20];
            sprintf(temp_label, "L%d", start_label+1);
            quadr("GOTO", "", "", temp_label);
            
            // Label for loop exit
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
            
            // Backpatch conditional jump
            sprintf(temp_label, "L%d", qc-1);
            ajour_quad(cond_label+1, 4, temp_label);
        }
    ;

do_while_statement
    : DO
        {
            // Label for loop body
            char temp_label[20];
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
            push(qc-1);  // Store position of body label
        }
      statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON
        {
            // Generate conditional jump back to loop body
            int body_label = pop();
            char temp_label[20];
            sprintf(temp_label, "L%d", body_label+1);
            quadr("JNZ", $6.code, "", temp_label);  // Jump if condition is true
        }
    ;

switch_statement
    : SWITCH LEFT_PAREN expression RIGHT_PAREN LEFT_BRACE
        {
            // Store the switch expression in a temporary
            char* temp = tempvar();
            quadr("=", $3.code, "", temp);
            push((int)(intptr_t)strdup(temp));  // Store the temporary name as an integer
        }
      switch_block RIGHT_BRACE
        {
            free((char*)(intptr_t)pop());  // Free the switch expression temporary
        }
    ;

switch_block
    : /* empty */
    | switch_labels
    ;

switch_labels
    : switch_label
    | switch_labels switch_label
    ;

switch_label
    : CASE expression COLON
        {
            // Compare switch expression with case value
            char* switch_expr = (char*)(intptr_t)top();
            char* temp = tempvar();
            quadr("==", switch_expr, $2.code, temp);
            
            // Conditional jump to skip case if not equal
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("JZ", temp, "", temp_label);
            push(qc-1);  // Store position of conditional jump
        }
      statement_list
        {
            // Jump to end of switch (will be backpatched later)
            char temp_label[20];
            sprintf(temp_label, "L%d", qc+1);
            quadr("GOTO", "", "", temp_label);
            
            // Backpatch conditional jump to next case
            int cond_jump = pop();
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
            ajour_quad(cond_jump, 4, temp_label);
        }
    | DEFAULT COLON
        {
            char temp_label[20];
            sprintf(temp_label, "L%d", qc);
            quadr("LABEL", "", "", temp_label);
        }
      statement_list
    ;

return_statement
    : RETURN expression_opt SEMICOLON
        {
            if ($2.code && strlen($2.code) > 0) {
                quadr("RETURN", $2.code, "", "");
            } else {
                quadr("RETURN", "", "", "");
            }
        }
    ;

try_catch_statement
    : TRY block catch_clauses
    | TRY block catch_clauses finally_clause
    ;

catch_clauses
    : catch_clause
    | catch_clauses catch_clause
    ;

catch_clause
    : CATCH LEFT_PAREN type IDENTIFIER RIGHT_PAREN
        {
            add_symbol($4, $3, line_num, 0);
            // Generate exception handling quadruple
            quadr("CATCH", $3, "", $4);
        }
      block
        {
            quadr("ENDCATCH", "", "", "");
        }
    ;

finally_clause
    : FINALLY
        {
            quadr("FINALLY", "", "", "");
        }
      block
        {
            quadr("ENDFINALLY", "", "", "");
        }
    ;

expression_opt
    : /* empty */ { $$.code = strdup(""); }
    | expression { $$ = $1; }
    ;

expression
    : assignment_expression { $$ = $1; }
    ;

assignment_expression
    : conditional_expression
        { $$ = $1; }
    | IDENTIFIER assignment_operator expression
        {
            symbol_table_entry* entry = lookup_symbol($1);
            if (entry == NULL) {
                printf("%sSemantic_Error, %d, %d, Undeclared variable: %s%s\n", ANSI_YELLOW, line_num, column_num, $1, ANSI_RESET);
                $$.code = strdup("error");
            } else {
                $$.code = $1;
                
                // Generate quadruple based on assignment operator
                switch($2) {
                    case ASSIGN:
                        quadr("=", $3.code, "", $1);
                        break;
                    case PLUS_ASSIGN: {
                        char* temp = tempvar();
                        quadr("+", $1, $3.code, temp);
                        quadr("=", temp, "", $1);
                        break;
                    }
                    case MINUS_ASSIGN: {
                        char* temp = tempvar();
                        quadr("-", $1, $3.code, temp);
                        quadr("=", temp, "", $1);
                        break;
                    }
                    case MULTIPLY_ASSIGN: {
                        char* temp = tempvar();
                        quadr("*", $1, $3.code, temp);
                        quadr("=", temp, "", $1);
                        break;
                    }
                    case DIVIDE_ASSIGN: {
                        char* temp = tempvar();
                        quadr("/", $1, $3.code, temp);
                        quadr("=", temp, "", $1);
                        break;
                    }
                    case MODULO_ASSIGN: {
                        char* temp = tempvar();
                        quadr("%", $1, $3.code, temp);
                        quadr("=", temp, "", $1);
                        break;
                    }
                }
            }
        }
         | postfix_expression DOT IDENTIFIER assignment_operator expression
        {
            // Nouveau code pour gérer les affectations de champs
            char field_access[100];
            sprintf(field_access, "%s.%s", $1.code, $3);
            
            // Générer les quadruples appropriés selon l'opérateur d'affectation
            switch($4) {
                case ASSIGN:
                    quadr("FIELD_ASSIGN", $1.code, $3, $5.code);
                    break;
                // Ajouter les autres opérateurs d'affectation si nécessaire
            }
            
            $$.code = field_access;
        }
    ;

assignment_operator
    : ASSIGN { $$ = ASSIGN; }
    | PLUS_ASSIGN { $$ = PLUS_ASSIGN; }
    | MINUS_ASSIGN { $$ = MINUS_ASSIGN; }
    | MULTIPLY_ASSIGN { $$ = MULTIPLY_ASSIGN; }
    | DIVIDE_ASSIGN { $$ = DIVIDE_ASSIGN; }
    | MODULO_ASSIGN { $$ = MODULO_ASSIGN; }
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OR logical_and_expression
        {
            char* temp = tempvar();
            quadr("||", $1.code, $3.code, temp);
            $$.code = temp;
        }
    ;

logical_and_expression
    : equality_expression { $$ = $1; }
    | logical_and_expression AND equality_expression
        {
            char* temp = tempvar();
            quadr("&&", $1.code, $3.code, temp);
            $$.code = temp;
        }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQUAL relational_expression
        {
            char* temp = tempvar();
            quadr("==", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | equality_expression NOT_EQUAL relational_expression
        {
            char* temp = tempvar();
            quadr("!=", $1.code, $3.code, temp);
            $$.code = temp;
        }
    ;

relational_expression
    : additive_expression { $$ = $1; }
    | relational_expression GREATER additive_expression
        {
            char* temp = tempvar();
            quadr(">", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | relational_expression LESS additive_expression
        {
            char* temp = tempvar();
            quadr("<", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | relational_expression GREATER_EQUAL additive_expression
        {
            char* temp = tempvar();
            quadr(">=", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | relational_expression LESS_EQUAL additive_expression
        {
            char* temp = tempvar();
            quadr("<=", $1.code, $3.code, temp);
            $$.code = temp;
        }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression PLUS multiplicative_expression
        {
            char* temp = tempvar();
            quadr("+", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | additive_expression MINUS multiplicative_expression
        {
            char* temp = tempvar();
            quadr("-", $1.code, $3.code, temp);
            $$.code = temp;
        }
    ;

multiplicative_expression
    : unary_expression { $$ = $1; }
    | multiplicative_expression MULTIPLY unary_expression
        {
            char* temp = tempvar();
            quadr("*", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | multiplicative_expression DIVIDE unary_expression
        {
            char* temp = tempvar();
            quadr("/", $1.code, $3.code, temp);
            $$.code = temp;
        }
    | multiplicative_expression MODULO unary_expression
        {
            char* temp = tempvar();
            quadr("%", $1.code, $3.code, temp);
            $$.code = temp;
        }
    ;

unary_expression
    : postfix_expression { $$ = $1; }
    | INCREMENT unary_expression
        {
            char* temp = tempvar();
            quadr("=", $2.code, "", temp); // Copy original value
            quadr("++", $2.code, "", $2.code); // Increment
            $$.code = temp;
        }
    | DECREMENT unary_expression
        {
            char* temp = tempvar();
            quadr("=", $2.code, "", temp); // Copy original value
            quadr("--", $2.code, "", $2.code); // Decrement
            $$.code = temp;
        }
    | PLUS unary_expression
        {
            $$ = $2; // Unary plus does nothing
        }
    | MINUS unary_expression
        {
            char* temp = tempvar();
            quadr("uminus", $2.code, "", temp);
            $$.code = temp;
        }
    | NOT unary_expression
        {
            char* temp = tempvar();
            quadr("!", $2.code, "", temp);
            $$.code = temp;
        }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression INCREMENT
        {
            char* temp = tempvar();
            quadr("=", $1.code, "", temp); // Copy original value
            quadr("++", $1.code, "", $1.code); // Increment after using the value
            $$.code = temp;
        }
    | postfix_expression DECREMENT
        {
            char* temp = tempvar();
            quadr("=", $1.code, "", temp); // Copy original value
            quadr("--", $1.code, "", $1.code); // Decrement after using the value
            $$.code = temp;
        }
    | postfix_expression LEFT_PAREN argument_list_opt RIGHT_PAREN
        {
            // Method call - get a temporary for the result
            char* temp = tempvar();
            quadr("CALL", $1.code, "", temp);
            $$.code = temp;
        }
    | postfix_expression DOT IDENTIFIER
        {
            // Object field access
            char* temp = tempvar();
            char field_access[100];
            sprintf(field_access, "%s.%s", $1.code, $3);
            quadr("FIELD", $1.code, $3, temp);
            $$.code = temp;
        }
    ;

primary_expression
    : THIS
        {
            $$.code = strdup("this");
        }
    | IDENTIFIER
        {
            symbol_table_entry* entry = lookup_symbol($1);
            if (entry == NULL) {
                printf("%sSemantic_Error, %d, %d, Undeclared variable: %s%s\n", ANSI_YELLOW, line_num, column_num, $1, ANSI_RESET);
                $$.code = strdup("error");
            } else {
                $$.code = strdup($1);
            }
        }
    | literal { $$ = $1; }
    | LEFT_PAREN expression RIGHT_PAREN { $$ = $2; }
    | PRINT LEFT_PAREN expression RIGHT_PAREN
        {
            quadr("PRINT", $3.code, "", "");
            $$.code = $3.code;
        }
    | PRINT LEFT_PAREN RIGHT_PAREN
        {
            quadr("PRINT", "", "", "");
            $$.code = strdup("");
        }
    ;

argument_list_opt
    : /* empty */
    | argument_list
    ;

argument_list
    : expression
        {
            // Generate parameter passing quadruple
            quadr("ARG", $1.code, "", "");
        }
    | argument_list COMMA expression
        {
            // Generate parameter passing quadruple
            quadr("ARG", $3.code, "", "");
        }
    ;

literal
    : INTEGER_LITERAL
        {
            char* temp = malloc(20);
            sprintf(temp, "%d", $1);
            $$.code = temp;
        }
    | FLOAT_LITERAL
        {
            char* temp = malloc(20);
            sprintf(temp, "%f", $1);
            $$.code = temp;
        }
    | CHAR_LITERAL
        {
            char* temp = malloc(5);
            sprintf(temp, "'%c'", $1);
            $$.code = temp;
        }
    | STRING_LITERAL
        {
            char* temp = malloc(strlen($1) + 3);
            sprintf(temp, "\"%s\"", $1);
            $$.code = temp;
        }
    | TRUE_VAL
        {
            $$.code = strdup("true");
        }
    | FALSE_VAL
        {
            $$.code = strdup("false");
        }
    | NULL_VAL
        {
            $$.code = strdup("null");
        }
    ;

%%

void yyerror(const char* s) {
    printf("%sSyntax_Error, %d, %d, %s%s\n", ANSI_ORANGE, line_num, column_num, s, ANSI_RESET);
     enhanced_syntax_error(s, current_filename);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Cannot open file %s\n", argv[1]);
            return 1;
        }
        yyin = file;
        // Stocker le nom du fichier pour les diagnostics
        strcpy(current_filename, argv[1]);
    } else {
        // Si aucun fichier n'est spécifié, indiquez une valeur par défaut
        strcpy(current_filename, "stdin");
    }
    
    yyparse();
    
    // Display generated quadruples
    printf("\n---------- Intermediate Code (Quadruples) ----------\n");
    afficher_qdr();
    

    // Générer une version optimisée des quadruplets pour la génération de code
    copy_quads_for_optimization();


        // Générer le code assembleur
    char output_file[256];
    if (argc > 1) {
        // Prendre le même nom de fichier que l'entrée mais avec extension .asm
        strcpy(output_file, argv[1]);
        char *dot = strrchr(output_file, '.');
        if (dot) *dot = '\0';  // Supprimer l'extension existante
        strcat(output_file, ".asm");
    } else {
        strcpy(output_file, "output.asm");
    }
    
    printf("\n---------- Generating Assembly Code ----------\n");
    generate_assembly_code(output_file);
    

    return 0;
}