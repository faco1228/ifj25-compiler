/**
 * @file scanner.h
 * @author xracekm00
 * @brief Contains functions for partial token processing
 * @version 0.3
 * @date 2025-10-28
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
 * @param token Token structure to fill
 * @return Pointer to a new token
 */
token_ptr process_next_token(token_ptr token){
    //Variable for reading charcters from input stream
    int character;

    //Reads char from input stream
    character = fgetc(stdin);

    //Skips whitespaces
    while (isspace(character)) {
        //When the WS is '\n'
        if (character == '\n') {
            token->type = EOL;
            token->value.other_value = EOL_V;
            return token;
        }
        //Reads next character
        character = fgetc(stdin);
    }

    //printf("%c\n", character);

    //Case: EOF was reached
    if (character == EOF){
        token->type = END_OF_FILE;
        token->value.other_value = EOF;
        return token;
    }
    else if (isalpha(character) || (character == '_')){
        //When the first character is alphabetic or underscore

        //Allocating memory for the IDENTs name
        if ((token->value.str_value = malloc(sizeof(char) * MAX_NAME_LEN)) == NULL){
            //warnings(99, "memory allocation failed at line: %d\n", 54);
            free_token(token);
            error_exit(99);
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
        //Decides what to do with the read character
        switch (character){
        case EOF:
            token->type = END_OF_FILE;
            token->value.other_value = EOF;
            return token;
        case '+':
            token->type = OPERATOR;
            token->value.other_value = PLUS_V;
            break;
        case '-':
            //!!! This can be either subtraction operator or unary operator !!!
            token->type = MINUS;
            token->value.other_value = MINUS_V;
            break;
        case '*':
            token->type = OPERATOR;
            token->value.other_value = ASTERISK_V;
            break;
        case '/':
            /*
            * The function handles all three situations:
            * a) oneline comment
            * b) multiline comment
            * c) operator
            */
            skip_comments(token);
            return token;
            break;
        case '=':
            token->type = OPERATOR;
            token->value.other_value = EQUAL_SIGN_V;
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
            break;
        case '\n':
            token->type = EOL;
            token->value.other_value = EOL_V;
            break;
        case '"':
            //This function processes both one and multiline str literals
            process_str_l(token);
            return token;
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
            if (check_equal(token, character)){
                return token;
            }
            else{
                token->type = OPERATOR;
                token->value.other_value = LESS_THAN_V;
            }
            break;
        case '>':
            if (check_equal(token, character)){
                return token;
            }
            else{
                token->type = OPERATOR;
                token->value.other_value = GREATER_THAN_V;
            }
            break;
        case '!':
            token->type = OPERATOR;
            token->value.other_value = EXC_MARK_V;
            break;
        case ',':
            token->type = COMMA;
            token->value.other_value = COMMA_V;
            break;
        default:
            //warnings(1, "unexpected character: '%c' (ASCII %d)\n", character, character);
            free_token(token);
            error_exit(1);
            break;
        }
    }

    return token;
}

/**
 * @brief Processes identifier or keyword token
 * 
 * @param token Token structure to fill
 */
void process_ident(token_ptr token){
    int next;                           //Variable for reading characters from input stream
    unsigned index = 1;                 //Index in the ident's name

    while((next = fgetc(stdin)) != EOF && is_ident(next)){
        //Checks whether there's enough space for the IDENT
        if (index >= MAX_NAME_LEN - 1) {
            //warnings(1, "identifier too long\n");
            free_token(token);
            error_exit(1);
        }

        //Updates IDENT name
        token->value.str_value[index] = next;
        //Incrementing index
        index++;
    
    }
    
    //The last character has to be '\0'
    token->value.str_value[index] = '\0';

    /* After while loop terminated there's an unwanted character stored in c:
     * A) EOF
     * B) white sapce: ' ', '\n', '\t'
     * C) Next tokens first character
    */

    //IDENT was successfully read, but EOF token will be axpected as well
    if (next == EOF){
        store_pending_eof();
    }
    //Retruns character read into input streams buffer
    else{
        ungetc(next, stdin);
    }

    //Now we have to compare IDENT's name with all possible key words
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
    }
    else{
        token->type = IDENT;
        //token->name has already been set
    }
}

/**
 * @brief Skips one-line and multiline comments
 * 
 * @param token Token structure to fill
 */
