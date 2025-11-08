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

/**
 * @brief Entry point of the recursive-descent parser.
 *
 * Grammar: <program> ::= <prolog> <class_def> EOF
 *
 * Behavior:
 *  - Skips leading EOLs.
 *  - Parses the prolog and the single class definition.
 *  - Requires EOF after the class.
 *
 * Side effects:
 *  - Uses tokens from scanner.
 *  - Calls scanner_cleanup() on success.
 *
 * Errors:
 *  - On any syntax error, calls error_exit(2) via syntax_error().
 *
 * Returns:
 *  - 0 on success. Never returns on syntax error.
 */
int parse_program(void){
    // edge case if multiple EOLs
    consume_eols();

    // parse prolog
    if (parse_prolog() != 0) syntax_error("Invalid prolog");

    consume_eols();
    
    // parse class def
    if (parse_class_def() != 0) syntax_error("Invalid class definition");

    consume_eols();

    token_ptr token = get_token();
    // expect EOF
    if (token->type != END_OF_FILE) {
        free_token(token);
        syntax_error("Expected EOF at the end");
    }
    free_token(token);

    scanner_cleanup();
    return 0;
}

/**
 * @brief Parse the mandatory prolog line:  import "ifj25" for Ifj EOL
 *
 * Grammar: <prolog> ::= "import" STRING_LITERAL "for" ID EOL
 *         where STRING_LITERAL must be "ifj25" and ID must be "Ifj".
 *
 * EOL rules:
 *  - No EOL allowed inside the prolog; the entire prolog must be on one line.
 *
 * Errors:
 *  - Missing/invalid keyword, string, identifier, or trailing EOL.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_prolog(void) { 
    token_ptr token;

    // check for expected "import"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "import") != 0) {
        if (token) free_token(token);
        syntax_error("Expected keyword 'import' in prolog");
    }
    free_token(token);

    // check for expected sting_literal
    token = get_token();
    if (token->type != ONE_L_STRING && token->type != MUL_L_STRING) {
        if (token) free_token(token);
        syntax_error("Expected string literal after 'import' in prolog");
    }
    // check for expected "ifj25"
    if (strcmp(token->value.str_value, "ifj25") != 0) {
        free_token(token);
        syntax_error("Expected string literal - \"ifj25\" in prolog");
    }
    free_token(token);

    // check for expected "for"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "for") != 0) {
        if (token) free_token(token);
        syntax_error("Expected 'for' in prolog");
    }
    free_token(token);

    // check for expected id, and then if id == Ifj
    token = get_token();
    if (token->type != IDENT) {
        if (token) free_token(token);
        syntax_error("Expected ident after 'for' in prolog");
    }
    if (strcmp(token->value.str_value, "Ifj") != 0) {
        free_token(token);
        syntax_error("Expected ident - 'Ifj'");
    }
    free_token(token);

    // check for expected EOL
    token = get_token();
    if (token->type != EOL) {
        free_token(token);
        syntax_error("Expected EOL after prolog");
    }
    free_token(token);

    return 0;
}

/**
 * @brief Parse the single required class: class Program { EOL <class_body> } (EOL)*
 *
 * Grammar: <class_def> ::= "class" "Program" "{" EOL <class_body> "}" (EOL)*
 *
 * EOL rules:
 *  - Exactly one EOL immediately after '{'.
 *  - Any number of trailing EOLs after '}'.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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
        syntax_error("Expected class name 'Program'");
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

    return 0;
}

/**
 * @brief Parse the body of the Program class.
 *
 * Grammar: <class_body> ::= ( "static" <definition> )*
 *
 * Behavior:
 *  - Tolerates blank lines between definitions.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse a static member definition: function, setter, or getter.
 *
 * Grammar: <definition> ::= <function_def> | <setter_def> | <getter_def>
 *
 * Decision:
 *   - Lookahead '('  → function_def
 *   - Lookahead '='  → setter_def
 *   - Otherwise      → getter_def
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse a static function definition.
 *
 * Grammar: <function_def> ::= "static" ID "(" <param_list> ")" <block> EOL
 *
 * EOL rules:
 *  - EOLs are allowed right after '('.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse a static setter definition.
 *
 * Grammar: <setter_def> ::= "static" ID "=" "(" ID ")" <block> EOL
 *
 * EOL rules:
 *  - EOLs are allowed right after '=' and '('.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_setter_def(token_ptr id) {
    token_ptr token;

    token = expect_type(OPERATOR);
    if (token->value.other_value != EQUAL_SIGN_V) {
        free_token(token);
        syntax_error("Expected '=' for setter");
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

    return 0;
}

/**
 * @brief Parse a static getter definition.
 *
 * Grammar: <getter_def> ::= "static" ID <block> EOL
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_getter_def(token_ptr id) {
    token_ptr token;

    parse_block();

    token = expect_type(EOL);
    free_token(token);

    return 0;
}

/**
 * @brief Parse a comma-separated parameter list.
 *
 * Grammar: <param_list> ::= ε | ID ( "," ID )*
 *
 * EOL rules:
 *  - EOLs allowed after each comma.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_param_list(void) {
    token_ptr token_ahead;
    token_ahead = look_ahead();

    //if function has no parameters
    if (token_ahead->type == RIGHT_PAR) {
        return 0;
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

    return 0;
}

/**
 * @brief Parse a block delimited by braces.
 *
 * Grammar: <block> ::= "{" EOL <statement_list> "}"
 *
 * EOL rules:
 *  - Exactly one EOL required immediately after "{".
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse a sequence of statements and/or nested blocks.
 *
 * Grammar: <statement_list> ::= ( <statement_or_block> )*
 *         Stop at '}' or EOF.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Decide between a nested <block> and a regular <statement>.
 *
 * Grammar: <statement_or_block> ::= <block> | <statement>
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_statement_or_block(void) {
    token_ptr token_ahead;
    token_ahead = look_ahead();

    if (token_ahead->type == LEFT_DOM_PAR) {
        parse_block();
    } else {
        parse_statement();
    }

    return 0;
}

/**
 * @brief Parse a single statement terminated by EOL, or an empty line.
 *
 * Grammar: <statement> ::= <var_def> EOL
 *                        | <assignment_or_call> EOL
 *                        | <ifj_call_stmt> EOL
 *                        | <if_stmt> EOL
 *                        | <while_stmt> EOL
 *                        | <return_stmt> EOL
 *                        | <for_stmt> EOL
 *                        | <break_stmt> EOL
 *                        | <continue_stmt> EOL
 *                        | EOL
 *
 * Notes:
 *  - Stand-alone user calls (ID '(' … ')') are not accepted as statements here
 *    unless you extend the grammar. Built-ins via Ifj.* are supported.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_statement(void) {
    token_ptr token_ahead, token;
    token_ahead = look_ahead();

    if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "var") == 0) {
        parse_var_def();
        token = expect_type(EOL);
        free_token(token);
    } else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "Ifj") == 0) {
        parse_ifj_call_statement();
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
        syntax_error("Unexpected token in statement");
    }

    return 0;
}

/**
 * @brief Parse a variable definition.
 *
 * Grammar: <var_def> ::= "var" ID
 *
 * Termination:
 *   - The caller (parse_statement) reads the trailing EOL.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_var_def(void) {
    token_ptr token;

    token = expect_keyword("var");
    free_token(token);
   
    token = expect_ident();
    free_token(token);
    
    return 0;
}

/**
 * @brief Parse the left-hand side of an assignment.
 *
 * Grammar: <assign_target> ::= ID | GLOBAL_ID
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_assign_target(void) {
    token_ptr token_ahead, token;
    token_ahead = look_ahead();

    if (token_ahead->type == IDENT) {
        token = expect_ident();
        free_token(token);
        return 0;
    } else if (token_ahead->type == GLOB_VAR) {
        token = expect_type(GLOB_VAR);
        free_token(token);
        return 0;
    } else {
        token = get_token();
        free_token(token);
        syntax_error("Expected assigment target Ident or Global_Ident");
    }

    return 0;
}


/**
 * @brief Parse an assignment statement.
 *
 * Grammar: <assignment_or_call> ::= <assign_target> "=" <rhs>
 *
 * EOL rules:
 *  - EOLs allowed immediately after '='.
 *
 * Expressions:
 *  - <rhs> is delegated to the PSA expression parser (OPERATORS, FUNEXP).
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_assignment_or_call(void) {
    parse_assign_target();

    token_ptr token;
    token = expect_type(OPERATOR);

    if (token->value.other_value != EQUAL_SIGN_V) {
        free_token(token);
        syntax_error("Expected '=' in assigment");
    }
    free_token(token);

    consume_eols();

    // TODO parse RIGHT HAND SIDE or expression
    // Precedencna analyza
    parse_rhs();

    return 0;
}

/**
 * @brief Parse a built-in call as a stand-alone statement.
 *
 * Grammar: <ifj_call_stmt> ::= "Ifj" "." (EOL)* ID "(" (EOL)* <arg_list> (EOL)* ")"
 *
 * EOL rules:
 *  - No EOL between "Ifj" and ".".
 *  - EOLs are allowed after '.' and '(' and after commas in arguments.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_ifj_call_statement(void) {
    token_ptr token;

    // check for 'Ifj'
    token = expect_keyword("Ifj");
    free_token(token);

    // check for '.'
    token = expect_type(DOT);
    free_token(token);

    // eol possible
    consume_eols();

    token = expect_ident();
    free_token(token);

    // '('
    token = expect_type(LEFT_PAR);
    free_token(token);

    consume_eols();
    parse_arg_list();

    token = expect_type(RIGHT_PAR);
    free_token(token);

    return 0;
}


/**
 * @brief Parse the right-hand side of an assignment.
 *
 * Grammar (handled by PSA): <rhs> ::= <expression> | ID "(" <arg_list> ")" | "Ifj" "." ID "(" <arg_list> ")" | ID
 *
 * Role:
 *  - This function is a hook into the PSA (precedence/syntax analyzer for expressions).
 *  - Must support OPERATORS and FUNEXP once PSA is integrated.
 *
 * Returns:
 *  - 0 for now (stub); later PSA should signal errors via syntax_error().
 */
