/**
 * @file parser_expression.c
 * @author xcillik00
 * @brief Main for the precedence_expression
 * @version 0.1
 * @date 2025-10-27
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "parser_expression.h"
#include "global_structures.h"

/**
 * @brief Precedence table for operators.
 *
 * Relations:
 * - precedence_shift: Push marker and current token
 * - precedence_reduce: Reduce handle to nonterminal E
 * - precedence_equal_reduce: Push current token (used for parentheses)
 * - precedence_error: Syntax error
 * - precedence_finish: End of expression parsing
 *
 * Rows = stack top terminal
 * Cols = current input token
 */
const precedence_relation precedence_table[OP_END + 1][OP_END + 1] = {
    //  +                       -                  *                 /                  <                 >                  <=                 >=                  ==                 !=               (                  )                   i                  $
    {precedence_reduce, precedence_reduce, precedence_shift, precedence_shift, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},   // +
    {precedence_reduce, precedence_reduce, precedence_shift, precedence_shift, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},   // -
    {precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce}, // *
    {precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce}, // /
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},         //
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},         // >
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},         // <=
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},         // >=
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},           // ==
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce},           // !=
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_equal_reduce, precedence_shift, precedence_error},      // (
    {precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_error, precedence_reduce, precedence_error, precedence_reduce}, // )
    {precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_error, precedence_reduce, precedence_error, precedence_reduce}, // i (operand)
    {precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_shift, precedence_finish}             // $
};

// doplnit brief doxxy
// z operandového tokenu (ident, literal) spraví AST leaf
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

// doplnit brief doxxy
// mapovanie scanner operátorov na enum operator_types v AST
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

        // case NULL_V:               return OP_IS;
        // case Q_MARK_V:             return OP_IS;

    default:
        error_exit(ERR_INTERNAL);
    }
    return OP_ERROR;
}

/**
 * @brief Maps token types from scanner to precedence table indices.
 *
 * Converts scanner token types to internal precedence table indices used for
 * determining operator precedence and associativity during expression parsing.
 *
 * @param token Pointer to token from scanner
 * @return precedence_index Index corresponding to the token in precedence table
 */
precedence_index token_to_index(token_ptr token)
{
    // Check if token is an operator
    switch (token->type)
    {
    case OPERATOR:
        // Map specific operator values to precedence indices
        switch (token->value.other_value)
        {
        case PLUS_V:
            return OP_ADD; // +
        case MINUS_V:
            return OP_SUB; // -
        case STAR_V:
            return OP_MULTIPLICATION; // *
        case SLASH_V:
            return OP_DIVISION; // /
        case LESS_THAN_V:
            return OP_LOWER; //
        case GREATER_THAN_V:
            return OP_GREATER; // >
        case LESS_OR_EQ_THAN_V:
            return OP_LOWER_EQUAL; // <=
        case GREATER_OR_EQ_THAN_V:
            return OP_GREATER_EQUAL; // >=
        case LOGICAL_EQUAL_V:
            return OP_EQUAL; // ==
        case LOGICAL_NOT_EQUAL_V:
            return OP_NOT_EQUAL; // !=
        default:
            return OP_UNRECOGNISED; // Fallback for unknown operators
        }

    // Map parentheses to precedence indices
    case LEFT_PAR:
        return OP_LPAR; // (
    case RIGHT_PAR:
        return OP_RPAR; // )

    // All operands (identifiers, literals) map to OP_OPERAND
    case IDENT:
    case GLOB_VAR:
    case INT_LIT:
    case FLOAT_LIT:
    case NULL_LIT:
    case ONE_L_STRING:
    case MUL_L_STRING:
        return OP_OPERAND;

    // End of file marker maps to OP_END ($)
    case END_OF_FILE:
        return OP_END;

    // Default case for any unrecognized token type
    default:
        return OP_UNRECOGNISED;
    }
}

