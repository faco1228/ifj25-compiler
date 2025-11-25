/**
 * @file psa.c
 * @brief Precedence syntax analyzer (PSA) for expressions.
 *
 * Podporované:
 *  - aritmetika: + - * /
 *  - relačné operátory: < > <= >=
 *  - typový operátor: is
 *  - rovnostné: == !=
 *  - rozsahy: .. ...
 *  - FUNEXP: volania funkcií ako operand (user + Ifj.*)
 *  - (TODO) unárny mínus
 */

#include "parser_expression.h"
#include "global_structures.h"

/* Pomocná funkcia na nájdenie top terminálu */
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

/* ------------------------------------------------------------------------
 *  Pomocné funkcie pre vytváranie AST z operandov a operátorov
 * ------------------------------------------------------------------------ */

static ASTNode_ptr ast_from_operand_token(token_ptr t)
{
    switch (t->type)
    {
    case IDENT:
    case GLOB_VAR:
        return ast_create_ident(t->value.str_value);

    case INT_LIT:
        return ast_create_int(t->value.int_value);

    case FLOAT_LIT:
        return ast_create_float(t->value.float_value);

    case ONE_L_STRING:
    case MUL_L_STRING:
        return ast_create_str(t->value.str_value);

    case NULL_LIT:
        return ast_create_null();

    default:
        free_token(t);
        error_exit(ERR_INTERNAL);
    }
    return NULL;
}

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

/* ------------------------------------------------------------------------
 *  Rozpoznanie špeciálnych tokenov
 * ------------------------------------------------------------------------ */

static bool token_is_is_operator(token_ptr t)
{
    return (t->type == KEY_WORD &&
            t->value.str_value != NULL &&
            strcmp(t->value.str_value, "is") == 0);
}

/* KEY_WORD "Ifj" – prefix pre built-in volania Ifj.* */
static bool token_is_ifj_keyword(token_ptr t)
{
    return (t->type == KEY_WORD &&
            t->value.str_value != NULL &&
            strcmp(t->value.str_value, "Ifj") == 0);
}

// skontroluje ci je keyword Num alebo String alebo Null
static bool token_is_type_keyword(token_ptr t)
{
    printf("DEBUG\n");
    return (t->type == KEY_WORD &&
            t->value.str_value != NULL &&
            (strcmp(t->value.str_value, "String") == 0 ||
             strcmp(t->value.str_value, "Num") == 0 ||
             strcmp(t->value.str_value, "Null") == 0));
}

/* EOL môže ukončiť výraz? (heuristika pre assignment/return) */
static bool psa_eol_end_expr(token_ptr current_token)
{
    switch (current_token->type)
    {
    case IDENT:
    case GLOB_VAR:
    case INT_LIT:
    case FLOAT_LIT:
    case NULL_LIT:
    case ONE_L_STRING:
    case MUL_L_STRING:
    case RIGHT_PAR:
        return true;
    default:
        return false;
    }
}

/* ------------------------------------------------------------------------
 *  Mapovanie tokenov na indexy precedenčnej tabuľky
 * ------------------------------------------------------------------------ */

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
        return OP_D_DOT; // ..
    case TRIPLE_DOT:
        return OP_T_DOT; // ...

    case IDENT:
    case GLOB_VAR:
    case INT_LIT:
    case FLOAT_LIT:
    case NULL_LIT:
    case ONE_L_STRING:
    case MUL_L_STRING:
        return OP_OPERAND;

    case KEY_WORD:
        if (token_is_is_operator(token))
            return OP_IS_TOK;
        if (token_is_ifj_keyword(token))
            return OP_OPERAND; // <-- PRIDANÉ: Ifj je operand
        if (token_is_type_keyword(token))
            return OP_OPERAND;
        return OP_UNRECOGNISED;

    case END_OF_FILE:
        return OP_END;

    default:
        return OP_UNRECOGNISED;
    }
}

/* ------------------------------------------------------------------------
 *  Precedenčná tabuľka
 * ------------------------------------------------------------------------ */

