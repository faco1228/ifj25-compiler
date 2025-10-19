#include "error.h"
#include "scanner.h"

//Initializing global variables
bool has_been_pushed = false;
token_ptr pushed_token = NULL;

const char *key_words_arr[] = {
    "class", "if", "else", "is", "null", "return", "var", "while", "Ifj",
    "static", "true", "false", "Num", "String", "Null", "Break", "Continue"
};

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
    //Initializing token
    token_ptr token;
    if ((token = malloc(sizeof(token_t)) == NULL)){
        //NOTE: don't forget to delete this later
        warnings(99, "memory allocation failed at line: %d\n", 33);
        error_exit(99);
    }

    //When the token has been returned from parser
    if (has_been_pushed){
        //Next token is the on that's been returned
        token = pushed_token;
        //Updating global variables
        has_been_pushed = false;
        pushed_token = NULL;

        return token;
    }else{
        //Asks for new token
        process_next_token(token); 
    }
    
    return token;
}

/**
 * @brief Reads input and identifies token type
 * 
 * @param token Token structure to fill
 * @return Pointer to new token
 */
token_ptr process_next_token(token_ptr token){
    //Variable for reading charcter from input stream
    int character;

    //Reads char from input stream
    character = fgetc(stdin);
    //Case: EOF was reached
    if (character == EOF){
        token->type = END_OF_FILE;
        token->value.other_value = EOF;
        return token;
    }
    
    //When the first character is alphabetic or "_"
    if (isalpha(character) || (character == '_')){
        //Allocating memory for the idents name
        if ((token->value.name = malloc(sizeof(char) * MAX_LEN)) == NULL){
            //NOTE: don't forget to delete this later
            warnings(99, "memory allocation failed at line: %d\n", 78);
            error_exit(99);
        }
        //Character read is the first letter of the name
        token->value.name[0] = character;
        //Will take care of the rest
        process_ident(token);
    }
    else if (isdigit(character)){
        //Will take care of the rest
        process_number(token, character);
    }
    
    else{
        //Skips whitespaces
        while (isspace(character)) {
            //When the WS is '\n'
            if (character == '\n') {
                token->type = EOL;
                token->value.other_value = '\n';
                return token;
            }
            //Reads next character
            character = fgetc(stdin);
            //When the WS is EOF
            if (character == EOF) {
                token->type = END_OF_FILE;
                token->value.other_value = EOF;
                return token;
            }
        }

        //Let's decide what to do with the read character
        switch (character){
        case '+':
            token->type = OPERATOR;
            token->value.other_value = '+';
            break;
        case '-':
            //Becareful, this can be either subtraction operator or unary operator
            token->type = MINUS;
            token->value.other_value = '-';
            break;
        case '*':
            token->type = OPERATOR;
            token->value.other_value = '*';
            break;
        case '/':
            /*
            * The function handles all three situations:
            * a) oneline comment
            * b) multiline comment
            * c) operator
            */
            skip_comments(token);
            break;
        case '=':
            token->type = OPERATOR;
            token->value.other_value = '=';
            break;
        case '(':
            token->type = LEFT_PAR;
            token->value.other_value = '(';
            break;
        case ')':
            token->type = RIGHT_PAR;
            token->value.other_value = ')';
            break;
        case '{':
            token->type = LEFT_DOM_PAR;
            token->value.other_value = '{';
            break;
        case '}':
            token->type = RIGHT_DOM_PAR;
            token->value.other_value = '}';
            break;
        case '.':
            /*
            * This function processes all three options:
            * a).
            * b)..
            * c)...
            */
            process_dots(token);
            break;
        case EOL:
            token->type = EOL;
            token->value.other_value = '\n';
            break;
        case '"':
            //This function processes both one and multiline str literals
            process_str_l(token);
            break;
        case '?':
            token->type = Q_MARK;
            token->value.other_value = '?';
            break;
        case ':':
            token->type = SEMICOLON;
            token->value.other_value = ':';
            break;
        case '<':
            token->type = OPERATOR;
            token->value.other_value = '<';
            break;
        case '>':
            token->type = OPERATOR;
            token->value.other_value = '>';
            break;
        case '!':
            token->type = OPERATOR;
            token->value.other_value = '!';
            break;
        case ',':
            token->type = COMMA;
            token->value.other_value = ',';
            break;
        default:
            //NOTE: don't forget to delete this later
            warnings(1, "unexpected character: '%c' (ASCII %d)\n", character, character);
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
    //Variable for reading characters from input stream
    int next;
    //Variable to determine whether the token is still being read
    bool token_unread;
    //Index of the ident's name
    unsigned index = 1;
    
    //Reads the rest of the token
    do{
        //Reads next character
        next = fgetc(stdin);
        
        //Evaluates whether the char still belongs to the indent being processed
        token_unread = is_ident(next);

        if (token_unread){
            //Updates IDENT name
            token->value.name[index] = next;
            //Incrementing index
            index++;
        }
    }while(token_unread);
    
    //The last character has to be '\0'
    token->value.name[index] = '\0';

    /** After while loop terminated there's unwanted value stored in c:
     * A) EOF
     * B) white sapce: ' ', '\n', '\t'
     * C) Next tokens first character
     */

    //Retruns character read into input streams buffer
    if (!isspace(next) && next != EOF){
        ungetc(next, stdin);
    }

    //Now we have to compare IDENT's name with all possible key words
    for (int i = 0; key_words_arr[i] != NULL; i++){
        //When match was found
        if (!strcmp(token->value.name, key_words_arr[i])){
            //Sets token's parameteres
            token->type = KEY_WORD;
            token->value.str_value = key_words_arr[i];
            return;
        }
    }

    //When the token isn't KW but it's global variable
    if (token->value.name[0] == '_' && token->value.name[1] == '_'){
        token->type = GLOB_VAR;
        //token->name is already set
    }
    else{
        //When the token isn't KW nor global
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
    //Variable for reading characters from input stream
    int next;
   
    //Reads next characet
    next = fgetc(stdin);

    //Scenario: one line comment
    if (next == '/') {
        //Skips one line
        while ((next = fgetc(stdin)) != '\n' && next != EOF);
        token->type = EOL;
        token->value.other_value = '\n';
    } else if (next == '*') {
        //Scenario: multiline line comment

        //Variable for storing previous character
        int previous = 0;
        //Information of scope (nesting depth)
        int depth = 1;
        //Skipping lines until the all the '*/' are found
        while (depth > 0 && (next = fgetc(stdin)) != EOF) {
            if (previous == '/' && next == '*'){
                depth++;
            }
            if (previous == '*' && next == '/'){
                depth--;
            }
            previous = next;
        }
        /**
         * Calls get_token() one more time, so the parser woudn't receive
         * an empty token after skipping comments.
         */
        token = get_token();
    }else {
        //Scenario: the '/' character was actually operator
        if (next != EOF){
            ungetc(next, stdin);
        }
        token->type = OPERATOR;
        token->value.other_value = '/';
    }
}

/**
 * @brief Processes string literals (single-line and multiline)
 * 
 * @param token Token structure to fill
 */
void process_str_l(token_ptr token){
    //Variable for reading characters from input stream
    int next;
    //String index
    unsigned index = 0;

    //Allocating memory for the string
    if ((token->value.str_value = malloc(sizeof(char) * MAX_LINE_LEN)) == NULL){
        //NOTE: don't forget to delete this later
        warnings(99, "memory allocation failed at line: %d\n", 336);
        error_exit(99);
    }

    //Reading next character until '"' / '\n' / EOF
    while (((next = fgetc(stdin)) != '"') && (next != EOF) && (next != '\n')){
        //Reads next character
        next = fgetc(stdin);

        //Checks if there's enough space to store the whole string
        if (index >= (MAX_LINE_LEN - 1)){
            not_enough_space(token->value.str_value);
        }
        
        //Checks if the character is an escape sequance
        if (next == '\\'){
            //Variable to store 2nd part of the esc seq character (\n, \r, \t, \\, \")
            int escape_char = fgetc(stdin);
            //Checks whether EOF was reached
            if (escape_char == EOF) {
                //NOTE: don't forget to delete this later
                warnings(1, "unterminated escape in string\n");
                error_exit(1);
            }
            //Variables to store hexadeximal number
            int hexa1, hexa_val_1;
            int hexa2, hexa_val_2;
            int ascii_repre = 0;

            switch (escape_char){
            case 'n':
                token->value.str_value[index] = '\n';
                break;
            case 'r':
                token->value.str_value[index] = '\r';
                break;
            case 't':
                token->value.str_value[index] = '\t';
                break;
            case '\\':
                token->value.str_value[index] = '\\';
                break;
            case '"':
                token->value.str_value[index] = '"';
                break;
            case 'x':
                //When 2 hexadecimal numbers follow, it represents an ASCII value of character
                hexa1 = fgetc(stdin);
                hexa2 = fgetc(stdin);
                //Checks whether EOF was reached
                if (hexa1 == EOF || hexa2 == EOF){
                    //NOTE: don't forget to delete this later
                    warnings(1, "invalid \\x escape (EOF)\n");
                    error_exit(1);
                }
                //Converts hexadecimal character into an integer value
                int hexa_val_1 = hex_digit_value(hexa1);
                int hexa_val_2 = hex_digit_value(hexa2);
                //Checks if the 2 hexa digits are valid
                if (hexa_val_1 < 0 || hexa_val_2 < 0){
                    //NOTE: don't forget to delete this later
                    warnings(1, "invalid hex escape sequence in string\n");
                    error_exit(1);
                }
                //Converts hexadec number into ascii value
                ascii_repre = (hexa_val_1 << 4) | hexa_val_2;
                //Stores char with matching ascii value into string
                token->value.str_value[index] = ascii_repre;
                index++;
                break;
            default:
                //NOTE: don't forget to delete this later
                warnings(1, "unknown escape sequence '\\%c'\n", escape_char);
                error_exit(1);
                break;
            }
        }else if (next == '"'){
            //Peaks one more character ahead
            int third = fgetc(stdin);
            // "" is also considered a valid string literal
            if (third != '"'){
                token->type = ONE_L_STRING;
                token->value.str_value = "";
                //If EOF wasn't reach, the character read has to be returned
                if (third != EOF){
                    ungetc(third, stdin);
                }
                
                //Terminate string and return
                token->value.str_value[index] = '\0';
                return;
            }
            
            //We have to process the multiline string literal
            process_mul_l_str(token);
            return;
        }else {
            //Stores character
            token->value.str_value[index] = next;
            //Increments index
            index++;
        }
    }

    //The last character has to be '\0'
    token->value.str_value[index] = '\0';

    //After the while loop terminated the character read is either: '"'/'EOF'/'\n'
    if (next == '"'){
        //Sets token parameters
        token->type = ONE_L_STRING;
        //token->value.str_value has already been set
    }else if (next == '\n'){
        //NOTE: don't forget to delete this later
        warnings(1, "unsoported string format at %d\n line of code", 452);
        error_exit(1);
    }else if (next == EOF){
        //NOTE: don't forget to delete this later
        warnings(1, "unterminated string literal (EOF found)\n");
        error_exit(1);
    }
}

/**
 * @brief Processes multiline string literals (""")
 * 
 * @param token Token structure to fill
 */
void process_mul_l_str(token_ptr token){
    //Variable for reading characters from input stream
    int next;
    //Position in the string
    unsigned index = 0;
    //Tracks whitespace after opening """
    bool to_ignore = true;

    //Sets token's parameters
    token->type = MUL_L_STRING;

    //Allocating memory for the string
    if ((token->value.str_value = malloc(sizeof(char) * MAX_LINE_LEN)) == NULL){
        warnings(99, "memory allocation failed\n");
        error_exit(99);
    }

    //Reading characters until we find terminating """
    while ((next = fgetc(stdin)) != EOF) {
        //Checks if this could be start of terminating """
        if (next == '"') {
            int second = fgetc(stdin);
            if (second == '"') {
                int third = fgetc(stdin);
                if (third == '"') {
                    //Terminating """ found - end of string reached
                    //Removs whitespace before closing """
                    while (index > 0 && isspace(token->value.str_value[index - 1])) {
                        index--;
                    }
                    token->value.str_value[index] = '\0';
                    return;
                }
                //Terminating """ weren't found, we have to put previously read characters back into input stream buffer
                if (third != EOF){
                    ungetc(third, stdin);
                }
                //Don't have to test this one for EOF, else woudn't have gotten here
                ungetc(second, stdin);
                //Updating status variable
                to_ignore = false;
                /*
                * Stores character (since terminating sequence """ wasn't found
                * we have to store the read '"' as normal character)
                */
                token->value.str_value[index] = next;
                //Increments index
                index++;
            }
            else {
                //Not even "", have to put the second character back...
                if (second != EOF){
                    ungetc(second, stdin);
                }
                //Updating status variable
                to_ignore = false;
                //Stores character
                token->value.str_value[index] = next;
                //Increments index
                index++;
            }
        }
        //Normal character (not a '"')
        else {
            //If we were after opening """ and this is not whitespace
            if (to_ignore && !isspace(next)) {
                to_ignore = false;
            }
            
            //If we were after opening """ and this is a whitespace
            if (to_ignore && isspace(next)) {
                continue;
            }
            
            //Store character
            token->value.str_value[index] = next;
            //Increments index
            index++;
        }

        // Reallocate if needed
        if (index >= (MAX_LINE_LEN - 1)) {
             not_enough_space(token->value.str_value);
        }
    }

    //When there is no terminating sequence '"""'
    //NOTE: don't forget to delete this later
    warnings(1, "unterminated multiline string literal\n");
    error_exit(1);
}

/**
 * @brief Converts hexadecimal character to integer value
 * 
 * @param c Character to convert ('0'-'9', 'a'-'f', 'A'-'F')
 * @return Integer value 0-15 or -1 if invalid
 */
int hex_digit_value(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

/**
 * @brief Processes dot operators (., .., ...)
 * 
 * @param token Token structure to fill
 */
void process_dots(token_ptr token){
    //Variable for reading next character from input stream
    int next = fgetc(stdin);

    /*
    * We have already read '.' and want to peak ahead to find out
    * whether there is second or maybe even a third dot, since we
    * want to differenciate these 3 tokens: DOT, DOUBLE_DOT, TRIPLE_DOT
    */

    if (next == '.') {
        //There already is "..", let's read one more character
        int third = fgetc(stdin);

        //Allocating memory for the ".." or "..." strings
        if ((token->value.str_value = malloc(sizeof(char) * 4)) == NULL){
            //NOTE: don't forget to delete this later
            warnings(99, "memory allocation failed at line: %d\n", 622);
            error_exit(99);
        }
        
        if (third == '.') {
            //There is "..."
            token->type = TRIPE_DOT;
            strcpy(token->value.str_value, "...");
        } else {
            //There is "..", we need to put back the previously read character
            if (third != EOF){
                ungetc(third, stdin);
            }
            token->type = DOUBLE_DOT;
            strcpy(token->value.str_value, "..");
        }
    } else {
        //There is ".", we need to put back the previously read character
        if (next != EOF){
            ungetc(next, stdin);
        }
        token->type = DOT;
        token->value.other_value = '.';
    }
}

/**
 * @brief Processes numeric literals (integer, float, hexadecimal)
 * 
 * @param token Token structure to fill
 * @param first_char First digit character (0-9) already read from input
 */
void process_number(token_ptr token, int first_char){
    //Variable for reading next character
    int digit;
    //Temporary buffer for storing numeric string
    char *temp_buffer;
    unsigned index = 0;
    //Tracking variables
    bool is_hexa = false;
    bool is_float = false;

    //Allocating memory for temp buffer
    if ((temp_buffer = malloc(sizeof(char) * MAX_LEN)) == NULL){
        warnings(99, "memory allocation failed\n");
        error_exit(99);
    }
    
    //Inserts first character
    temp_buffer[index] = first_char;
    index++;

    //When first character was '0'
    if (first_char == '0'){
        //Reads next character
        digit = fgetc(stdin);
        
        switch (digit){
        case '.':
            //It's a float starting with 0
            is_float = true;
            process_float(token, temp_buffer, index);
            break;
        case 'e':
        case 'E':
            //It's exponential notation starting with 0
            process_exp(token, temp_buffer, index);
            break;
        case 'x':
            //It's a hexadecimal number
            is_hexa = true;
            //Moves past 'x'
            index++;  
            
            //Reads hexadecimal digits
            while (isxdigit(digit = fgetc(stdin))){
                temp_buffer[index] = digit;
                index++;
                //Check if buffer is full
                if (index >= MAX_LEN - 1){
                    not_enough_space(temp_buffer);
                }
            }

            //If no hexadecimal digits were added after '0x'
            if (index == 2){
                //NOTE: Don't forget to delete this later
                warnings(1, "invalid hexadecimal literal\n");
                error_exit(1);
            }

            //The previously read character has to be returned
            if (digit != EOF){
                ungetc(digit, stdin);
            }
            break;
        default:
            //Leading zeros are not allowed for decimal numbers
            if (isdigit(digit)){
                //NOTE: Don't forget to delete this later
                warnings(1, "invalid number format (leading zeros)\n");
                error_exit(1);
            }
            //Return the character if it's not a digit
            if (digit != EOF){
                ungetc(digit, stdin);
            }
            break;
        }
    }
    //When first character was non-zero digit (1-9)
    else if (isdigit(digit) && (digit != '0')){
        //Stores the initial digit
        temp_buffer[index] = digit;
        index++;
    
        //Reads while the characters are digit characters
        while (isdigit(digit = fgetc(stdin))){
            temp_buffer[index] = digit;
            index++;
            //Check if buffer is full
            if (index >= MAX_LEN - 1){
                not_enough_space(temp_buffer);
            }
        }
        
        //After the while loop terminated, check what character stopped the loop
        switch (digit){
        case '.':
            //It's a float
            is_float = true;
            process_float(token, temp_buffer, index);
            break;
        case 'e':
        case 'E':
            //It's exponential notation
            process_exp(token, temp_buffer, index);
            break;
        default:
            //The character doesn't belong to the number
            if (digit != EOF){
                ungetc(digit, stdin);
            }
        }
    }

    //Set token type if not already set by process_float or process_exp
    if (token->type != FLOAT_LIT){
        token->type = INT_LIT;
    }
    
    //Convert string to appropriate numeric value
    if (is_hexa){
        token->value.int_value = strtol(temp_buffer, NULL, 16);
    }
    else if (is_float){
        token->value.float_value = strtod(temp_buffer, NULL);
    }
    else{
        token->value.int_value = strtol(temp_buffer, NULL, 10);
    }
    
    free(temp_buffer);
}

/**
 * @brief Processes decimal part and optional exponent of float
 * 
 * @param token Token structure to fill
 * @param buffer String buffer containing digits before decimal point
 * @param buf_index Current position in buffer
 */
void process_float(token_ptr token, char *buffer, unsigned buf_index){
    //Varaible for reading characters from input stream
    int digit;
    
    //Add the decimal point to buffer
    buffer[buf_index] = '.';
    buf_index++;
    
    //Read decimal digits
    while (isdigit(digit = fgetc(stdin))){
        buffer[buf_index] = digit;
        buf_index++;
    }
    
    //Check if there's an exponent
    if (digit == 'e' || digit == 'E'){
        process_exp(token, buffer, buf_index);
    }
    else{
        //Return the character that stopped the loop
        if (digit != EOF){
            ungetc(digit, stdin);
        }
        //Terminate the string
        buffer[buf_index] = '\0';
        token->type = FLOAT_LIT;
    }
}

/**
 * @brief Processes exponent notation (e/E) of numeric literal
 * 
 * @param token Token structure to fill
 * @param buffer String buffer containing digits and decimal point
 * @param buf_index Current position in buffer
 */
void process_exp(token_ptr token, char *buffer, unsigned buf_index){
    //Variable for reading characters from input stream
    int digit;
    
    //Add the 'e' or 'E' to buffer
    buffer[buf_index] = 'e';
    buf_index++;
    
    //Reads next character (optional sign or digit)
    digit = fgetc(stdin);
    
    //Check for optional sign
    if (digit == '+' || digit == '-'){
        buffer[buf_index] = digit;
        buf_index++;
        digit = fgetc(stdin);
    }
    
    //Exponent must have at least one digit
    if (!isdigit(digit)){
        //NOTE: don't forget to delete this later
        warnings(1, "invalid exponent format\n");
        error_exit(1);
    }
    
    //Read exponent digits
    while (isdigit(digit)){
        buffer[buf_index] = digit;
        buf_index++;
        digit = fgetc(stdin);
    }
    
    //Return the character that stopped the loop
    if (digit != EOF){
        ungetc(digit, stdin);
    }
    
    //Terminate the string
    buffer[buf_index] = '\0';
}