/**
 * @brief Performs reduction when precedence table indicates reduce action ('>').
 *
 * This function implements the reduction phase of precedence parsing:
 * 1. Pops tokens from stack until a MARKER ('<') is found
 * 2. Matches the popped sequence against grammar rules
 * 3. Replaces the matched handle with nonterminal E
 * 4. Frees the tokens that were reduced
 *
 * Grammar rules:
 * - E → i (operand becomes expression)
 * - E → E (nonterminal - for final reductions)
 * - E → (E) (parenthesized expression)
 * - E → E op E (binary operation)
 *
 * @param stack Pointer to the parsing stack
 */
void precedence_reduce_func(Stack *stack)
{
    // Check if stack is empty (should never happen)
    if (stack_is_empty(stack))
    {
        stack_free(stack);
        // printf("Sem som sa dostal 1 \n");
        error_exit(ERR_SYNTACTIC);
    }

    // Buffer for collecting tokens to reduce (max 5 for any grammar rule)
    token_ptr items[5];
    int count = 0; // Number of tokens in the handle

    // Pop tokens from stack until MARKER is found
    // Use stack_pop_no_free() to avoid freeing tokens prematurely
    while (!stack_is_empty(stack))
    {
        token_ptr top = stack_top(stack);

        // Found marker - stop popping and free the marker
        if (top->type == MARKER)
        {
            stack_pop(stack); // Marker can be freed normally
            break;
        }

        // Sanity check: no grammar rule needs more than 5 tokens
        if (count >= 5)
        {
            //  NOW free the tokens that were reduced
            // Don't free nonterminals (they will be used in further reductions)
            for (int i = 0; i < count; i++)
            {
                free_token(items[i]);
            }

            stack_free(stack);
            // printf("Sem som sa dostal 2 \n");
            error_exit(ERR_SYNTACTIC);
        }

        // Save token and pop from stack WITHOUT freeing the token
        items[count++] = top;
        stack_pop_no_free(stack); //  Don't free token yet - we need it
    }

    // Reverse the order of popped items for easier pattern matching
    // Items were popped right-to-left, we need them left-to-right
    token_ptr first_stack_item = (count >= 1) ? items[count - 1] : NULL;
    token_ptr second_stack_item = (count >= 2) ? items[count - 2] : NULL;
    token_ptr third_stack_item = (count >= 3) ? items[count - 3] : NULL;

    bool matched = false; // Flag to check if any grammar rule matched
    ASTNode_ptr reduced_ast = NULL;

    // Grammar rule: E → i (single operand)
    if (!matched && count == 1 && first_stack_item->type != OPERATOR &&
        first_stack_item->type != MARKER &&
        first_stack_item->type != NONTERMINAL_E)
    {
        reduced_ast = ast_from_operand_token(first_stack_item);
        matched = true;
    }

    // Grammar rule: E → E (nonterminal - for final reductions)
    if (!matched && count == 1 && first_stack_item->type == NONTERMINAL_E)
    {
        reduced_ast = (ASTNode_ptr)first_stack_item->ast;
        matched = true;
    }

    // Grammar rule: E → (E) (parenthesized expression)
    if (!matched && count == 3 &&
        first_stack_item->type == LEFT_PAR &&
        second_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == RIGHT_PAR)
    {
        reduced_ast = (ASTNode_ptr)second_stack_item->ast;
        matched = true;
    }

    // Grammar rule: E → E op E (binary operation)
    if (!matched && count == 3 &&
        first_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == NONTERMINAL_E &&
        second_stack_item->type == OPERATOR)
    {
        // Verify the operator is a valid binary operator
        switch (second_stack_item->value.other_value)
        {
        case PLUS_V:               // +
        case MINUS_V:              // -
        case STAR_V:               // *
        case SLASH_V:              // /
        case LESS_THAN_V:          //
        case GREATER_THAN_V:       // >
        case LESS_OR_EQ_THAN_V:    // <=
        case GREATER_OR_EQ_THAN_V: // >=
        case LOGICAL_EQUAL_V:      // ==
        case LOGICAL_NOT_EQUAL_V:
        { // !=

            operator_types op = map_op_to_ast(second_stack_item->value.other_value);

            ASTNode_ptr lhs = (ASTNode_ptr)first_stack_item->ast;
            ASTNode_ptr rhs = (ASTNode_ptr)third_stack_item->ast;

            reduced_ast = ast_create_binary(lhs, rhs, op);
            matched = true;
            break;
        }
        default:
            break;
        }
    }

    // If no grammar rule matched, it's a syntax error
    if (!matched)
    {
        //  NOW free the tokens that were reduced
        // Don't free nonterminals (they will be used in further reductions)
        for (int i = 0; i < count; i++)
        {
            free_token(items[i]);
        }

        stack_free(stack);
        // printf("Sem som sa dostal  3 \n");
        error_exit(ERR_SYNTACTIC);
    }

    // Create new nonterminal E to represent the reduced expression
    token_ptr newE = malloc(sizeof(token_t));
    if (!newE)
    {
        //  NOW free the tokens that were reduced
        // Don't free nonterminals (they will be used in further reductions)
        for (int i = 0; i < count; i++)
        {
            free_token(items[i]);
        }
        stack_free(stack);
        error_exit(ERR_INTERNAL);
    }

    memset(newE, 0, sizeof(token_t));
    newE->type = NONTERMINAL_E;
    newE->ast = reduced_ast;

    // Push the nonterminal E back onto stack
    stack_push(stack, newE);

    //  NOW free the tokens that were reduced
    // Don't free nonterminals (they will be used in further reductions)
    for (int i = 0; i < count; i++)
    {
        free_token(items[i]);
    }
}

