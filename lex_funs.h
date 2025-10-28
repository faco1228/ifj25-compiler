/**
 * @file lex_funs.h
 * @author xracekm00
 * @brief Header file for lexical analyzer's functions
 * @version 0.2
 * @date 2025-10-26
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef LEX_FUNS_H
#define LEX_FUNS_H

#include "scanner.h"
#include "error.h"
#include <stdio.h>
#include <ctype.h>

//Macro to determine whether the input character still belongs to the IDENT token being processed
#define is_ident(c) \
    ((isalnum(c)) || (c == '_'))

//Macro for safe memory reallocation with error handling
#define not_enough_space(buffer) \
    do{ \
        if((buffer = realloc(buffer, (strlen(buffer) + 1) * 2)) == NULL){ \
            warnings(99, "memory allocation failed\n"); \
            error_exit(99); \
        } \
    } while (0)


//********************************* Function prototypes *********************************//

token_ptr process_next_token(token_ptr);

void process_ident(token_ptr);

void skip_comments(token_ptr);

void process_str_l(token_ptr);

void process_escape_sequence(token_ptr, unsigned *);

void process_hex_escape(token_ptr, unsigned *);

void process_mul_l_str(token_ptr);

void process_dots(token_ptr);

void process_number(token_ptr, int);

void process_float(token_ptr, char*, unsigned*);

void process_exp(token_ptr, char*, unsigned*);

void store_pending_eof();

void free_token(token_ptr);

#endif
