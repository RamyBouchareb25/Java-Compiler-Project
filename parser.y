%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colors.h" // Ajout de l'inclusion pour les couleurs
#include "parser_helper.h" // Ajout de l'inclusion pour les fonctions d'aide
#include "token_defs.h" // Pour les définitions des opérateurs TAC
#include "tac.h"
void enhanced_syntax_error(const char* msg, const char* filename);
extern int line_num;
extern int column_num;
extern char* yytext;
extern FILE* yyin;

void yyerror(const char* s);
int yylex();

// Symbol table declaration (basic version)
typedef struct symbol_table_entry {
    char* name;
    char* type;
    int scope;
    struct symbol_table_entry* next;
} symbol_table_entry;

extern symbol_table_entry* symbol_table;
extern int current_scope;


// Function prototypes for symbol table operations
void add_symbol(char* name, char* type, int line_num, int is_method);
symbol_table_entry* lookup_symbol(char* name);
void enter_scope();
void exit_scope();
%}

// Cette section sera ajoutée au fichier d'en-tête généré (parser.tab.h)

%code requires {
    #include "tac.h"
}

%union {
    int int_val;
    float float_val;
    char char_val;
    char* string_val;
    char* id;
    tac_operand operand; // Ajout pour le code intermédiaire
}

/* Token definitions */
%token CLASS PUBLIC PRIVATE PROTECTED STATIC VOID MAIN
%token INT FLOAT DOUBLE CHAR BOOLEAN
%token IF ELSE SWITCH CASE DEFAULT FOR WHILE DO BREAK CONTINUE RETURN
%token TRY CATCH FINALLY THROW
%token TRUE_VAL FALSE_VAL NULL_VAL
%token PRINT
%token THIS
%token NEW
%token CAST_INT CAST_FLOAT CAST_DOUBLE CAST_CHAR CAST_BOOLEAN CAST_STRING CAST_CUSTOM

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
%token <id> CAST_CUSTOM


%token ARITHMETIC_EXCEPTION EXCEPTION

%type <id> type
%type <operand> expression expression_opt
%type <operand> assignment_expression conditional_expression
%type <operand> logical_or_expression logical_and_expression
%type <operand> equality_expression relational_expression
%type <operand> additive_expression multiplicative_expression
%type <operand> unary_expression postfix_expression primary_expression
%type <operand> literal
%%

/* Grammar Rules */

program
    : class_declaration
    | program class_declaration
    ;

class_declaration
    : CLASS IDENTIFIER LEFT_BRACE
        { 
            printf("Class declared: %s\n", $2); 
            // Ajouter la classe à la table des symboles comme un type
            add_symbol($2, "class", line_num, 1);
        }
      class_body 
      RIGHT_BRACE
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
        }
    ;

method_declaration
    : type IDENTIFIER LEFT_PAREN
            { enter_scope(); } /* Enter scope before parameters */
        parameter_list_opt RIGHT_PAREN LEFT_BRACE
        statement_list 
        RIGHT_BRACE
            { 
                printf("Method declared: %s returning %s\n", $2, $1);
                exit_scope();
            }
    | type IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
        { enter_scope(); }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Method declared: %s returning %s\n", $2, $1);
            exit_scope();
        }
    | VOID IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
        { enter_scope(); }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Void method declared: %s\n", $2);
            exit_scope();
        }
    | IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN LEFT_BRACE
        { 
            enter_scope();
            // This is a constructor - it has the same name as the class
            printf("Constructor declared for class: %s\n", $1);
        }
      statement_list 
      RIGHT_BRACE
        { 
            exit_scope();
        }
    | PUBLIC STATIC VOID MAIN LEFT_PAREN IDENTIFIER LEFT_BRACKET RIGHT_BRACKET IDENTIFIER RIGHT_PAREN LEFT_BRACE
        { enter_scope(); }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Main method declared\n");
            exit_scope();
        }
    | PUBLIC STATIC VOID MAIN LEFT_PAREN RIGHT_PAREN LEFT_BRACE
        { enter_scope(); }
      statement_list 
      RIGHT_BRACE
        { 
            printf("Main method declared without parameters\n");
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
        }
        | type LEFT_BRACKET RIGHT_BRACKET IDENTIFIER
        {
            char array_type[64];
            sprintf(array_type, "%s[]", $1); // Construct "int[]" or "float[]"
            printf("Parameter: %s of type %s\n", $4, array_type);
            add_symbol($4, array_type, line_num, 0);
        }
    ;

