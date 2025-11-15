#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "parser_expression.h"
#include "scanner.h"
#include "stack.h"

#ifndef MARKER
#define MARKER 132456
#endif

#ifndef NONTERMINAL_E 
#define NONTERMINAL_E 123456789
#endif

const precedence_relation precedence_table[OP_END+1][OP_END+1] = {
    //  +                       -                  *                 /                  <                 >                  <=                 >=                  ==                 !=               (                  )                   i                  $
    { precedence_reduce, precedence_reduce, precedence_shift, precedence_shift, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // +
    { precedence_reduce, precedence_reduce, precedence_shift, precedence_shift, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // -
    { precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // *
    { precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // /
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // <
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // >
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // <=
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_error, precedence_error, precedence_error, precedence_error, precedence_reduce, precedence_reduce, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // >=
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // ==
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_error, precedence_shift, precedence_reduce, precedence_shift, precedence_reduce }, // !=
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_equal_reduce, precedence_shift, precedence_error }, // (
    { precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_error, precedence_reduce, precedence_error, precedence_reduce }, // )
    { precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_reduce, precedence_error, precedence_reduce, precedence_error, precedence_reduce }, // i (operand)
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_shift, precedence_finish }  // $
};

precedence_index token_to_index(token_ptr token) {
    switch (token->type) {
        case OPERATOR:
            switch (token->value.other_value) {
                case PLUS_V:              return OP_ADD;
                case MINUS_V:             return OP_SUB;
                case ASTERISK_V:          return OP_MUL;
                case DIVISON_V:           return OP_DIV;
                case LESS_THAN_V:         return OP_LOWER;
                case GREATER_THAN_V:      return OP_GREATER;
                case LESS_OR_EQ_THAN_V:   return OP_LOWER_EQUAL;
                case GREATER_OR_EQ_THAN_V:return OP_GREATER_EQUAL;
                case EQUAL_SIGN_V:        return OP_EQUAL;
                case EXC_MARK_V:          return OP_NOT_EQUAL;
                default:                  return OP_OPERAND;
            }
        case LEFT_PAR:   return OP_LPAR;
        case RIGHT_PAR:  return OP_RPAR;
        case IDENT:
        case GLOB_VAR:
        case INT_LIT:
        case FLOAT_LIT:
        case ONE_L_STRING:
        case MUL_L_STRING:
            return OP_OPERAND;
        case END_OF_FILE:
            return OP_END;
        default:
            return OP_OPERAND;
    }
}

void precedence_reduce_func(Stack *stack) {
    if (stack_is_empty(stack)){
        printf("V precedence reduce func stack_is_empty Error \n");
        error_exit(ERR_SYNTACTIC);
    }

    token_ptr items[5]; 
    int count = 0;

    while (!stack_is_empty(stack)) {
        token_ptr top = stack_top(stack);
        if (top->type == MARKER) {
            stack_pop(stack);
            break;
        }
        if (count >= 5){
            printf("V precedence reduce func ked je count vacsi rovny 5 \n");
            error_exit(ERR_SYNTACTIC);
        }
        items[count++] = top;
        stack_pop(stack);
    }

    token_ptr first_stack_item   = (count >= 1) ? items[count - 1] : NULL;
    token_ptr second_stack_item  = (count >= 2) ? items[count - 2] : NULL;
    token_ptr third_stack_item   = (count >= 3) ? items[count - 3] : NULL;

    bool matched = false;

    // ✅ E → i (terminal/operand)
    if (count == 1 && first_stack_item->type != OPERATOR &&
        first_stack_item->type != MARKER &&
        first_stack_item->type != NONTERMINAL_E) {
        matched = true;
    }

    // ✅ E → E (nonterminal - pre finálne redukcie)
    if (count == 1 && first_stack_item->type == NONTERMINAL_E) {
        matched = true;
    }

    // ✅ E → (E)
    if (count == 3 &&
        first_stack_item->type == LEFT_PAR &&
        second_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == RIGHT_PAR) {
        matched = true;
    }

    // ✅ E → E op E (binárne operátory)
    if (count == 3 &&
        first_stack_item->type == NONTERMINAL_E &&
        third_stack_item->type == NONTERMINAL_E &&
        second_stack_item->type == OPERATOR) {
        switch (second_stack_item->value.other_value) {
            case PLUS_V:
            case MINUS_V:
            case ASTERISK_V:
            case DIVISON_V:
            case LESS_THAN_V:
            case GREATER_THAN_V:
            case LESS_OR_EQ_THAN_V:
            case GREATER_OR_EQ_THAN_V:
            case EQUAL_SIGN_V:
            case EXC_MARK_V:
                matched = true;
                break;
            default:
                break;
        }
    }

    if (!matched){
        printf("V precedence reduce func neni match \n");
        error_exit(ERR_SYNTACTIC);        
    }

    token_ptr newE = malloc(sizeof(token_t));
    if (!newE) error_exit(ERR_INTERNAL);
    newE->type = NONTERMINAL_E;
    stack_push(stack, newE);
}

bool precedence_table_compare(Stack *stack, token_ptr current_token, token_ptr *top_terminal) {
    token_ptr top_token = stack_top(stack);
    if (top_token == NULL){
        printf("V precedence_table_compare ked top token je NULL \n");
        error_exit(ERR_SYNTACTIC);
    }

    // ✅ VŽDY AKTUALIZUJ top_terminal - prejdi celý stack a nájdi najvrchnejší terminál
    *top_terminal = NULL;
    StackItem *tmp = stack->head;
    StackItem *last_terminal_item = NULL;
    
    while (tmp != NULL) {
        if (tmp->token->type != NONTERMINAL_E && tmp->token->type != MARKER) {
            *top_terminal = tmp->token;
            last_terminal_item = tmp;
        }
        tmp = tmp->next;
    }

    if(*top_terminal == NULL){
        printf("V precedence_table_compare ked top_terminal je NULL error \n");
        error_exit(ERR_SYNTACTIC);
    }
    
    if (last_terminal_item != NULL) {
        stack_set_top_terminal_pointer(stack, last_terminal_item);
    }

    precedence_index top_index = token_to_index(*top_terminal);
    precedence_index curr_index = token_to_index(current_token);
    precedence_relation rel = precedence_table[top_index][curr_index];

    switch (rel) {
        case precedence_shift: {
            token_ptr marker = malloc(sizeof(token_t));
            if (!marker) error_exit(ERR_INTERNAL);
            marker->type = MARKER;
            marker->value.other_value = '<';
            stack_push_after(stack, marker);
            stack_push(stack, current_token);
            return true;
        }
        case precedence_equal_reduce:
            stack_push(stack, current_token);
            return true;
        case precedence_reduce:
            precedence_reduce_func(stack);
            return false;
        case precedence_finish:
            return true;
        case precedence_error:
        default:
            printf("V precedence_table_compare v casey precedence_error \n");
            error_exit(ERR_SYNTACTIC);
    }
    return true;
}

bool parse_expression(token_ptr recognition_token) {
    printf("DEBUG: parse_expression called, recognition_token type=%d\n", 
           recognition_token->type);

    Stack stack;
    stack_init(&stack);

    token_ptr special_char = malloc(sizeof(token_t));
    if (!special_char) 
        error_exit(ERR_INTERNAL);
    special_char->type = END_OF_FILE;
    special_char->value.other_value = '$';
    stack_push(&stack, special_char);

    token_ptr current_token = get_token();
    token_ptr top_terminal = NULL;

    switch (recognition_token->type) {
        case OPERATOR: 
            switch (recognition_token->value.other_value) {
                case EQUAL_SIGN_V: {
                    while (true) {
                        while (current_token->type == EOL) {
                            current_token = get_token();
                        }

                        if (current_token->type == END_OF_FILE ||
                            current_token->type == LEFT_DOM_PAR) {
                            printf("V assignmente ked je EOF alebo { \n");
                            error_exit(ERR_SYNTACTIC);
                        }

                        printf("DEBUG: current_token type=%d\n", current_token->type);

                        bool should_advance = precedence_table_compare(&stack, current_token, &top_terminal);

                        if (should_advance) {
                            current_token = get_token();

                            if (current_token->type == EOL) {
                                token_ptr peek = get_token();
                                
                                if (peek->type != OPERATOR) {
                                    push_token(peek);
                                    break;
                                }
                                
                                push_token(peek);
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;

        case LEFT_PAR: {
            int ifj_left_par_count = 1;
            int ifj_right_par_count = 0;

            while (ifj_left_par_count > ifj_right_par_count) {
                if (current_token->type == EOL) {
                    current_token = get_token();
                    continue;
                }

                if (current_token->type == END_OF_FILE ||
                    current_token->type == LEFT_DOM_PAR) {
                    printf("V conditione ked je EOF alebo { \n");
                    error_exit(ERR_SYNTACTIC);
                }

                printf("DEBUG: current_token type=%d\n", current_token->type);

                if (current_token->type == LEFT_PAR)
                    ifj_left_par_count++;
                else if (current_token->type == RIGHT_PAR)
                    ifj_right_par_count++;

                bool should_advance = precedence_table_compare(&stack, current_token, &top_terminal);

                if (should_advance) {
                    current_token = get_token();
                }
            }
            break;
        }

        case KEY_WORD:
        default: 
            break;
    }

    printf("Dostal som sa az po kontrolu s $\n");

    token_ptr end_token = malloc(sizeof(token_t));
    if (!end_token)
        error_exit(ERR_INTERNAL);
    end_token->type = END_OF_FILE;
    end_token->value.other_value = '$';

    token_ptr top_terminal_final = NULL;

    while (true) {
        StackItem *tmp = stack.head;
        top_terminal_final = NULL;
        while (tmp) {
            if (tmp->token->type != NONTERMINAL_E && tmp->token->type != MARKER)
                top_terminal_final = tmp->token;
            tmp = tmp->next;
        }

        if (top_terminal_final && top_terminal_final->type == END_OF_FILE) {
            break;
        }

        precedence_table_compare(&stack, end_token, &top_terminal_final);
    }

    if (stack.stack_size == 2 &&
        stack.head &&
        stack.head->token->type == END_OF_FILE &&
        stack.top &&
        stack.top->token->type == NONTERMINAL_E) {
        free(end_token);
        stack_free(&stack);
        return true;
    }
    else {
        free(end_token);
        printf("Uplne nakonci vsetkeho \n");       
        error_exit(ERR_SYNTACTIC);
    }

    return false;
}