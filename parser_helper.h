#ifndef PARSER_HELPER_H
#define PARSER_HELPER_H

// Déclaration de la variable globale pour le nom du fichier
extern char current_filename[256];

// Déclaration de la fonction enhanced_syntax_error
void enhanced_syntax_error(const char* msg, const char* filename);

// Déclaration de la fonction syntax_error_diagnostics
void syntax_error_diagnostics(int line, int column, const char* input_filename);

#endif // PARSER_HELPER_H