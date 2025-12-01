/**
 * @file scanner.c
 * @author Martin Racek (xracekm00)
 * @brief Contains scanners backbone as well as functions for parsers use
 * @version 0.4
 * @date 2025-11-28
 *
 * @copyright Copyright (c) 2025
 */

#include "error.h"
#include "scanner.h"
#include "lex_funs.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Initializing global array of keywords
// NOTE: NULL at the end is just a breakpoint
const char *key_words_arr[] = {
    "class", "if", "else", "is", "return", "var", "while", "Ifj", "null",
    "static", "true", "false", "Num", "String", "Null", "break", "continue", 
    "for", "import", "in", NULL
};

// Initializing global variables
bool has_been_pushed = false;
token_ptr pushed_token = NULL;

bool eof_reached = false;
token_ptr pending_eof_token = NULL;

/**
 * @brief Returns token back to scanner
 *
 * @param token to be filled
 */
void push_token(token_ptr token)
{
    has_been_pushed = true;
    pushed_token = token;
}

/**
 * @brief Reads and decodes token from IFJ25 source code
 *
 * @return Pointer to newly allocated token structure
 */
token_ptr get_token()
{
    token_ptr token; // Token to be returned

    if ((pending_eof_token != NULL) && eof_reached)
    {
        // Next token is the one that was unintentionally processed
        token = pending_eof_token;

        // Updating gloval variables
        eof_reached = false;
        pending_eof_token = NULL;

        return token;
    }

    // When the token has been returned from parser
    if (has_been_pushed && (pushed_token != NULL))
    {
        // Next token is the on that's been returned
        token = pushed_token;

        // Updating global variables
        has_been_pushed = false;
        pushed_token = NULL;

        return token;
    }

    // Allocating memory for a new token
    if ((token = malloc(sizeof(token_t))) == NULL)
    {
        // warnings(99, "memory allocation failed at line: %d\n", 73);
        error_exit(ERR_INTERNAL);
    }

    // uprava pre PSA
    token->ast = NULL;
    
    //Takes care of the rest
    process_next_token(token); 

    return token;
}

/**
 * @brief Frees memory allocated for token
 *
 * @param token to be freed
 */
void free_token(token_ptr token)
{
    // Checks parameters validity
    if (token == NULL)
    {
        return;
    }

    // Frees memory based on the token type
    switch (token->type)
    {
    case IDENT:
    case GLOB_VAR:
    case KEY_WORD:
    case ONE_L_STRING:
    case MUL_L_STRING:
        if (token->value.str_value != NULL)
        {
            free(token->value.str_value);
            token->value.str_value = NULL;
        }
        break;
    default:
        break;
    }

    // Freing token structure itself
    free(token);
    token = NULL;

    return;
}

/**
 * @brief After the parser is done using scanner, it need to call this function
 *
 */
void scanner_cleanup()
{
    // Frees memory used by global variables
    if (pending_eof_token != NULL)
    {
        free(pending_eof_token);
        pending_eof_token = NULL;
    }

    if (pushed_token != NULL)
    {
        free(pushed_token);
        pushed_token = NULL;
    }

    // Resets global variables (not necesary) but good habit
    eof_reached = false;
    has_been_pushed = false;

    return;
}