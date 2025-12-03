/**
 * @file parser_expression.h
 * @authors Samuel Facka (xfackas00),
 *          Kristian Cilling (xcillik00)
 * @brief Precedence syntax analyzer (PSA) for expressions.
 *
 * Supported:
 *  - arithmetic : + - * /
 *  - relational : < > <= >=
 *  - equality :   == !=
 *  - type test :  is
 *  - ranges :     .. ...
 *  - FUNEXP : function calls as operands (user + Ifj.*)
 */

#include "parser_expression.h"
#include "global_structures.h"

// Forward declarations of static (internal) functions
static char *str_copy(const char *str);
static bool token_is_type_keyword(token_ptr t);
static token_ptr find_top_terminal(Stack *stack);
static bool token_is_is_operator(token_ptr t);
static bool token_is_ifj_keyword(token_ptr t);
static ASTNode_ptr ast_from_operand_token(token_ptr t);
static operator_types map_op_to_ast(enum other_value_type op_val);
static bool psa_eol_end_expr(token_ptr current_token);
static precedence_index token_to_index(token_ptr token);
static bool psa_fun_call_starts_here(token_ptr first_token);
static void psa_parse_call_args(ASTNode_ptr call_node);
static ASTNode_ptr psa_parse_fun_call_operand(token_ptr first_token);
static ASTNode_ptr psa_parse_operand(token_ptr first_token);
static void psa_reduce_fun(Stack *stack, token_ptr recognition_token);
static bool psa_table_compare(Stack *stack, token_ptr current_token, token_ptr *top_terminal, token_ptr recognition_token);

/*******************  Helper functions implementation ********************/

/**
 * @brief Allocate and copy a null-terminated C string.
 *
 * @param str Source string (may be NULL).
 * @return Newly allocated copy or NULL if str is NULL or allocation fails.
 */
static char *str_copy(const char *str)
{
    if (!str)
        return NULL;

    size_t len = strlen(str) + 1;
    char *copy = malloc(len);

    if (!copy)
        return NULL;

    strcpy(copy, str);
    return copy;
}

/**
 * @brief Find the topmost terminal symbol on the PSA stack.
 *
 * @note
 *  - Skips internal markers (MARKER) and reduced nonterminals (NONTERMINAL_E).
 *
 * @param stack PSA stack.
 * @return Pointer to the topmost terminal token or NULL if none found.
 */
static token_ptr find_top_terminal(Stack *stack)
{
    StackItem *tmp = stack->head;
    token_ptr top_terminal = NULL;

    while (tmp)
    {
        if (tmp->token->type != NONTERMINAL_E &&
            tmp->token->type != MARKER)
        {
            top_terminal = tmp->token;
        }
        tmp = tmp->next;
    }
    return top_terminal;
}

/**
 * @brief Check if the token is the 'is' operator.
 */
static bool token_is_is_operator(token_ptr t)
{
    return (t->type == KEY_WORD &&
            t->value.str_value != NULL &&
            strcmp(t->value.str_value, "is") == 0);
}

/**
 * @brief Check if the token is the 'Ifj' keyword (prefix for built-in calls).
 */
static bool token_is_ifj_keyword(token_ptr t)
{
    return (t->type == KEY_WORD &&
            t->value.str_value != NULL &&
            strcmp(t->value.str_value, "Ifj") == 0);
}

/*******************  Helpers for building AST from operands and operators   ********************/

/**
 * @brief Convert a single operand token into an AST node.
 *
 * Handles:
 *  - IDENT / GLOB_VAR,
 *  - numeric literals,
 *  - string literals,
 *  - type literals (Num/String/Null),
 *  - null literal (keyword "null").
 *
 * @param token Token to convert. On errors, this function frees token and calls error_exit().
 */
