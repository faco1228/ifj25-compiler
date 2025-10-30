#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "parser_expression.h"
#include "scanner.h"
#include "stack.h"


/**
 * @brief Internal token type for precedence parser marker ('<')
 * This value is not from the scanner; it is only used inside the expression parser.
 */

 // TOTO MI PORADIL CHAT LEBO MY SME NEMALI TOKEN ZE MARKER A PODLA MOJEJ IMPLEMENTACIE STACKU TO TAM POTREBUEJEM
#ifndef MARKER
#define MARKER 9237492384  // must not collide with real token types
#endif



/**
 * @brief Precedence table for operators.
 * 
 * Relations: < shift, > reduce, = equal, E error
 * 
 * Rows = stack top
 * Cols = current token
 */
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
    { precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift,  precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_shift, precedence_error, precedence_shift, precedence_error }  // $
};


/**
 * @brief Maps token types from scanner to precedence table indices.
 * 
 * @param token pointer to token
 * @return precedence_index corresponding to the token
 */
precedence_index token_to_index(token_ptr token) {

    switch (token->type) {
        case OPERATOR:
            switch (token->value.other_value) {
                case '+': return OP_ADD;
                case '-': return OP_SUB;
                case '*': return OP_MUL;
                case '/': return OP_DIV;
                case '<': return OP_LOWER;
                case '>': return OP_GREATER;
                case '=': return OP_EQUAL;
                default: return OP_OPERAND;
            }

        case LEFT_PAR:  return OP_LPAR;
        case RIGHT_PAR: return OP_RPAR;
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



/**
 * @brief Performs one comparison between stack top and current token.
 * Adds marker '<' when shifting, and reduces until marker on reduction.
 */
void precedence_table_compare(Stack *stack, token_ptr current_token) {
    // To get the top of the stack
    token_ptr top_token = stack_top(stack);
    if (!top_token)
        error_exit(2); // If Stack is empty then its syntax ERR
    // To get the position needed for Precedence relations table
    precedence_index top_index = token_to_index(top_token);
    precedence_index curr_index = token_to_index(current_token);

    precedence_relation rel = precedence_table[top_index][curr_index];
    // Precedence's work with Stack
    switch (rel) {

        case precedence_shift:
            // Insert a marker before pushing the new token
            {
                token_ptr marker = malloc(sizeof(token_t));
                if (!marker) error_exit(99);
                marker->type = MARKER; // Special internal token type
                marker->value.other_value = '<';
                stack_push(stack, marker);
                stack_push(stack, current_token);
            }
            break;

        case precedence_equal_reduce:
            // Equal precedence (for parentheses): remove top token
            stack_pop(stack);
            break;

        case precedence_reduce:
            // Reduce: pop tokens until marker '<' is found
            while (!stack_is_empty(stack) && stack_top(stack)->type != MARKER) {
                stack_pop(stack);
            }
            // Pop the marker itself
            if (!stack_is_empty(stack))
                stack_pop(stack);
            break;

        case precedence_error:
        default:
            // Syntax error according to precedence table
            error_exit(2);
    }
}




bool parse_expression() {

    // Initialize the stack
    Stack stack;
    stack_init(&stack);

    // Push special symbol ($)
    token_ptr special_char = malloc(sizeof(token_t));
    if (!special_char) 
        error_exit(99);
    special_char->type = END_OF_FILE;
    special_char->value.other_value = '$';
    stack_push(&stack, special_char);

    // Get first token
    token_ptr current_token = get_token();

    // Parse until '{' (temporary end of expression)
    while (current_token->type != LEFT_DOM_PAR) {

        // Skip EOL tokens so they won't affect precedence
        if (current_token->type == EOL) {
            current_token = get_token();
            continue;
        }

        // Perform precedence based analysis
        precedence_table_compare(&stack, current_token);

        
        current_token = get_token();
    }

    // Free memory and stack
    stack_free(&stack);
    return true;
}





// moj parser by nevedel spravit priklad ako A>=B lebo nemame tokey >= samostatne iba ako dva tokedy > a = 
// nevie rozoznat unarny -
// nevie kedy konci expression 
// MARKER ako token mi poradil chat 






// pri EOL line musis peakovat aby si vedel co mas a ci mas pokracovat 
// do errorov nedavat cisla ale nazvy z enumu 
// push a pop porobit 


// vyraz je syntakticky spravny ked nam na stacku ostane vstupny vyraz $ a jedno cislo / id / expression
// koniec expressionu budem riesit tak ze si to rozdelim na situacie ked je to if/while(expresion) a ked je to A = expression ze samo by mi mohol poslat posledny token pred zavolanim expressiony aby som vedel ktora z tych 2 situacii to je lebo keby to je if(exp) tak viem ze sa exp konci ked prite patricne )
