/**
 * @file scanner.h
 * @author xracekm00
 * @brief Contains functions for partial token processing
 * @version 0.4
 * @date 2025-11-17
 * 
 * @copyright Copyright (c) 2025
 */

#include "lex_funs.h"
#include "scanner.h"
#include "error.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/**
 * @brief Reads input and identifies token type
 * 
 * @param token to be filled
 * @return pointer to a token with updated attributes
 */
token_ptr process_next_token(token_ptr token){
    //Variable for reading charcters from input stream
    int character;

    //Reads next character
    character = fgetc(stdin);

    //Skips whitespaces
    while (isspace(character)) {
        //When the WS is '\n'
        if (character == '\n') {
            token->type = END_OF_LINE;
            token->value.other_value = EOL_V;
            return token;
        }

        //Reads next character
        character = fgetc(stdin);
    }

    //When end of file was reached
    if (character == EOF){
        token->type = END_OF_FILE;
        token->value.other_value = EOF_V;
        return token;
    }
    else if (isalpha(character) || (character == '_')){
        //Let's process IDENT

        //Allocating memory for IDENT's name
        if ((token->value.str_value = malloc(sizeof(char) * MAX_NAME_LEN)) == NULL){
            //warnings(99, "memory allocation failed at line: %d\n", 58);
            free_token(token);
            error_exit(ERR_INTERNAL);
        }

        //Character read is the first letter of the name
        token->value.str_value[0] = character;

        //Will take care of the rest of the name
        process_ident(token);

        return token;
    }
    else if (isdigit(character)){
        //Will take care of the rest of the number
        process_number(token, character);
        return token;
    }
    else{
        //Let's decides what to do with the character read
        switch (character){
        case EOF:
            token->type = END_OF_FILE;
            token->value.other_value = EOF_V;
            return token;
        case '+':
            token->type = OPERATOR;
            token->value.other_value = PLUS_V;
            break;
        case '-':
            //!!! This can be either subtraction operator or unary operator !!!
            token->type = OPERATOR;
            token->value.other_value = MINUS_V;
            break;
        case '*':
            token->type = OPERATOR;
            token->value.other_value = STAR_V;
            break;
        case '/':
            /*
            * The function handles all three situations:
            * a) oneline comment
            * b) multiline comment
            * c) operator
            */
            process_slash(token);

            return token;
        case '=':
            //Has to peak one character ahead to determine whether the operator isn't ==
            if (check_equal(token, character)){}
            else{
                token->type = OPERATOR;
                token->value.other_value = EQUAL_SIGN_V;
            }

            break;
        case '(':
            token->type = LEFT_PAR;
            token->value.other_value = LEFT_PAR_V;
            break;
        case ')':
            token->type = RIGHT_PAR;
            token->value.other_value = RIGHT_PAR_V;
            break;
        case '{':
            token->type = LEFT_DOM_PAR;
            token->value.other_value = LEFT_DOM_PAR_V;
            break;
        case '}':
            token->type = RIGHT_DOM_PAR;
            token->value.other_value = RIGHT_DOM_PAR_V;
            break;
        case '.':
            /*
            * This function handles all three options:
            * a).
            * b)..
            * c)...
            */
            process_dots(token);

            return token;
        case '\n':
            token->type = END_OF_LINE;
            token->value.other_value = EOL_V;
            break;
        case '"':
            //This function processes both oneline and multiline string literals
            process_str_lit(token);

            break;
        case '?':
            token->type = Q_MARK;
            token->value.other_value = Q_MARK_V;
            break;
        case ':':
            token->type = SEMICOLON;
            token->value.other_value = SEMICOLON_V;
            break;
        case '<':
            //Has to peak one character ahead to determine whether the operator isn't <=
            if (check_equal(token, character)){}
            else{
                token->type = OPERATOR;
                token->value.other_value = LESS_THAN_V;
            }

            break;
        case '>':
            //Has to peak one character ahead to determine whether the operator isn't >=
            if (check_equal(token, character)){}
            else{
                token->type = OPERATOR;
                token->value.other_value = GREATER_THAN_V;
            }

            break;
        case '!':
            //Has to peak one character ahead to determine whether the operator isn't !=
            if (check_equal(token, character)){}
            else{
                token->type = EXC_MARK;
                token->value.other_value = EXC_MARK_V;
            }

            break;
        case ',':
            token->type = COMMA;
            token->value.other_value = COMMA_V;
            break;
        default:
            //warnings(1, "unexpected character: '%c' (ASCII %d)\n", character, character);
            free_token(token);
            error_exit(ERR_LEXICAL);
            break;
        }
    }

    return token;
}

