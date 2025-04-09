#include <stdio.h>
#include "ast.h"        // Added to define ast_node
#include "parser.tab.h" // Now ast_node is known
#include "tac_generator.h"
#include "symbol_table.h"
#include "intermediate_code.h"
#include "semantic_analysis.h"

extern FILE *yyin;
extern int yyparse();
extern ast_node *root;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input_file> [output_file]\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin)
    {
        fprintf(stderr, "Error: Could not open input file %s\n", argv[1]);
        return 1;
    }

    if (yyparse() != 0)
    {
        fprintf(stderr, "Parsing failed\n");
        fclose(yyin);
        return 1;
    }
    // if (yyparse() == 0)
    // {
    //     analyze_ast(root);
    //     generate_tac_from_ast(root);
    //     print_tac_code();
    //     optimize_tac_code();
    //     printf("\nOptimized TAC:\n");
    //     print_tac_code();
    //     generate_assembly_code(output_file);
    // }
    generate_tac_from_ast(root);
    print_tac_code();

    optimize_tac_code();
    printf("\nOptimized TAC:\n");
    print_tac_code();

    const char *output_file = (argc > 2) ? argv[2] : "output.asm";
    generate_assembly_code(output_file);

    free_ast_node(root);
    free_tac_code();
    free_symbol_table();
    fclose(yyin);

    return 0;
}