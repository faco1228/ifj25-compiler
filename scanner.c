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
 * @param token 
 */
void push_token(token_ptr token){
    has_been_pushed = true;
    pushed_token = token;
}

/**
 * @brief Reades, decodes and returns token from input WREN-like code
 * 
 * @return token_ptr 
 */
token_ptr get_token(){
    //Initializing token
    token_ptr token;
    if ((token = malloc(sizeof(token_t)) == NULL)){
        //NOTE: don't forget to delete this later
        warnings(99, "memory allocation failed at line: %d\n", 33);
        error_exit(99);
    }

    //When the token has been returned from parcer
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
 * @brief Reads input and identifies tokens
 * 
 * @param token 
 * @return token_ptr 
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
 * @brief processes tokens that are indetificators
 * 
 * @param token 
 */
void process_ident(token_ptr token){
    //Variable:
    //For reading characters from input stream
    int next;
    //To determine whether the token has been read
    bool token_unread;
    //Index of the idents name
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

    //Now we have to compare IDENTs name with all possible key words:
    for (int i = 0; key_words_arr[i] != NULL; i++){
        //When match was found
        if (!strcmp(token->value.name, key_words_arr[i])){
            //Sets tokens parameteres
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
 * @brief Skips one and multiline comments in the input stream
 * 
 * @param token
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
        //Information of scope
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
 * @brief processes tokens that are string literals
 * 
 * @note by string literals I mean both one and multiline literals
 * 
 * @param token 
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
            if ((token->value.str_value = realloc(token->value.str_value, strlen(token->value.str_value + 1) * 2)) == NULL){
                //NOTE: don't forget to delete this later
                warnings(99, "memory allocation failed at line: %d\n", 349);
                error_exit(99);
            }
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
            default:
                //NOTE: don't forget to delete this later
                warnings(1, "unknown escape sequence '\\%c'\n", escape_char);
                error_exit(1);
                break;
            }
            //Increments position in string literal
            index++;
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
                
                //With this break, it will skip the token paramter updates at the end of the function
                break;
            }
            
            //We have to process the multiline string literal
            process_mul_l_str(token);
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
 * @param token
 *
 */
void process_mul_l_str(token_ptr token){
    //Variable for reading characters from input stream
    int next;
    //Position in the string
    unsigned index = 0;
    //Tracks the position in string, where whitespaces should be ignored
    bool to_ignore = true;

    //Sets tokens parameters
    token->type = MUL_L_STRING;

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
            if((token->value.str_value = realloc(token->value.str_value, (strlen(token->value.str_value) + 1) * 2)) == NULL){
                //NOTE: don't forget to delete this later
                warnings(99, "memory allocation failed\n");
                error_exit(99);
            }
        }
    }

    //When there is no terminating sequence '"""'
    //NOTE: don't forget to delete this later
    warnings(1, "unterminated multiline string literal\n");
    error_exit(1);
}

/**
 * @brief Converts hexadecimal character to its integer value
 * 
 * @param c 
 * @return int 
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
 * @param token 
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

void process_number(token_ptr token, int first_char){
    //Variable for reading next character
    int digit;
    //Temp array for storying individual digits
    int *temp_buffer;
    unsigned index = 0;
    //Helpful tracking variables
    bool is_hexa = false;
    bool is_float = false;

    if ((temp_buffer = malloc(sizeof(char) * MAX_LEN)) == NULL){
            //NOTE: don't forget to delete this later
            warnings(99, "memory allocation failed at line: %d\n", 627);
            error_exit(99);
    }
    //When the number is too big and doesn't fit into the temp_buffer
    if (strlen(temp_buffer) >= MAX_LEN){
        if((temp_buffer = realloc(temp_buffer, (strlen(temp_buffer) + 1) * 2)) == NULL){
            //NOTE: don't forget to delete this later
            warnings(99, "memory allocation failed at line: %d\n", 635);
            error_exit(99);
        }
    }
    
    //Inserts first character
    temp_buffer[index] = first_char;
    index++;

    //Reads next character
    digit = fgetc(stdin);

    if (digit == '0'){
        //Stores the initial digit
        temp_buffer[index] = digit;
        index++;
        //reads next character
        digit = fgetc(stdin);
        
        switch (digit){
        case '.':
            process_float(token);
            token->type = FLOAT_LIT;
            is_float = true;
            break;
        case 'e':
        case 'E':
            process_exp(token);
            break;
        case 'x':
            is_hexa = true;
            while (isxdigit(digit = fgetc(stdin))){
                temp_buffer[index] = digit;
                index++;
            }
            //If the loop didn't add any new charcater
            if (temp_buffer[index] == 'x'){            
                //NOTE: Don't forget to delete this later
                warnings(1, "Invalid hexadecimal literal\n");
                error_exit(1);
            }

            /*
            * After the while loop terminated and some digits were added...
            * The previously read character has to be returned.
            */
            if (digit != EOF){
                ungetc(digit, stdin);
            }
            break;
        default:
            //Nonzero integers can't begin with extra 0

            //NOTE: don't forget to delete this later
            warnings(1, "invalid number format detected at line %d\n", 675);
            error_exit(1);
            break;
        }
    }
    else if (isdigit(digit) && (digit != '0')){
        //Stores the initial digit
        temp_buffer[index] = digit;
        index++;
    
        //Reads while the characters are numbers digits
        while (isdigit(digit = fgetc(stdin))){
            temp_buffer[index] = digit;
            index++;
        }
        //After the while loop terminated, there are few options
        switch (digit){
        case '.':
            is_float = true;
            process_float(token);
            token->type = FLOAT_LIT;
            break;
        case 'e':
        case 'E':
            process_exp(token);
            break;
        default:
            // the character doesn't belong to the hexa number
            if (digit != EOF){
                ungetc(digit, stdin);
            }
        }
    }

    if (token->type != FLOAT_LIT){
        token->type = INT_LIT;
    }
        
    if (is_hexa){
        token->value.int_value = strtol(temp_buffer, NULL, 16);
    }
    else{
        token->value.int_value = strtol(temp_buffer, NULL, 16);   
    }
}

void process_float(token_ptr token){

}

void process_exp(token_ptr token){

}
