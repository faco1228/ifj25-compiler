/**
 * @file ast.h
 * @author Samuel Facka (xfackas00), 
 *         Martin Racek (xracekm00)
 * @brief Abstract Syntax Tree (AST) structures and builder functions.
 *
 * The AST represents the parsed IFJ25 program in a structured tree form.
 * Each node has:
 *  - a NodeType describing its role (statement, expression, literal, ...),
 *  - an array of children (for tree structure),
 *  - a union with node-specific data.
 *
 * @version 0.1
 * @date 2025-11-10
 *
 * @copyright Copyright (c) 2025
 */

#ifndef _AST_H_
#define _AST_H_

#include "error.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Supported operators for expressions.
 */
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
    OP_RANGE,
    OP_ERROR
} operator_types;

/**
 * @brief Function type: normal function, getter, or setter.
 */
typedef enum
{
    FUN_F, // normal function
    FUN_G, // getter
    FUN_S  // setter
} function_type;

/**
 * @brief Static type of an expression (used mainly during semantic analysis).
 */
typedef enum
{
    TYPE_UNKNOWN,
    TYPE_NUM,
    TYPE_STRING,
    TYPE_BOOL
} value_type;

/**
 * @brief All node types used inside the AST.
 */
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

/**
 * @brief Single AST node.
 *
 * Common rules for children:
 *  - For NODE_FUNCTION_DEF, children are: [0..arg_count-1] params, [arg_count] body.
 *  - For NODE_IF: [0] condition, [1] then-block, [2] else-block (if present).
 *  - For NODE_WHILE: [0] condition, [1] body.
 *  - For NODE_FOR: [0] iterator identifier, [1] range/expression, [2] body.
 *  - For NODE_ASSIGN: [0] lhs, [1] rhs.
 *  - For NODE_TERNARY: [0] condition, [1] then-expression, [2] else-expression.
 *  - For NODE_RANGE: [0] start, [1] end.
 *  - For NODE_EXPR_STMNT: [0] expression.
 */
struct ASTNode
{
    NodeType type;

    // Structure of the tree
    ASTNode_ptr *children; // Dynamic array of child pointers
    size_t child_count;    // Number of children for index access

    // Different data one node can store
    union
    {
        // Identifier usage and variable declarations
        struct
        {
            char *name;
            char *code_gen_name;
            ID_Type id_type;
            bool is_global;
            bool declared_in_loop;
        } identifier;

        // Function definition
        struct
        {
            char *name;
            unsigned arg_count; // Number of parameters
            function_type type; // Normal function/getter/setter
        } function_def;

        // Function call
        struct
        {
            char *name;           // Function name, normal or built-in
            unsigned param_count; // Number of call arguments
            bool is_builtin;      // Is true for Ifj.* built-in functions
        } function_call;

        // Binary operations
        struct
        {
            operator_types op_type; // plus, minus, equal, not equal, ...
        } binary_operator;

        // For loops
        struct
        {
            char *name_iter; // Iterator var name
        } for_statement;

        // Expression statement wrapper
        struct
        {
            value_type exp_type; // Type of expression if known, else TYPE_UNKNOWN
        } exp_statement;

        // Range (required in for loops)
        struct
        {
            bool inclusive; // true: a..b includes end, false: a...b excludes end
        } range;

        // Literals
        struct
        {
            union
            {
                long long int int_val;
                long double float_val;
                char *str_value;
            } data;
        } literal;
    } data;
};

/**
 * @brief Wrapper around the AST root (not heavily used at the moment).
 */
typedef struct
{
    ASTNode_ptr root;
} ASTree;

/*******************  Core functions declarations ********************/

/**
 * @brief Allocate and initialize a new AST node of the given type.
 *
 * @param type Node type to assign to the new node.
 * @return Newly allocated AST node.
 * @note On allocation error, calls error_exit(ERR_INTERNAL).
 */
ASTNode_ptr ast_create(NodeType type);

/**
 * @brief Append a child to a parent node.
 *
 * @param parent Parent node (must not be NULL).
 * @param child  Child node to append (must not be NULL).
 */
void add_child(ASTNode_ptr parent, ASTNode_ptr child);

/**
 * @brief Recursively free an AST subtree.
 *
 * Frees:
 *  - all children,
 *  - node-specific dynamically allocated strings,
 *  - the node itself.
 *
 * @param node Root of the subtree to free (can be NULL).
 */
void ast_free(ASTNode_ptr node);

/*******************  AST builder functions declarations - parser ********************/

/**
 * @brief Create the root node of the whole program (NODE_PROGRAM).
 */
ASTNode_ptr ast_create_program(void);

