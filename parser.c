/**
 * @file parser.c
 * @author Samuel Facka (xfackas00)
 * @brief 
 * @version 0.1
 * @date 2025-10-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "parser.h"

#define PARSE_OK 0
#define PARSE_ERROR ERR_SYNTACTIC

// forward prototypes (internal)
static int  parse_prolog(void);
static int  parse_class_def(void);
static int  parse_class_body(void);
static int  parse_definition(void);
static int  parse_function_def(token_ptr id);
static int  parse_setter_def(token_ptr id);
static int  parse_getter_def(token_ptr id);
static int  parse_param_list(void);
static int  parse_block(void);
static int  parse_statement_list(void);
static int  parse_statement_or_block(void);
static int  parse_statement(void);
static int  parse_var_def(void);
static int  parse_assign_target(void);
static int  parse_assignment_or_call(void);
static int  parse_exp_right_side(void);
static int  parse_arg_list(void);
static int  parse_if_statement(void);
static int  parse_while_statement(void);
static int  parse_return_statement(void);
static int  parse_for_statement(void);
static int  parse_break_statement(void);
static int  parse_continue_statement(void);

// helper functions
static token_ptr expect_keyword(char *keyword);
static token_ptr expect_ident(void);
static token_ptr expect_type(enum token_type exp_tok);
static token_ptr look_ahead(void);
static void consume_eols(void);

/**
 * @brief Entry point of the recursive-descent parser.
 *
 * Grammar:
 * @code
 * <program> ::= <prolog> <class_def> EOF
 * @endcode
 *
 * @note
 *  - Skips leading EOLs.
 *  - Parses the prolog and the single class definition.
 *  - Requires EOF after the class.
 *
 * @return PARSE_OK (0) on success. On a syntax error, it calls
 *         error_exit(ERR_SYNTACTIC) and the function does not return.
 */
int parse_program(void){
    // edge case if multiple EOLs
    consume_eols();

    if (parse_prolog() != 0) error_exit(PARSE_ERROR);

    consume_eols();
    
    if (parse_class_def() != 0) error_exit(PARSE_ERROR);

    consume_eols();

    token_ptr token = get_token();
    // expect EOF
    if (token->type != END_OF_FILE) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    scanner_cleanup();
    return PARSE_OK;
}

/**
 * @brief Parse the mandatory prolog line: @c import "ifj25" for Ifj EOL
 *
 * Grammar:
 * @code
 * <prolog> ::= "import" STRING_LITERAL "for" ID EOL
 * @endcode
 * where STRING_LITERAL must be "ifj25" and ID must be "Ifj".
 *
 * @note
 *  - EOLs are currently tolerated after @c import and @c for
 *    (according to project discussion), but never directly after "ifj25"
 *    before @c for.
 *
 * @return PARSE_OK on success, otherwise calls error_exit(ERR_SYNTACTIC).
 */
static int parse_prolog(void) { 
    token_ptr token;

    // check for expected "import"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "import") != 0) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();
    
    // check for expected string_literal
    token = get_token();
    if (token->type != ONE_L_STRING && token->type != MUL_L_STRING) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    // check for expected "ifj25"
    if (strcmp(token->value.str_value, "ifj25") != 0) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // check for expected "for"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "for") != 0) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    // check for expected id, and then if id == Ifj
    token = get_token();
    if (token->type != IDENT) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    if (strcmp(token->value.str_value, "Ifj") != 0) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // check for expected EOL
    token = get_token();
    if (token->type != EOL) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse the single required class definition.
 *
 * Grammar:
 * @code
 * <class_def> ::= "class" "Program" "{" EOL <class_body> "}" (EOL)*
 * @endcode
 *
 * @note
 *  - Exactly one EOL is required after '{'.
 *  - Any number of trailing EOLs is allowed after '}'.
 *
 * @return PARSE_OK on success, otherwise volá error_exit(ERR_SYNTACTIC).
 */
static int parse_class_def(void) {
    token_ptr token;

    // check for expected "class"
    token = expect_keyword("class");
    free_token(token);

    // check for expected id, id == Program
    token = expect_ident();
    if (strcmp(token->value.str_value, "Program") != 0) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // check for expected left_dom_par, eol
    token = expect_type(LEFT_DOM_PAR);
    free_token(token);
    token = expect_type(EOL);
    free_token(token);

    // parse "inside" of class
    parse_class_body();

    // check for expected right_dom_par
    token = expect_type(RIGHT_DOM_PAR);
    free_token(token);

    // optional multiple EOLs
    consume_eols();

    return PARSE_OK;
}

/**
 * @brief Parse the body of the Program class.
 *
 * Grammar:
 * @code
 * <class_body> ::= ( "static" <definition> )*
 * @endcode
 *
 * @note
 *  - Tolerates empty lines (EOLs) between definitions.
 *
 * @return PARSE_OK on success.
 */
static int parse_class_body(void) {
    while (1) {
        consume_eols();

        token_ptr token_ahead = look_ahead();
        if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "static") == 0) {
            token_ptr token = expect_keyword("static");
            free_token(token);
            parse_definition();
            continue;
        }
        break;
    }

    return PARSE_OK;
}