static const precedence_relation precedence_table[OP_END + 1][OP_END + 1] = {
    //          +,−         *,/         <           >           <=          >=          ==          !=          (           )              i           is           ..          ...         $
    /* +,− */ {psa_reduce, psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* *,/ */ {psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* <   */ {psa_shift, psa_shift, psa_error, psa_error, psa_error, psa_error, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_shift, psa_reduce},
    /* >   */ {psa_shift, psa_shift, psa_error, psa_error, psa_error, psa_error, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_shift, psa_reduce},
    /* <=  */ {psa_shift, psa_shift, psa_error, psa_error, psa_error, psa_error, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_shift, psa_reduce},
    /* >=  */ {psa_shift, psa_shift, psa_error, psa_error, psa_error, psa_error, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_shift, psa_reduce},
    /* ==  */ {psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_error, psa_error, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_shift, psa_reduce},
    /* !=  */ {psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_error, psa_error, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_shift, psa_reduce},
    /* (   */ {psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_eq_reduce, psa_shift, psa_shift, psa_shift, psa_shift, psa_error},
    /* )   */ {psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_error, psa_reduce, psa_error, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* i   */ {psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_error, psa_reduce, psa_reduce, psa_reduce, psa_reduce},
    /* is  */ {psa_shift, psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_error, psa_reduce, psa_reduce, psa_reduce},
    /* ..  */ {psa_shift, psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_error, psa_error, psa_reduce},
    /* ... */ {psa_shift, psa_shift, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_reduce, psa_shift, psa_reduce, psa_shift, psa_reduce, psa_error, psa_error, psa_reduce},
    /* $   */ {psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_shift, psa_error, psa_shift, psa_shift, psa_shift, psa_shift, psa_finish}};

/* ------------------------------------------------------------------------
 *  FUNEXP – volanie funkcie ako operand (user aj Ifj.*)
 * ------------------------------------------------------------------------ */

ASTNode_ptr parse_expression(token_ptr recognition_token);

/* Rozhodne, či tento token je začiatkom volania funkcie (FUNEXP). */
static bool psa_fun_call_starts_here(token_ptr first_token)
{
    token_ptr la;

    /* User funkcia: foo(...) */
    if (first_token->type == IDENT)
    {
        /* pozrieme sa na ďalší token zo scanneru */
        la = get_token();

        if (la->type == LEFT_PAR)
        {
            /* je to foo( ... ) → volanie funkcie */
            push_token(la);
            return true;
        }

        /* nie je to volanie, vrátime token späť a berieme to ako obyčajný ident */
        push_token(la);
        return false;
    }

    /* Built-in: Ifj.something(...) */
    if (token_is_ifj_keyword(first_token)) /* KEY_WORD "Ifj" */
    {
        la = get_token();

        if (la->type == DOT)
        {
            /* Ifj . ... → začiatok built-in volania */
            push_token(la);
            return true;
        }

        push_token(la);
        return false;
    }

    return false;
}

/* <arg_list> ::= ε | <expression> ( "," <expression> )* */
static void psa_parse_call_args(ASTNode_ptr call_node)
{
    unsigned arg_count = 0;

    /* sme už za menom a za '(' */
    consume_eols();
    token_ptr la = get_token();

    if (la->type == RIGHT_PAR)
    {
        /* volanie bez argumentov: foo() / Ifj.read_str() */
        free_token(la);
        call_node->data.function_call.param_count = 0;
        return;
    }

    /* nie je hneď ')', vrátime token späť a čítame prvý výraz */
    push_token(la);

    while (1)
    {
        /* každý argument je výraz – PSA, bez špeciálneho recognition tokenu */
        ASTNode_ptr arg = parse_expression(NULL);
        add_child(call_node, arg);
        arg_count++;

        consume_eols();
        token_ptr t = get_token();

        if (t->type == COMMA)
        {
            /* ďalší argument */
            free_token(t);
            consume_eols();
            continue;
        }
        else if (t->type == RIGHT_PAR)
        {
            /* koniec argumentov */
            free_token(t);
            break;
        }
        else
        {
            /* čokoľvek iné je syntaktická chyba */
            free_token(t);
            error_exit(ERR_SYNTACTIC);
        }
    }

    call_node->data.function_call.param_count = arg_count;
}

/* Parsuje volanie funkcie (user aj built-in Ifj.*) ako operand. */
static ASTNode_ptr psa_parse_fun_call_operand(token_ptr first_token)
{
    bool is_builtin = false;
    char *func_name = NULL;
    token_ptr t;

    if (first_token->type == IDENT)
    {
        /* user funkcia: foo(...) */
        func_name = first_token->value.str_value;
        is_builtin = false;

        t = get_token();
        if (t->type != LEFT_PAR)
        {
            free_token(t);
            error_exit(ERR_SYNTACTIC);
        }
        free_token(t);
    }
    else if (token_is_ifj_keyword(first_token))
    {
        /* built-in: Ifj.read_str(...) */
        t = get_token();
        if (t->type != DOT)
        {
            free_token(t);
            error_exit(ERR_SYNTACTIC);
        }
        free_token(t);

        t = get_token();
        if (t->type != IDENT)
        {
            free_token(t);
            error_exit(ERR_SYNTACTIC);
        }
        func_name = t->value.str_value; /* meno built-inu (read_str, write, ...) */
        free_token(t);

        t = get_token();
        if (t->type != LEFT_PAR)
        {
            free_token(t);
            error_exit(ERR_SYNTACTIC);
        }
        free_token(t);

        is_builtin = true;
    }
    else
    {
        /* sem by sme sa nemali dostať, ak psa_fun_call_starts_here funguje správne */
        error_exit(ERR_INTERNAL);
    }

    /* vytvor CALL node, počet parametrov doplníme po parsovaní arg listu */
    ASTNode_ptr call = ast_create_call(func_name, 0, is_builtin);

    /* naparsuj argumenty a nastav param_count */
    psa_parse_call_args(call);

    return call;
}

/* Vráti operand pre PSA – buď obyčajný literal/ident, alebo volanie funkcie. */
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

/* ------------------------------------------------------------------------
 *  Redukcia – handle → NONTERMINAL_E + AST
 * ------------------------------------------------------------------------ */

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

    /* E -> i */
    if (!matched &&
        count == 1 &&
        first_stack_item->type != MARKER &&
        first_stack_item->type != NONTERMINAL_E)
    {
        reduced_ast = psa_parse_operand(first_stack_item);
        matched = true;
    }

    /* E -> E */
    if (!matched &&
        count == 1 &&
        first_stack_item->type == NONTERMINAL_E)
    {
        reduced_ast = (ASTNode_ptr)first_stack_item->ast;
        matched = true;
    }

    /* E -> (E) */
    if (!matched &&
        count == 3 &&
        first_stack_item->type == LEFT_PAR &&
        second_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == RIGHT_PAR)
    {
        reduced_ast = (ASTNode_ptr)second_stack_item->ast;
        matched = true;
    }

    /* E -> E op E (binárne + is + .. + ...) */
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
            reduced_ast = ast_create_binary(lhs, rhs, OP_IS);
            matched = true;
        }
        else if (op_tok->type == DOUBLE_DOT || op_tok->type == TRIPLE_DOT)
        {
            bool inclusive = (op_tok->type == DOUBLE_DOT);
            reduced_ast = ast_create_range(lhs, rhs, inclusive);
            matched = true;
        }
    }

    /* TODO: E -> -E (unárny mínus) */

    if (!matched)
    {
        for (int i = 0; i < count; i++)
            free_token(items[i]);
        stack_free(stack);
        if (recognition_token)
            free_token(recognition_token);
        error_exit(ERR_SYNTACTIC);
    }

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

/* ------------------------------------------------------------------------
 *  Porovnanie podľa precedenčnej tabuľky
 * ------------------------------------------------------------------------ */

static bool psa_table_compare(Stack *stack,
                              token_ptr current_token,
                              token_ptr *top_terminal,
                              token_ptr recognition_token)
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

/* ------------------------------------------------------------------------
 *  Hlavná funkcia – parse_expression
 * ------------------------------------------------------------------------ */

ASTNode_ptr parse_expression(token_ptr recognition_token)
{
    Stack stack;
    stack_init(&stack);

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

    /* ─────────────── základný (NULL) kontext ─────────────── */
    if (recognition_token == NULL)
    {
        while (true)
        {
            /* 1) Konec výrazu – tieto tokeny už PSA nesmie spotrebovať */
            if (current_token->type == END_OF_LINE ||
                current_token->type == END_OF_FILE ||
                current_token->type == COMMA ||
                current_token->type == RIGHT_PAR)
            {
                push_token(current_token);
                break;
            }

            top_terminal = find_top_terminal(&stack);

            /* 2a) FUNEXP heuristika – built-in: Ifj . something(...) */
            if (current_token->type == DOT &&
                top_terminal &&
                token_is_ifj_keyword(top_terminal))
            {
                /* '.' nemá ísť do PSA – necháme ho na psa_parse_fun_call_operand() */
                push_token(current_token);
                break;
            }

            /* 2b) FUNEXP heuristika – user/built-in volanie: ident/Ifj + '(' */
            if (current_token->type == LEFT_PAR &&
                top_terminal &&
                (top_terminal->type == IDENT ||
                 token_is_ifj_keyword(top_terminal)))
            {
                /* '(' nemá ísť do PSA – necháme ju na psa_parse_fun_call_operand() */
                push_token(current_token);
                break;
            }

            /* 3) Normálny PSA režim */
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

    /* ─────────────── špeciálne kontexty (return, =, in, '(') ─────────────── */
    else
    {
        switch (recognition_token->type)
        {
        case KEY_WORD:
            /* return <expr> */
            if (recognition_token->value.str_value != NULL &&
                strcmp(recognition_token->value.str_value, "return") == 0)
            {

                while (true)
                {
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
                            token_ptr peek_token = get_token();
                            if (peek_token->type == END_OF_LINE)
                            {
                                push_token(peek_token);
                                break;
                            }
                            push_token(peek_token);
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
            /* for (... in <expr>) */
            else if (strcmp(recognition_token->value.str_value, "in") == 0)
            {
                while (true)
                {
                    bool should_advance =
                        psa_table_compare(&stack, current_token, &top_terminal, recognition_token);

                    if (should_advance)
                    {
                        current_token = get_token();
                        if (current_token->type == RIGHT_PAR)
                        {
                            push_token(current_token);
                            break;
                        }
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
            /* assignment RHS: = <expr> */
            if (recognition_token->value.other_value == EQUAL_SIGN_V)
            {
                while (true)
                {
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
                            token_ptr peek_token = get_token();
                            if (peek_token->type == END_OF_LINE)
                            {
                                push_token(peek_token);
                                break;
                            }
                            push_token(peek_token);
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

            (void)psa_table_compare(&stack, recognition_token, &top_terminal, NULL);
            recognition_token = NULL; /* už je v zásobníku */

            while (left_par_count > right_par_count)
            {
                consume_eols();

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

    /* ─────────────── finálne doredukovanie pomocou '$' ─────────────── */

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
    return NULL;
}
