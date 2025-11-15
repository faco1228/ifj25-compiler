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
#include <string.h>


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
int parse_program(void);

#endif