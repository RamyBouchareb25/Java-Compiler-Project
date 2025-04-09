// ast.h
#ifndef AST_H
#define AST_H

#include "token_defs.h"

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_BOOLEAN,
    TYPE_VOID,
    TYPE_OBJECT
} type_code;

typedef struct ast_node {
    int type;  // Use AST_* constants from token_defs.h
    union {
        struct {
            int op;
            struct ast_node* left;
            struct ast_node* right;
        } binary_op;
        struct {
            int op;
            struct ast_node* expr;
        } unary_op;
        struct {
            type_code value_type;
            union {
                int int_val;
                float float_val;
                char char_val;
            } value;
        } literal;
        struct {
            char* name;
        } identifier;
        struct {
            struct ast_node* lhs;
            struct ast_node* rhs;
        } assignment;
        struct {
            char* function_name;
            struct ast_node** args;
            int arg_count;
        } call;
        struct {
            struct ast_node* condition;
            struct ast_node* true_branch;
            struct ast_node* false_branch;
        } if_stmt;
        struct {
            struct ast_node* condition;
            struct ast_node* body;
        } while_stmt;
        struct {
            struct ast_node* body;
            struct ast_node* condition;
        } do_while_stmt;
        struct {
            struct ast_node* init;
            struct ast_node* condition;
            struct ast_node* update;
            struct ast_node* body;
        } for_stmt;
        struct {
            struct ast_node* expr;
        } return_stmt;
        struct {
            struct ast_node** statements;
            int statement_count;
        } block;
        struct {
            char* name;
            struct ast_node* init_expr;
        } declaration;
    } data;
} ast_node;

ast_node* create_binary_op_node(int op, ast_node* left, ast_node* right);
ast_node* create_unary_op_node(int op, ast_node* expr);
ast_node* create_literal_node(type_code value_type, void* value);
ast_node* create_identifier_node(const char* name);
ast_node* create_assignment_node(ast_node* lhs, ast_node* rhs);
ast_node* create_call_node(const char* function_name, ast_node** args, int arg_count);
ast_node* create_if_node(ast_node* condition, ast_node* true_branch, ast_node* false_branch);
ast_node* create_while_node(ast_node* condition, ast_node* body);
ast_node* create_do_while_node(ast_node* body, ast_node* condition);
ast_node* create_for_node(ast_node* init, ast_node* condition, ast_node* update, ast_node* body);
ast_node* create_return_node(ast_node* expr);
ast_node* create_block_node(ast_node** statements, int statement_count);
ast_node* create_declaration_node(const char* name, ast_node* init_expr);
void free_ast_node(ast_node* node);

#endif /* AST_H */