/**
 * @file scanner.h
 * @author xracekm00
 * @brief Header file for lexical analyzer - scanner
 * @version 0.3
 * @date 2025-10-26
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef SCANNER_H
#define SCANNER_H

#include <stdbool.h>

//Enum defining different types of token
enum token_type {IDENT, KEY_WORD, GLOB_VAR, INT_LIT, FLOAT_LIT, ONE_L_STRING, MUL_L_STRING, 
                 OPERATOR, LEFT_PAR, RIGHT_PAR, LEFT_DOM_PAR, RIGHT_DOM_PAR, EOL, END_OF_FILE,
                 DOUBLE_DOT, TRIPE_DOT, DOT, Q_MARK, SEMICOLON, MINUS, COMMA};

union token_info{
    long long int_value;
    long double float_value;
    char *str_value;
    char* name;
    int other_value; //operators, parentheses, domain pars,...
};

//Token data type
typedef struct token{
    enum token_type type;
    union token_info value;
}token_t, *token_ptr;
                
//Global array of keywords
extern const char *key_words_arr[];

//Global variables to keep track of returned tokens from parser
extern bool has_been_pushed;
extern token_ptr pushed_token;

//Global variables for EOF encounters during lookahead
extern bool eof_reached;
extern token_ptr pending_token;

//Defining max length of variable name
#define MAX_NAME_LEN 100

//Defining max length of line
#define MAX_LINE_LEN 1024

//Defining max ammount of digits in storable number
#define MAX_DIGITS 50

//********************************* Function prototypes *********************************//

void push_token(token_ptr);

token_ptr get_token();

#endif
