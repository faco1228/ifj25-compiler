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
#include "global_structures.h"

// forward prototypes (internal)
static int parse_prolog(void);
static int parse_class_def(ASTNode_ptr program);
static int parse_class_body(ASTNode_ptr program);
static ASTNode_ptr parse_definition(void);
static ASTNode_ptr parse_function_def(token_ptr id);
static ASTNode_ptr parse_setter_def(token_ptr id);
static ASTNode_ptr parse_getter_def(token_ptr id);
static ASTNode_ptr parse_block(void);
static int parse_statement_list(ASTNode_ptr block);
static ASTNode_ptr parse_statement(void);
static ASTNode_ptr parse_var_def(void);
static ASTNode_ptr parse_assign_target(void);
static ASTNode_ptr parse_assignment_or_call(void);
static ASTNode_ptr parse_exp_rhs(token_ptr token);
// static int parse_arg_list(void); // moze byt vyuzita ako sablona pre PSA
static ASTNode_ptr parse_if_statement(void);
static ASTNode_ptr parse_while_statement(void);
static ASTNode_ptr parse_return_statement(void);
static ASTNode_ptr parse_for_statement(void);
static ASTNode_ptr parse_break_statement(void);
static ASTNode_ptr parse_continue_statement(void);

// helper functions
static token_ptr expect_keyword(char *keyword);
static token_ptr expect_ident(void);
static token_ptr look_ahead(void);

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
ASTNode_ptr parse_program(void)
{
    error_set_parser_func_symtable(g_func_symtable);
    error_set_parser_glob_symtable(g_global_symtable);

    // edge case if multiple EOLs
    consume_eols();

    if (parse_prolog() != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

    consume_eols();

    // ast root
    ASTNode_ptr program = ast_create_program();
    error_set_parser_ast_root(program);

    if (parse_class_def(program) != 0)
    {
        error_exit(PARSE_ERROR);
    }
    consume_eols();

    token_ptr token = get_token();
    // expect EOF
    if (token->type != END_OF_FILE)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    scanner_cleanup();
    return program;
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
static int parse_prolog(void)
{
    token_ptr token;

    // check for expected "import"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "import") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    // check for expected string_literal
    token = get_token();
    if (token->type != ONE_L_STRING && token->type != MUL_L_STRING)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    // check for expected "ifj25"
    if (strcmp(token->value.str_value, "ifj25") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // check for expected "for"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "for") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    // check for expected id, and then if id == Ifj
    token = get_token();
    if (token->type != KEY_WORD)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    if (strcmp(token->value.str_value, "Ifj") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // check for expected EOL
    token = get_token();
    if (token->type != END_OF_LINE)
    {
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
static int parse_class_def(ASTNode_ptr program)
{
    token_ptr token;

    // check for expected "class"
    token = expect_keyword("class");
    free_token(token);

    // check for expected id, id == Program
    token = expect_ident();
    if (strcmp(token->value.str_value, "Program") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // check for expected left_dom_par, eol
    token = expect_type(LEFT_DOM_PAR);
    free_token(token);
    token = expect_type(END_OF_LINE);
    free_token(token);

    // parse "inside" of class
    if (parse_class_body(program) != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

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
static int parse_class_body(ASTNode_ptr program)
{
    while (1)
    {
        consume_eols();

        token_ptr token_ahead = look_ahead();
        if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "static") == 0)
        {
            token_ptr token = expect_keyword("static");
            free_token(token);

            ASTNode_ptr def = parse_definition();
            add_child(program, def);
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
static ASTNode_ptr parse_definition()
{
    // saving next two tokens, for id and then token ahead, to decide which function
    token_ptr ident = expect_ident();

    // we used expect_ident (which consumed), but original code expected to pass id to parse_...
    token_ptr token_ahead = look_ahead();

    ASTNode_ptr def_node = NULL;

    if (token_ahead->type == LEFT_PAR)
    {
        def_node = parse_function_def(ident);
    }
    else if (token_ahead->type == OPERATOR && token_ahead->value.other_value == EQUAL_SIGN_V)
    {
        def_node = parse_setter_def(ident);
    }
    else
    {
        def_node = parse_getter_def(ident);
    }
    free_token(ident);

    return def_node;
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
static ASTNode_ptr parse_function_def(token_ptr id) // parse parameter list
{
    token_ptr token;

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols();

    // function node needs to be created before we try to add any args as children
    // right now we only know the name of the function, the other values are just temporary
    ASTNode_ptr fun = ast_create_function(id->value.str_value, 0, FUN_F, NULL);

    unsigned arg_count = 0;
    if (parse_param_list(fun, &arg_count) != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

    // now that we know the real arg_count we can overwrite the temporaty value
    fun->data.function_def.arg_count = arg_count;

    token = expect_type(RIGHT_PAR);
    free_token(token);


    ASTNode_ptr body = parse_block();
    add_child(fun, body); // we add body of the function as a child node 

    // token = expect_type(END_OF_LINE);
    // free_token(token);

    // fun = ast_create_function(id->value.str_value, arg_count, FUN_F, body);

    // adds new function to func symtable
    Key *key = st_create_function_key(id->value.str_value, arg_count, FUNCTION);
    ST_Node *new = st_create_node(key);
    g_func_symtable = st_insert_node(g_func_symtable, new);

    key_dispose(key);

    return fun;
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
static ASTNode_ptr parse_setter_def(token_ptr id)
{
    token_ptr token;

    token = expect_type(OPERATOR);
    if (token->value.other_value != EQUAL_SIGN_V)
    {
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

    ASTNode_ptr body = parse_block();

    // token = expect_type(END_OF_LINE);
    // free_token(token);

    ASTNode_ptr fun = ast_create_function(id->value.str_value, 1, FUN_S, body);

    // adds new setter to func symtable
    Key *key = st_create_function_key(id->value.str_value, 1, SETTER);
    ST_Node *new = st_create_node(key);
    g_func_symtable = st_insert_node(g_func_symtable, new);

    key_dispose(key);

    return fun;
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
static ASTNode_ptr parse_getter_def(token_ptr id)
{
    // token_ptr token;

    ASTNode_ptr body = parse_block();

    // token = expect_type(END_OF_LINE);
    // free_token(token);

    ASTNode_ptr fun = ast_create_function(id->value.str_value, 0, FUN_G, body);

    // adds new getter to func symtable
    Key *key = st_create_function_key(id->value.str_value, 0, GETTER);
    ST_Node *new = st_create_node(key);
    g_func_symtable = st_insert_node(g_func_symtable, new);

    key_dispose(key);

    return fun;
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
int parse_param_list(ASTNode_ptr node, unsigned *arg_count)
{
    token_ptr token_ahead = look_ahead();
    *arg_count = 0;

    // if function has no parameters
    if (token_ahead->type == RIGHT_PAR)
    {
        return PARSE_OK;
    }

    ASTNode_ptr param;

    // next token should be id - of the parameter
    token_ptr token = expect_ident();
    (*arg_count)++;

    param = ast_create_ident(token->value.str_value);
    add_child(node, param);
    free_token(token);

    while (1)
    {
        token_ahead = look_ahead();
        if (token_ahead->type == COMMA)
        {
            token = expect_type(COMMA);
            free_token(token);
            consume_eols();

            token = expect_ident();
            (*arg_count)++;
            
            param = ast_create_ident(token->value.str_value);
            add_child(node, param);
            free_token(token);

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
static ASTNode_ptr parse_block(void)
{
    token_ptr token = expect_type(LEFT_DOM_PAR);
    free_token(token);

    token = expect_type(END_OF_LINE);
    free_token(token);

    ASTNode_ptr block = ast_create_block();

    if (parse_statement_list(block) != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

    token = expect_type(RIGHT_DOM_PAR);
    free_token(token);

    return block;
}

/**
 * @brief Parse a sequence of statements and/or nested blocks.
 *
 * Grammar:
 * @code
 * <statement_list> ::= ( <block> | <statement> )*
 *
 * @endcode
 * Parsing stops at '}' or EOF.
 *
 * @return PARSE_OK on success.
 */
static int parse_statement_list(ASTNode_ptr block)
{
    while (1)
    {
        token_ptr token_ahead = look_ahead();
        if (token_ahead->type == RIGHT_DOM_PAR || token_ahead->type == END_OF_FILE)
        {
            break;
        }

        if (token_ahead->type == LEFT_DOM_PAR)
        { // if nahradil parse_stmnt_or_block
            ASTNode_ptr nested = parse_block();
            add_child(block, nested);
        }
        else
        {
            ASTNode_ptr stmt = parse_statement();
            if (stmt != NULL)
            {
                add_child(block, stmt);
            }
        }
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
// static int parse_statement_or_block(void) {
//     token_ptr token_ahead;
//     token_ahead = look_ahead();

//     if (token_ahead->type == LEFT_DOM_PAR) {
//         parse_block();
//     } else {
//         parse_statement();
//     }

//     return PARSE_OK;
// }

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
static ASTNode_ptr parse_statement()
{
    token_ptr token;
    token_ptr token_ahead = look_ahead();

    if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "var") == 0)
    {
        ASTNode_ptr var = parse_var_def();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return var;
    }
    else if (token_ahead->type == IDENT || token_ahead->type == GLOB_VAR)
    {
        ASTNode_ptr assign = parse_assignment_or_call();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return assign;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "if") == 0)
    {
        ASTNode_ptr if_node = parse_if_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return if_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "while") == 0)
    {
        ASTNode_ptr while_node = parse_while_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return while_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "return") == 0)
    {
        ASTNode_ptr ret_node = parse_return_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return ret_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "for") == 0)
    {
        ASTNode_ptr for_node = parse_for_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return for_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "break") == 0)
    {
        ASTNode_ptr br = parse_break_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return br;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "continue") == 0)
    {
        ASTNode_ptr cont = parse_continue_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return cont;
    }
    else if (token_ahead->type == END_OF_LINE)
    {
        token = expect_type(END_OF_LINE);
        free_token(token);
        return NULL;
    }
    else
    {
        token = get_token();
        free_token(token);
        error_exit(PARSE_ERROR);
    }

    return NULL;
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
 * @return PARSE_OK on success.=
 */
static ASTNode_ptr parse_var_def(void)
{
    token_ptr token;

    token = expect_keyword("var");
    free_token(token);

    token = expect_ident();

    // tvorenie ast node
    ASTNode_ptr node = ast_create_var_dec(token->value.str_value);

    free_token(token);

    return node;
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
static ASTNode_ptr parse_assign_target(void)
{
    token_ptr token;
    token_ptr token_ahead = look_ahead();

    if (token_ahead->type == IDENT)
    {
        token = expect_ident();
        ASTNode_ptr id = ast_create_ident(token->value.str_value);
        free_token(token);
        return id;
    }
    else if (token_ahead->type == GLOB_VAR)
    {
        token = expect_type(GLOB_VAR);
        ASTNode_ptr id = ast_create_ident(token->value.str_value);

        // adds new glob variable to g_global_symtable
        Key *key = st_create_variable_key(token->value.str_value);

        if (!st_search(g_global_symtable, key)) // does not already exist so we can add a new one
        {
            ST_Node *new = st_create_node(key);
            g_global_symtable = st_insert_node(g_global_symtable, new);
        }

        key_dispose(key);

        free_token(token);
        return id;
    }
    else
    {
        token = get_token();
        free_token(token);
        error_exit(PARSE_ERROR);
    }

    return NULL;
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
static ASTNode_ptr parse_assignment_or_call(void)
{
    ASTNode_ptr lhs = parse_assign_target();

    token_ptr token = expect_type(OPERATOR);

    if (token->value.other_value != EQUAL_SIGN_V)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    consume_eols();

    ASTNode_ptr rhs = parse_exp_rhs(token);

    // free_token(token);

    return ast_create_assignment(lhs, rhs);
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
static ASTNode_ptr parse_exp_rhs(token_ptr token)
{
    ASTNode_ptr expr = parse_expression(token);
    if (expr == NULL)
    {
        error_exit(PARSE_ERROR);
    }
    return ast_create_exp_statement(expr);
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
// static int parse_arg_list(void) {
//     token_ptr token_ahead;
//     token_ahead = look_ahead();
//     // unsigned args = 0;

//     if (token_ahead->type == RIGHT_PAR) {
//         return PARSE_OK;
//     }

//     ASTNode_ptr expr = parse_expression(NULL);
//     // add_child(current_call_node, expr);   // argument 0

//     while (1) {
//         token_ahead = look_ahead();
//         if (token_ahead->type != COMMA) break;

//         token_ptr token;
//         token = expect_type(COMMA);
//         free_token(token);
//         consume_eols();

//         expr = parse_expression(NULL);
//         // add_child(current_call_node, expr);
//     }

//     return PARSE_OK;
// }

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
static ASTNode_ptr parse_if_statement(void)
{
    token_ptr token;

    token = expect_keyword("if");
    free_token(token);

    token = expect_type(LEFT_PAR);
    ASTNode_ptr cond = parse_exp_rhs(token);

    // free_token(token);
    consume_eols();

    // This should be here but, kikos precedence analysis already processes
    // ) parent, will fix later

    // token = expect_type(RIGHT_PAR);
    // free_token(token);

    ASTNode_ptr then_block = parse_block();

    token = expect_keyword("else");
    free_token(token);

    ASTNode_ptr else_block = parse_block();

    return ast_create_if(cond, then_block, else_block);
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
static ASTNode_ptr parse_while_statement(void)
{
    token_ptr token;

    token = expect_keyword("while");
    free_token(token);

    token = expect_type(LEFT_PAR);
    ASTNode_ptr cond = parse_exp_rhs(token);

    // free_token(token);
    consume_eols();

    // This should be here but, kikos precedence analysis already processes
    // ) parent, will fix later

    // token = expect_type(RIGHT_PAR);
    // free_token(token);

    ASTNode_ptr body = parse_block();

    return ast_create_while(cond, body);
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
static ASTNode_ptr parse_return_statement(void)
{
    token_ptr token = expect_keyword("return");

    ASTNode_ptr value = parse_exp_rhs(token);
    // free_token(token);

    return ast_create_return(value);
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
static ASTNode_ptr parse_for_statement(void)
{
    token_ptr token;

    token = expect_keyword("for");
    free_token(token);

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols();

    token_ptr id_token = expect_ident();
    char *iter_name = id_token->value.str_value;

    token = expect_keyword("in");

    ASTNode_ptr iter_expr = parse_exp_rhs(token); // ???
    // free_token(token);

    // if (iter_expr->type != NODE_RANGE)

    token = expect_type(RIGHT_PAR);
    free_token(token);

    ASTNode_ptr body = parse_block();

    ASTNode_ptr node = ast_create_for(iter_name, iter_expr, body);
    free_token(id_token);
    return node;
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
static ASTNode_ptr parse_break_statement(void)
{
    token_ptr token = expect_keyword("break");
    free_token(token);

    return ast_create_break();
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
static ASTNode_ptr parse_continue_statement(void)
{
    token_ptr token = expect_keyword("continue");
    free_token(token);

    return ast_create_continue();
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
static token_ptr expect_keyword(char *keyword)
{
    token_ptr token = get_token();

    // checking if is keyword
    if (token->type != KEY_WORD)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    // checking expected keyword
    if (strcmp(token->value.str_value, keyword) != 0)
    {
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
static token_ptr expect_ident(void)
{
    token_ptr token = get_token();

    if (token->type != IDENT)
    {
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
static token_ptr look_ahead(void)
{
    token_ptr token = get_token();
    push_token(token);
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
token_ptr expect_type(enum token_type exp_tok)
{
    token_ptr token = get_token();

    if (token->type != exp_tok)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    return token;
}
/**
 * @brief Consume a maximal sequence of EOL tokens as soft whitespace.
 *
 * @note
 *  - Typical usage: after '(', after ',', and after operators like '=' or '.'.
 *  - Internally reads and frees all contiguous EOL tokens.
 */
void consume_eols(void)
{
    while (1)
    {
        token_ptr token;
        token = look_ahead();
        if (token->type == END_OF_LINE)
        {
            token = expect_type(END_OF_LINE);
            free_token(token);
            continue;
        }
        break;
    }
}