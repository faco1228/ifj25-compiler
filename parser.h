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
#include <stdio.h>


/**
 * @brief Entry point of the recursive-descent parser.
 *
 * Grammar: <program> ::= <prolog> <class_def> EOF
 *
 * Behavior:
 *  - Skips leading EOLs.
 *  - Parses the prolog and the single class definition.
 *  - Requires EOF after the class.
 *
 * Side effects:
 *  - Uses tokens from scanner.
 *  - Calls scanner_cleanup() on success.
 *
 * Errors:
 *  - On any syntax error, calls error_exit(2) via syntax_error().
 *
 * Returns:
 *  - 0 on success. Never returns on syntax error.
 */
int parse_program(void);

#endif