/**
 * @brief Processes identifier or keyword token
 * 
 * @note  I had to amend this function so it detects null as token of type NULL_LIT
 * 
 * @param token to be filled
 */
void process_ident(token_ptr token){
    int next;                 //Variable for reading characters from input stream
    unsigned index = 1;       //Index in the ident's name

    //Reads next characters
    while((next = fgetc(stdin)) != EOF && is_ident(next)){
        //Checks whether there's enough space for the IDENT
        if (index >= MAX_NAME_LEN - 1) {
            //warnings(1, "identifier too long\n");
            free_token(token);
            error_exit(ERR_LEXICAL);
        }

        //Updates IDENT name
        token->value.str_value[index] = next;
        //Incrementing index
        index++;
    }
    
    //Strings have to be null terminated
    token->value.str_value[index] = '\0';

    /* After while loop ended there's an unwanted character stored in c:
     * A) EOF
     * B) white sapce: ' ', '\n', '\t'
     * C) Next tokens first character
    */

    //When IDENT was successfully read, but EOF token will be expected as well
    if (next == EOF){
        store_pending_eof();
    }
    //Retruns character read into input streams buffer
    else{
        ungetc(next, stdin);
    }

    //Has to compare IDENT's name with all possible key words
    for (int i = 0; key_words_arr[i] != NULL; i++){
        //When match was found
        if (!strcmp(token->value.str_value, key_words_arr[i])){
            //Sets token's parameteres
            token->type = KEY_WORD;
            strcpy(token->value.str_value, key_words_arr[i]);
            
            return;
        }
    }
    
    //When the token isn't KW but it's global variable
    if (token->value.str_value[0] == '_' && token->value.str_value[1] == '_'){
        token->type = GLOB_VAR;
        //token->name is already set

        return;
    }
    else if(token->value.str_value[0] == '_'){
        //Since token isn't global variable, other idents can not start with '_'
        error_exit(ERR_LEXICAL);
    }
    else{
        token->type = IDENT;
        //token->name has already been set
        
        return;
    }
}

/**
 * @brief Skips one-line and multiline comments / stores division operator
 * 
 * @param token to be filled
 */
void process_slash(token_ptr token){
    //Variable for reading characters from input stream (reads next char)
    int next = fgetc(stdin);

    //Scenario: one line comment
    if (next == '/') {
        //Skips one line
        while ((next = fgetc(stdin)) != '\n' && next != EOF);

        //Sets token's atributes
        if (next == EOF){
            token->type = END_OF_FILE;
            token->value.other_value = EOF_V; 
        }
        else{
            token->type = END_OF_LINE;
            token->value.other_value = EOL_V;
        }
    }
    //Scenario: multiline line comment
    else if (next == '*') {

        int previous = 0;   //stores previous character
        int depth = 1;      //Information of scope (nesting depth)

        //Skips lines until all '/*' found their matching '*/'
        while (depth > 0 && (next = fgetc(stdin)) != EOF) {
            if (previous == '/' && next == '*'){
                depth++;
            }
            if (previous == '*' && next == '/'){
                depth--;
            }
            previous = next;
        }

        //When EOF was reached but the nested comment didn't end
        if (depth > 0) {
            //warnings(1, "unterminated comment\n");
            free_token(token);
            error_exit(ERR_LEXICAL);
        }

        //Multiline comment should be treated as whitespace -> let's ask for a new token
        token_ptr new_token = get_token();
        *token = *new_token;
        
        //Clean up after the temp token
        free(new_token);
        new_token = NULL;
    }
    //Scenario: the '/' character was actually operator
    else {
        if (next == EOF){
            store_pending_eof();
        }
        else{
            ungetc(next, stdin);
        }
        token->type = OPERATOR;
        token->value.other_value = SLASH_V;
    }
}

