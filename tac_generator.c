#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "intermediate_code.h"
#include "semantic_analysis.h"
#include "token_defs.h"
#include "tac_generator.h"
void set_current_method_type(const char *type_name);
// Global variable to track current method return type (simplified)
static type_info current_method_type = {TYPE_VOID, NULL, 0};

// Set current method return type (called from parser)
void set_current_method_type(const char *type_name)
{
    current_method_type.type_code = get_type_code(type_name);
    if (current_method_type.type_code == TYPE_OBJECT)
    {
        current_method_type.class_name = strdup(type_name);
    }
    current_method_type.is_array = 0;
}

// Forward declarations
tac_operand generate_expression(ast_node *node);
void generate_statement(ast_node *node);

// Convert AST binary operation to TAC operation
tac_op ast_binop_to_tac_op(int ast_op)
{
    switch (ast_op)
    {
    case PLUS:
        return OP_ADD;
    case MINUS:
        return OP_SUB;
    case MULTIPLY:
        return OP_MUL;
    case DIVIDE:
        return OP_DIV;
    case MODULO:
        return OP_MOD;
    case AND:
        return OP_AND;
    case OR:
        return OP_OR;
    case GREATER:
        return OP_GT;
    case LESS:
        return OP_LT;
    case GREATER_EQUAL:
        return OP_GE;
    case LESS_EQUAL:
        return OP_LE;
    case EQUAL:
        return OP_EQ;
    case NOT_EQUAL:
        return OP_NE;
    default:
        return OP_ADD; // Default case
    }
}

// Generate TAC for a binary operation expression with type checking
tac_operand generate_binary_op(ast_node *node)
{
    tac_operand left_operand = generate_expression(node->data.binary_op.left);
    tac_operand right_operand = generate_expression(node->data.binary_op.right);

    // Semantic check
    type_info left_type = {TYPE_INT, NULL, 0}; // Simplified: infer from operand type if needed
    type_info right_type = {TYPE_INT, NULL, 0};
    int result_type = check_binary_operation_type(node->data.binary_op.op, left_type, right_type);
    if (result_type == TYPE_ERROR)
    {
        return create_none();
    }

    tac_op op = ast_binop_to_tac_op(node->data.binary_op.op);
    return generate_binary_op(op, left_operand, right_operand);
}

// Generate TAC for a unary operation expression
tac_operand generate_unary_op(ast_node *node)
{
    tac_operand expr_operand = generate_expression(node->data.unary_op.expr);

    tac_op op;
    switch (node->data.unary_op.op)
    {
    case MINUS:
        op = OP_NEG;
        break;
    case NOT:
        op = OP_NOT;
        break;
    default:
        op = OP_NEG; // Default case
    }

    return generate_unary_op(op, expr_operand);
}

// Generate TAC for a literal expression
tac_operand generate_literal(ast_node *node)
{
    switch (node->data.literal.value_type)
    {
    case TYPE_INT:
        return create_int_literal(node->data.literal.value.int_val);
    case TYPE_FLOAT:
        return create_float_literal(node->data.literal.value.float_val);
    default:
        // Handle other types as needed
        return create_int_literal(0); // Default case
    }
}

// Generate TAC for an identifier expression
tac_operand generate_identifier(ast_node *node)
{
    return create_variable(node->data.identifier.name);
}

// Generate TAC for an assignment expression
tac_operand generate_assignment(ast_node *node)
{
    tac_operand lhs;
    if (node->data.assignment.lhs->type == AST_IDENTIFIER)
    {
        lhs = create_variable(node->data.assignment.lhs->data.identifier.name);
        type_info lhs_type = get_identifier_type(node->data.assignment.lhs->data.identifier.name);
        tac_operand rhs = generate_expression(node->data.assignment.rhs);
        type_info rhs_type = {TYPE_INT, NULL, 0}; // Simplified: infer from rhs

        if (!check_assignment_compatibility(lhs_type, rhs_type))
        {
            return create_none();
        }
        generate_assignment(lhs, rhs);
        return rhs;
    }
    fprintf(stderr, "Error: Left-hand side of assignment must be an identifier\n");
    return create_none();
}

// Generate TAC for a function call
tac_operand generate_function_call(ast_node *node)
{
    // Generate code for each argument
    for (int i = 0; i < node->data.call.arg_count; i++)
    {
        tac_operand arg = generate_expression(node->data.call.args[i]);
        generate_param(arg);
    }

    // Generate the function call
    return generate_call(node->data.call.function_name, node->data.call.arg_count);
}

// Generate TAC for any expression
tac_operand generate_expression(ast_node *node)
{
    switch (node->type)
    {
    case AST_LITERAL:
        return create_int_literal(node->data.literal.value.int_val);
    case AST_IDENTIFIER:
        return create_variable(node->data.identifier.name);
    case AST_BINARY_OP:
    {
        tac_operand left = generate_expression(node->data.binary_op.left);
        tac_operand right = generate_expression(node->data.binary_op.right);
        return generate_binary_op(token_to_tac_op(node->data.binary_op.op), left, right);
    }
    }
    return create_none();
}