static ASTNode_ptr ast_from_operand_token(token_ptr token)
{
    switch (token->type)
    {
    case IDENT:
        // If the token already carries an AST (e.g. CALL), reuse it
        if (token->ast != NULL)
        {
            return (ASTNode_ptr)token->ast;
        }
        // Normal ident
        return ast_create_ident(token->value.str_value, false);
    case GLOB_VAR:
        return ast_create_ident(token->value.str_value, true);

    case INT_LIT:
        return ast_create_int(token->value.int_value);

    case FLOAT_LIT:
        return ast_create_float(token->value.float_value);

    case ONE_L_STRING:
    case MUL_L_STRING:
        return ast_create_str(token->value.str_value);

    case KEY_WORD:
        if (token_is_type_keyword(token))
        {   // Type literal used in 'is' operator
            return ast_create_type_lit(token->value.str_value);
        }
        else if (token->value.str_value != NULL && strcmp(token->value.str_value, "null") == 0)
        {   // Null literal as a keyword
            return ast_create_null();
        }
        else
        {
            // Different keywords are not valid operands
            free_token(token);
            error_exit(ERR_SYNTACTIC);
            return NULL;
        }

    default:
        // Any other token type: internal error
        free_token(token);
        error_exit(ERR_INTERNAL);
        return NULL;
    }
    return NULL;
}

/**
 * @brief Map operator token value to AST operator type.
 *
 * @param op_val Token's other_value field.
 * @return Corresponding operator_types value.
 */
static operator_types map_op_to_ast(enum other_value_type op_val)
{
    switch (op_val)
    {
    case PLUS_V:
        return OP_PLUS;
    case MINUS_V:
        return OP_MINUS;
    case STAR_V:
        return OP_MUL;
    case SLASH_V:
        return OP_DIV;

    case LOGICAL_EQUAL_V:
        return OP_EQ;
    case LOGICAL_NOT_EQUAL_V:
        return OP_NEQ;

    case LESS_THAN_V:
        return OP_LT;
    case LESS_OR_EQ_THAN_V:
        return OP_LTE;
    case GREATER_THAN_V:
        return OP_GT;
    case GREATER_OR_EQ_THAN_V:
        return OP_GTE;

    default:
        error_exit(ERR_INTERNAL);
    }
    return OP_ERROR;
}

/*******************  Recognizing special tokens ********************/

/**
 * @brief Check if keyword is one of the type keywords: Num, String, Null.
 */
static bool token_is_type_keyword(token_ptr t)
{
    return (t->type == KEY_WORD &&
            t->value.str_value != NULL &&
            (strcmp(t->value.str_value, "String") == 0 ||
             strcmp(t->value.str_value, "Num") == 0 ||
             strcmp(t->value.str_value, "Null") == 0));
}

/**
 * @brief Decide whether an EOL can safely terminate the current expression.
 * 
 * @note
 *  - Used as a heuristic mainly for assignment and return contexts.
 */
static bool psa_eol_end_expr(token_ptr current_token)
{
    switch (current_token->type)
    {
    case IDENT:
    case GLOB_VAR:
    case INT_LIT:
    case FLOAT_LIT:
    case ONE_L_STRING:
    case MUL_L_STRING:
    case RIGHT_PAR:
        return true;

    case KEY_WORD:
        if (token_is_type_keyword(current_token) ||
            (current_token->value.str_value != NULL &&
            strcmp(current_token->value.str_value, "null") == 0))
        {
            return true;
        }
        return false;

    default:
        return false;
    }
}


/******************* Mapping tokens to precedence table indices ********************/

/**
 * @brief Map a token to a precedence_index used in the precedence table.
 */
static precedence_index token_to_index(token_ptr token)
{
    switch (token->type)
    {
    case OPERATOR:
        switch (token->value.other_value)
        {
        case PLUS_V:
        case MINUS_V:
            return OP_ADD_SUB;
        case STAR_V:
        case SLASH_V:
            return OP_MUL_DIV;

        case LESS_THAN_V:
            return OP_LOWER;
        case GREATER_THAN_V:
            return OP_GREATER;
        case LESS_OR_EQ_THAN_V:
            return OP_LOWER_EQUAL;
        case GREATER_OR_EQ_THAN_V:
            return OP_GREATER_EQUAL;

        case LOGICAL_EQUAL_V:
            return OP_EQUAL;
        case LOGICAL_NOT_EQUAL_V:
            return OP_NOT_EQUAL;

        default:
            return OP_UNRECOGNISED;
        }

    case LEFT_PAR:
        return OP_LPAR;
    case RIGHT_PAR:
        return OP_RPAR;

    case COMMA:
        return OP_UNRECOGNISED;

    case DOUBLE_DOT:
        return OP_D_DOT;
    case TRIPLE_DOT:
        return OP_T_DOT;

    case IDENT:
    case GLOB_VAR:
    case INT_LIT:
    case FLOAT_LIT:
    case ONE_L_STRING:
    case MUL_L_STRING:
        return OP_OPERAND;

    case KEY_WORD:
        if (token_is_is_operator(token))
            return OP_IS_TOK;
        if (token_is_ifj_keyword(token))
            return OP_OPERAND;
        if (token_is_type_keyword(token))
            return OP_OPERAND;
        if (token->value.str_value != NULL &&
            strcmp(token->value.str_value, "null") == 0)
            return OP_OPERAND;
        return OP_UNRECOGNISED;

    case END_OF_FILE:
        return OP_END;

    default:
        return OP_UNRECOGNISED;
    }
}