/**
 * @brief Peaks one character ahead to determine whether the token should contain '='
 * 
 * @param token to be filled
 * @return true when equal sign follows
 * @return false otherwise
 */
bool check_equal(token_ptr token, int operator){
    //Variable for reading characters from input stream (reads next character)
    int next = fgetc(stdin); 

    if (next == '='){
        //Based on what was the first character let's compose the token value
        switch (operator)
        {
        case '<':
            token->value.other_value = LESS_OR_EQ_THAN_V;
            break;
        case '>':
            token->value.other_value = GREATER_OR_EQ_THAN_V;
            break;
        case '=':
            token->value.other_value = LOGICAL_EQUAL_V;
            break;
        case '!':
            token->value.other_value = LOGICAL_NOT_EQUAL_V;
            break;
        default:
            //The function was called for wrong character
            free(token);
            error_exit(ERR_INTERNAL);
            break;
        }

        //Sets tokens attributes
        token->type = OPERATOR;

        //Equal sign was found successfully
        return true;
    }
    else if(next == EOF){
        store_pending_eof();
    }
    else{
        ungetc(next, stdin);
    }

    //Equal sign wasn't found
    return false;
}

/**
 * @brief Processes string literals (single-line and multiline)
 * 
 * @param token to be filled
 */
void process_str_lit(token_ptr token){
    int next;                 //Variable for reading characters from input stream
    unsigned index = 0;       //String index

    //Allocating memory for the string
    if ((token->value.str_value = malloc(sizeof(char) * MAX_LINE_LEN)) == NULL){
        //warnings(99, "memory allocation failed at line: %d\n", 384);
        free_token(token);
        error_exit(ERR_INTERNAL);
    }

    //Checks for potential multiline string
    int second = fgetc(stdin);
    if (second == '"'){
        //Scanerio: "" read
        int third = fgetc(stdin);

        if (third == '"'){
            //Scanerio: """ read -> It's a multiline string
            process_mul_l_str(token);

            return;
        }
        else{
            //Third quot. mark wasn't found -> Empty string
            if (third == EOF){
                store_pending_eof();
            }
            else{
                ungetc(third, stdin);    
            }

            //Sets tokens attributes
            token->type = ONE_L_STRING;
            token->value.str_value[0] = '\0';
            return;
        }
    }
    else{
        //Let's process one line string and put second back
        if (second != EOF && second != '\n'){
            ungetc(second, stdin);
        }
        else{
            //warnings(1, "unterminated string literal(EOF or newline)\n");
            free_token(token);
            error_exit(ERR_LEXICAL);
        }
    }

    //Reading characters until EOF or closing " / """
    while ((next = fgetc(stdin)) != EOF && (next != '"') && (next != '\n')){
        
        //Checks if there's enough space to store the whole string
        if (index >= (MAX_LINE_LEN - 1)){
            not_enough_space(token->value.str_value);
        }

        //When the character is an escape-sequance
        if (next == '\\'){
            process_escape_sequence(token, &index);
        }
        else{
            //Stores character
            token->value.str_value[index] = next;
            //Increments index
            index++;
        }
    }

    //Strings have to be null terminated
    token->value.str_value[index] = '\0';

    //After the while loop ended, the character read could be: EOF / " / \n
    if (next == EOF){
        //warnings(1, "unterminated string literal(EOF)\n");
        free_token(token);
        error_exit(ERR_LEXICAL);
    }
    else if(next == '\n'){
        //warnings(1, "unterminated string literal(newline)\n");
        free_token(token);
        error_exit(ERR_LEXICAL);
    }
    
    //If got here, everything was read successfully
    token->type = ONE_L_STRING;

    return;
}

/**
 * @brief Processes escape sequences in strings
 * 
 * @param token to be filled
 * @param index pointer to a current index in string buffer
 */
