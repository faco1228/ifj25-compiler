/**
 * @file lex_funs.h
 * @author Martin Racek (xracekm00)
 * @brief Header file for lexical analyzers functions
 * @version 0.4
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef _LEX_FUNS_H_
#define _LEX_FUNS_H_

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include "scanner.h"
#include "error.h"

//Macro to determine whether the input character still belongs to the IDENT token being processed
#define is_ident(c) \
    ((isalnum(c)) || (c == '_'))

//Macro for safe memory reallocation with error handling
#define not_enough_space(buffer) \
    do{ \
        if((buffer = realloc(buffer, (strlen(buffer) + 1) * 2)) == NULL){ \
            error_exit(ERR_INTERNAL); \
        } \
    } while (0)

//********************************* Function prototypes *********************************//

/**
 * @brief Reads input and identifies token type
 * 
 * @param token to be filled
 * @return pointer to a token with updated attributes
 */
token_ptr process_next_token(token_ptr token);

/**
 * @brief Processes identifier or keyword token
 * 
 * @param token to be filled
 */
void process_ident(token_ptr token);

/**
 * @brief Skips one-line and multiline comments / stores division operator
 * 
 * @param token to be filled
 */
void process_slash(token_ptr token);

/**
 * @brief Peaks one character ahead to determine whether the token should contain '='
 * 
 * @param token to be filled
 * @return true when equal sign follows
 * @return false otherwise
 */
bool check_equal(token_ptr token, int operator);

/**
 * @brief Processes string literals (single-line and multiline)
 * 
 * @param token to be filled
 */
void process_str_lit(token_ptr token);

/**
 * @brief Processes escape sequences in strings
 * 
 * @param token to be filled
 * @param index pointer to a current index in string buffer
 */
void process_escape_sequence(token_ptr token, unsigned *index);

/**
 * @brief Processes hexadecimal escape sequences (\xHH)
 * 
 * @param token to be filled
 * @param index pointer to a current index
 */
void process_hex_escape(token_ptr token, unsigned *index);

/**
 * @brief Processes multiline string literals
 * 
 * @param token to be filled
 */
void process_mul_l_str(token_ptr token);

/**
 * @brief Processes dot operators (., .., ...)
 * 
 * @param token to be filled
 */
void process_dots(token_ptr token);

/**
 * @brief Processes numeric literals (integer, float, hexadecimal)
 * 
 * @param token to be filled
 * @param first_char first digit (0-9) that has already been read
 */
void process_number(token_ptr token, int first_char);

/**
 * @brief Processes decimal part and optional exponent of float
 * 
 * @param token to be filled
 * @param buffer string containing digits (before decimal point)
 * @param buf_index current position in buffer
 */
void process_float(token_ptr token, char *buffer, unsigned *buf_index);

/**
 * @brief Processes numeric literals with scientific/exponent notation (e/E)
 * 
 * @param token to be filled
 * @param buffer string containing digits (maybe as well decimal point)
 * @param buf_index current position in buffer
 */
void process_exp(token_ptr token, char *buffer, unsigned *buf_index);

/**
 * @brief Creates and stores pending EOF token
 * 
 */
void store_pending_eof();

#endif