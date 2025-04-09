%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic_analysis.h"
#include "intermediate_code.h"
#include "ast.h"

extern int line_num;
extern int column_num;
extern char* yytext;
extern FILE* yyin;

void yyerror(const char* s);
int yylex();

typedef struct symbol_table_entry {
    char* name;
    char* type;
    int scope;
    struct symbol_table_entry* next;
} symbol_table_entry;

extern symbol_table_entry* symbol_table;
extern int current_scope;
extern void generate_tac_for_expression(ast_node* node);
extern void generate_tac_for_statement(ast_node* node);
extern void optimize_tac_code();
extern void generate_assembly_code(const char* output_file);
extern ast_node* root;

void add_symbol(char* name, char* type, int line_num, int is_method);
symbol_table_entry* lookup_symbol(char* name);
void enter_scope();
void exit_scope();
%}

%union {
    int int_val;
    float float_val;
    char char_val;
    char* string_val;
    char* id;
    ast_node* node;
}

/* Token definitions */
%token CLASS PUBLIC PRIVATE PROTECTED STATIC VOID MAIN
%token INT FLOAT DOUBLE CHAR BOOLEAN
%token IF ELSE SWITCH CASE DEFAULT FOR WHILE DO BREAK CONTINUE RETURN
%token TRY CATCH FINALLY THROW
%token TRUE_VAL FALSE_VAL NULL_VAL
%token PRINT

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

/* Type declarations */
%type <id> type
%type <node> program class_declaration

/* Precedence and associativity */
%left OR
%left AND
%left EQUAL NOT_EQUAL
%left GREATER LESS GREATER_EQUAL LESS_EQUAL
%left PLUS MINUS
%left MULTIPLY DIVIDE MODULO
%right NOT INCREMENT DECREMENT
%nonassoc IF
%nonassoc ELSE

/* Expect 1 shift/reduce conflict (dangling else) */
%expect 1

%%

/* Grammar rules remain unchanged; key snippet: */
if_statement
    : IF LEFT_PAREN expression RIGHT_PAREN statement
    | IF LEFT_PAREN expression RIGHT_PAREN statement ELSE statement
    ;

/* Rest of the grammar as previously provided */
program
    : class_declaration
        { root = $1; }
    | program class_declaration
        { root = create_block_node((ast_node*[]){$1, $2}, 2); }
    ;

class_declaration
    : CLASS IDENTIFIER LEFT_BRACE class_body RIGHT_BRACE
        { 
            printf("Class declared: %s\n", $2);
            { $$ = create_block_node($4.statements, $4.statement_count); }        
        }
    ;

class_body
    : { $$.statements = NULL; $$.statement_count = 0; }
    | class_member_declarations
    ;

class_member_declarations
    : class_member_declaration
    | class_member_declarations class_member_declaration
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
        }
    ;

method_declaration
    : type IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
        { 
            enter_scope(); 
            set_current_method_type($1); 
            ast_node** stmts = malloc(sizeof(ast_node*));
            stmts[0] = create_declaration_node($1.id, $2.id, NULL);
            $$ = create_block_node(stmts, 1);
            add_symbol($2.id, $1.id, line_num, 1);
        }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Method declared: %s returning %s\n", $2, $1);
            exit_scope();
        }
    | VOID IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
        { enter_scope(); set_current_method_type("void"); }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Void method declared: %s\n", $2);
            exit_scope();
        }
    | PUBLIC STATIC VOID MAIN LEFT_PAREN IDENTIFIER LEFT_BRACKET RIGHT_BRACKET IDENTIFIER RIGHT_PAREN LEFT_BRACE
        { enter_scope(); set_current_method_type("void"); }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Main method declared\n");
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
            add_symbol($2, $1, line_num, 1);
        }
    ;

type
    : INT { $$ = strdup("int"); }
    | FLOAT { $$ = strdup("float"); }
    | DOUBLE { $$ = strdup("double"); }
    | CHAR { $$ = strdup("char"); }
    | BOOLEAN { $$ = strdup("boolean"); }
    | IDENTIFIER { $$ = $1; }
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
        }
    | type IDENTIFIER ASSIGN expression SEMICOLON
        { 
            printf("Variable declared with initialization: %s of type %s\n", $2, $1);
            add_symbol($2, $1, line_num, 0);
        }
    ;

