/**
 * @file parser_expression.h
 * @authors Samuel Facka (xfackas00),
 *          Kristian Cilling (xcillik00)
 * @brief Precedence syntax analyzer (PSA) interface for expressions.
 * 
 * The PSA parses expressions into AST using:
 *  - a precedence table,
 *  - a custom Stack,
 *  - internal marker tokens (MARKER, NONTERMINAL_E),
 *  - helper functions for building AST nodes.
 * 
 * @version 0.1
 * @date 2025-11-23
 *
 * @copyright Copyright (c) 2025
 */

#ifndef _PARSER_EXPRESSION_H_
#define _PARSER_EXPRESSION_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "stack.h"
#include "ast.h"
#include "error.h"
#include "parser.h"


/**
 * @brief Internal token->type marker used on PSA stack for the '<' marker.
 * 
 * This value is not from the scanner; it is only used inside the expression parser.
 */
#define MARKER 13245

/**
 * @brief Internal token->type marker used on PSA stack for nonterminal E.
 *
 * NONTERMINAL_E tokens store pointer to the root AST of the reduced expression in token->ast.
 */
#define NONTERMINAL_E 123456789

/**
 * @brief Relation between top-of-stack operator and incoming operator
 *        in the precedence table.
 */
typedef enum relation
{
    psa_shift,     // <
    psa_reduce,    // >
    psa_eq_reduce, // =
    psa_error,     // error
    psa_finish     // comparing two $
} precedence_relation;

/**
 * @brief Indices into the precedence table for groups of operators/tokens.
 */
typedef enum
{
    OP_ADD_SUB,       // + and -
    OP_MUL_DIV,       // * and /
    OP_LOWER,         // <
    OP_GREATER,       // >
    OP_LOWER_EQUAL,   // <=
    OP_GREATER_EQUAL, // >=
    OP_EQUAL,         // ==
    OP_NOT_EQUAL,     // !=
    OP_LPAR,          // (
    OP_RPAR,          // )
    OP_OPERAND,       // i (literal, ident, ...)
    OP_IS_TOK,        // is
    OP_D_DOT,         // ..
    OP_T_DOT,         // ...
    OP_END,           // $
    OP_UNRECOGNISED
} precedence_index;

/**
 * @brief Parse an expression using the precedence syntax analyzer.
 *
 * Different contexts are handled using the recognition token:
 *  - NULL : standalone expression terminated by EOL, ',', ')' or EOF.
 *  - KEY_WORD "return" : right side of return.
 *  - KEY_WORD "in" : expression inside for (... in <expr>).
 *  - OPERATOR '=' : right-hand side of assignment.
 *  - LEFT_PAR '(' : expression inside parentheses.
 *
 * The function:
 *  - builds a PSA stack with '$' sentinel,
 *  - repeatedly uses the precedence table to shift/reduce,
 *  - at the end returns AST root of the expression.
 *
 * @param recognition_token
 *        Optional token describing the context where the expression starts.
 *        Ownership is taken and it will be freed inside this function.
 *        Pass NULL when parsing a standalone expression.
 *
 * @return AST node pointer representing the root of the parsed expression.
 *         On syntax or internal errors, error_exit() is called.
 */
ASTNode_ptr parse_expression(token_ptr recognition_token);

#endif