void process_escape_sequence(token_ptr token, unsigned *index) {
    //Variable to store 2nd part of the esc seq (\n, \r, \t, \\, \")
    int escape_char = fgetc(stdin);

    //Checks whether EOF was reached
    if (escape_char == EOF){
        //warnings(1, "unterminated escape sequence in string\n");
        free_token(token);
        error_exit(ERR_LEXICAL);
    }

    switch (escape_char) {
        case 'n':
            token->value.str_value[*index] = '\n';
            break;
        case 'r':
            token->value.str_value[*index] = '\r';
            break;
        case 't':
            token->value.str_value[*index] = '\t';
            break;
        case '\\':
            token->value.str_value[*index] = '\\';
            break;
        case '"':
            token->value.str_value[*index] = '"';
            break;
        case 'x':
            //Handles hexadecimal escape-sequances
            process_hex_escape(token, index);
            break;
        default:
            //warnings(1, "unknown escape sequence '\\%c'\n", escape_char);
            free_token(token);
            error_exit(ERR_LEXICAL);
    }

    //Increments index in string
    (*index)++;
}

/**
 * @brief Processes hexadecimal escape sequences (\xHH)
 * 
 * @param token to be filled
 * @param index pointer to a current index
 */
void process_hex_escape(token_ptr token, unsigned *index) {
    //Buffer to store 2 hex digits and null terminator
    char hex_buffer[3];

    //Read exactly 2 hex digits
    int hex1 = fgetc(stdin);
    int hex2 = fgetc(stdin);
    
    //Checks whether EOF was reached
    if (hex1 == EOF || hex2 == EOF) {
        //warnings(1, "incomplete \\x escape sequence (EOF)\n");
        free_token(token);
        error_exit(ERR_LEXICAL);
    }
    
    //Validate hex digits
    if (!isxdigit(hex1) || !isxdigit(hex2)) {
        //warnings(1, "invalid hex escape sequence \\x%c%c\n", hex1, hex2);
        free_token(token);
        error_exit(ERR_LEXICAL);
    }
    
    //Creating string from hexa digits
    hex_buffer[0] = hex1;
    hex_buffer[1] = hex2;
    hex_buffer[2] = '\0';
    
    //Converting to ASCII value
    long ascii_value = strtol(hex_buffer, NULL, 16);
    
    //Store character to a string
    token->value.str_value[*index] = ascii_value;

    return;
}

/**
 * @brief Processes multiline string literals
 * 
 * @param token to be filled
 */
void process_mul_l_str(token_ptr token){
    int next;               //Variable for reading characters from input stream
    unsigned index = 0;     //Position in the string
    bool to_ignore = true;  //Tracks whitespace after opening """

    //Sets token's atributes
    token->type = MUL_L_STRING;

    //Skips whitespaces after opening """
    while (to_ignore && (next = fgetc(stdin)) != EOF){
        //When non-whitespace character is encountered
        if (!isspace(next)){
            ungetc(next, stdin);
            to_ignore = false;
        }
    }
    
    //Reading characters until terminating """ is found
    while ((next = fgetc(stdin)) != EOF){

        //Checks if there's enough space for the whole string
        if (index >= (MAX_LINE_LEN - 1)){
            not_enough_space(token->value.str_value);
        }

        //Checks if this could be start of terminating """ sequance
        if (next == '"') {
            int second = fgetc(stdin);

            if (second == '"') {
                //Scenario: "" read

                int third = fgetc(stdin);
                if (third == '"') {
                    //Scenario """ read -> end of multiline string reached

                    //Removes whitespace before closing """
                    while (index > 0 && isspace(token->value.str_value[index - 1])) {
                        index--;
                    }

                    //Strings have to be null terminated
                    token->value.str_value[index] = '\0';

                    return;
                }
                else{
                    //Third quot. mark wasn't found
                    if (third == EOF){
                        //warnings(1, "unterminated string literal (EOF found)\n");
                        free_token(token);
                        error_exit(ERR_LEXICAL);
                    }

                    //Stores character
                    token->value.str_value[index] = third;
                    //Increments index
                    index++;
                }
            }
            else{
                //Not even second quot. mark wasn't found
                if (second == EOF){
                    //warnings(1, "unterminated string literal (EOF found)\n");
                    free_token(token);
                    error_exit(ERR_LEXICAL);
                }

                //Stores character
                token->value.str_value[index] = second;
                //Increments index
                index++;
            }
        }
        //Non quot. mark character was read
        else {
            if (next == EOF){
                //warnings(1, "unterminated string literal(EOF)\n");
                free_token(token);
                error_exit(ERR_LEXICAL);
            }
            
            //Store character
            token->value.str_value[index] = next;
            //Increments index
            index++;
        }
    }

    //If program got here, there is no terminating sequence """

    //warnings(1, "unterminated multiline string literal\n");
    free_token(token);
    error_exit(ERR_LEXICAL);
}