/**
 * @brief Parse a static member definition: function, setter, or getter.
 *
 * Grammar:
 * @code
 * <definition> ::= <function_def> | <setter_def> | <getter_def>
 * @endcode
 *
 * @note
 *  - Distinguishes definitions by lookahead after the identifier:
 *      - '('  → function_def
 *      - '='  → setter_def
 *      - else → getter_def
 *
 * @return PARSE_OK on success.
 */
static int parse_definition(void) {
    // saving next two tokens, for id and then token ahead, to decide which function
    token_ptr token = expect_ident();
    // we used expect_ident (which consumed), but original code expected to pass id to parse_...
    token_ptr token_ahead = look_ahead();

    if (token_ahead->type == LEFT_PAR) {
        parse_function_def(token);
    } else if (token_ahead->type == OPERATOR && token_ahead->value.other_value == EQUAL_SIGN_V) {
        parse_setter_def(token);
    } else {
        parse_getter_def(token);
    }
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse a static function definition.
 *
 * Grammar:
 * @code
 * <function_def> ::= "static" ID "(" <param_list> ")" <block> EOL
 * @endcode
 *
 * @note
 *  - EOLs are allowed immediately after '('.
 *
 * @param id Identifier token of the function (already read by caller).
 * @return PARSE_OK on success.
 */
static int parse_function_def(token_ptr id) {
    token_ptr token;

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols();

    parse_param_list();

    token = expect_type(RIGHT_PAR);
    free_token(token);

    parse_block();

    token = expect_type(EOL);
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse a static setter definition.
 *
 * Grammar:
 * @code
 * <setter_def> ::= "static" ID "=" "(" ID ")" <block> EOL
 * @endcode
 *
 * @note
 *  - EOLs are allowed immediately after '=' and '('.
 *
 * @param id Identifier token of the setter (already read by caller).
 * @return PARSE_OK on success, otherwise volá error_exit(ERR_SYNTACTIC).
 */
static int parse_setter_def(token_ptr id) {
    token_ptr token;

    token = expect_type(OPERATOR);
    if (token->value.other_value != EQUAL_SIGN_V) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols();

    token = expect_ident();
    free_token(token);

    token = expect_type(RIGHT_PAR);
    free_token(token);

    parse_block();

    token = expect_type(EOL);
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse a static getter definition.
 *
 * Grammar:
 * @code
 * <getter_def> ::= "static" ID <block> EOL
 * @endcode
 *
 * @param id Identifier token of the getter (already read by caller).
 * @return PARSE_OK on success.
 */
static int parse_getter_def(token_ptr id) {
    token_ptr token;

    parse_block();

    token = expect_type(EOL);
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse a comma-separated parameter list.
 *
 * Grammar:
 * @code
 * <param_list> ::= ε | ID ( "," ID )*
 * @endcode
 *
 * @note
 *  - EOLs are allowed after each comma.
 *
 * @return PARSE_OK on success.
 */
static int parse_param_list(void) {
    token_ptr token_ahead;
    token_ahead = look_ahead();

    //if function has no parameters
    if (token_ahead->type == RIGHT_PAR) {
        return PARSE_OK;
    }

    // next token should be id - of the parameter
    token_ptr token1, token2;
    token1 = expect_ident();
    free_token(token1);

    while (1) {
        token_ahead = look_ahead();
        if (token_ahead->type == COMMA) {
            token2 = expect_type(COMMA);
            free_token(token2);
            consume_eols();

            token1 = expect_ident();
            free_token(token1);
            continue;
        }
        break;
    }

    return PARSE_OK;
}

/**
 * @brief Parse a block delimited by braces.
 *
 * Grammar:
 * @code
 * <block> ::= "{" EOL <statement_list> "}"
 * @endcode
 *
 * @note
 *  - Exactly one EOL is required immediately after '{'.
 *
 * @return PARSE_OK on success.
 */
static int parse_block(void) {
    token_ptr token;

    token = expect_type(LEFT_DOM_PAR);
    free_token(token);

    token = expect_type(EOL);
    free_token(token);

    parse_statement_list();

    token = expect_type(RIGHT_DOM_PAR);
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse a sequence of statements and/or nested blocks.
 *
 * Grammar:
 * @code
 * <statement_list> ::= ( <statement_or_block> )*
 * @endcode
 * Parsing stops at '}' or EOF.
 *
 * @return PARSE_OK on success.
 */
static int parse_statement_list(void) {
    while (1) {
        token_ptr token_ahead;
        token_ahead = look_ahead();
        if (token_ahead->type == RIGHT_DOM_PAR || token_ahead->type == END_OF_FILE) {
            break;
        }
        parse_statement_or_block();
    }

    return PARSE_OK;
}
 
/**
 * @brief Decide between a nested block and a regular statement.
 *
 * Grammar:
 * @code
 * <statement_or_block> ::= <block> | <statement>
 * @endcode
 *
 * @return PARSE_OK on success.
 */
static int parse_statement_or_block(void) {
    token_ptr token_ahead;
    token_ahead = look_ahead();

    if (token_ahead->type == LEFT_DOM_PAR) {
        parse_block();
    } else {
        parse_statement();
    }

    return PARSE_OK;
}

/**
 * @brief Parse a single statement or an empty line (EOL).
 *
 * Grammar:
 * @code
 * <statement> ::= <var_def> EOL
 *               | <assignment_or_call> EOL
 *               | <ifj_call_stmt> EOL
 *               | <if_stmt> EOL
 *               | <while_stmt> EOL
 *               | <return_stmt> EOL
 *               | <for_stmt> EOL
 *               | <break_stmt> EOL
 *               | <continue_stmt> EOL
 *               | EOL
 * @endcode
 *
 * @note
 *  - Stand-alone user function calls (ID '(' ... ')') as statements
 *    are not supported at this time; built-in calls are handled by the
 *    @c Ifj.* branch.
 *
 * @return PARSE_OK on success, otherwise calls error_exit(ERR_SYNTACTIC).
 */
static int parse_statement(void) {
    token_ptr token_ahead, token;
    token_ahead = look_ahead();

    if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "var") == 0) {
        parse_var_def();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == IDENT || token_ahead->type == GLOB_VAR) {
        parse_assignment_or_call();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "if") == 0) {
        parse_if_statement();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "while") == 0) {
        parse_while_statement();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "return") == 0) {
        parse_return_statement();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "for") == 0) {
        parse_for_statement();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "break") == 0) {
        parse_break_statement();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "continue") == 0) {
        parse_continue_statement();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == EOL) {
        token = expect_type(EOL);
        free_token(token);
    } else {
        token = get_token();
        free_token(token);
        error_exit(PARSE_ERROR);
    }

    return PARSE_OK;
}