/**
 * @brief Performs one comparison step between stack top terminal and current token.
 *
 * Uses the precedence table to determine action:
 * - precedence_shift: Insert marker '<' after top terminal, push current token
 * - precedence_reduce: Call reduction function to reduce handle on stack
 * - precedence_equal_reduce: Push current token without marker (for parentheses)
 * - precedence_finish: End of expression reached ($ compared with $)
 * - precedence_error: Invalid token combination
 *
 * @param stack Pointer to the parsing stack
 * @param current_token Current input token to process
 * @param top_terminal Pointer to store the top terminal from stack
 * @return true if token was shifted, false if reduction occurred
 */
bool precedence_table_compare(Stack *stack, token_ptr current_token, token_ptr *top_terminal)
{

    // Get the topmost item from stack
    token_ptr top_token = stack_top(stack);
    if (top_token == NULL)
    {
        stack_free(stack);
        free_token(current_token);
        // printf("Sem som sa dostal  4\n");
        error_exit(ERR_SYNTACTIC); // Stack should never be empty during parsing
    }

    // Find the topmost terminal on the stack (skip nonterminals and markers)
    *top_terminal = NULL;
    StackItem *tmp = stack->head;
    StackItem *last_terminal_item = NULL;

    // Traverse stack to find the topmost terminal
    while (tmp != NULL)
    {
        if (tmp->token->type != NONTERMINAL_E && tmp->token->type != MARKER)
        {
            *top_terminal = tmp->token;
            last_terminal_item = tmp;
        }
        tmp = tmp->next;
    }

    // Top terminal should always exist (at minimum, '$' is on stack)
    if (*top_terminal == NULL)
    {
        stack_free(stack);
        free_token(current_token);
        // printf("Sem som sa dostal 5 \n");
        error_exit(ERR_SYNTACTIC);
    }

    // Set pointer to top terminal for marker insertion
    if (last_terminal_item != NULL)
    {
        stack_set_top_terminal_pointer(stack, last_terminal_item);
    }

    // Convert tokens to precedence table indices
    precedence_index top_index = token_to_index(*top_terminal);
    if (top_index == OP_UNRECOGNISED)
    {
        stack_free(stack);
        free_token(current_token);
        // printf("Sem som sa dostal 6 \n");
        error_exit(ERR_SYNTACTIC);
    }

    precedence_index curr_index = token_to_index(current_token);
    if (curr_index == OP_UNRECOGNISED)
    {
        stack_free(stack);
        free_token(current_token);
        // printf("Sem som sa dostal  7 \n");
        error_exit(ERR_SYNTACTIC);
    }

    // Look up the precedence relation in the table
    precedence_relation rel = precedence_table[top_index][curr_index];

    // Execute action based on precedence relation
    switch (rel)
    {
    case precedence_shift:
    {
        // Shift action: insert marker after top terminal, then push current token
        token_ptr marker = malloc(sizeof(token_t));
        if (!marker)
        {
            stack_free(stack);
            free_token(current_token);
            error_exit(ERR_INTERNAL);
        }
        marker->type = MARKER; // Special internal token type
        marker->value.other_value = '<';

        // Insert marker after the top terminal
        stack_push_after(stack, marker);
        // Push current token on top
        stack_push(stack, current_token);
        return true;
    }
    case precedence_equal_reduce:
        // Equal precedence: push current token without marker (used for parentheses)
        stack_push(stack, current_token);
        return true;
    case precedence_reduce:
        // Reduce action: call reduction function to reduce handle
        precedence_reduce_func(stack);
        return false;
    case precedence_finish:
        // Comparing $ with $ means end of expression
        return true;
    case precedence_error:
    default:
        // Invalid token combination according to precedence table
        // printf("DEBUG: current_token type=%d\n", current_token->type);
        stack_free(stack);
        free_token(current_token);

        // printf("Sem som sa dostal 8\n");
        error_exit(ERR_SYNTACTIC);
    }
    return true;
}