type
    : INT { $$ = strdup("int"); }
    | FLOAT { $$ = strdup("float"); }
    | DOUBLE { $$ = strdup("double"); }
    | CHAR { $$ = strdup("char"); }
    | BOOLEAN { $$ = strdup("boolean"); }
    | IDENTIFIER { 
        // Vérifier si l'identifiant est une classe existante
        symbol_table_entry* entry = lookup_symbol($1);
        if (entry == NULL) {
            printf("%sSemantic_Error, %d, %d, Undefined class type: %s%s\n", 
                   ANSI_YELLOW, line_num, column_num, $1, ANSI_RESET);
        }
        $$ = $1; 
    }
    | type LEFT_BRACKET RIGHT_BRACKET {
        // Handle array type like int[]
        char* array_type = malloc(strlen($1) + 3); // +3 for [] and null terminator
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
    | BREAK SEMICOLON    /* Ajout de cette règle pour gérer les 'break;' */
    | CONTINUE SEMICOLON /* Ajout de cette règle pour gérer les 'continue;' */
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
    | type LEFT_BRACKET RIGHT_BRACKET IDENTIFIER ASSIGN array_initializer SEMICOLON
        {
            char array_type[64];
            sprintf(array_type, "%s[]", $1);
            printf("Array declared with initialization: %s of type %s\n", $4, array_type);
            add_symbol($4, array_type, line_num, 0);
        }
    ;
// Ajout d'une règle pour l'initialisation d'un tableau
array_initializer
    : LEFT_BRACE array_elements RIGHT_BRACE
        { printf("Array initializer with elements\n"); }
    ;

array_elements
    : expression
        { printf("Array element added\n"); }
    | array_elements COMMA expression
        { printf("Array element added\n"); }
    ;

if_statement
    : IF LEFT_PAREN expression RIGHT_PAREN 
        {
            // Générer le code intermédiaire pour l'expression de condition
            // Créer une étiquette pour le "else" ou la fin du if
            tac_operand else_label = create_label();
            
            // Genérer un jump conditionnel - si la condition est fausse, sauter à l'étiquette else/end
            generate_ifnot($3, else_label);
            
            // Empiler l'étiquette pour une utilisation ultérieure
            $<operand>$ = else_label;
        }
      statement
        {
            // Créer une étiquette pour la fin du if
            tac_operand end_label = create_label();
            
            // Générer un saut inconditionnel vers la fin (pour sauter le bloc else)
            generate_goto(end_label);
            
            // Placer l'étiquette else/end ici
            generate_label_tac($<operand>5);
            
            // Empiler l'étiquette de fin pour une utilisation avec else
            $<operand>$ = end_label;
        }
    | IF LEFT_PAREN expression RIGHT_PAREN
        {
            // Générer le code intermédiaire pour l'expression de condition
            // Créer une étiquette pour le "else"
            tac_operand else_label = create_label();
            
            // Genérer un jump conditionnel - si la condition est fausse, sauter au else
            generate_ifnot($3, else_label);
            
            // Empiler l'étiquette pour une utilisation ultérieure
            $<operand>$ = else_label;
        }
      statement ELSE
        {
            // Créer une étiquette pour la fin du if-else
            tac_operand end_label = create_label();
            
            // Générer un saut inconditionnel vers la fin (pour sauter le bloc else)
            generate_goto(end_label);
            
            // Placer l'étiquette else ici
            generate_label_tac($<operand>5);
            
            // Empiler l'étiquette de fin
            $<operand>$ = end_label;
        }
      statement
        {
            // Placer l'étiquette de fin après le bloc else
            generate_label_tac($<operand>8);
        }
    ;

for_statement
    : FOR LEFT_PAREN expression_opt SEMICOLON expression_opt SEMICOLON expression_opt RIGHT_PAREN statement
    | FOR LEFT_PAREN declaration_statement expression_opt SEMICOLON expression_opt RIGHT_PAREN statement
    | FOR LEFT_PAREN type IDENTIFIER COLON primary_expression RIGHT_PAREN 
        {
            // Créer un nouveau scope pour la boucle for-each
            enter_scope();
            
            // Ajouter la variable d'itération à la table des symboles
            printf("For-each loop with variable %s of type %s\n", $4, $3);
            add_symbol($4, $3, line_num, 0);
        }
        statement
        {
            // Sortir du scope à la fin de la boucle for-each
            exit_scope();
        }
    ;

while_statement
    : WHILE LEFT_PAREN expression RIGHT_PAREN statement
    ;

do_while_statement
    : DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON
    ;


switch_statement
    : SWITCH LEFT_PAREN expression RIGHT_PAREN LEFT_BRACE switch_block RIGHT_BRACE
      {
        printf("Completed parsing switch statement\n");
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
    : CASE expression COLON statement_list
    | CASE expression COLON
    | DEFAULT COLON statement_list
    | DEFAULT COLON
    ;

return_statement
    : RETURN expression_opt SEMICOLON
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
        { enter_scope(); add_symbol($4, $3, line_num, 0); }
      block
        { exit_scope(); }
    | CATCH LEFT_PAREN ARITHMETIC_EXCEPTION IDENTIFIER RIGHT_PAREN
        { enter_scope(); printf("Catch block for predefined exception: ArithmeticException\n"); add_symbol($4, "ArithmeticException", line_num, 0); }
      block
        { exit_scope(); }
    | CATCH LEFT_PAREN EXCEPTION IDENTIFIER RIGHT_PAREN
        { enter_scope(); printf("Catch block for generic Exception\n"); add_symbol($4, "Exception", line_num, 0); }
      block
        { exit_scope(); }
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
                printf("%sSemantic_Error, %d, %d, Undeclared variable: %s%s\n", ANSI_YELLOW, line_num, column_num, $1, ANSI_RESET);
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
        {
            $$ = $1;
        }
    | logical_or_expression OR logical_and_expression
        {
            $$ = generate_binary_op(OP_OR, $1, $3);
        }
    ;

logical_and_expression
    : equality_expression
        {
            $$ = $1;
        }
    | logical_and_expression AND equality_expression
        {
            $$ = generate_binary_op(OP_AND, $1, $3);
        }
    ;

equality_expression
    : relational_expression
        {
            $$ = $1;
        }
    | equality_expression EQUAL relational_expression
        {
            $$ = generate_binary_op(OP_EQ, $1, $3);
        }
    | equality_expression NOT_EQUAL relational_expression
        {
            $$ = generate_binary_op(OP_NE, $1, $3);
        }
    ;

relational_expression
    : additive_expression
        {
            $$ = $1;
        }
    | relational_expression GREATER additive_expression
        {
            $$ = generate_binary_op(OP_GT, $1, $3);
        }
    | relational_expression LESS additive_expression
        {
            $$ = generate_binary_op(OP_LT, $1, $3);
        }
    | relational_expression GREATER_EQUAL additive_expression
        {
            $$ = generate_binary_op(OP_GE, $1, $3);
        }
    | relational_expression LESS_EQUAL additive_expression
        {
            $$ = generate_binary_op(OP_LE, $1, $3);
        }
    ;

additive_expression
    : multiplicative_expression
        {
            $$ = $1;
        }
    | additive_expression PLUS multiplicative_expression
        {
            $$ = generate_binary_op(OP_ADD, $1, $3);
        }
    | additive_expression MINUS multiplicative_expression
        {
            $$ = generate_binary_op(OP_SUB, $1, $3);
        }
    ;

multiplicative_expression
    : unary_expression
        {
            $$ = $1;
        }
    | multiplicative_expression MULTIPLY unary_expression
        {
            $$ = generate_binary_op(OP_MUL, $1, $3);
        }
    | multiplicative_expression DIVIDE unary_expression
        {
            $$ = generate_binary_op(OP_DIV, $1, $3);
        }
    | multiplicative_expression MODULO unary_expression
        {
            $$ = generate_binary_op(OP_MOD, $1, $3);
        }
    ;

unary_expression
    : postfix_expression
        {
            $$ = $1;
        }
    | INCREMENT unary_expression
        {
            // Génération de code pour pré-incrément
            $$ = generate_unary_op(OP_INC, $2);
        }
    | DECREMENT unary_expression
        {
            // Génération de code pour pré-décrément
            $$ = generate_unary_op(OP_DEC, $2);
        }
    | PLUS unary_expression
        {
            // Pas d'effet sur l'expression
            $$ = $2;
        }
    | MINUS unary_expression
        {
            // Génération de code pour négation
            $$ = generate_unary_op(OP_NEG, $2);
        }
    | NOT unary_expression
        {
            // Génération de code pour NOT logique
            $$ = generate_unary_op(OP_NOT, $2);
        }
    | CAST_INT unary_expression
        {
            // Type casting to int
            printf("Type casting to int\n");
            $$ = $2; // Pour simplifier, on ignore le cast dans le code intermédiaire
        }
    | CAST_FLOAT unary_expression
        {
            // Type casting to float
            printf("Type casting to float\n");
            $$ = $2;
        }
    | CAST_DOUBLE unary_expression
        {
            // Type casting to double
            printf("Type casting to double\n");
            $$ = $2;
        }
    | CAST_CHAR unary_expression
        {
            // Type casting to char
            printf("Type casting to char\n");
            $$ = $2;
        }
    | CAST_BOOLEAN unary_expression
        {
            // Type casting to boolean
            printf("Type casting to boolean\n");
            $$ = $2;
        }
    | CAST_STRING unary_expression
        {
            // Type casting to String
            printf("Type casting to String\n");
            $$ = $2;
        }
    | CAST_CUSTOM unary_expression
        {
            // Type casting to custom type
            printf("Type casting to custom type: %s\n", $1);
            $$ = $2;
        }
    ;

postfix_expression
    : primary_expression
        {
            $$ = $1;
        }
    | postfix_expression INCREMENT
        {
            // Génération de code pour post-incrément
            $$ = generate_unary_op(OP_POST_INC, $1);
        }
    | postfix_expression DECREMENT
        {
            // Génération de code pour post-décrément
            $$ = generate_unary_op(OP_POST_DEC, $1);
        }
    | postfix_expression LEFT_PAREN argument_list_opt RIGHT_PAREN
        {
            // Appel de fonction ou méthode
            // Pour simplifier, on crée un temporaire
            $$ = create_temporary();
        }
    | postfix_expression DOT IDENTIFIER
        {
            // Accès à un champ
            $$ = generate_field_load($1, $3);
        }
    | postfix_expression DOT IDENTIFIER LEFT_PAREN argument_list_opt RIGHT_PAREN
        {
            // Appel de méthode
            // Pour simplifier, on crée un temporaire
            $$ = create_temporary();
        }
    ;

primary_expression
    : THIS
        {
            $$ = create_variable("this");
        }
    | IDENTIFIER
        {
            symbol_table_entry* entry = lookup_symbol($1);
            if (entry == NULL) {
                printf("%sSemantic_Error, %d, %d, Undeclared variable: %s%s\n", ANSI_YELLOW, line_num, column_num, $1, ANSI_RESET);
                // Créer quand même un opérande pour éviter des erreurs en cascade
                $$ = create_variable($1);
            } else {
                $$ = create_variable($1);
            }
        }
    | literal
    | LEFT_PAREN expression RIGHT_PAREN
        {
            $$ = $2;
        }
    | PRINT LEFT_PAREN expression RIGHT_PAREN
        {
            // Appel de fonction pour print
            $$ = $3; // Retourne l'expression
        }
    | PRINT LEFT_PAREN RIGHT_PAREN
        {
            // Création d'un opérande temporaire pour le résultat
            $$ = create_temporary();
        }
    | NEW IDENTIFIER LEFT_PAREN argument_list_opt RIGHT_PAREN
        {
            // Vérifier si la classe existe
            symbol_table_entry* entry = lookup_symbol($2);
            if (entry == NULL) {
                printf("%sSemantic_Error, %d, %d, Undefined class type: %s%s\n", ANSI_YELLOW, line_num, column_num, $2, ANSI_RESET);
            }
            printf("Object instantiation of class: %s\n", $2);
            $$ = generate_new_object($2);
        }
    | NEW type LEFT_BRACKET expression RIGHT_BRACKET
        {
            // Création d'un tableau
            printf("Array instantiation of type: %s\n", $2);
            $$ = generate_new_array($2, $4);
        }
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
        {
            $$ = create_int_literal($1);
        }
    | FLOAT_LITERAL
        {
            // Pour simplifier, nous utilisons une conversion vers int
            $$ = create_float_literal($1);
        }
    | CHAR_LITERAL
        {
            // Conversion du caractère en valeur entière pour le code intermédiaire
            $$ = create_int_literal((int)$1);
        }
    | STRING_LITERAL
        {
            // Pour les chaînes, on crée un opérande temporaire
            $$ = create_temporary();
        }
    | TRUE_VAL
        {
            $$ = create_int_literal(1); // true = 1
        }
    | FALSE_VAL
        {
            $$ = create_int_literal(0); // false = 0
        }
    | NULL_VAL
        {
            $$ = create_int_literal(0); // null = 0 pour le code intermédiaire
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
    
    return 0;
}