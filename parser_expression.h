/**
 * @file psa.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <stdbool.h>
#include "scanner.h"
#include "stack.h"
#include "ast.h"
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "scanner.h"
#include "stack.h"
#include "parser.h"

#define MARKER 132456     // interný token type pre '<' marker na zásobníku
#define NONTERMINAL_E 123456789  // interný token type pre zredukovaný výraz E

typedef enum relation {
    psa_shift,          // <
    psa_reduce,         // >
    psa_eq_reduce,      // =
    psa_error,          // error
    psa_finish          // comparing two $
} precedence_relation;


typedef enum {
    OP_ADD_SUB,         // + and -
    OP_MUL_DIV,         // * and /
    OP_LOWER,           // <
    OP_GREATER,         // >
    OP_LOWER_EQUAL,     // <=
    OP_GREATER_EQUAL,   // >=
    OP_EQUAL,           // ==
    OP_NOT_EQUAL,       // !=
    OP_LPAR,            // (
    OP_RPAR,            // )
    OP_OPERAND,         // i (literal, ident, ...)
    OP_IS_TOK,          // is
    OP_D_DOT,           // ..
    OP_T_DOT,           // ...
    OP_END,             // $
    OP_UNRECOGNISED
} precedence_index;

ASTNode_ptr parse_expression(token_ptr recognition_token);
