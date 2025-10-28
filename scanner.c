/**
 * @file scanner.c
 * @author xracekm00
 * @brief Contains scanner's backbone as well as functions for parser's use
 * @version 0.3
 * @date 2025-10-26
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
const char *key_words_arr[] = {
    "class", "if", "else", "is", "null", "return", "var", "while", "Ifj",
    "static", "true", "false", "Num", "String", "Null", "Break", "Continue", 
    "for", NULL
};

//Initializing global variables
bool has_been_pushed = false;
token_ptr pushed_token = NULL;

bool eof_reached = false;
token_ptr pending_token = NULL;

/**
 * @brief Returns token back to scanner
 * 
 * @param token Token to be returned
 */
void push_token(token_ptr token){
    has_been_pushed = true;
    pushed_token = token;
}

/**
 * @brief Reads and decodes token from IFJ25 source code
 * 
 * @return Pointer to newly allocated token structure
 */
token_ptr get_token(){
    token_ptr token; //Token to be returned

    if ((pending_token != NULL) && eof_reached){
        //Next token is the one that was unintentionally processed
        token = pending_token;
        //Updating gloval variables
        eof_reached = false;
        pending_token = NULL;

        return token;
    }

    //When the token has been returned from parser
    if (has_been_pushed &&( pushed_token != NULL)){
        //Next token is the on that's been returned
        token = pushed_token;
        //Updating global variables
        has_been_pushed = false;
        pushed_token = NULL;

        return token;
    }
    
    //Allocating new token
    if ((token = malloc(sizeof(token_t))) == NULL){
        //NOTE: don't forget to delete this later
        warnings(99, "memory allocation failed at line: %d\n", 73);
        error_exit(99);
    }

    //Process next token
    process_next_token(token); 

    return token;
}