static int parse_rhs(void) {
    /* If we want special treatment for IFJ builtins, check here (IFJ_ID token not defined separately in header;
       if you will treat certain keywords as IFJ builtin, check KEY_WORD + value.str_value). For now, call PSA. */
    
    // TODO
    // parse_expression(); PSA
    return 0;
}

/**
 * @brief Parse a comma-separated argument list for function calls.
 *
 * Grammar: <arg_list> ::= ε | <expression> ( "," <expression> )*
 *
 * EOL rules:
 *  - EOLs allowed after each comma and after the opening '(' (handled by caller).
 *
 * Role:
 *  - Delegates each <expression> to PSA.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_arg_list(void) {
    token_ptr token_ahead;
    token_ahead = look_ahead();

    if (token_ahead->type == RIGHT_PAR) {
        return 0;
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

    return 0;
}

/**
 * @brief Parse an if-statement with an else-branch.
 *
 * Grammar: <if_stmt> ::= "if" "(" (EOL)* <expression> ")" <block> "else" <block>
 *
 * EOL rules:
 *  - EOLs allowed immediately after '('.
 *  - No EOL allowed between ')' and the following block, nor between the then-block and 'else'.
 *
 * Role:
 *  - The condition expression is parsed by PSA.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse a while-loop.
 *
 * Grammar: <while_stmt> ::= "while" "(" (EOL)* <expression> ")" <block>
 *
 * EOL rules:
 *  - EOLs allowed immediately after '('.
 *  - No EOL allowed between ')' and the block.
 *
 * Role:
 *  - The condition expression is parsed by PSA.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse a return statement with an optional expression.
 *
 * Grammar: <return_stmt> ::= "return" | "return" <expression>
 *
 * Role:
 *  - Optional expression is parsed by PSA.
 *
 * Termination:
 *  - The caller (parse_statement) reads the trailing EOL.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_return_statement(void) {
    token_ptr token;

    token = expect_keyword("return");
    free_token(token);

    // TODO
    // parse_expression(); PSA

    return 0;
}

/**
 * @brief Parse a for-loop with an identifier iterator and a range/expression.
 *
 * Grammar: <for_stmt> ::= "for" "(" (EOL)* ID "in" <expression> ")" <block>
 *
 * EOL rules:
 *  - EOLs allowed immediately after '('.
 *  - No EOL allowed between ')' and the block.
 *
 * Semantics:
 *  - The range form (e.g., a..b or a...b) is validated at semantic stage
 *    to ensure exactly one range operator.
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
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

    return 0;
}

/**
 * @brief Parse the 'break' statement.
 *
 * Grammar: <break_stmt> ::= "break"
 *
 * Semantics:
 *  - Valid only inside loops; enforced later (semantic pass).
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_break_statement(void) {
    token_ptr token;

    token = expect_keyword("break");
    free_token(token);

    return 0;
}

/**
 * @brief Parse the 'continue' statement.
 *
 * Grammar: <continue_stmt> ::= "continue"
 *
 * Semantics:
 *  - Valid only inside loops; enforced later (semantic pass).
 *
 * Returns:
 *  - 0 on success; otherwise calls syntax_error().
 */
