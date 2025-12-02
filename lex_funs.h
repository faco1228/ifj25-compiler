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

token_ptr process_next_token(token_ptr token);

void process_ident(token_ptr token);

void process_slash(token_ptr token);

bool check_equal(token_ptr token, int operator);

void process_str_lit(token_ptr token);

void process_escape_sequence(token_ptr token, unsigned *index);

void process_hex_escape(token_ptr token, unsigned *index);

void process_mul_l_str(token_ptr token);

void process_dots(token_ptr token);

void process_number(token_ptr token, int first_char);

void process_float(token_ptr token, char *buffer, unsigned *buf_index);

void process_exp(token_ptr token, char *buffer, unsigned *buf_index);

void store_pending_eof();

#endif