/*******************  Precedence table  ********************/

static const precedence_relation precedence_table[OP_END + 1][OP_END + 1] = {
    //          +,−         *,/         <           >           <=          >=          ==          !=          (           )              i           is           ..          ...         $
    /* +,− */ {psa_reduce, psa_shift,  psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* *,/ */ {psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* <   */ {psa_shift,  psa_shift,  psa_error,  psa_error,  psa_error,  psa_error,  psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_shift,  psa_shift,  psa_reduce},
    /* >   */ {psa_shift,  psa_shift,  psa_error,  psa_error,  psa_error,  psa_error,  psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_shift,  psa_shift,  psa_reduce},
    /* <=  */ {psa_shift,  psa_shift,  psa_error,  psa_error,  psa_error,  psa_error,  psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_shift,  psa_shift,  psa_reduce},
    /* >=  */ {psa_shift,  psa_shift,  psa_error,  psa_error,  psa_error,  psa_error,  psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_shift,  psa_shift,  psa_reduce},
    /* ==  */ {psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_error,  psa_error,  psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_shift,  psa_shift,  psa_reduce},
    /* !=  */ {psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_error,  psa_error,  psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_shift,  psa_shift,  psa_reduce},
    /* (   */ {psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift, psa_eq_reduce, psa_shift, psa_shift,  psa_shift,  psa_shift,  psa_error},
    /* )   */ {psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_error, psa_reduce,    psa_error, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* i   */ {psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_error, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* is  */ {psa_shift,  psa_shift,  psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_error,  psa_reduce, psa_reduce, psa_reduce},
    /* ..  */ {psa_shift,  psa_shift,  psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_error,  psa_error,  psa_reduce},
    /* ... */ {psa_shift,  psa_shift,  psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce,    psa_shift, psa_reduce, psa_error,  psa_error,  psa_reduce},
    /* $   */ {psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift,  psa_shift, psa_error,     psa_shift, psa_shift,  psa_shift,  psa_shift,  psa_finish}};


/******************* FUNEXP – function call as operand (user + Ifj.*)  ********************/

/**
 * @brief Decide whether this token is the start of a function call (FUNEXP).
 *
 * @note
 *  - user function: IDENT '(' ...
 *  - built-in: Ifj '.' IDENT '(' ...
 */
static bool psa_fun_call_starts_here(token_ptr first_token)
{
    token_ptr la;

    // User function: foo(...)
    if (first_token->type == IDENT)
    {
        la = look_ahead();
        return (la->type == LEFT_PAR);
    }

    // Built-in: Ifj.name(...)
    if (token_is_ifj_keyword(first_token))
    {
        la = look_ahead();
        return (la->type == DOT);
    }

    return false;
}

/**
 * @brief Parse a call argument list: <arg_list> ::= ε | <expr> ("," <expr>)*.
 * 
 * @code
 * <arg_list> ::= ε | <expression> ( "," <expression> )*
 * @endcode
 *
 * Grammar previously designed for parser
 * 
 * Each argument is parsed as a full expression via PSA and wrapped in
 * NODE_EXPR_STMNT before being attached to the CALL node.
 */
static void psa_parse_call_args(ASTNode_ptr call_node)
{
    unsigned arg_count = 0;

    // We are already after the function name and '('
    consume_eols();
    token_ptr la = get_token();

    if (la->type == RIGHT_PAR)
    {
        // Call without arguments
        free_token(la);
        call_node->data.function_call.param_count = 0;
        return;
    }

    // Not immediately ')', so push back and parse first expression*/
    push_token(la);

    while (1)
    {
        // Each argument is a full expression
        ASTNode_ptr arg_expr = parse_expression(NULL);
        ASTNode_ptr arg_stmt = ast_create_exp_statement(arg_expr);
        add_child(call_node, arg_stmt);
        arg_count++;

        consume_eols();
        token_ptr t = get_token();

        if (t->type == COMMA)
        {
            // Next argument follows
            free_token(t);
            consume_eols();
            continue;
        }
        else if (t->type == RIGHT_PAR)
        {
            // End of argument list
            free_token(t);
            break;
        }
        else
        {
            // Anything else is syntax error
            free_token(t);
            error_exit(ERR_SYNTACTIC);
        }
    }

    call_node->data.function_call.param_count = arg_count;
}

/**
 * @brief Parse function call as an operand (user or built-in).
 *
 * @param first_token Token that triggered call detection (IDENT or Ifj keyword).
 * @return AST CALL node.
 */
static ASTNode_ptr psa_parse_fun_call_operand(token_ptr first_token)
{
    bool is_builtin = false;
    char *func_name = NULL;

    if (first_token->type == IDENT)
    {
        // User function: foo(...)
        func_name = first_token->value.str_value;
        is_builtin = false;

        free_token(expect_type(LEFT_PAR));
    }
    else if (token_is_ifj_keyword(first_token))
    {
        // Built-in: Ifj.read_str(...)
        free_token(expect_type(DOT));

        token_ptr ident_tok = expect_ident();
        func_name = str_copy(ident_tok->value.str_value); // Name of the built-in function
        free_token(ident_tok);

        free_token(expect_type(LEFT_PAR));

        is_builtin = true;
    }
    else
    {
        // Should not happen if psa_fun_call_starts_here is correct
        error_exit(ERR_INTERNAL);
    }

    ASTNode_ptr call = ast_create_call(func_name, 0, is_builtin);

    // Parse arguments and fill param_count
    psa_parse_call_args(call);

    return call;
}

/**
 * @brief Return operand AST for PSA: literal, identifier or function call.
 */
static ASTNode_ptr psa_parse_operand(token_ptr first_token)
{
    if (psa_fun_call_starts_here(first_token))
    {
        return psa_parse_fun_call_operand(first_token);
    }
    else
    {
        return ast_from_operand_token(first_token);
    }
}

/******************* Reduction: handle → NONTERMINAL_E + AST ********************/

/**
 * @brief Perform a single reduction step according to grammar patterns.
 *
 * @note
 *  - pops tokens until MARKER is found,
 *  - matches patterns like "E", "(E)", "E op E",
 *  - builds corresponding AST nodes,
 *  - pushes NEW NONTERMINAL_E token carrying the reduced AST.
 */
static void psa_reduce_fun(Stack *stack, token_ptr recognition_token)
{
    if (stack_is_empty(stack))
    {
        stack_free(stack);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    token_ptr items[5];
    int count = 0;

    // Pop until we hit the MARKER, collect items to reduce
    while (!stack_is_empty(stack))
    {
        token_ptr t = stack_top(stack);
        stack_pop_no_free(stack);

        if (t->type == MARKER)
        {
            free_token(t);
            break;
        }

        if (count >= 5)
        {
            for (int i = 0; i < count; i++)
                free_token(items[i]);
            stack_free(stack);
            if (recognition_token)
                free_token(recognition_token);
            error_exit(ERR_INTERNAL);
        }

        items[count++] = t;
    }

    if (count == 0)
    {
        stack_free(stack);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    token_ptr first_stack_item = items[count - 1];
    token_ptr second_stack_item = (count >= 2) ? items[count - 2] : NULL;
    token_ptr third_stack_item = (count >= 3) ? items[count - 3] : NULL;

    ASTNode_ptr reduced_ast = NULL;
    bool matched = false;

    // E -> i (single operand)
    if (!matched &&
        count == 1 &&
        first_stack_item->type != MARKER &&
        first_stack_item->type != NONTERMINAL_E)
    {
        reduced_ast = psa_parse_operand(first_stack_item);
        matched = true;
    }

    // E -> E (already reduced nonterminal)
    if (!matched &&
        count == 1 &&
        first_stack_item->type == NONTERMINAL_E)
    {
        reduced_ast = (ASTNode_ptr)first_stack_item->ast;
        matched = true;
    }

    // E -> ( E )
    if (!matched &&
        count == 3 &&
        first_stack_item->type == LEFT_PAR &&
        second_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == RIGHT_PAR)
    {
        reduced_ast = (ASTNode_ptr)second_stack_item->ast;
        second_stack_item = NULL;
        matched = true;
    }

    // E -> E op E (binary ops including is, ranges)
    if (!matched &&
        count == 3 &&
        first_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == NONTERMINAL_E)
    {
        token_ptr op_tok = second_stack_item;
        ASTNode_ptr lhs = (ASTNode_ptr)first_stack_item->ast;
        ASTNode_ptr rhs = (ASTNode_ptr)third_stack_item->ast;

        if (op_tok->type == OPERATOR)
        {
            switch (op_tok->value.other_value)
            {
            case PLUS_V:
            case MINUS_V:
            case STAR_V:
            case SLASH_V:
            case LESS_THAN_V:
            case GREATER_THAN_V:
            case LESS_OR_EQ_THAN_V:
            case GREATER_OR_EQ_THAN_V:
            case LOGICAL_EQUAL_V:
            case LOGICAL_NOT_EQUAL_V:
            {
                operator_types op = map_op_to_ast(op_tok->value.other_value);
                reduced_ast = ast_create_binary(lhs, rhs, op);
                matched = true;
                break;
            }
            default:
                break;
            }
        }
        else if (token_is_is_operator(op_tok))
        {
            // RHS must be a type literal for 'is'
            if (rhs->type != NODE_TYPE_LIT)
            {
                ast_free(lhs);
                ast_free(rhs);
                ast_free(reduced_ast);
                error_exit(ERR_SEM_TYPE_MISMATCH);
            }

            reduced_ast = ast_create_binary(lhs, rhs, OP_IS);
            matched = true;
        }
        else if (op_tok->type == DOUBLE_DOT || op_tok->type == TRIPLE_DOT)
        {
            bool inclusive = (op_tok->type == DOUBLE_DOT);
            reduced_ast = ast_create_range(lhs, rhs, inclusive);
            matched = true;
        }

        if (matched) {
            first_stack_item->ast = NULL;
            third_stack_item->ast = NULL;
        }

    }

    if (!matched)
    {
        for (int i = 0; i < count; i++)
            free_token(items[i]);
        stack_free(stack);
        if (recognition_token)
            free_token(recognition_token);

        ast_free(reduced_ast);
        error_exit(ERR_SYNTACTIC);
    }

    // Create NONTERMINAL_E token holding the reduced AST
    token_ptr newE = malloc(sizeof(token_t));
    if (!newE)
    {
        for (int i = 0; i < count; i++)
            free_token(items[i]);
        stack_free(stack);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_INTERNAL);
    }

    memset(newE, 0, sizeof(token_t));
    newE->type = NONTERMINAL_E;
    newE->ast = reduced_ast;

    stack_push(stack, newE);

    for (int i = 0; i < count; i++)
        free_token(items[i]);
}

/******************* Precedence table operation ********************/

/**
 * @brief Compare top-of-stack operator and current token via precedence table.
 *
 * @note
 *  - psa_shift: push marker '<' and current token, return true (caller reads next).
 *  - psa_eq_reduce: just push current token, return true.
 *  - psa_reduce: perform reduction, return false (caller re-processes token).
 *  - psa_finish: '$' to '$', parsing finished, return true.
 *  - psa_error: syntax error.
 */
static bool psa_table_compare(Stack *stack, token_ptr current_token, token_ptr *top_terminal, token_ptr recognition_token)
{
    token_ptr top_token = stack_top(stack);
    if (!top_token)
    {
        stack_free(stack);
        free_token(current_token);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    *top_terminal = NULL;
    StackItem *tmp = stack->head;
    StackItem *last_terminal_item = NULL;

    // Find last terminal in the stack
    while (tmp != NULL)
    {
        if (tmp->token->type != NONTERMINAL_E &&
            tmp->token->type != MARKER)
        {
            *top_terminal = tmp->token;
            last_terminal_item = tmp;
        }
        tmp = tmp->next;
    }

    if (*top_terminal == NULL)
    {
        stack_free(stack);
        free_token(current_token);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    if (last_terminal_item != NULL)
        stack_set_top_terminal_pointer(stack, last_terminal_item);

    precedence_index top_index = token_to_index(*top_terminal);
    if (top_index == OP_UNRECOGNISED)
    {
        stack_free(stack);
        free_token(current_token);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    precedence_index curr_index = token_to_index(current_token);
    if (curr_index == OP_UNRECOGNISED)
    {
        stack_free(stack);
        free_token(current_token);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    precedence_relation rel = precedence_table[top_index][curr_index];

    switch (rel)
    {
    case psa_shift:
    {
        // Insert marker '<' after last terminal and then push token
        token_ptr marker = malloc(sizeof(token_t));
        if (!marker)
        {
            stack_free(stack);
            free_token(current_token);
            if (recognition_token)
                free_token(recognition_token);
            error_exit(ERR_INTERNAL);
        }
        memset(marker, 0, sizeof(token_t));
        marker->type = MARKER;
        marker->value.other_value = '<';

        stack_push_after(stack, marker);
        stack_push(stack, current_token);
        return true;
    }
    case psa_eq_reduce:
        stack_push(stack, current_token);
        return true;

    case psa_reduce:
        psa_reduce_fun(stack, recognition_token);
        return false;

    case psa_finish:
        return true;

    case psa_error:
    default:
        stack_free(stack);
        free_token(current_token);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

    return true;
}

/******************* Main PSA function – parse_expression ********************/

/**
 * @brief Parse an expression using the precedence syntax analyzer.
 *
 * Different contexts are handled using the recognition token:
 *  - NULL : standalone expression terminated by EOL, ',', ')' or EOF.
 *  - KEY_WORD "return" : right side of return.
 *  - KEY_WORD "in" : expression inside for (... in <expr>).
 *  - OPERATOR '=' : right-hand side of assignment.
 *  - LEFT_PAR '(' : expression inside parentheses.
 *
 * The function:
 *  - builds a PSA stack with '$' sentinel,
 *  - repeatedly uses the precedence table to shift/reduce,
 *  - at the end returns AST root of the expression.
 *
 * @param recognition_token
 *        Optional token describing the context where the expression starts.
 *        Ownership is taken and it will be freed inside this function.
 *        Pass NULL when parsing a standalone expression.
 *
 * @return AST node pointer representing the root of the parsed expression.
 *         On syntax or internal errors, error_exit() is called.
 */
ASTNode_ptr parse_expression(token_ptr recognition_token)
{
    Stack stack;
    stack_init(&stack);

    // Push initial '$' sentinel on the stack
    token_ptr special_char = malloc(sizeof(token_t));
    if (!special_char)
    {
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_INTERNAL);
    }
    memset(special_char, 0, sizeof(token_t));
    special_char->type = END_OF_FILE;
    special_char->value.other_value = '$';
    special_char->ast = NULL;
    stack_push(&stack, special_char);

    token_ptr current_token = get_token();
    token_ptr top_terminal = NULL;

    // Basic (NULL) context
    if (recognition_token == NULL)
    {
        while (true)
        {
            // Inline FUNEXP handling – user and built-in calls
            if (current_token->type == IDENT || token_is_ifj_keyword(current_token))
            {
                if (psa_fun_call_starts_here(current_token))
                {
                    ASTNode_ptr call_ast = psa_parse_fun_call_operand(current_token);
                    free_token(current_token);

                    token_ptr call_token = malloc(sizeof(token_t));
                    if (!call_token)
                    {
                        stack_free(&stack);
                        if (recognition_token)
                            free_token(recognition_token);
                        error_exit(ERR_INTERNAL);
                    }
                    memset(call_token, 0, sizeof(token_t));

                    // Treat as IDENT so token_to_index maps to OP_OPERAND
                    call_token->type = IDENT;
                    call_token->ast = call_ast;

                    current_token = call_token;
                }
            }

            // Expression end – these tokens are not consumed by PSA
            if (current_token->type == END_OF_LINE ||
                current_token->type == END_OF_FILE ||
                current_token->type == COMMA ||
                current_token->type == RIGHT_PAR)
            {
                push_token(current_token);
                break;
            }

            top_terminal = find_top_terminal(&stack);

            // Normal PSA step
            bool should_advance =
                psa_table_compare(&stack, current_token, &top_terminal, NULL);

            if (should_advance)
            {
                current_token = get_token();
            }
            else
            {
                precedence_index curr_index = token_to_index(current_token);
                if (curr_index == OP_UNRECOGNISED)
                {
                    stack_free(&stack);
                    free_token(current_token);
                    error_exit(ERR_SYNTACTIC);
                }
            }
        }
    }

    // Special contexts
    else
    {
        switch (recognition_token->type)
        {
        case KEY_WORD:
            // return <expr>
            if (recognition_token->value.str_value != NULL &&
                strcmp(recognition_token->value.str_value, "return") == 0)
            {

                while (true)
                {
                    // Inline FUNEXP in return expressions
                    if (current_token->type == IDENT || token_is_ifj_keyword(current_token))
                    {
                        if (psa_fun_call_starts_here(current_token))
                        {
                            ASTNode_ptr call_ast = psa_parse_fun_call_operand(current_token);
                            free_token(current_token);

                            token_ptr call_token = malloc(sizeof(token_t));
                            if (!call_token)
                            {
                                stack_free(&stack);
                                free_token(recognition_token);
                                error_exit(ERR_INTERNAL);
                            }
                            memset(call_token, 0, sizeof(token_t));

                            call_token->type = IDENT;
                            call_token->ast = call_ast;

                            current_token = call_token;
                        }
                    }
                    bool should_advance =
                        psa_table_compare(&stack, current_token, &top_terminal, recognition_token);

                    if (should_advance)
                    {
                        if (current_token->type == OPERATOR ||
                            current_token->type == DOUBLE_DOT ||
                            current_token->type == TRIPLE_DOT ||
                            token_is_is_operator(current_token))
                        {
                            consume_eols();
                        }
                        else if (psa_eol_end_expr(current_token))
                        {
                            // Peek ahead with get_token/push_token, not parser's look_ahead
                            token_ptr peek_token = look_ahead();
                            if (peek_token->type == END_OF_LINE)
                            {
                                break;
                            }
                        }

                        current_token = get_token();
                    }
                    else
                    {
                        precedence_index curr_index = token_to_index(current_token);
                        if (curr_index == OP_UNRECOGNISED)
                        {
                            stack_free(&stack);
                            free_token(current_token);
                            free_token(recognition_token);
                            error_exit(ERR_SYNTACTIC);
                        }
                    }
                }
            }
            // for (... in <expr>)
            else if (strcmp(recognition_token->value.str_value, "in") == 0)
            {
                int par_depth = 0; // Depth of inner parentheses inside the expression
                bool token_is_new = true;

                while (true)
                {
                    // Inline FUNEXP in 'in' expressions
                    if (current_token->type == IDENT || token_is_ifj_keyword(current_token))
                    {
                        if (psa_fun_call_starts_here(current_token))
                        {
                            ASTNode_ptr call_ast = psa_parse_fun_call_operand(current_token);
                            free_token(current_token);

                            token_ptr call_token = malloc(sizeof(token_t));
                            if (!call_token)
                            {
                                stack_free(&stack);
                                free_token(recognition_token);
                                error_exit(ERR_INTERNAL);
                            }
                            memset(call_token, 0, sizeof(token_t));

                            call_token->type = IDENT;
                            call_token->ast = call_ast;

                            current_token = call_token;

                            token_is_new = true;
                        }
                    }

                    // Track inner parentheses only once per new token
                    if (token_is_new)
                    {
                        if (current_token->type == LEFT_PAR)
                        {
                            par_depth++;
                        }
                        else if (current_token->type == RIGHT_PAR)
                        {
                            if (par_depth == 0)
                            {
                                // This ')' belongs to the for header, not the expression
                                push_token(current_token);
                                break;
                            }
                            else
                            {
                                par_depth--;
                            }
                        }
                    }

                    bool should_advance = psa_table_compare(&stack, current_token, &top_terminal, recognition_token);

                    // Current token has just been processed, do not treat it as new again
                    token_is_new = false;

                    if (should_advance)
                    {
                        current_token = get_token();
                        token_is_new = true; // new token from scanner
                    }
                    else
                    {
                        precedence_index curr_index = token_to_index(current_token);
                        if (curr_index == OP_UNRECOGNISED)
                        {
                            stack_free(&stack);
                            free_token(current_token);
                            free_token(recognition_token);
                            error_exit(ERR_SYNTACTIC);
                        }
                    }
                }
            }
            break;

        case OPERATOR:
            // assignment RHS: = <expr>
            if (recognition_token->value.other_value == EQUAL_SIGN_V)
            {
                while (true)
                {
                    // Inline FUNEXP on RHS of '='
                    if (current_token->type == IDENT || token_is_ifj_keyword(current_token))
                    {
                        if (psa_fun_call_starts_here(current_token))
                        {
                            ASTNode_ptr call_ast = psa_parse_fun_call_operand(current_token);
                            free_token(current_token);

                            token_ptr call_token = malloc(sizeof(token_t));
                            if (!call_token)
                            {
                                stack_free(&stack);
                                free_token(recognition_token);
                                error_exit(ERR_INTERNAL);
                            }
                            memset(call_token, 0, sizeof(token_t));

                            call_token->type = IDENT;
                            call_token->ast = call_ast;

                            current_token = call_token;
                        }
                    }

                    bool should_advance =
                        psa_table_compare(&stack, current_token, &top_terminal, recognition_token);

                    if (should_advance)
                    {
                        if (current_token->type == OPERATOR ||
                            current_token->type == DOUBLE_DOT ||
                            current_token->type == TRIPLE_DOT ||
                            token_is_is_operator(current_token))
                        {
                            consume_eols();
                        }
                        else if (psa_eol_end_expr(current_token))
                        {
                            token_ptr peek_token = look_ahead();
                            if (peek_token->type == END_OF_LINE)
                            {
                                break;
                            }
                        }

                        current_token = get_token();
                    }
                    else
                    {
                        precedence_index curr_index = token_to_index(current_token);
                        if (curr_index == OP_UNRECOGNISED)
                        {
                            stack_free(&stack);
                            free_token(current_token);
                            free_token(recognition_token);
                            error_exit(ERR_SYNTACTIC);
                        }
                    }
                }
            }
            break;

        case LEFT_PAR:
        {
            int left_par_count = 1;
            int right_par_count = 0;

            // Push the initial '(' into the PSA stack
            (void)psa_table_compare(&stack, recognition_token, &top_terminal, NULL);
            recognition_token = NULL; // already in stack

            while (left_par_count > right_par_count)
            {
                consume_eols();

                // FUNEXP inline – calls inside parentheses (Ifj.write(""))
                if (current_token->type == IDENT || token_is_ifj_keyword(current_token))
                {
                    if (psa_fun_call_starts_here(current_token))
                    {
                        ASTNode_ptr call_ast = psa_parse_fun_call_operand(current_token);
                        free_token(current_token);

                        token_ptr call_token = malloc(sizeof(token_t));
                        if (!call_token)
                        {
                            stack_free(&stack);
                            error_exit(ERR_INTERNAL);
                        }
                        memset(call_token, 0, sizeof(token_t));

                        call_token->type = IDENT;
                        call_token->ast = call_ast;

                        current_token = call_token;
                    }
                }

                bool should_advance =
                    psa_table_compare(&stack, current_token, &top_terminal, NULL);

                if (should_advance)
                {
                    if (current_token->type == LEFT_PAR)
                        left_par_count++;
                    else if (current_token->type == RIGHT_PAR)
                        right_par_count++;

                    if (left_par_count != right_par_count)
                        current_token = get_token();
                }
                else
                {
                    precedence_index curr_index = token_to_index(current_token);
                    if (curr_index == OP_UNRECOGNISED)
                    {
                        stack_free(&stack);
                        free_token(current_token);
                        error_exit(ERR_SYNTACTIC);
                    }
                }
            }
            break;
        }

        default:
            break;
        }

        if (recognition_token)
        {
            free_token(recognition_token);
            recognition_token = NULL;
        }
    }

    // Final reduction using '$'

    token_ptr end_token = malloc(sizeof(token_t));
    if (!end_token)
    {
        free_token(current_token);
        stack_free(&stack);
        error_exit(ERR_INTERNAL);
    }

    memset(end_token, 0, sizeof(token_t));
    end_token->type = END_OF_FILE;
    end_token->value.other_value = '$';
    end_token->ast = NULL;

    token_ptr top_terminal_final = NULL;

    while (true)
    {
        top_terminal_final = find_top_terminal(&stack);
        psa_table_compare(&stack, end_token, &top_terminal_final, NULL);
        top_terminal_final = find_top_terminal(&stack);

        if (top_terminal_final && top_terminal_final->type == END_OF_FILE)
        {
            break;
        }
    }

    if (stack.stack_size == 2 &&
        stack.head &&
        stack.head->token->type == END_OF_FILE &&
        stack.top &&
        stack.top->token->type == NONTERMINAL_E)
    {

        ASTNode_ptr psa_root = (ASTNode_ptr)stack.top->token->ast;

        free(end_token);
        stack_free(&stack);
        return psa_root;
    }
    else
    {
        free(end_token);
        stack_free(&stack);
        error_exit(ERR_SYNTACTIC);
    }
    return NULL; // Unreachable
}
