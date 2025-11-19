/**
 * @file parser.h
 * @author Samuel Facka (xfackas00)
 * @brief 
 * @version 0.1
 * @date 2025-10-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "error.h"
#include "ast.h"
#include "scope_stack.h"
#include "symtable.h"
#include "semantic_analysis.h"
#include "parser_expression.h"
#include <stdio.h>
#include <string.h>

#define PARSE_OK 0
#define PARSE_ERROR ERR_SYNTACTIC

/**
 * @brief Parse the entire IFJ25 program using the scanner.
 *
 * Grammar:
 * @code
 * <program> ::= <prolog> <class_def> EOF
 * @endcode
 *
 * @note
 *  - Uses tokens produced by the scanner.
 *  - On successful parse, calls @c scanner_cleanup().
 *  - On a syntactic error, calls @c error_exit(ERR_SYNTACTIC).
 *
 * @return 0 (PARSE_OK) on success. On syntax error the function does not return,
 *         because @c error_exit(ERR_SYNTACTIC) terminates the program.
 */
ASTNode_ptr parse_program(void);

/**
 * @brief Parse a comma-separated parameter list.
 *
 * Grammar:
 * @code
 * <param_list> ::= ε | ID ( "," ID )*
 * @endcode
 *
 * @note
 *  - EOLs are allowed after each comma.
 *
 * @return PARSE_OK on success.
 */
int parse_param_list(unsigned *arg_count);

/**
 * @brief Read and return a token of the required type.
 *
 * @param exp_tok Expected token type.
 *
 * @pre Next token's type must match @p exp_tok.
 * @return Token pointer owned by the caller (must call free_token()).
 * @note On mismatch calls error_exit(ERR_SYNTACTIC).
 */
token_ptr expect_type(enum token_type exp_tok);

/**
 * @brief Consume a maximal sequence of EOL tokens as soft whitespace.
 *
 * @note
 *  - Typical usage: after '(', after ',', and after operators like '=' or '.'.
 *  - Internally reads and frees all contiguous EOL tokens.
 */
void consume_eols(void);

#endif