static int parse_continue_statement(void) {
    token_ptr token;

    token = expect_keyword("continue");
    free_token(token);

    return 0;
}


/* Helper functions */


/**
 * @brief Read and return a required keyword token.
 *
 * Preconditions:
 *  - Next token must be KEY_WORD with matching text.
 *
 * Errors:
 *  - On mismatch, calls syntax_error().
 *
 * Ownership:
 *  - Returns an owned token_ptr; caller must free_token().
 */
static token_ptr expect_keyword(char *keyword) {
    token_ptr token = get_token();

    // checking if is keyword
    if (token->type != KEY_WORD) {
        free_token(token);
        syntax_error("Expected keyword");
    }
    // checking expected keyword
    if (strcmp(token->value.str_value, keyword) != 0) {
        free_token(token);
        syntax_error("Unexpected keyword");
    }

    return token;
}

/**
 * @brief Read and return a required identifier token.
 *
 * Preconditions:
 *  - Next token must be IDENT.
 *
 * Errors:
 *  - On mismatch, calls syntax_error().
 *
 * Ownership:
 *  - Returns an owned token_ptr; caller must free_token().
 */
static token_ptr expect_ident(void) {
    token_ptr token = get_token();

    if (token->type != IDENT) {
        free_token(token);
        syntax_error("Expected identifier");
    }
    return token;
}