/**
 * @brief Parse a variable definition.
 *
 * Grammar:
 * @code
 * <var_def> ::= "var" ID
 * @endcode
 *
 * @note
 *  - Trailing EOL is consumed by @c parse_statement().
 *
 * @return PARSE_OK on success.
 */
static int parse_var_def(void) {
    token_ptr token;

    token = expect_keyword("var");
    free_token(token);
   
    token = expect_ident();
    free_token(token);
    
    return PARSE_OK;
}

/**
 * @brief Parse the left-hand side of an assignment.
 *
 * Grammar:
 * @code
 * <assign_target> ::= ID | GLOBAL_ID
 * @endcode
 *
 * @return PARSE_OK on success, otherwise calls error_exit(ERR_SYNTACTIC).
 */
static int parse_assign_target(void) {
    token_ptr token_ahead, token;
    token_ahead = look_ahead();

    if (token_ahead->type == IDENT) {
        token = expect_ident();
        free_token(token);
        return PARSE_OK;
    } else if (token_ahead->type == GLOB_VAR) {
        token = expect_type(GLOB_VAR);
        free_token(token);
        return PARSE_OK;
    } else {
        token = get_token();
        free_token(token);
        error_exit(PARSE_ERROR);
    }

    return PARSE_OK;
}


/**
 * @brief Parse an assignment statement.
 *
 * Grammar:
 * @code
 * <assignment_or_call> ::= <assign_target> "=" <rhs>
 * @endcode
 *
 * @note
 *  - EOLs are allowed immediately after '='.
 *  - The expression on the right-hand side is processed by the PSA
 *    (OPERATORS, FUNEXP).
 *
 * @return PARSE_OK on success.
 */
