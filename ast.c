// ast.c - Implementation of AST functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

// Create a binary operation node
ast_node* create_binary_op_node(int op, ast_node* left, ast_node* right) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_BINARY_OP;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

// Create a unary operation node
ast_node* create_unary_op_node(int op, ast_node* expr) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.expr = expr;
    return node;
}

// Create an integer literal node
ast_node* create_int_literal_node(int value) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_LITERAL;
    node->data.literal.value_type = TYPE_INT;
    node->data.literal.value.int_val = value;
    return node;
}

// Create a float literal node
ast_node* create_float_literal_node(float value) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_LITERAL;
    node->data.literal.value_type = TYPE_FLOAT;
    node->data.literal.value.float_val = value;
    return node;
}

// Create an identifier node
ast_node* create_identifier_node(char* name) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_IDENTIFIER;
    node->data.identifier.name = strdup(name);
    return node;
}

// Create an assignment node
ast_node* create_assignment_node(ast_node* lhs, ast_node* rhs) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_ASSIGNMENT;
    node->data.assignment.lhs = lhs;
    node->data.assignment.rhs = rhs;
    return node;
}

// Create an if statement node
ast_node* create_if_node(ast_node* condition, ast_node* true_branch, ast_node* false_branch) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_IF_STMT;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.true_branch = true_branch;
    node->data.if_stmt.false_branch = false_branch;
    return node;
}

// Create a for loop node
ast_node* create_for_node(ast_node* init, ast_node* condition, ast_node* update, ast_node* body) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_FOR_STMT;
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.update = update;
    node->data.for_stmt.body = body;
    return node;
}

// Create a while loop node
ast_node* create_while_node(ast_node* condition, ast_node* body) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_WHILE_STMT;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

// Create a do-while loop node
ast_node* create_do_while_node(ast_node* body, ast_node* condition) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_DO_WHILE_STMT;
    node->data.do_while_stmt.condition = condition;
    node->data.do_while_stmt.body = body;
    return node;
}

// Create a return statement node
ast_node* create_return_node(ast_node* expr) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_RETURN_STMT;
    node->data.return_stmt.expr = expr;
    return node;
}

// Create a function call node
ast_node* create_call_node(char* function_name, ast_node** args, int arg_count) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_CALL;
    node->data.call.function_name = strdup(function_name);
    node->data.call.args = args;
    node->data.call.arg_count = arg_count;
    return node;
}

// Create a block node
ast_node* create_block_node(ast_node** statements, int statement_count) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_BLOCK;
    node->data.block.statements = statements;
    node->data.block.statement_count = statement_count;
    return node;
}

// Create a declaration node
ast_node* create_declaration_node(char* type, char* name, ast_node* init_expr) {
    ast_node* node = (ast_node*)malloc(sizeof(ast_node));
    node->type = AST_DECLARATION;
    node->data.declaration.type = strdup(type);
    node->data.declaration.name = strdup(name);
    node->data.declaration.init_expr = init_expr;
    return node;
}

// Free memory for an AST node and its children
void free_ast_node(ast_node* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_BINARY_OP:
            free_ast_node(node->data.binary_op.left);
            free_ast_node(node->data.binary_op.right);
            break;
            
        case AST_UNARY_OP:
            free_ast_node(node->data.unary_op.expr);
            break;
            
        case AST_LITERAL:
            if (node->data.literal.value_type == TYPE_STRING) {
                free(node->data.literal.value.string_val);
            }
            break;
            
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
            
        case AST_ASSIGNMENT:
            free_ast_node(node->data.assignment.lhs);
            free_ast_node(node->data.assignment.rhs);
            break;
            
        case AST_IF_STMT:
            free_ast_node(node->data.if_stmt.condition);
            free_ast_node(node->data.if_stmt.true_branch);
            if (node->data.if_stmt.false_branch) {
                free_ast_node(node->data.if_stmt.false_branch);
            }
            break;
            
        case AST_FOR_STMT:
            if (node->data.for_stmt.init) free_ast_node(node->data.for_stmt.init);
            if (node->data.for_stmt.condition) free_ast_node(node->data.for_stmt.condition);
            if (node->data.for_stmt.update) free_ast_node(node->data.for_stmt.update);
            free_ast_node(node->data.for_stmt.body);
            break;
            
        case AST_WHILE_STMT:
            free_ast_node(node->data.while_stmt.condition);
            free_ast_node(node->data.while_stmt.body);
            break;
            
        case AST_DO_WHILE_STMT:
            free_ast_node(node->data.do_while_stmt.condition);
            free_ast_node(node->data.do_while_stmt.body);
            break;
            
        case AST_RETURN_STMT:
            if (node->data.return_stmt.expr) {
                free_ast_node(node->data.return_stmt.expr);
            }
            break;
            
        case AST_CALL:
            free(node->data.call.function_name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                free_ast_node(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
            
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.statement_count; i++) {
                free_ast_node(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;
            
        case AST_DECLARATION:
            free(node->data.declaration.type);
            free(node->data.declaration.name);
            if (node->data.declaration.init_expr) {
                free_ast_node(node->data.declaration.init_expr);
            }
            break;
    }
    
    free(node);
}