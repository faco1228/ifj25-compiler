/**
 * @file scanner.h
 * @author xracekm00
 * @brief Header file for lexical analyzer - scanner
 * @version 0.4
 * @date 2025-11-17
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef SCANNER_H
#define SCANNER_H

#include <stdbool.h>

//Enum defining different types of token
//Note: Type operator includes both aritmetical and logical operators
//Note: Subtraction operator has separate type, since it can be used as unary operator (extension)
enum token_type {
    IDENT, KEY_WORD, GLOB_VAR, 
    INT_LIT, FLOAT_LIT, NULL_LIT,
    ONE_L_STRING, MUL_L_STRING, 
    OPERATOR, MINUS,
    LEFT_PAR, RIGHT_PAR, LEFT_DOM_PAR, RIGHT_DOM_PAR, 
    END_OF_LINE, END_OF_FILE,
    DOT, DOUBLE_DOT, TRIPLE_DOT,
    Q_MARK, EXC_MARK, SEMICOLON, COMMA
};

//Enum defining different values a token's attribute other_value can obtain
//Note: Token type END_OF_FILE uses value EOF - It's not included in enum because it's built in constant
enum other_value_type {
    EOL_V, EOF_V, NULL_V,
    PLUS_V, MINUS_V, SLASH_V, STAR_V, EQUAL_SIGN_V, 
    LEFT_PAR_V, RIGHT_PAR_V, LEFT_DOM_PAR_V, RIGHT_DOM_PAR_V, 
    Q_MARK_V, EXC_MARK_V, SEMICOLON_V, COMMA_V,
    LESS_THAN_V, GREATER_THAN_V, LESS_OR_EQ_THAN_V, GREATER_OR_EQ_THAN_V,
    LOGICAL_EQUAL_V, LOGICAL_NOT_EQUAL_V,
    DOT_V, DOUBLE_DOT_V, TRIPLE_DOT_V
};

//Each token can store one of these types of data
union token_info{
    long long int_value;                //INT_LIT
    long double float_value;            //FLOAT_LIT
    char *str_value;                    //Could be either name or string literal value
    enum other_value_type other_value;  //Other values, see line 30
};

//Token data type
typedef struct token{
    enum token_type type; 
    union token_info value;
    // ast pointer help pre PSA
    void *ast;
} token_t, *token_ptr;
                
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

void push_token(token_ptr token);

token_ptr get_token();

void free_token(token_ptr token);

void scanner_cleanup();

#endif