/**
 * @brief Processes dot operators (., .., ...)
 * 
 * @param token to be filled
 */
void process_dots(token_ptr token){
    //Variable for reading characters from input stream (reads next char)
    int next = fgetc(stdin);

    /*
    * We have already read '.' and want to peak ahead to find out
    * whether there is second or maybe even a third dot, since we
    * want to differenciate 3 token types: DOT, DOUBLE_DOT, TRIPLE_DOT
    */

    if (next == '.') {
        //Scenario: ".." read
        int third = fgetc(stdin);
        
        if (third == '.') {
            //Scenario: "..." read

            //Sets tokens attributes
            token->type = TRIPLE_DOT;
            token->value.other_value = TRIPLE_DOT_V;

            return;
        }
        else {
            //Third dot wasn't found
            if (third == EOF){
                store_pending_eof();
            }
            else{
                ungetc(third, stdin);
            }

            //Sets tokens atributes
            token->type = DOUBLE_DOT;
            token->value.other_value = DOUBLE_DOT_V;

            return;
        }
    }
    else{
        //Not even second dot was found
        if (next == EOF){
            store_pending_eof();
        }
        else{
            ungetc(next, stdin);
        }
        
        //Sets tokens attributes
        token->type = DOT;
        token->value.other_value = DOT_V;
    }

    return;
}

/**
 * @brief Processes numeric literals (integer, float, hexadecimal)
 * 
 * @param token to be filled
 * @param first_char first digit (0-9) that has already been read
 */
