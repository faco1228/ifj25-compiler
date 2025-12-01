/**
 * @file parser.c
 * @author Samuel Facka (xfackas00)
 * @brief Recursive-descent parser implementation (non-expression grammar).
 */

#include "parser.h"
#include "global_structures.h"

// Forward declarations of static (internal) functions
static int parse_prolog(void);
static int parse_class_def(ASTNode_ptr program);
static int parse_class_body(ASTNode_ptr program);
static ASTNode_ptr parse_definition(void);
static ASTNode_ptr parse_function_def(token_ptr id);
static ASTNode_ptr parse_setter_def(token_ptr id);
static ASTNode_ptr parse_getter_def(token_ptr id);
static ASTNode_ptr parse_block(void);
static int parse_statement_list(ASTNode_ptr block);
static int parse_param_list(ASTNode_ptr node, unsigned *arg_count);
static ASTNode_ptr parse_statement(void);
static ASTNode_ptr parse_var_def(void);
static ASTNode_ptr parse_assign_target(void);
static ASTNode_ptr parse_assignment_or_call(void);
static ASTNode_ptr parse_exp_rhs(token_ptr token);
static ASTNode_ptr parse_if_statement(void);
static ASTNode_ptr parse_while_statement(void);
static ASTNode_ptr parse_return_statement(void);
static ASTNode_ptr parse_for_statement(void);
static ASTNode_ptr parse_break_statement(void);
static ASTNode_ptr parse_continue_statement(void);

// helper function
static token_ptr expect_keyword(char *keyword);

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
 * @return Root AST node of the parsed program.
 * @note On a syntax error, calls error_exit(ERR_SYNTACTIC).
 */
