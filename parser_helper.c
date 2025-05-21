#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "token_defs.h"
#include "diagnostic.c"

extern int line_num;
extern int column_num;
extern char* yytext;

// Déclaration de la fonction de diagnostic des erreurs
extern void syntax_error_diagnostics(int line, int column, const char* input_filename);

// Tableau des messages d'erreur courants pour aider à identifier les problèmes
const char* common_syntax_errors[] = {
    "Il manque peut-être un point-virgule (;) à la fin d'une instruction précédente",
    "Vérifiez si les parenthèses sont correctement équilibrées",
    "Les accolades peuvent ne pas être correctement équilibrées",
    "Il peut y avoir un mot-clé ou un identificateur mal orthographié",
    "Les types incompatibles dans une expression ou une affectation",
    "Utilisation incorrecte d'un opérateur",
    "Déclaration de variable ou de méthode incorrecte",
    "Utilisation d'une variable avant sa déclaration",
    "Expression incorrecte dans une condition ou une boucle"
};

// Cette fonction est appelée lorsqu'une erreur de syntaxe est détectée
void enhanced_syntax_error(const char* msg, const char* filename) {
    printf("Syntax_Error, %d, %d, %s\n", line_num, column_num, msg);
    
    // Appeler la fonction de diagnostic pour plus d'informations
    syntax_error_diagnostics(line_num, column_num, filename);
    
    // Afficher des suggestions d'erreurs courantes
    printf("Suggestions d'erreurs courantes à cette position:\n");
    for (int i = 0; i < sizeof(common_syntax_errors) / sizeof(common_syntax_errors[0]); i++) {
        printf("- %s\n", common_syntax_errors[i]);
    }
    
    printf("\nDernier token analysé: '%s'\n", yytext);
}

// Cette fonction peut être utilisée pour tester l'analyseur sur un fichier spécifique
int parse_with_diagnostics(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Impossible d'ouvrir le fichier %s\n", filename);
        return 1;
    }
    
    extern FILE* yyin;
    yyin = file;
    
    // Stocker le nom du fichier pour les diagnostics
    extern int yyparse();
    int result = yyparse();
    
    if (result != 0) {
        printf("L'analyse a échoué avec le code %d\n", result);
        // On pourrait appeler la fonction de diagnostic ici
    } else {
        printf("L'analyse a réussi!\n");
    }
    
    fclose(file);
    return result;
}