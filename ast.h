/**
 * @file ast.h
 * @author Samuel Facka (xfackas00)
 * @brief
 * @version 0.1
 * @date 2025-11-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include "error.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef enum
{
    ONLY_NUM,
    ONLY_STR,
    UNDETERMINED
} exp_restriction_t;

// Data type representing values of logical and aritmetical operators
typedef enum
{
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_IS,
    OP_ERROR
} operator_types;

// Data type for function/getter/setter
typedef enum
{
    FUN_F,
    FUN_G,
    FUN_S
} function_type;

// data types for expression
typedef enum
{
    TYPE_UNKNOWN,
    TYPE_NUM,
    TYPE_STRING,
    TYPE_BOOL
} ValueType;

// Data type representing different types of AST nodes
typedef enum
{
    // Basic program structure and declarations
    NODE_PROGRAM,
    NODE_FUNCTION_DEF,
    NODE_BLOCK,

    // Statements
    NODE_VAR_DECL,
    NODE_ASSIGN,
    NODE_IF,
    NODE_RETURN,
    NODE_WHILE,
    NODE_FOR,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_EXPR_STMNT,

    // Expressions
    NODE_IDENTIFIER,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_CALL,
    NODE_TERNARY,
    NODE_RANGE,

    // Literals
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STR_LIT,
    NODE_NULL_LIT,
    NODE_TYPE_LIT,
} NodeType;

// Forward declaration and pointer alias for the ASTNode structure
typedef struct ASTNode ASTNode_t, *ASTNode_ptr;

// Data type representing AST node
struct ASTNode
{
    NodeType type;

    // used with other node types
    ASTNode_ptr *children; // pole ukazatelov na children nodes
    size_t child_count;    // pocet prvkov pola pre lahsi priamy pristup

    // different data one node can store
    union
    {
        // IDENT, VAR_DECL
        struct
        { // bool is_initialized
            char *name;
            ID_Type id_type;
        } identifier;

        // FUNCTION_DEF
        struct
        {
            char *name;
            unsigned arg_count;
            function_type type;
            // ASTNode_ptr body; // do children [0]
        } function_def;

        // CALL - keep as is
        struct
        {
            char *name;           // fun() or Ifj.write()
            unsigned param_count; // num of args
            bool is_builtin;      // true for IFj.*
        } function_call;

        // // ASSIGN
        // struct
        // {
        //     // ASTNode_ptr lhs; // typicky NODE_IDENTIFIER // children
        //     // ASTNode_ptr rhs; // expression // children
        // } assign;

        // BINARY operation
        struct
        {
            // ASTNode_ptr lhs;  // children [0]
            // ASTNode_ptr rhs;  // children [1]
            operator_types op_type;
        } binary_operator;

        // UNARY operation
        struct
        {
            // ASTNode_ptr expres; // children
            operator_types op_type; // unary minus - OP_MINUS
        } unary_operator;

        // IF
        // struct
        // {
        //     // ASTNode_ptr condition; // children [0]
        //     // ASTNode_ptr block_then; // NODE_BLOCK // children [1]
        //     // ASTNode_ptr block_else; // NODE_BLOCK or NULL // children [2]
        // } if_statement;

        // WHILE
        // struct
        // {
        //     // ASTNode_ptr condition; // children [0]
        //     // ASTNode_ptr body; // NODE_BLOCK children [1]
        // } while_statement;

        // FOR
        struct
        {
            char *name_iter;
            // ASTNode_ptr expr_iter; // NODE_RANGE // children [0]
            // ASTNode_ptr body; // children [1]
        } for_statement;

        // RETURN
        // struct
        // {
        //     // ASTNode_ptr value; // children [0]
        // } ret;

        // EXPRESION statement;
        struct
        { // 0-left 1-right
            // ASTNode_ptr exp; // children [0]
            ValueType result_type;
            exp_restriction_t restriction; 
        } exp_statement;

        // RANGE
        struct
        {
            // ASTNode_ptr start; // children [0]
            // ASTNode_ptr stop; // children [1]
            bool inclusive; // true: a..b (inclusive, "<a,b>"); false: a...b (excluisive, "<a,b)")
        } range;

        // TERNARY
        // struct
        // {
        //     // ASTNode_ptr condition; // children [0]
        //     // ASTNode_ptr expr_then; // children [1]
        //     // ASTNode_ptr expr_else; // children [2]
        // } ternary;

        // LITERAL
        struct
        {
            union
            { // prerobit na union
                long long int int_val;
                long double float_val;
                char *str_value;
            } data;
        } literal;

    } data;
};

// Data type representing AST root
typedef struct
{
    ASTNode_ptr root;
} ASTree;

////////// functions declarations //////////

// core functions
ASTNode_ptr ast_create(NodeType type);
void add_child(ASTNode_ptr parent, ASTNode_ptr child);
void ast_free(ASTNode_ptr node);

// walk-through
// void ast_walk(ASTNode_ptr root);

// builders - parser
ASTNode_ptr ast_create_program(void);
ASTNode_ptr ast_create_function(const char *name, unsigned args, function_type type, ASTNode_ptr body);
ASTNode_ptr ast_create_block(void);
ASTNode_ptr ast_create_var_dec(const char *name);
ASTNode_ptr ast_create_assignment(ASTNode_ptr lhs, ASTNode_ptr rhs);
ASTNode_ptr ast_create_if(ASTNode_ptr cond, ASTNode_ptr b_then, ASTNode_ptr b_else);
ASTNode_ptr ast_create_return(ASTNode_ptr val);
ASTNode_ptr ast_create_while(ASTNode_ptr cond, ASTNode_ptr body);
ASTNode_ptr ast_create_for(const char *name, ASTNode_ptr iter, ASTNode_ptr body);
ASTNode_ptr ast_create_break(void);
ASTNode_ptr ast_create_continue(void);
ASTNode_ptr ast_create_exp_statement(ASTNode_ptr exp);
ASTNode_ptr ast_create_ident(const char *name);

// builders - PSA
ASTNode_ptr ast_create_binary(ASTNode_ptr lhs, ASTNode_ptr rhs, operator_types op);
ASTNode_ptr ast_create_unary(operator_types op, ASTNode_ptr exp);
ASTNode_ptr ast_create_call(const char *name, unsigned param_c, bool builtin);
ASTNode_ptr ast_create_ternary(ASTNode_ptr cond, ASTNode_ptr b_then, ASTNode_ptr b_else);
ASTNode_ptr ast_create_range(ASTNode_ptr l, ASTNode_ptr r, bool inclusive);
ASTNode_ptr ast_create_int(long long int val);
ASTNode_ptr ast_create_float(long double val);
ASTNode_ptr ast_create_str(const char *string);
ASTNode_ptr ast_create_null(void);
ASTNode_ptr ast_create_type_lit(const char *type_name);