/**
 * @brief Read and return a token of the required type.
 *
 * Preconditions:
 *  - Next token's type must match 'exp_tok'.
 *
 * Errors:
 *  - On mismatch, calls syntax_error().
 *
 * Ownership:
 *  - Returns an owned token_ptr; caller must free_token().
 */
static token_ptr expect_type(enum token_type exp_tok) {
    token_ptr token = get_token();

    if (token->type != exp_tok) {
        free_token(token);
        syntax_error("Unexpected token type");
    }
    return token;
}

/**
 * @brief One-token lookahead: fetch the next token and push it back.
 *
 * Behavior:
 *  - Calls get_token() then push_token() with the same instance.
 *
 * Ownership:
 *  - Do NOT free the returned pointer. The token will be returned again by get_token().
 *
 * Returns:
 *  - Peeked token pointer.
 */
static token_ptr look_ahead(void) {
    token_ptr token = get_token();
    push_token(token);
    return token;
}

/**
 * @brief Report a syntax error and terminate the program.
 *
 * Side effects:
 *  - Prints "Parser error: <msg>" to stderr.
 *  - Calls error_exit(2).
 *
 * Note:
 *  - Does not return.
 */
static void syntax_error(char *msg) {
    fprintf(stderr, "Parser error: %s\n", msg);
    error_exit(2);
}

/**
 * @brief Consume a maximal sequence of EOL tokens as soft whitespace.
 *
 * Where to use:
 *  - After '(' in if/while/for and calls.
 *  - After ',' in parameter/argument lists.
 *  - After operators, including '='.
 *  - After '.' in "Ifj . ID" (never before '.').
 *
 * Ownership:
 *  - Internally reads and frees EOL tokens.
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