void skip_comments(token_ptr token){
    int next = fgetc(stdin);      //Variable for reading characters from input stream

    //Scenario: one line comment
    if (next == '/') {
        //Skips one line
        while ((next = fgetc(stdin)) != '\n' && next != EOF);
        //Sets token's atributes
        if (next == EOF){
            token->type = END_OF_FILE;
            token->value.other_value = EOF; 
        }
        else{
            token->type = EOL;
            token->value.other_value = EOL_V;   
        }
    } 
    else if (next == '*') {
        //Scenario: multiline line comment

        int previous = 0;   //stores previous character
        int depth = 1;      //Information of scope (nesting depth)

        //Skipping lines until the all the '/*' found their matching '*/'
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
            error_exit(1);
        }

        //Multiline comment should be treated as a whitespace, therefore let's ask for new token
        token_ptr new_token = get_token();
        *token = *new_token;   // prepíš obsah pôvodného tokenu
        free(new_token);
        new_token = NULL;
    }
    else {
        //Scenario: the '/' character was actually operator
        if (next == EOF){
            store_pending_eof();
        }
        else{
            ungetc(next, stdin);
        }
        token->type = OPERATOR;
        token->value.other_value = DIVISON_V;
    }
}

/**
 * @brief Peaks one character ahead to determine whether the token is <= / >= or just < / >
 * 
 * @param token to be filled
 * @return true when equal sign follow
 * @return false otherwise
 */