for_statement
    : FOR LEFT_PAREN expression_opt SEMICOLON expression_opt SEMICOLON expression_opt RIGHT_PAREN statement
    | FOR LEFT_PAREN declaration_statement expression_opt SEMICOLON expression_opt RIGHT_PAREN statement
    ;

while_statement
    : WHILE LEFT_PAREN expression RIGHT_PAREN statement
    ;

do_while_statement
    : DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON
    ;

switch_statement:
      SWITCH LEFT_PAREN expression RIGHT_PAREN switch_block
      { $$ = create_block_node(NULL, 0); }  // Simplified for now
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
    : CASE expression COLON statement_list
    | DEFAULT COLON statement_list
    ;

return_statement
    : RETURN expression_opt SEMICOLON
    ;

try_catch_statement:
    TRY block catch_clauses
        { $$ = create_block_node(NULL, 0); }
    | TRY block catch_clauses finally_clause
        { $$ = create_block_node(NULL, 0); }
    ;

catch_clauses
    : catch_clause
    | catch_clauses catch_clause
    ;

catch_clause
    : CATCH LEFT_PAREN type IDENTIFIER RIGHT_PAREN block
        { add_symbol($4, $3, line_num, 0); }
    ;

finally_clause
    : FINALLY block
    ;

expression_opt
    : /* empty */
    | expression
    ;

expression
    : assignment_expression
    ;

assignment_expression
    : conditional_expression
    | IDENTIFIER assignment_operator expression
        {
            symbol_table_entry* entry = lookup_symbol($1);
            if (entry == NULL) {
                printf("Semantic_Error, %d, %d, Undeclared variable: %s\n", line_num, column_num, $1);
            }
        }
    ;

assignment_operator
    : ASSIGN
    | PLUS_ASSIGN
    | MINUS_ASSIGN
    | MULTIPLY_ASSIGN
    | DIVIDE_ASSIGN
    | MODULO_ASSIGN
    ;

conditional_expression
    : logical_or_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR logical_and_expression
    ;

logical_and_expression
    : equality_expression
    | logical_and_expression AND equality_expression
    ;

equality_expression
    : relational_expression
    | equality_expression EQUAL relational_expression
    | equality_expression NOT_EQUAL relational_expression
    ;

relational_expression
    : additive_expression
    | relational_expression GREATER additive_expression
    | relational_expression LESS additive_expression
    | relational_expression GREATER_EQUAL additive_expression
    | relational_expression LESS_EQUAL additive_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression PLUS multiplicative_expression
    | additive_expression MINUS multiplicative_expression
    ;

multiplicative_expression
    : unary_expression
    | multiplicative_expression MULTIPLY unary_expression
    | multiplicative_expression DIVIDE unary_expression
    | multiplicative_expression MODULO unary_expression
    ;

unary_expression
    : postfix_expression
    | INCREMENT unary_expression
    | DECREMENT unary_expression
    | PLUS unary_expression
    | MINUS unary_expression
    | NOT unary_expression
    ;

postfix_expression
    : primary_expression
    | postfix_expression INCREMENT
    | postfix_expression DECREMENT
    | postfix_expression LEFT_PAREN argument_list_opt RIGHT_PAREN
    | postfix_expression DOT IDENTIFIER
    ;

primary_expression
    : IDENTIFIER
        {
            symbol_table_entry* entry = lookup_symbol($1);
            if (entry == NULL) {
                printf("Semantic_Error, %d, %d, Undeclared variable: %s\n", line_num, column_num, $1);
            }
        }
    | literal
    | LEFT_PAREN expression RIGHT_PAREN
    | PRINT LEFT_PAREN expression RIGHT_PAREN
    ;

argument_list_opt
    : /* empty */
    | argument_list
    ;

argument_list
    : expression
    | argument_list COMMA expression
    ;

literal
    : INTEGER_LITERAL
    | FLOAT_LITERAL
    | CHAR_LITERAL
    | STRING_LITERAL
    | TRUE_VAL
    | FALSE_VAL
    | NULL_VAL
    ;

%%

void yyerror(const char* s) {
    printf("Syntax_Error, %d, %d, %s\n", line_num, column_num, s);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Cannot open file %s\n", argv[1]);
            return 1;
        }
        yyin = file;
    }
    
    yyparse();
    
    return 0;
}