void process_number(token_ptr token, int first_char){
    int digit;                 //Variable for reading characters from input stream
    char *temp_buffer = NULL;  //Temporary buffer for storing numeric string
    unsigned index = 0;        //Index in the buffer
    bool is_hexa = false;      //Tracks whether the variable is hexadecimal

    //Allocating memory for temp buffer
    if ((temp_buffer = malloc(sizeof(char) * MAX_DIGITS)) == NULL){
        //warnings(99, "memory allocation failed\n");
        free_token(token);
        error_exit(ERR_INTERNAL);
    }
    
    //Inserts first character
    temp_buffer[index] = first_char;
    //Incrementing index in the buffer
    index++; 

    //When the first character was '0'
    if (first_char == '0'){
        //Reads next character
        digit = fgetc(stdin);
        
        switch (digit){
        case '.':
            // Checks whetherr the index is in valid range
            if (index >= MAX_DIGITS - 1){
                //warnings(1, "numeric literal too long\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //Storing decimal point
            temp_buffer[index] = digit;
            index++;

            //Takes care of the float
            process_float(token, temp_buffer, &index);

            break;
        case 'e':
        case 'E':
            //Exponential notation starting with 0

            // Checks whetherr the index is in valid range
            if (index >= MAX_DIGITS - 1){
                //warnings(1, "numeric literal too long\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //Storying e/E
            temp_buffer[index] = digit;
            index++;

            //Takes care of the rest
            process_exp(token, temp_buffer, &index);

            break;
        case 'x':
            //Hexadecimal number
            is_hexa = true;

            // Checks whetherr the index is in valid range
            if (index >= MAX_DIGITS - 1){
                //warnings(1, "numeric literal too long\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //Storing x
            temp_buffer[index] = 'x';
            index++;
            
            //Reads hexadecimal digits
            while (isxdigit(digit = fgetc(stdin))){
                //Check if buffer is full
                if (index >= MAX_DIGITS - 1){
                    //warnings(1, "numeric literal too long\n");
                    free_token(token);
                    free(temp_buffer);
                    temp_buffer = NULL;
                    error_exit(ERR_LEXICAL);
                }

                //Stores digit
                temp_buffer[index] = digit;
                index++;
            }

            //If no hexadecimal digits were added after '0x'
            if (index <= 2){
                //warnings(1, "invalid hexadecimal literal\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //The previously read character has to be returned
            if (digit == EOF){
                store_pending_eof();
            }
            else{
                ungetc(digit, stdin);
            }

            break;
        default:
            //Leading zeros are not allowed for decimal numbers
            if (isdigit(digit)){
                //warnings(1, "invalid number format (leading zeros)\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }
            else{
                if (digit == EOF){
                    store_pending_eof();
                }
                else{
                    ungetc(digit, stdin);
                }
            }
            break;
        }
    }
    //When first character was digit different form zeto
    else if (isdigit(first_char) && (first_char != '0')){

        //Reads while the characters are digits
        while (isdigit(digit = fgetc(stdin))){
            //Check if buffer is full
            if (index >= MAX_DIGITS - 1){
                //warnings(1, "numeric literal too long\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //Stores digit
            temp_buffer[index] = digit;
            index++;
        }
        
        //After the while loop terminated, check what character stopped the loop
        switch (digit){
        case '.':
            // Checks whetherr the index is in valid range
            if (index >= MAX_DIGITS - 1){
                //warnings(1, "numeric literal too long\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //Storing decimal point
            temp_buffer[index] = digit;
            index++;

            //Takes care of the rest
            process_float(token, temp_buffer, &index);

            break;
        case 'e':
        case 'E':
            //Exponential notation

            // Checks whetherr the index is in valid range
            if (index >= MAX_DIGITS - 1){
                //warnings(1, "numeric literal too long\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(ERR_LEXICAL);
            }

            //Storing decimal point
            temp_buffer[index] = digit;
            index++;

            //Takes care of the rest
            process_exp(token, temp_buffer, &index);

            break;
        default:
            //The character doesn't belong to the number
            if (digit == EOF){
                store_pending_eof();
            }
            else{
                ungetc(digit, stdin);
            }
        }
    }

    // Checks whetherr the index is in valid range
    if (index >= MAX_DIGITS){
        //warnings(1, "numeric literal too long\n");
        free_token(token);
        free(temp_buffer);
        temp_buffer = NULL;
        error_exit(ERR_LEXICAL);
    }    

    //Strings have to be null terminated
    temp_buffer[index] = '\0';

    //Converts string to appropriate numeric value
    if (is_hexa){
        //Converts string containing hexadecimal number into integer value
        token->value.int_value = strtol(temp_buffer, NULL, 0);
        token->type = INT_LIT;
    }
    else if (token->type == FLOAT_LIT){
        //Converts string containing float number into float value
        token->value.float_value = strtold(temp_buffer, NULL);
    }
    else{
        //Converts string containing integer number into integer value
        token->value.int_value = strtol(temp_buffer, NULL, 10);
        token->type = INT_LIT;
    }
    
    //Memory cleanup
    free(temp_buffer);
    temp_buffer = NULL;

    return;
}

/**
 * @brief Processes decimal part and optional exponent of float
 * 
 * @param token to be filled
 * @param buffer string containing digits (before decimal point)
 * @param buf_index current position in buffer
 */
void process_float(token_ptr token, char *buffer, unsigned *buf_index){
    int digit;      //Varaible for reading characters from input stream
    int count = 0;  //Counts how many numbers the number contains after decimal point
    
    //Reads decimal digits
    while (isdigit(digit = fgetc(stdin))){
        // Checks whetherr the index is in valid range
        if (*buf_index >= MAX_DIGITS - 1){
            //warnings(1, "numeric literal too long\n");
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(ERR_LEXICAL);
        }

        buffer[*buf_index] = digit;
        (*buf_index)++;
        count++;
    }

   // After decimal point, there must be at least one digit
    if (count == 0) {
        free_token(token);
        free(buffer);
        buffer = NULL;
        error_exit(ERR_LEXICAL);
    }

    //Sets tokens attributes
    token->type = FLOAT_LIT;

    // Checks whetherr the index is in valid range
    if (*buf_index >= MAX_DIGITS - 1){
        //warnings(1, "numeric literal too long\n");
        free_token(token);
        free(buffer);
        buffer = NULL;
        error_exit(ERR_LEXICAL);
    }


    //Strings have to be null terminated
    buffer[*buf_index] = '\0';

    //After the while loop ended, ther's an unwanted character
    if (digit == EOF){
        store_pending_eof();
    }
    //Check if there's an exponent
    else if(digit == 'e' || digit == 'E'){
        // Checks whetherr the index is in valid range
        if (*buf_index >= MAX_DIGITS - 1){
            //warnings(1, "numeric literal too long\n");
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(ERR_LEXICAL);
        }

        //Storing decimal point
        buffer[*buf_index] = digit;
        (*buf_index)++;

        //Takes care of the rest
        process_exp(token, buffer, buf_index);
        
        return;
    }
    else{        
        //Return the character that stopped the loop
        ungetc(digit, stdin);
    }
    
    return;
}

/**
 * @brief Processes numeric literals with scientific/exponent notation (e/E)
 * 
 * @param token to be filled
 * @param buffer string containing digits (maybe as well decimal point)
 * @param buf_index current position in buffer
 */
void process_exp(token_ptr token, char *buffer, unsigned *buf_index){
    int digit;      //Variable for reading characters from input stream
    int count = 0;  //Variable to determine how many digits were read
    
    /* Even though integer literal can use scientific notation, there's not smart enough function
    *  in C that could detect and convert this number from string into decimal. 
    *  Therefore I'm treating those numbers as float. 
    */

    //Sets tokens attributes
    token->type = FLOAT_LIT;

    //Reads next characte
    digit = fgetc(stdin);

    if (digit == EOF){
        //warnings(1, "invalid exponent format\n");
        free_token(token);
        free(buffer);
        buffer = NULL;
        error_exit(ERR_LEXICAL);
    }
    //Check for optional sign
    else if (digit == '+' || digit == '-'){
        // Checks whetherr the index is in valid range
        if (*buf_index >= MAX_DIGITS - 1){
            //warnings(1, "numeric literal too long\n");
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(ERR_LEXICAL);
        }

        //Stores sign
        buffer[*buf_index] = digit;
        (*buf_index)++;
            
        //Reads next digit
        digit = fgetc(stdin);
    }

    //Read exponent digits
    while (isdigit(digit)){
        // Checks whetherr the index is in valid range
        if (*buf_index >= MAX_DIGITS - 1){
            //warnings(1, "numeric literal too long\n");
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(ERR_LEXICAL);
        }

        //Tracks how many digits were read
        count++;

        //Stores digits into buffer
        buffer[*buf_index] = digit;
        (*buf_index)++;

        //Reads next digit
        digit = fgetc(stdin);
    }

    //Exponent needs at least one digit
    if (count == 0){
        //warnings(1, "invalid exponent format\n");
        free_token(token);
        free(buffer);
        buffer = NULL;

        error_exit(ERR_LEXICAL);
    }
    
    //Strings have to be null terminated
    buffer[*buf_index] = '\0';

    //Return the character that stopped the loop
    if (digit == EOF){
        store_pending_eof();
    }
    else{
        //Return the character that stopped the loop
        ungetc(digit, stdin);
    }
}

/**
 * @brief Creates and stores pending EOF token
 * 
 */
void store_pending_eof() {
    //Alocating new pending token
    if (pending_token == NULL) {
        if((pending_token = malloc(sizeof(token_t))) == NULL){
            //warnings(99, "memory allocation failed for EOF token\n");
            error_exit(ERR_INTERNAL);
        }
    }

    //Sets tokens attributes
    pending_token->type = END_OF_FILE;
    pending_token->value.other_value = EOF_V;

    //Updates global variable
    eof_reached = true;
}