static int parse_assignment_or_call(void) {
    parse_assign_target();

    token_ptr token;
    token = expect_type(OPERATOR);

    if (token->value.other_value != EQUAL_SIGN_V) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    // TODO parse RIGHT HAND SIDE or expression
    // PSA
    parse_exp_right_side();

    return PARSE_OK;
}

/**
 * @brief Parse the right-hand side of an assignment (expression stub).
 *
 * @note
 *  - This is a hook into the PSA (precedence syntax analyzer).
 *  - Must support OPERATORS and FUNEXP once PSA is integrated.
 *
 * @return PARSE_OK for now (stub). PSA will signal syntax errors
 *         via error_exit(ERR_SYNTACTIC).
 */
static int parse_exp_right_side(void) {
    /* If we want special treatment for IFJ builtins, check here (IFJ_ID token not defined separately in header;
       if you will treat certain keywords as IFJ builtin, check KEY_WORD + value.str_value). For now, call PSA. */
    
    // TODO
    // parse_expression(); PSA
    return PARSE_OK;
}

/**
 * @brief Parse a comma-separated argument list for function calls.
 *
 * Grammar:
 * @code
 * <arg_list> ::= ε | <expression> ( "," <expression> )*
 * @endcode
 *
 * @note
 *  - EOLs are allowed after each comma and after the opening '('
 *    (the latter is handled by the caller).
 *  - Each <expression> will be parsed by PSA.
 *
 * @return PARSE_OK on success.
 */
static int parse_arg_list(void) {
    token_ptr token_ahead;
    token_ahead = look_ahead();

    if (token_ahead->type == RIGHT_PAR) {
        return PARSE_OK;
    }

    // TODO
    // parse_expression(); PSA

    while (1) {
        token_ahead = look_ahead();
        if (token_ahead->type != COMMA) break;

        token_ptr token;
        token = expect_type(COMMA);
        free_token(token);

        consume_eols();
        // TODO
        // parse_expression(); PSA
    }

    return PARSE_OK;
}

/**
 * @brief Parse an @c if statement with an @c else branch.
 *
 * Grammar:
 * @code
 * <if_stmt> ::= "if" "(" (EOL)* <expression> ")" <block> "else" <block>
 * @endcode
 *
 * @note
 *  - EOLs are allowed immediately after '('.
 *  - No EOL allowed between ')' and the following block,
 *    nor between the then-block and 'else'.
 *  - Condition expression is parsed by PSA.
 *
 * @return PARSE_OK on success.
 */
static int parse_if_statement(void) {
    token_ptr token;

    token = expect_keyword("if");
    free_token(token);

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols(); 

    // TODO
    // parse_expression(); PSA

    token = expect_type(RIGHT_PAR);
    free_token(token);

    parse_block();

    token = expect_keyword("else");
    free_token(token);

    parse_block();

    return PARSE_OK;
}

/**
 * @brief Parse a @c while loop.
 *
 * Grammar:
 * @code
 * <while_stmt> ::= "while" "(" (EOL)* <expression> ")" <block>
 * @endcode
 *
 * @note
 *  - EOLs are allowed immediately after '('.
 *  - No EOL allowed between ')' and the block.
 *  - Condition expression is parsed by PSA.
 *
 * @return PARSE_OK on success.
 */
static int parse_while_statement(void) {
    token_ptr token;

    token = expect_keyword("while");
    free_token(token);

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols(); 

    // TODO
    // parse_expression(); PSA

    token = expect_type(RIGHT_PAR);
    free_token(token);

    parse_block();

    return PARSE_OK;
}

/**
 * @brief Parse a @c return statement with an optional expression.
 *
 * Grammar:
 * @code
 * <return_stmt> ::= "return" | "return" <expression>
 * @endcode
 *
 * @note
 *  - Optional expression is parsed by PSA.
 *  - Trailing EOL is consumed by @c parse_statement().
 *
 * @return PARSE_OK on success.
 */
