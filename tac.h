#ifndef TAC_H
#define TAC_H

// Définition du type tac_operand pour le code intermédiaire
typedef struct {
    int type;           // Type d'opérande (variable, constante, étiquette, etc.)
    union {
        int int_val;    // Valeur entière pour les littéraux
        float float_val; // Valeur flottante pour les littéraux
        char* str_val;  // Nom pour les variables ou valeur pour les chaînes
    } value;
} tac_operand;

// Prototypes pour les fonctions de code intermédiaire
extern tac_operand create_label();
extern tac_operand create_int_literal(int value);
extern tac_operand create_float_literal(float value);
extern tac_operand create_variable(char* name);
extern tac_operand create_temporary();
extern void generate_ifnot(tac_operand condition, tac_operand false_label);
extern void generate_goto(tac_operand label);
extern void generate_label_tac(tac_operand label);
extern tac_operand generate_binary_op(int op, tac_operand left, tac_operand right);
extern tac_operand generate_unary_op(int op, tac_operand operand);
extern tac_operand generate_field_load(tac_operand object, char* field);
extern tac_operand generate_new_object(char* class_name);
extern tac_operand generate_new_array(char* type, tac_operand size);

#endif /* TAC_H */