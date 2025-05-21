#include <stdio.h>
#include <stdlib.h>
#include "parser_helper.h"
#include "colors.h" // Inclure les définitions de couleurs

// Une nouvelle fonction pour aider au diagnostic des erreurs de syntaxe
void syntax_error_diagnostics(int line, int column, const char* input_filename) {
    FILE* file = fopen(input_filename, "r");
    if (!file) {
        printf("%sImpossible d'ouvrir le fichier source pour le diagnostic: %s%s\n", 
               ANSI_ORANGE, input_filename, ANSI_RESET);
        return;
    }
    
    // Afficher le contexte de l'erreur (lignes avant et après)
    char line_buffer[256];
    int current_line = 0;
    int context_lines = 3; // Nombre de lignes à afficher avant et après
    
    printf("\n%s===== Diagnostic de l'erreur de syntaxe (ligne %d, colonne %d) =====%s\n", 
           ANSI_ORANGE, line, column, ANSI_RESET);
    
    // Lire le fichier jusqu'à quelques lignes après celle de l'erreur
    while (fgets(line_buffer, sizeof(line_buffer), file) && current_line <= line + context_lines) {
        current_line++;
        
        // Afficher les lignes dans la fenêtre de contexte
        if (current_line >= line - context_lines && current_line <= line + context_lines) {
            if (current_line == line) {
                printf("%s-> %d: %s%s", ANSI_ORANGE, current_line, line_buffer, ANSI_RESET);
            } else {
                printf("   %d: %s", current_line, line_buffer);
            }
            
            // Indiquer la position exacte de l'erreur
            if (current_line == line) {
                printf("%s   ", ANSI_ORANGE);
                for (int i = 0; i < column - 1; i++) {
                    printf(" ");
                }
                printf("^%s\n", ANSI_RESET);
            }
        }
    }
    
    printf("\n%sConseils courants pour cette erreur:%s\n", ANSI_ORANGE, ANSI_RESET);
    printf("%s- Vérifiez s'il manque un point-virgule (;) à la fin d'une instruction%s\n", ANSI_ORANGE, ANSI_RESET);
    printf("%s- Vérifiez si une accolade ou une parenthèse n'est pas correctement fermée%s\n", ANSI_ORANGE, ANSI_RESET);
    printf("%s- Vérifiez la syntaxe des déclarations de variables et de méthodes%s\n", ANSI_ORANGE, ANSI_RESET);
    printf("%s- Assurez-vous que les opérateurs binaires ont des opérandes valides%s\n", ANSI_ORANGE, ANSI_RESET);
    printf("%s=================================================================%s\n\n", ANSI_ORANGE, ANSI_RESET);
    
    fclose(file);
}