static int parse_return_statement(void) {
    token_ptr token;

    token = expect_keyword("return");
    free_token(token);

    // TODO
    // parse_expression(); PSA

    return PARSE_OK;
}

/**
 * @brief Parse a @c for loop with an identifier iterator and a range/expression.
 *
 * Grammar:
 * @code
 * <for_stmt> ::= "for" "(" (EOL)* ID "in" <expression> ")" <block>
 * @endcode
 *
 * @note
 *  - EOLs are allowed immediately after '('.
 *  - No EOL allowed between ')' and the block.
 *  - Range form (a..b / a...b) is validated later in the semantic phase.
 *
 * @return PARSE_OK on success.
 */
static int parse_for_statement(void) {
    token_ptr token;

    token = expect_keyword("for");
    free_token(token);

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols(); 

    token = expect_ident();
    free_token(token);

    token = expect_keyword("in");
    free_token(token);

    // TODO
    // parse_expression(); PSA

    token = expect_type(RIGHT_PAR);
    free_token(token);

    parse_block();

    return PARSE_OK;
}

/**
 * @brief Parse the @c break statement.
 *
 * Grammar:
 * @code
 * <break_stmt> ::= "break"
 * @endcode
 *
 * @note
 *  - Semantic check (allowed only inside loops) is done later.
 *
 * @return PARSE_OK on success.
 */
static int parse_break_statement(void) {
    token_ptr token;

    token = expect_keyword("break");
    free_token(token);

    return PARSE_OK;
}

/**
 * @brief Parse the @c continue statement.
 *
 * Grammar:
 * @code
 * <continue_stmt> ::= "continue"
 * @endcode
 *
 * @note
 *  - Semantic check (allowed only inside loops) is done later.
 *
 * @return PARSE_OK on success.
 */
static int parse_continue_statement(void) {
    token_ptr token;

    token = expect_keyword("continue");
    free_token(token);

    return PARSE_OK;
}


// helper functions


/**
 * @brief Read and return a required keyword token.
 *
 * @param keyword Expected keyword text.
 *
 * @pre Next token must be of type KEY_WORD with matching @p keyword.
 * @return Token pointer owned by the caller (must call free_token()).
 * @note On mismatch calls error_exit(ERR_SYNTACTIC).
 */
static token_ptr expect_keyword(char *keyword) {
    token_ptr token = get_token();

    // checking if is keyword
    if (token->type != KEY_WORD) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    // checking expected keyword
    if (strcmp(token->value.str_value, keyword) != 0) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }

    return token;
}

/**
 * @brief Read and return a required identifier token.
 *
 * @pre Next token must be of type IDENT.
 * @return Token pointer owned by the caller (must call free_token()).
 * @note On mismatch calls error_exit(ERR_SYNTACTIC).
 */
static token_ptr expect_ident(void) {
    token_ptr token = get_token();

    if (token->type != IDENT) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    return token;
}

/**
 * @brief Read and return a token of the required type.
 *
 * @param exp_tok Expected token type.
 *
 * @pre Next token's type must match @p exp_tok.
 * @return Token pointer owned by the caller (must call free_token()).
 * @note On mismatch calls error_exit(ERR_SYNTACTIC).
 */
static token_ptr expect_type(enum token_type exp_tok) {
    token_ptr token = get_token();

    if (token->type != exp_tok) {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    return token;
}

/**
 * @brief One-token lookahead: fetch the next token and push it back.
 *
 * @note
 *  - The returned token must NOT be freed by the caller.
 *  - The same instance will be returned again by @c get_token().
 *
 * @return Pointer to the peeked token.
 */
static token_ptr look_ahead(void) {
    token_ptr token = get_token();
    push_token(token);
    return token;
}

/**
 * @brief Consume a maximal sequence of EOL tokens as soft whitespace.
 *
 * @note
 *  - Typical usage: after '(', after ',', and after operators like '=' or '.'.
 *  - Internally reads and frees all contiguous EOL tokens.
 */
static void consume_eols(void) {
    while (1) {
        token_ptr token;
        token = look_ahead();
        if (token->type == EOL) {
            token = expect_type(EOL);
            free_token(token);
            continue;
        }
        break;
    }
}