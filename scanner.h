/**
 * @file scanner.h
 * @author Martin Racek (xracekm00)
 * @brief Header file for lexical analyzer - scanner
 * @version 0.4
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <stdbool.h>

//Enum defining different types of token
//Note: Type operator includes both aritmetical and logical operators
//Note: Subtraction operator has separate type, since it can be used as unary operator (extension)
enum token_type {
    IDENT, KEY_WORD, GLOB_VAR, 
    INT_LIT, FLOAT_LIT,
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
    EOL_V, EOF_V,
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
extern token_ptr pending_eof_token;

//Defining max length of variable name
#define MAX_NAME_LEN 512

//Defining max length of line
#define MAX_LINE_LEN 1024

//Defining max ammount of digits in storable number
#define MAX_DIGITS 50

//********************************* Function prototypes *********************************//

/**
 * @brief Returns token back to scanner
 *
 * @param token to be filled
 */
void push_token(token_ptr token);

/**
 * @brief Reads and decodes token from IFJ25 source code
 *
 * @return Pointer to newly allocated token structure
 */
token_ptr get_token();

/**
 * @brief Frees memory allocated for token
 *
 * @param token to be freed
 */
void free_token(token_ptr token);

/**
 * @brief After the parser is done using scanner, it need to call this function
 *
 */
void scanner_cleanup();

#endif