ASTNode_ptr parse_program(void)
{
    error_set_parser_func_symtable(g_func_symtable);
    error_set_parser_glob_symtable(g_global_symtable);

    // Edge case: ignore multiple EOLs
    consume_eols();

    if (parse_prolog() != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

    consume_eols();

    // AST root node
    ASTNode_ptr program = ast_create_program();
    error_set_parser_ast_root(program);

    if (parse_class_def(program) != 0)
    {
        error_exit(PARSE_ERROR);
    }
    consume_eols();

    token_ptr token = get_token();
    // Expect EOF at the end
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
 * @brief Parse the mandatory prolog line: import "ifj25" for Ifj EOL
 *
 * Grammar:
 * @code
 * <prolog> ::= "import" STRING_LITERAL "for" ID EOL
 * @endcode
 * where STRING_LITERAL must be "ifj25" and ID must be "Ifj".
 *
 * @note
 *  - EOLs are currently tolerated after import and for, but never 
 *    directly after "ifj25" before for.
 * 
 * @return PARSE_OK on success, otherwise calls error_exit(ERR_SYNTACTIC).
 */
static int parse_prolog(void)
{
    token_ptr token;

    // "import"
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "import") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    // "ifj25" as a string literal
    token = get_token();
    if (token->type != ONE_L_STRING && token->type != MUL_L_STRING)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    if (strcmp(token->value.str_value, "ifj25") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // "for" keyword
    token = get_token();
    if (token->type != KEY_WORD || strcmp(token->value.str_value, "for") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    consume_eols();

    // "Ifj" keyword
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

    // Necessary EOL at the end
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
 * @param program Root NODE_PROGRAM AST node.
 * @return PARSE_OK on success, otherwise calls error_exit().
 */
static int parse_class_def(ASTNode_ptr program)
{
    token_ptr token;

    // "class" keyword
    token = expect_keyword("class");
    free_token(token);

    // Class name: Program
    token = expect_ident();
    if (strcmp(token->value.str_value, "Program") != 0)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    free_token(token);

    // '{' and a required EOL
    token = expect_type(LEFT_DOM_PAR);
    free_token(token);

    token = expect_type(END_OF_LINE);
    free_token(token);

    // Parse the inside of the class body
    if (parse_class_body(program) != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

    // Closing '}'
    token = expect_type(RIGHT_DOM_PAR);
    free_token(token);

    // Zero or more trailing EOLs
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
 * @param program Root NODE_PROGRAM AST node to which definitions are attached.
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
 * @return AST node representing the definition.
 */
static ASTNode_ptr parse_definition()
{
    // Identifier for function/getter/setter name
    token_ptr ident = expect_ident();

    // Token, to decide which definition it is
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
 * @return AST node representing the function definition.
 */
static ASTNode_ptr parse_function_def(token_ptr id) // parse parameter list
{
    token_ptr token;

    token = expect_type(LEFT_PAR);
    free_token(token);
    consume_eols();

    // Create the function node before adding parameters as children
    ASTNode_ptr fun = ast_create_function(id->value.str_value, 0, FUN_F, NULL);

    unsigned arg_count = 0;
    if (parse_param_list(fun, &arg_count) != PARSE_OK)
    {
        error_exit(PARSE_ERROR);
    }

    // Now we know the real arg_count, overwrite temporary value
    fun->data.function_def.arg_count = arg_count;

    token = expect_type(RIGHT_PAR);
    free_token(token);

    ASTNode_ptr body = parse_block();
    add_child(fun, body); // Body is added as a child node

    // Insert function into function symbol table
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
 * @return AST node representing the setter.
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

    ASTNode_ptr fun = ast_create_function(id->value.str_value, 0, FUN_S, NULL);

    // Setter has exactly one parameter
    token = expect_ident();
    ASTNode_ptr param = ast_create_ident(token->value.str_value, false);
    add_child(fun, param);
    free_token(token);

    fun->data.function_def.arg_count = 1;

    token = expect_type(RIGHT_PAR);
    free_token(token);

    ASTNode_ptr body = parse_block();
    add_child(fun, body);

    // Insert setter into function symbol table
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
 * @return AST node representing the getter.
 */
static ASTNode_ptr parse_getter_def(token_ptr id)
{
    ASTNode_ptr body = parse_block();

    // Getter has no parameters
    ASTNode_ptr fun = ast_create_function(id->value.str_value, 0, FUN_G, body);

    // Insert getter into function symbol table
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
 * @param node Function node to which parameters will be attached.
 * @param arg_count Output: number of parameters.
 *
 * @return PARSE_OK on success.
 */
int parse_param_list(ASTNode_ptr node, unsigned *arg_count)
{
    token_ptr token_ahead = look_ahead();
    *arg_count = 0;

    // Function with no parameters
    if (token_ahead->type == RIGHT_PAR)
    {
        return PARSE_OK;
    }

    // First parameter
    ASTNode_ptr param;
    token_ptr token = expect_ident();
    (*arg_count)++;

    param = ast_create_ident(token->value.str_value, false);
    add_child(node, param);
    free_token(token);

    // Parse more parameters after commas
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

            param = ast_create_ident(token->value.str_value, false);
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
 * @return AST node representing the block.
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
 * @endcode
 * @note
 *  - Parsing stops at '}' or EOF.
 *
 * @param block Block node to which parsed statements/blocks are attached.
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
        {
            // Nested block
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
 *    are not supported at this time; built-in calls are handled by the Ifj.* branch.
 *
 * @return AST node representing the statement, or NULL for an empty line.
 */
static ASTNode_ptr parse_statement()
{
    token_ptr token;
    token_ptr token_ahead = look_ahead();

    if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "var") == 0)
    {   // Variable declaration
        ASTNode_ptr var = parse_var_def();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return var;
    }
    else if (token_ahead->type == IDENT || token_ahead->type == GLOB_VAR)
    {   // Assignment or variable reference starting with IDENT/GLOB_VAR
        ASTNode_ptr assign = parse_assignment_or_call();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return assign;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "if") == 0)
    {   // If statement
        ASTNode_ptr if_node = parse_if_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return if_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "while") == 0)
    {   // While statement
        ASTNode_ptr while_node = parse_while_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return while_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "return") == 0)
    {   // Return statement
        ASTNode_ptr ret_node = parse_return_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return ret_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "for") == 0)
    {   // For statement
        ASTNode_ptr for_node = parse_for_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return for_node;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "break") == 0)
    {   // Break statement
        ASTNode_ptr br = parse_break_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return br;
    }
    else if (token_ahead->type == KEY_WORD && strcmp(token_ahead->value.str_value, "continue") == 0)
    {   // Continue statement
        ASTNode_ptr cont = parse_continue_statement();
        token = expect_type(END_OF_LINE);
        free_token(token);
        return cont;
    }
    else if (token_ahead->type == END_OF_LINE)
    {   // Empty line
        token = expect_type(END_OF_LINE);
        free_token(token);
        return NULL;
    }
    else
    {   // Any other token at statement start is a syntax error
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
 *  - Trailing EOL is consumed by parse_statement().
 *
 * @return AST node representing the variable declaration.
 */
static ASTNode_ptr parse_var_def(void)
{
    token_ptr token;

    token = expect_keyword("var");
    free_token(token);

    token = expect_ident();

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
 * @return AST node representing the identifier.
 */
static ASTNode_ptr parse_assign_target(void)
{
    token_ptr token;
    token_ptr token_ahead = look_ahead();

    if (token_ahead->type == IDENT)
    {
        token = expect_ident();
        ASTNode_ptr id = ast_create_ident(token->value.str_value, false);
        free_token(token);
        return id;
    }
    else if (token_ahead->type == GLOB_VAR)
    {
        token = expect_type(GLOB_VAR);
        ASTNode_ptr id = ast_create_ident(token->value.str_value, true);

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
 *
 * @return AST node representing the assignment.
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

    return ast_create_assignment(lhs, rhs);
}

/**
 * @brief Parse the right-hand side of an assignment (expression stub).
 *
 * @note
 *  - Uses precedence syntax analyzer (PSA) for the actual expression.
 *
 * @param token Token that triggered this expression (recognition token for PSA).
 * @return NODE_EXPR_STMNT wrapping the parsed expression AST..
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
 * @brief Parse an if statement with an else branch.
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
 * @return AST node representing the if statement.
 */
static ASTNode_ptr parse_if_statement(void)
{
    token_ptr token;

    token = expect_keyword("if");
    free_token(token);

    token = expect_type(LEFT_PAR);
    ASTNode_ptr cond = parse_exp_rhs(token);
    // PSA consumes the closing ')'

    consume_eols();

    ASTNode_ptr then_block = parse_block();

    token = expect_keyword("else");
    free_token(token);

    ASTNode_ptr else_block = parse_block();

    return ast_create_if(cond, then_block, else_block);
}

/**
 * @brief Parse a while loop.
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
 * @return AST node representing the while statement.
 */
static ASTNode_ptr parse_while_statement(void)
{
    token_ptr token;

    token = expect_keyword("while");
    free_token(token);

    token = expect_type(LEFT_PAR);
    ASTNode_ptr cond = parse_exp_rhs(token);
    // PSA consumes the closing ')'

    consume_eols();

    ASTNode_ptr body = parse_block();

    return ast_create_while(cond, body);
}

/**
 * @brief Parse a return statement with an optional expression.
 *
 * Grammar:
 * @code
 * <return_stmt> ::= "return" | "return" <expression>
 * @endcode
 *
 * @note
 *  - Optional expression is parsed by PSA.
 *  - Trailing EOL is consumed by parse_statement().
 *
 * @return PARSE_OK on success.
 */
static ASTNode_ptr parse_return_statement(void)
{
    token_ptr token = expect_keyword("return");

    ASTNode_ptr value = parse_exp_rhs(token);

    return ast_create_return(value);
}

/**
 * @brief Parse a for loop with an identifier iterator and a range/expression.
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
 * @return AST node representing the for loop.
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

    ASTNode_ptr iter_expr = parse_exp_rhs(token);

    token = expect_type(RIGHT_PAR);
    free_token(token);

    ASTNode_ptr body = parse_block();

    ASTNode_ptr node = ast_create_for(iter_name, iter_expr, body);
    free_token(id_token);

    return node;
}

/**
 * @brief Parse the break statement.
 *
 * Grammar:
 * @code
 * <break_stmt> ::= "break"
 * @endcode
 *
 * @return AST node representing break.
 */
static ASTNode_ptr parse_break_statement(void)
{
    token_ptr token = expect_keyword("break");
    free_token(token);

    return ast_create_break();
}

/**
 * @brief Parse the continue statement.
 *
 * Grammar:
 * @code
 * <continue_stmt> ::= "continue"
 * @endcode
 *
 * @return AST node representing continue.
 */
static ASTNode_ptr parse_continue_statement(void)
{
    token_ptr token = expect_keyword("continue");
    free_token(token);

    return ast_create_continue();
}


/******************** Helper functions implementations ********************/

/**
 * @brief Read and return a required keyword token.
 *
 * @param keyword Expected keyword text.
 *
 * @pre Next token must be of type KEY_WORD with matching keyword.
 * 
 * @return Token pointer owned by the caller.
 * @note On mismatch, calls error_exit(ERR_SYNTACTIC).
 */
static token_ptr expect_keyword(char *keyword)
{
    token_ptr token = get_token();

    // Must be a keyword
    if (token->type != KEY_WORD)
    {
        free_token(token);
        error_exit(PARSE_ERROR);
    }
    // Must match the expected text
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
 * 
 * @return Token pointer owned by the caller.
 * @note On mismatch, calls error_exit(ERR_SYNTACTIC).
 */
token_ptr expect_ident(void)
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
 *  - The same instance will be returned again by get_token().
 *
* @return Pointer to the peeked token (do not free).
 */
token_ptr look_ahead(void)
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
 * @pre Next token's type must match exp_tok.
 * 
 * @return Token pointer owned by the caller.
 * @note On mismatch, calls error_exit(ERR_SYNTACTIC).
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
 * @brief Consume a maximal sequence of EOL tokens as "soft" whitespace.
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