/**
 * @brief Main expression parsing function using precedence analysis.
 *
 * Parses expressions in different contexts (assignment, condition, return) based on
 * the recognition token. Handles operator precedence, associativity, and
 * special cases like newlines after operators.
 *
 * Algorithm:
 * 1. Initialize stack with $ marker
 * 2. Process tokens based on context (assignment/return vs condition)
 * 3. Use precedence table to shift/reduce
 * 4. After expression ends, reduce remaining items until $E is on stack
 *
 * @param recognition_token Token that indicates context (= for assignment, ( for condition, return keyword)
 * @return true if expression is syntactically valid, exits with error otherwise
 */
ASTNode_ptr parse_expression(token_ptr recognition_token)
{

    // Initialize the parsing stack
    Stack stack;
    stack_init(&stack);

    // Push special end marker ($) onto stack as bottom marker
    token_ptr special_char = malloc(sizeof(token_t));
    if (!special_char)
    {
        free_token(recognition_token);
        // we dont have to call stack free since this stack is initialized localy and not allocated on heap
        // stack_free(stack);
        error_exit(ERR_INTERNAL);
    }

    special_char->type = END_OF_FILE;
    special_char->value.other_value = '$';
    stack_push(&stack, special_char);

    // Get first token from input
    token_ptr current_token = get_token();

    // Pointer to track the topmost terminal on stack (for precedence comparison)
    token_ptr top_terminal = NULL;

    // Determine parsing context based on recognition token
    switch (recognition_token->type)
    {
    // Return context: return expression
    case KEY_WORD:
    { // ← Otvárajúca zátvorka, ale ZATIAĽ NEUZATVÁRAJ!
        // Check if the keyword is "return"
        if (recognition_token->value.str_value != NULL &&
            strcmp(recognition_token->value.str_value, "return") == 0)
        {
            // Parse assignment expression until EOL after non-operator
            while (true)
            {

                bool should_advance = precedence_table_compare(&stack, current_token, &top_terminal);

                if (should_advance)
                {
                    if (current_token->type == OPERATOR)
                    {
                        consume_eols();
                    }
                    else if (eol_end_expr(current_token))
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
                    precedence_index curr_index = token_to_index(current_token);

                    if (curr_index == OP_UNRECOGNISED)
                    {
                        // printf("Sem som sa dostal 9 \n");

                        stack_free(&stack);
                        free_token(current_token);
                        free_token(recognition_token);

                        error_exit(ERR_SYNTACTIC);
                    }
                }
            }
        }
        break;
    }
    case OPERATOR:
        switch (recognition_token->value.other_value)
        {
        // Assignment context: var = expression
        case EQUAL_SIGN_V:
        {

            // Parse assignment expression until EOL after non-operator
            while (true)
            {
                // printf("DEBUG: current_token type=%d\n", current_token->type);

                // Process current token with precedence comparison
                bool should_advance = precedence_table_compare(&stack, current_token, &top_terminal);
                // free(current_token); // erase ??

                // If we should advance (shift or equal operation)
                if (should_advance)
                {
                    // If current_token is OPERATOR , we check if there are any EOF
                    // IF there are not then it does nothing
                    if (current_token->type == OPERATOR)
                    {
                        consume_eols();
                    }
                    else if (eol_end_expr(current_token))
                    {
                        token_ptr peek_token = get_token();

                        if (peek_token->type == END_OF_LINE)
                        {
                            push_token(peek_token);
                            break;
                        }
                        // expression ended, the rest of precedence analysis will terminate

                        push_token(peek_token);
                    }

                    // reading next token
                    current_token = get_token();
                    precedence_index curr_index = token_to_index(current_token);

                    if (curr_index == OP_UNRECOGNISED)
                    {
                        // printf("DEBUG: current_token type=%d\n", current_token->type);
                        // printf("Sem som sa dostal 10 \n");

                        stack_free(&stack);
                        free_token(current_token);
                        free_token(recognition_token);

                        error_exit(ERR_SYNTACTIC);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
        break;

        // Condition context: if (expression) or while (expression)
    case LEFT_PAR:
    {
        int left_par_count = 1;
        int right_par_count = 0;

        bool should_advance = precedence_table_compare(&stack, recognition_token, &top_terminal);

        while (left_par_count > right_par_count)
        {
            consume_eols();

            should_advance = precedence_table_compare(&stack, current_token, &top_terminal);

            if (should_advance)
            {
                if (current_token->type == LEFT_PAR)
                    left_par_count++;
                else if (current_token->type == RIGHT_PAR){
                    right_par_count++;
                }

                if(left_par_count != right_par_count)
                    current_token = get_token();
            }
        }
        break;
    }
    default:
        break;
    }

    // printf("Dostal som sa az po kontrolu s $ (Vysiel som z pytania si tokenov) \n ");
    // After main expression parsing, create end token for final reductions
    token_ptr end_token = malloc(sizeof(token_t));
    if (!end_token)
    {
        free_token(recognition_token);
        free_token(current_token);
        stack_free(&stack);
        error_exit(ERR_INTERNAL);
    }

    end_token->type = END_OF_FILE;
    end_token->value.other_value = '$';

    token_ptr top_terminal_final = NULL;

    // Final reduction phase: reduce all remaining handles until only $E remains
    while (true)
    {
        // Find the topmost terminal on stack
        StackItem *tmp = stack.head;
        top_terminal_final = NULL;
        while (tmp)
        {
            if (tmp->token->type != NONTERMINAL_E && tmp->token->type != MARKER)
                top_terminal_final = tmp->token;
            tmp = tmp->next;
        }

        // Compare top terminal with $ (end marker)
        precedence_table_compare(&stack, end_token, &top_terminal_final);

        // Update top terminal after potential reduction
        tmp = stack.head;
        top_terminal_final = NULL;
        while (tmp)
        {
            if (tmp->token->type != NONTERMINAL_E && tmp->token->type != MARKER)
                top_terminal_final = tmp->token;
            tmp = tmp->next;
        }

        // Check if we reached final state ($ on top)
        if (top_terminal_final && top_terminal_final->type == END_OF_FILE)
        {
            break;
        }
    }

    // Verify final stack state: should be exactly $E
    if (stack.stack_size == 2 &&
        stack.head &&
        stack.head->token->type == END_OF_FILE &&
        stack.top &&
        stack.top->token->type == NONTERMINAL_E)
    {
        // Success: expression is syntactically valid

        // root pre PSA
        ASTNode_ptr psa_root = (ASTNode_ptr)stack.top->token->ast;

        // printf("Everything went fine (precedence analysis) \n ");

        free(end_token);
        // free(recognition_token);
        // free(current_token);
        stack_free(&stack);
        return psa_root;
    }
    else
    {

        // Error: stack not in expected final state
        free(end_token);
        stack_free(&stack);
        free(recognition_token);
        free(current_token);
        // printf("Sem som sa dostal 11 \n");
        error_exit(ERR_SYNTACTIC);
    }

    return NULL;
}
