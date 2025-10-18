/**
 * @file scanner.h
 * @author xracekm00
 * @brief Header file for scanner
 * @version 0.1
 * @date 2025-10-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef SCANNER_H
#define SCANNER_H

//Enum defining different types of token
enum token_type {IDENT, KEY_WORD, GLOB_VAR, INT_LIT, FLOAT_LIT, ONE_L_STRING, MUL_L_STRING, 
                OPERATOR, LEFT_PAR, RIGHT_PAR, LEFT_DOM_PAR, RIGHT_DOM_PAR, EOL, END_OF_FILE,
                DOUBLE_DOT, TRIPE_DOT, DOT, Q_MARK, SEMICOLON, MINUS, COMMA};

union token_info{
    long int_value;
    double float_value;
    char *str_value;
    char* name;
    int other_value; //operators, parentheses, ., domain pars,...
};

//Token data type
typedef struct token{
    enum token_type type;
    union token_info value;
}token_t, *token_ptr;
                
//Global array of keywords
extern const char *key_words_arr[];

//Global variables to keep track of returned tokens from parcer
extern bool has_been_pushed;
extern token_ptr pushed_token;

//Defining max length of variable
#define MAX_LEN 100

//Defining possible max length of line
#define MAX_LINE_LEN 100

//Macro to determine whether the input characters still belongs to the ident token being processed
#define is_ident(c) \
        ((isalnum(c)) || (c == '_')) \

//************************************** Function prototypes **************************************//
void push_token(token_ptr);

token_ptr get_token();

token_ptr process_next_token(token_ptr);

void process_ident(token_ptr);

void skip_comments(token_ptr);

void process_str_l(token_ptr);

void process_mul_l_str(token_ptr);

void process_dots(token_ptr);

int hex_digit_value(int);

void process_dots(token_ptr);

void process_number(token_ptr, int);

void process_float(token_ptr, char*, unsigned);

void process_exp(token_ptr, char*, unsigned);

#endif
