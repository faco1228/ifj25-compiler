/**
 * @file chat.c
 * @author xracekm00 and xmezeim00
 * @brief
 * @version 0.1
 * @date 2025-11-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "../scanner.h"

enum operator_types
{
    PLUS_V,
    MINUS_V,
    SLASH_V,
    STAR_V,
    EQUAL_SIGN_V,
    LESS_THAN_V,
    GREATER_THAN_V,
    LESS_OR_EQ_THAN_V,
    GREATER_OR_EQ_THAN_V,
    LOGICAL_EQUAL_V,
    LOGICAL_NOT_EQUAL_V
};

// Data type representing different types of AST nodes
typedef enum
{
    // program structure
    NODE_PROGRAM,
    NODE_FUNCTION_DEF,
    NODE_GETTER_DEF,
    NODE_SETTER_DEF,

    // stmts
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_FOR,

    NODE_RANGE_2DOT, // !might change
    NODE_RANGE_3DOT, // !might change

    // expression
    NODE_CALL, // only if we decide to do FUNEXP extension
    NODE_FUNC_PARAM,
    NODE_IDENTIFIER,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_IS_EXPR,

    // Literals
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STRING_LIT,
    NODE_NULL_LIT
} NodeType;

// Data type of AST node
typedef struct ASTNode
{
    NodeType type;

    // only used when traversing expression subtrees via inv postorder traversal
    // expression subtrees are binary
    // struct ASTNode *params;
    // struct ASTNode *body;
    struct ASTNode *left;
    struct ASTNode *right;

    // used with other node types
    struct ASTNode **children; // pole ukazatelov na children nodes
    size_t child_count;        // pocet prvkov pola pre lahsi priamy pristup

    // different data one node can store
    union
    {
        struct
        {
            char *name;
        } identifier;

        // todo : doriesit function typy
        struct
        {
            char *name;
            // struct ASTNode *params;
            // struct ASTNode *body;
        } function_def;

        // todo : doriesit function typy
        struct
        {
            char *func_name;
            unsigned argc;
        } function_call;

        struct
        {
            long double float_value;
            long long int int_value;
            char *str_value;
            bool bool_value; // used with NODE_IS_EXPR
        } literal;

        struct
        {
            enum operator_types operator_type;
        } binary_operator;


    // todo : doriesit
    // NODE_GETTER_DEF,
    // NODE_SETTER_DEF,

    // // stmts
    // NODE_VAR_DECL,
    // NODE_ASSIGN,
    
    // NODE_RANGE_2DOT, // !might change
    // NODE_RANGE_3DOT, // !might change

    // // expression
    // NODE_CALL, // only if we decide to do FUNEXP extension
    // NODE_FUNC_PARAM, 


    } data;
} ASTNode;

// AST root data type
typedef struct
{
    ASTNode *root;
} ASTree;

/*
PROGRAM
└── CLASS(Program)
    ├── FUNCTION(main)
    │   └── BLOCK
    │       ├── ASSIGN(__r1)
    │       │   └── CALL(foo)
    │       │       └── ARG(5)
    │       ├── ASSIGN(__r2)
    │       │   └── CALL(foo)
    │       │       ├── ARG(2)
    │       │       └── ARG(3)
    │       └── ASSIGN(__d)
    │           └── CALL(Ifj.write)
    │               └── ARG(BINARY_OP '+')
    │                   ├── ID(__r1)
    │                   └── ID(__r2)
    ├── FUNCTION(foo_1param)
    │   └── RETURN(BINARY_OP '+')
    │       ├── ID(x)
    │       └── LITERAL(10)
    └── FUNCTION(foo_2param)
        └── RETURN(BINARY_OP '+')
            ├── ID(x)
            └── ID(y)
*/

// Prototype of traversal function
void generate_code(ASTNode *n)
{
    if (!n)
        return;

    switch (n->type)
    {
    case NODE_ASSIGN:
        generate_code(n->right);
        printf("POPS %s\n", n->left->data.identifier.name);
        break;
    case NODE_BINARY_OP:
        generate_code(n->left);
        generate_code(n->right);
        if (strcmp(n->data.binary.op, "+") == 0)
            printf("ADD\n");
        break;
    case NODE_CALL:
        for (size_t i = 0; i < n->child_count; i++)
            generate_code(n->children[i]);
        printf("CALL %s\n", n->data.call.func_name);
        break;
    default:
        break;
    }
}