/**
 * @brief Create a function definition node (NODE_FUNCTION_DEF).
 *
 * @param name Name of the function (string is copied).
 * @param args Number of parameters the function takes.
 * @param type Kind of function (FUN_F, FUN_G, FUN_S).
 * @param body Body node (usually NODE_BLOCK), may be NULL and added later.
 *
 * @return New function definition node.
 */
ASTNode_ptr ast_create_function(const char *name, unsigned args, function_type type, ASTNode_ptr body);

/**
 * @brief Create an empty block node (NODE_BLOCK).
 */
ASTNode_ptr ast_create_block(void);

/**
 * @brief Create a variable declaration node (NODE_VAR_DECL).
 *
 * @param name Variable name (string is copied).
 */
ASTNode_ptr ast_create_var_dec(const char *name);

/**
 * @brief Create an assignment node (NODE_ASSIGN).
 *
 * @param lhs Left-hand side expression (e.g. identifier).
 * @param rhs Right-hand side expression.
 */
ASTNode_ptr ast_create_assignment(ASTNode_ptr lhs, ASTNode_ptr rhs);

/**
 * @brief Create an if-else node (NODE_IF).
 *
 * @param cond   Condition expression.
 * @param b_then Then-branch block.
 * @param b_else Else-branch block (may be NULL).
 */
ASTNode_ptr ast_create_if(ASTNode_ptr cond, ASTNode_ptr b_then, ASTNode_ptr b_else);

/**
 * @brief Create a return statement node (NODE_RETURN).
 *
 * @param val Optional expression to return (may be NULL).
 */
ASTNode_ptr ast_create_return(ASTNode_ptr val);

/**
 * @brief Create a while loop node (NODE_WHILE).
 *
 * @param cond Condition expression.
 * @param body Loop body block.
 */
ASTNode_ptr ast_create_while(ASTNode_ptr cond, ASTNode_ptr body);

/**
 * @brief Create a for loop node (NODE_FOR).
 *
 * @param name Iterator variable name.
 * @param iter Range/expression node used for iteration.
 * @param body Loop body block.
 */
ASTNode_ptr ast_create_for(const char *name, ASTNode_ptr iter, ASTNode_ptr body);

/**
 * @brief Create a break statement node (NODE_BREAK).
 */
ASTNode_ptr ast_create_break(void);

/**
 * @brief Create a continue statement node (NODE_CONTINUE).
 */
ASTNode_ptr ast_create_continue(void);

/**
 * @brief Wrap an expression into a redundand statement node (NODE_EXPR_STMNT).
 *
 * @param exp Expression node.
 */
ASTNode_ptr ast_create_exp_statement(ASTNode_ptr exp);

/**
 * @brief Create an identifier node (NODE_IDENTIFIER).
 *
 * @param name      Identifier name (string is copied).
 * @param is_global True if this refers to a global variable.
 */
ASTNode_ptr ast_create_ident(const char *name, bool is_global);

/*******************  AST builder functions declarations - psa ********************/

/**
 * @brief Create a binary operator node (NODE_BINARY_OP).
 *
 * @param lhs Left-hand side operand.
 * @param rhs Right-hand side operand.
 * @param op  Operator type.
 */
ASTNode_ptr ast_create_binary(ASTNode_ptr lhs, ASTNode_ptr rhs, operator_types op);

/**
 * @brief Create a function call node (NODE_CALL).
 *
 * @param name     Function name (string is copied).
 * @param param_c  Number of arguments (will be updated later if needed).
 * @param builtin  True for Ifj.* built-in calls.
 */
ASTNode_ptr ast_create_call(const char *name, unsigned param_c, bool builtin);

/**
 * @brief Create a range node (NODE_RANGE).
 *
 * @param l         Start expression.
 * @param r         End expression.
 * @param inclusive True for inclusive range (a..b), false for exclusive (a...b).
 */
ASTNode_ptr ast_create_range(ASTNode_ptr l, ASTNode_ptr r, bool inclusive);

/**
 * @brief Create an integer literal node (NODE_INT_LIT).
 *
 * @param val Integer value.
 */
ASTNode_ptr ast_create_int(long long int val);

/**
 * @brief Create a float literal node (NODE_FLOAT_LIT).
 *
 * @param val Floating-point value.
 */
ASTNode_ptr ast_create_float(long double val);

/**
 * @brief Create a string literal node (NODE_STR_LIT).
 *
 * @param string Null-terminated string (copied).
 */
ASTNode_ptr ast_create_str(const char *string);

/**
 * @brief Create a null literal node (NODE_NULL_LIT).
 */
ASTNode_ptr ast_create_null(void);

/**
 * @brief Create a type literal node (NODE_TYPE_LIT).
 *
 * @param type_name Type name (Num/String/Null) – string is copied.
 */
ASTNode_ptr ast_create_type_lit(const char *type_name);

#endif