// Generate TAC for an if statement
void generate_if_statement(ast_node *node)
{
    // Generate code for the condition
    tac_operand condition = generate_expression(node->data.if_stmt.condition);

    // Create labels for true and end branches
    tac_operand true_label = create_label();
    tac_operand end_label = create_label();

    // Generate if condition check
    generate_if(condition, true_label);

    // Generate false branch if it exists
    if (node->data.if_stmt.false_branch)
    {
        generate_statement(node->data.if_stmt.false_branch);
    }

    // Jump to end
    generate_goto(end_label);

    // True branch
    generate_label_tac(true_label);
    generate_statement(node->data.if_stmt.true_branch);

    // End label
    generate_label_tac(end_label);
}

// Generate TAC for a while loop
void generate_while_statement(ast_node *node)
{
    // Create labels for loop condition and end
    tac_operand cond_label = create_label();
    tac_operand end_label = create_label();

    // Loop condition
    generate_label_tac(cond_label);
    tac_operand condition = generate_expression(node->data.while_stmt.condition);

    // If condition is false, exit loop
    generate_ifnot(condition, end_label);

    // Loop body
    generate_statement(node->data.while_stmt.body);

    // Jump back to condition check
    generate_goto(cond_label);

    // End label
    generate_label_tac(end_label);
}

// Generate TAC for a do-while loop
void generate_do_while_statement(ast_node *node)
{
    // Create labels for loop start and condition
    tac_operand start_label = create_label();
    tac_operand cond_label = create_label();

    // Loop start
    generate_label_tac(start_label);

    // Loop body
    generate_statement(node->data.do_while_stmt.body);

    // Loop condition
    generate_label_tac(cond_label);
    tac_operand condition = generate_expression(node->data.do_while_stmt.condition);

    // If condition is true, jump back to start
    generate_if(condition, start_label);
}

// Generate TAC for a for loop
void generate_for_statement(ast_node *node)
{
    // Create labels for loop condition, update, and end
    tac_operand cond_label = create_label();
    tac_operand update_label = create_label();
    tac_operand end_label = create_label();

    // Generate initialization code if it exists
    if (node->data.for_stmt.init)
    {
        generate_expression(node->data.for_stmt.init);
    }

    // Loop condition
    generate_label_tac(cond_label);
    if (node->data.for_stmt.condition)
    {
        tac_operand condition = generate_expression(node->data.for_stmt.condition);
        generate_ifnot(condition, end_label);
    }

    // Loop body
    generate_statement(node->data.for_stmt.body);

    // Update expression
    generate_label_tac(update_label);
    if (node->data.for_stmt.update)
    {
        generate_expression(node->data.for_stmt.update);
    }

    // Jump back to condition check
    generate_goto(cond_label);

    // End label
    generate_label_tac(end_label);
}

// Generate TAC for a return statement
void generate_return_statement(ast_node *node)
{
    if (node->data.return_stmt.expr)
    {
        tac_operand return_value = generate_expression(node->data.return_stmt.expr);
        type_info return_type = {TYPE_INT, NULL, 0}; // Simplified: infer from expr
        if (!check_return_type(return_type, current_method_type))
        {
            return;
        }
        generate_return(return_value);
    }
    else
    {
        type_info void_type = {TYPE_VOID, NULL, 0};
        if (!check_return_type(void_type, current_method_type))
        {
            return;
        }
        generate_return(create_none());
    }
}
// Generate TAC for a block of statements
void generate_block(ast_node *node)
{
    for (int i = 0; i < node->data.block.statement_count; i++)
    {
        generate_statement(node->data.block.statements[i]);
    }
}

// Generate TAC for a variable declaration
void generate_declaration(ast_node *node)
{
    // If there's an initialization expression
    if (node->data.declaration.init_expr)
    {
        tac_operand var = create_variable(node->data.declaration.name);
        tac_operand init_value = generate_expression(node->data.declaration.init_expr);
        generate_assignment(var, init_value);
    }
    // Otherwise, nothing to do for simple declarations at the TAC level
}

// Generate TAC for any statement
void generate_statement(ast_node *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case AST_IF_STMT:
        generate_if_statement(node);
        break;
    case AST_WHILE_STMT:
        generate_while_statement(node);
        break;
    case AST_DO_WHILE_STMT:
        generate_do_while_statement(node);
        break;
    case AST_FOR_STMT:
        generate_for_statement(node);
        break;
    case AST_RETURN_STMT:
        generate_return_statement(node);
        break;
    case AST_BLOCK:
        generate_block(node);
        break;
    case AST_DECLARATION:
        generate_declaration(node);
        break;
    case AST_ASSIGNMENT:
    case AST_CALL:
        // Expressions used as statements
        generate_expression(node);
        break;
    default:
        fprintf(stderr, "Error: Unsupported statement type in TAC generation\n");
        break;
    }
}

// Main entry point for generating TAC from an AST
void generate_tac_from_ast(ast_node *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case AST_ASSIGNMENT:
    {
        tac_operand rhs = generate_expression(node->data.assignment.rhs);
        tac_operand lhs = create_variable(node->data.assignment.lhs->data.identifier.name);
        generate_assignment(lhs, rhs);
        break;
    }
    case AST_IF_STMT:
    {
        tac_operand cond = generate_expression(node->data.if_stmt.condition);
        tac_operand true_label = create_label();
        tac_operand end_label = create_label();

        generate_if(cond, true_label);
        generate_tac_from_ast(node->data.if_stmt.true_branch);
        generate_goto(end_label);
        generate_label_tac(true_label);
        if (node->data.if_stmt.false_branch)
        {
            generate_tac_from_ast(node->data.if_stmt.false_branch);
        }
        generate_label_tac(end_label);
        break;
    }
        // Add more control structures
    }
}