bool check_equal(token_ptr token, int operator){
    int next = fgetc(stdin); //Variable for reading characters from input stream

    if (next == '='){
        //Sets tokens attributes
        token->type = OPERATOR;
        if (operator == '<'){
            token->value.other_value = LESS_OR_EQ_THAN_V;
        }
        else{
            token->value.other_value = GREATER_OR_EQ_THAN_V;
        }
        //Equalsign was found successfully
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
 * @param token Token structure to fill
 */
void process_str_l(token_ptr token){
    int next;                           //Variable for reading characters from input stream
    unsigned index = 0;                 //String index

    //Allocating memory for the string
    if ((token->value.str_value = malloc(sizeof(char) * MAX_LINE_LEN)) == NULL){
        //warnings(99, "memory allocation failed at line: %d\n", 336);
        free_token(token);
        error_exit(99);
    }

    //Checking for potential multiline string
    int second = fgetc(stdin);
    if (second == '"'){
        int third = fgetc(stdin);
        if (third == '"'){
            //It's a multiline string
            process_mul_l_str(token);
            return;
        }
        else{
            //Empty string found
            if (third == EOF){
                store_pending_eof();
            }
            else{
                ungetc(third, stdin);    
            }
            token->type = ONE_L_STRING;
            token->value.str_value[0] = '\0';
            return;
        }
    }
    else{
        //We are processing one line string, put second back
        if (second != EOF && second != '\n'){
            ungetc(second, stdin);
        }
        else{
            //warnings(1, "unterminated string literal(EOF or newline)\n");
            free_token(token);
            error_exit(1);
        }
    }

    //Processing single line string - reading characters until EOF or '"'/' """ '
    while ((next = fgetc(stdin)) != EOF && next != '"' && next != '\n'){

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

    //The last character has to be '\0'
    token->value.str_value[index] = '\0';

    //After the while loop ended, the latest character read is: EOF / " / \n
    if (next == EOF){
        //warnings(1, "unterminated string literal(EOF)\n");
        free_token(token);
        error_exit(1);
    }
    else if(next == '\n'){
        //warnings(1, "unterminated string literal(newline)\n");
        free_token(token);
        error_exit(1);
    }
    
    //If got here, everything was read successfully
    token->type = ONE_L_STRING;
}

/**
 * @brief Processes escape sequences in strings
 * 
 * @param token Token structure being processed
 * @param index Pointer to current index in string buffer
 */
void process_escape_sequence(token_ptr token, unsigned *index) {
    //Variable to store 2nd part of the esc seq (\n, \r, \t, \\, \")
    int escape_char = fgetc(stdin);
    //Checks whether EOF was reached
    if (escape_char == EOF){
        //warnings(1, "unterminated escape sequence in string\n");
        free_token(token);
        error_exit(1);
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
            //Handles hecadecimal escape-sequances
            process_hex_escape(token, index);
            break;
        default:
            //warnings(1, "unknown escape sequence '\\%c'\n", escape_char);
            free_token(token);
            error_exit(1);
    }

    //Incrementing string index
    (*index)++;
}

/**
 * @brief Processes hexadecimal escape sequences (\xHH)
 * 
 * @param token Token structure being processed
 * @param index Pointer to current index
 */
void process_hex_escape(token_ptr token, unsigned *index) {
    //Buffer to store 2 hex digits + null terminator
    char hex_buffer[3];

    //Read exactly 2 hex digits
    int hex1 = fgetc(stdin);
    int hex2 = fgetc(stdin);
    
    //Checks whether EOF was reached
    if (hex1 == EOF || hex2 == EOF) {
        //warnings(1, "incomplete \\x escape sequence (EOF)\n");
        free_token(token);
        error_exit(1);
    }
    
    //Validate hex digits
    if (!isxdigit(hex1) || !isxdigit(hex2)) {
        //warnings(1, "invalid hex escape sequence \\x%c%c\n", hex1, hex2);
        free_token(token);
        error_exit(1);
    }
    
    //Creating string from hexa digits
    hex_buffer[0] = hex1;
    hex_buffer[1] = hex2;
    hex_buffer[2] = '\0';
    
    //Converting to ASCII
    long ascii_value = strtol(hex_buffer, NULL, 16);
    
    //Store character
    token->value.str_value[*index] = ascii_value;
    //Increments the string index
    //(*index)++;
}

/**
 * @brief Processes multiline string literals (""")
 * 
 * @param token Token structure to fill
 */
void process_mul_l_str(token_ptr token){
    int next;               //Variable for reading characters from input stream
    unsigned index = 0;     //Position in the string
    bool to_ignore = true;  //Tracks whitespace after opening """

    //Sets token's atributes
    token->type = MUL_L_STRING;

    //Skips whitespaces after opening """
    while (to_ignore && (next = fgetc(stdin)) != EOF){
        //When non-whitespace character encountered
        if (!isspace(next)){
            ungetc(next, stdin);
            to_ignore = false;
        }
    }
    
    //Reading characters until terminating """
    while ((next = fgetc(stdin)) != EOF){
        //Checks if there's enough space for the while string
        if (index >= (MAX_LINE_LEN - 1)){
            not_enough_space(token->value.str_value);
        }

        //Checks if this could be start of terminating """
        if (next == '"') {
            int second = fgetc(stdin);
            if (second == '"') {
                int third = fgetc(stdin);
                if (third == '"') {
                    //Terminating """ found - end of string reached
                    //Removes whitespace before closing """
                    while (index > 0 && isspace(token->value.str_value[index - 1])) {
                        index--;
                    }
                    token->value.str_value[index] = '\0';
                    return;
                }
                else{
                    //Checks whether EOF was reached
                    if (third == EOF){
                        //warnings(1, "unterminated string literal (EOF found)\n");
                        free_token(token);
                        error_exit(1);
                    }
                    //Stores character
                    token->value.str_value[index] = third;
                    //Increments index
                    index++;
                }
            }
            else{
                if (second == EOF){
                    //warnings(1, "unterminated string literal (EOF found)\n");
                    free_token(token);
                    error_exit(1);
                }
                //Stores character
                token->value.str_value[index] = second;
                //Increments index
                index++;
            }
        }
        //Normal character, not a "
        else {
            if (next == EOF){
                //warnings(1, "unterminated string literal(EOF)\n");
                free_token(token);
                error_exit(1);
            }
            
            //Store character
            token->value.str_value[index] = next;
            //Increments index
            index++;
        }
    }

    //There is no terminating sequence '"""'
    //warnings(1, "unterminated multiline string literal\n");
    free_token(token);
    error_exit(1);
}


/**
 * @brief Processes dot operators (., .., ...)
 * 
 * @param token Token structure to fill
 */
void process_dots(token_ptr token){
    int next = fgetc(stdin); //Variable for reading characters from input stream

    /*
    * We have already read '.' and want to peak ahead to find out
    * whether there is second or maybe even a third dot, since we
    * want to differenciate those 3 token types: DOT, DOUBLE_DOT, TRIPLE_DOT
    */

    if (next == '.') {
        //There already is "..", let's read one more character
        int third = fgetc(stdin);
        
        if (third == '.') {
            //There is "..."
            token->type = TRIPLE_DOT;
            token->value.other_value = TRIPLE_DOT_V;
            return;
        } else {
            //There is "..", we need to put back the previously read character
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
        if (next == EOF){
            store_pending_eof();
        }
        else{
            ungetc(next, stdin);
        }
        
        token->type = DOT;
        token->value.other_value = DOT_V;
    }
}

/**
 * @brief Processes numeric literals (integer, float, hexadecimal)
 * 
 * @param token Token structure to fill
 * @param first_char First digit character (0-9) already read from input
 */
void process_number(token_ptr token, int first_char){
    int digit;              //Variable for reading next character
    char *temp_buffer;      //Temporary buffer for storing numeric string
    unsigned index = 0;     //Index in the buffer
    bool is_hexa = false;   //Tracks whether the variable is hexadecimal

    //Allocating memory for temp buffer
    if ((temp_buffer = malloc(sizeof(char) * MAX_DIGITS)) == NULL){
        //warnings(99, "memory allocation failed\n");
        free_token(token);
        error_exit(99);
    }
    
    //Inserts first character
    temp_buffer[index] = first_char;
    //Incrementing index
    index++; 

    //When the first character was '0'
    if (first_char == '0'){
        //Reads next character
        digit = fgetc(stdin);
        
        switch (digit){
        case '.':
            //Storying decimal point
            temp_buffer[index] = digit;
            index++;
            //Takes care of the float
            process_float(token, temp_buffer, &index);
            break;
        case 'e':
        case 'E':
            //It's exponential notation starting with 0

            //Storying decimal point
            temp_buffer[index] = digit;
            index++;
            //Takes care of the rest
            process_exp(token, temp_buffer, &index);
            break;
        case 'x':
            //It's a hexadecimal number
            is_hexa = true;
            //Stores x for later convertion from hexa to decimal
            temp_buffer[index] = 'x';
            index++;
            
            //Reads hexadecimal digits
            while (isxdigit(digit = fgetc(stdin))){
                temp_buffer[index] = digit;
                index++;
                //Check if buffer is full
                if (index >= MAX_DIGITS){
                    //warnings(1, "numeric literal too long\n");
                    free_token(token);
                    free(temp_buffer);
                    temp_buffer = NULL;
                    error_exit(1);
                }
            }

            //If no hexadecimal digits were added after '0x'
            if (index == 2){
                //warnings(1, "invalid hexadecimal literal\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(1);
            }

            //Have to check whether the character that ended the loop could immediately follow number
            if (isalpha(digit) || digit == '_'){
                free_token(token);
                free(temp_buffer);
                error_exit(1);
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
                error_exit(1);
            }
            else{
                //Have to check whether the character that ended the loop could immediately follow number
                if (isalpha(digit) || digit == '_'){
                    free_token(token);
                    free(temp_buffer);
                    temp_buffer = NULL;
                    error_exit(1);
                }

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
    //When first character was non-zero digit (1-9)
    else if (isdigit(first_char) && (first_char != '0')){
        //Reads while the characters are digit characters
        while (isdigit(digit = fgetc(stdin))){
            temp_buffer[index] = digit;
            index++;
            //Check if buffer is full
            if (index >= MAX_DIGITS){
                //warnings(1, "invalid number format (leading zeros)\n");
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(1);
            }
        }
        
        //After the while loop terminated, check what character stopped the loop
        switch (digit){
        case '.':
            //Storying decimal point
            temp_buffer[index] = digit;
            index++;
            //Takes care of the rest
            process_float(token, temp_buffer, &index);
            break;
        case 'e':
        case 'E':
            //It's exponential notation

            //Storying decimal point
            temp_buffer[index] = digit;
            index++;
            //Takes care of the rest
            process_exp(token, temp_buffer, &index);
            break;
        default:
            //Have to check whether the character that ended the loop could immediately follow number
            if (isalpha(digit) || digit == '_'){
                free_token(token);
                free(temp_buffer);
                temp_buffer = NULL;
                error_exit(1);
            }

            //The character doesn't belong to the number
            if (digit == EOF){
                store_pending_eof();
            }
            else{
                ungetc(digit, stdin);
            }
        }
    }

    //Terminate buffer with '\0'
    temp_buffer[index] = '\0';

    //Convert string to appropriate numeric value
    if (is_hexa){
        token->value.int_value = strtol(temp_buffer, NULL, 0);
        token->type = INT_LIT;
    }
    else if (token->type == FLOAT_LIT){
        token->value.float_value = strtod(temp_buffer, NULL);
    }
    else{
        token->value.int_value = strtol(temp_buffer, NULL, 10);
        token->type = INT_LIT;
    }
    
    free(temp_buffer);
    temp_buffer = NULL;
}

/**
 * @brief Processes decimal part and optional exponent of float
 * 
 * @param token Token structure to fill
 * @param buffer String buffer containing digits before decimal point
 * @param buf_index Current position in buffer
 */
void process_float(token_ptr token, char *buffer, unsigned *buf_index){
    int digit;      //Varaible for reading characters from input stream
    int count = 0;  //Counts how many decimal numbers the number contains
    
    //Read decimal digits
    while (isdigit(digit = fgetc(stdin))){
        buffer[*buf_index] = digit;
        (*buf_index)++;
        count++;
    }

    //Options that digit could contain after the while loop ended
    if (digit == EOF){
        buffer[*buf_index] = '\0';
        store_pending_eof();
    }
    //Check if there's an exponent
    else if(digit == 'e' || digit == 'E'){
        //Storying decimal point
        buffer[*buf_index] = digit;
        (*buf_index)++;
        //Takes care of the rest
        process_exp(token, buffer, buf_index);
    }
    else{
        //Have to check whether the character that ended the loop could immediately follow number
        if (isalpha(digit) || digit == '_'){
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(1);
        }

        //Nummbers like 5. without decimal point are invalid
        if (count == 0){
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(1);
        }
        
        //Return the character that stopped the loop
        ungetc(digit, stdin);
        //Terminate the string
        buffer[*buf_index] = '\0';
    }
    
    token->type = FLOAT_LIT;
}

/**
 * @brief Processes exponent notation (e/E) of numeric literal
 * 
 * @param token Token structure to fill
 * @param buffer String buffer containing digits and decimal point
 * @param buf_index Current position in buffer
 */
void process_exp(token_ptr token, char *buffer, unsigned *buf_index){
    int digit;      //Variable for reading characters from input stream
    int count = 0;  //Variable to determine how many digits were read
    
    /* Even though int literal can use scientific notation, there's not smart enough function
    *  in C that could detect and convert this number from string into decimal. Therefore
    *  I'm treating this number as float, idc. 
    */

    token->type = FLOAT_LIT;

    //Reads next character (optional sign or digit)
    digit = fgetc(stdin);
    if (digit == EOF){
        //warnings(1, "invalid exponent format\n");
        free_token(token);
        free(buffer);
        buffer = NULL;
        error_exit(1);
    }
    //Check for optional sign
    else if (digit == '+' || digit == '-'){
        buffer[*buf_index] = digit;
        (*buf_index)++;
            
        //Reads next digit
        digit = fgetc(stdin);
    }

    //Read exponent digits
    while (isdigit(digit)){
        //Tracks how many digits were read
        count++;
        //Stores digits into buffer
        buffer[*buf_index] = digit;
        (*buf_index)++;
        digit = fgetc(stdin);
    }

    //Exponent need at least one digit
    if (count == 0){
        //warnings(1, "invalid exponent format\n");
        free_token(token);
        free(buffer);
        buffer = NULL;

        error_exit(1);
    }
    
    //Return the character that stopped the loop
    if (digit == EOF){
        buffer[*buf_index] = '\0';
        store_pending_eof();
    }
    else{
        //Have to check whether the character that ended the loop could immediately follow number
        if (isalpha(digit) || digit == '_'){
            free_token(token);
            free(buffer);
            buffer = NULL;
            error_exit(1);
        }
        //Else just terminate string and return the character read
        buffer[*buf_index] = '\0';
        ungetc(digit, stdin);
    }
}

/**
 * @brief Creates and stores pending EOF token
 * 
 */
void store_pending_eof() {
    //Alocates new pending token
    if (pending_token == NULL) {
        if((pending_token = malloc(sizeof(token_t))) == NULL){
            //warnings(99, "memory allocation failed for EOF token\n");
            error_exit(99);
        }
    }
    //Sets tokens attributes
    pending_token->type = END_OF_FILE;
    pending_token->value.other_value = EOF;
    //Updates global variable
    eof_reached = true;
}
