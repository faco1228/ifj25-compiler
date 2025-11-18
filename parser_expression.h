/**
 * @file parser_expression.h
 * @author xcillik00
 * @brief Header file for precedence expression parser
 * @version 0.1
 * @date 2025-10-27
 */

#ifndef PARSER_EXPRESSION_H
#define PARSER_EXPRESSION_H

#include <stdbool.h>
#include "scanner.h"
#include "stack.h"



/**
 * @brief Internal token type for precedence parser marker ('<')
 * This value is not from the scanner; it is only used inside the expression parser.
 */

#define MARKER 132456  // Must not collide with real token types


/**
 * @brief Nonterminal E used to represent reduced expressions on the stack
 */

#define NONTERMINAL_E 123456789


/**
 * @brief Parses and checks syntax of expression.
 * 
 */
bool parse_expression(token_ptr recognition_token );


/**
 * @brief Compares current token with the one on the top of the Stack
 * 
 */
bool precedence_table_compare ( Stack *stack , token_ptr current_token, token_ptr *top_terminal);


/**
 * @brief  Performs reduction when precedence_table gives '>'
 * Pops tokens until '<' marker and replaces recognized handle with NONTERMINAL_E.
 * 
 */ 
void precedence_reduce_func(Stack *stack);


/**
 * @brief Will set relation between current Token and the Stack top token
 * 
 */
typedef enum relation {
    precedence_shift,   // <
    precedence_reduce,   // >
    precedence_equal_reduce,   // =
    precedence_error,   // error
    precedence_finish // comparing two $
} precedence_relation;


typedef enum {
    OP_ADD,             // +
    OP_SUB,             // -
    OP_MULT,             // *
    OP_DIVI,             // /
    OP_LOWER,           // <
    OP_GREATER,         // >
    OP_LOWER_EQUAL,     // <=
    OP_GREATER_EQUAL,   // >=
    OP_EQUAL,           // ==
    OP_NOT_EQUAL,       // !=
    OP_LPAR,            // (
    OP_RPAR,            // )
    OP_OPERAND,         // literal, identifikátor, getter
    OP_END,              // $
    OP_UNRECOGNISED
} precedence_index;


#endif