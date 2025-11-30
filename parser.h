/**
 * @file parser.h
 * @author Samuel Facka (xfackas00)
 * @brief Recursive-descent parser interface (non-expression part).
 * 
 * Parser:
 *  - uses the scanner to obtain tokens,
 *  - parses program structure, statements, and blocks,
 *  - delegates expression parsing to the precedence analyzer (parse_expression),
 *  - builds an AST for the whole program.
 * 
 * @version 0.1
 * @date 2025-10-26
 *
 * @copyright Copyright (c) 2025
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
 *  - On successful parse, calls calls scanner_cleanup().
 *  - On a syntactic error, calls calls error_exit(ERR_SYNTACTIC).
 *
 * @return Root AST node of the parsed program. On syntax error, this function
 *         does not return because error_exit terminates the program.
 */
ASTNode_ptr parse_program(void);

// helper functions for parser amd expression parser

/**
 * @brief Read and return a required identifier token.
 *
 * @pre Next token must be of type IDENT.
 * @return Token pointer owned by the caller (must call free_token()).
 * @note On mismatch calls error_exit(ERR_SYNTACTIC).
 */
token_ptr expect_ident(void);

/**
 * @brief Read and return a token of the required type.
 *
 * @param exp_tok Expected token type.
 *
 * @pre Next token's type must match exp_tok.
 * @return Token pointer owned by the caller (must call free_token()).
 * @note On mismatch calls error_exit(ERR_SYNTACTIC).
 */
token_ptr expect_type(enum token_type exp_tok);

/**
 * @brief One-token lookahead: fetch the next token and push it back.
 *
 * @note
 *  - The same instance will be returned again by get_token().
 *
* @return Pointer to the peeked token (do not free).
 */
token_ptr look_ahead(void);

/**
 * @brief Consume a maximal sequence of EOL tokens as "soft whitespace.
 *
 * @note
 *  - Typical usage: after '(', after ',', and after operators like '=' or '.'.
 *  - Internally reads and frees all contiguous EOL tokens.
 */
void consume_eols(void);

#endif