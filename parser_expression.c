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
#define MARKER 12345  // must not collide with real token types
#endif
// Urobil som si nonterminal E aby som mohol davat na stack
#ifndef NONTERMINAL_E 
#define NONTERMINAL_E 123456789
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
                case PLUS_V:              return OP_ADD;            // +
                case MINUS_V:             return OP_SUB;            // -
                case ASTERISK_V:          return OP_MUL;            // *
                case DIVISON_V:           return OP_DIV;            // /
                case LESS_THAN_V:         return OP_LOWER;          // <
                case GREATER_THAN_V:      return OP_GREATER;        // >
                case LESS_OR_EQ_THAN_V:   return OP_LOWER_EQUAL;    // <=
                case GREATER_OR_EQ_THAN_V:return OP_GREATER_EQUAL;  // >=
                case EQUAL_SIGN_V:        return OP_EQUAL;          // ==
                case EXC_MARK_V:          return OP_NOT_EQUAL;      // !=
                default:                  return OP_OPERAND;        // fallback
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






/**
 * @brief Performs reduction when precedence_table gives '>'
 * Pops tokens until '<' marker and replaces recognized handle with NONTERMINAL_E.
 */
void precedence_reduce_func(Stack *stack) {
    if (stack_is_empty(stack))
        error_exit(ERR_SYNTACTIC);

    // Buffer for saving last 5 terminals/nonterminals
    token_ptr items[5]; 
    int count = 0; // Count that will hepl us to know which rule to use

    // Pops the items in stak until MARKER
    while (!stack_is_empty(stack)) {
        token_ptr top = stack_top(stack);
        if (top->type == MARKER) {
            stack_pop(stack); // To remove MARKER
            break;
        }
        // If we have to pop more than 5 items there is a problem
        if (count >= 5)
            error_exit(ERR_SYNTACTIC);
        items[count++] = top;
        stack_pop(stack);
    }


    // now we have the handle reversed, i.e. right to left
    // we will reverse the order for easy comparison
    token_ptr first_stack_item   = (count >= 1) ? items[count - 1] : NULL;
    token_ptr second_stack_item  = (count >= 2) ? items[count - 2] : NULL;
    token_ptr third_stack_item   = (count >= 3) ? items[count - 3] : NULL;

    bool matched = false;

    
    // E -> i
    if (count == 1) {
        token_ptr t = first_stack_item;
        if (t->type != OPERATOR &&
            t->type != MARKER &&
            t->type != NONTERMINAL_E) {
            matched = true;
        }
    }
    // E -> (E)
    else if (count == 3 &&
             first_stack_item->type == LEFT_PAR &&
             second_stack_item->type == NONTERMINAL_E &&
             third_stack_item->type == RIGHT_PAR) {
        matched = true;
    }
    // Binary operators: E -> E op E
    else if (count == 3 &&
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
    // If there is no rule for it then it's Syntax error
    if (!matched)
        error_exit(ERR_SYNTACTIC);

    // Insert the NONTERMINAL_E 
    token_ptr newE = malloc(sizeof(token_t));
    if (!newE) error_exit(ERR_INTERNAL);
    newE->type = NONTERMINAL_E;

    stack_push(stack, newE);
}











/**
 * @brief Performs one comparison between stack top and current token.
 * Adds marker '<' when shifting, and reduces until marker on reduction.
 */
void precedence_table_compare(Stack *stack, token_ptr current_token, token_ptr *top_terminal) {
    
    // To get the top of the stack
    token_ptr top_token = stack_top(stack);
    if (top_token == NULL)
        error_exit(ERR_SYNTACTIC); // If Stack is empty then its syntax ERR




 

    // The first token should be a terminal if not then its syntax err 
    if(top_token->type != NONTERMINAL_E && top_token->type != MARKER){
        *top_terminal = top_token;
        // Sets the pointer on the top terminal in case of < to know after which terminal to push it 
        stack_set_top_terminal_pointer(stack , stack->top);
    }
    // malo by to zapezpecit to aby som mal vzdy najvrchnejsi terminal
    // This shouldnt happen because there always should be '$' on the beggining of the stack
    if(top_terminal == NULL){
        error_exit(ERR_SYNTACTIC);
    }
   

  
    // Special case: comparing $ with $ means its the end of the analysis
    if (top_token->type == END_OF_FILE && current_token->type == END_OF_FILE) {
        return;
    }




    // To get the position needed for Precedence relations table
    precedence_index top_index = token_to_index(top_terminal);
    precedence_index curr_index = token_to_index(current_token);



    precedence_relation rel = precedence_table[top_index][curr_index];


    // Depending on what operator we get from the precedence table we will proceed
    switch (rel) {

        case precedence_shift:
            // Insert a marker after top terminal before pushing the new token
            {
                token_ptr marker = malloc(sizeof(token_t));
                if (!marker) error_exit(ERR_INTERNAL);
                marker->type = MARKER; // Special internal token type
                marker->value.other_value = '<';
                
                stack_push_after(stack, marker);
                stack_push(stack, current_token);
            }
            break;

        case precedence_equal_reduce:
            // Equal precedence then we gonna push the Current token
                stack_push(stack, current_token);
            break;


        case precedence_reduce: {

             precedence_reduce_func(stack);

        break; 
        }




            
        case precedence_error:
        default:
            // Syntax error according to precedence table
            error_exit(2);
    }
}


















// Puropose of the recognition token is to know if im in assignement or condition
bool parse_expression(token_ptr recognition_token) {

    // Initialize the stack
    Stack stack;
    stack_init(&stack);

    // Push special symbol ($) on the stack 
    token_ptr special_char = malloc(sizeof(token_t));
    if (!special_char) 
        error_exit(ERR_INTERNAL);
    special_char->type = END_OF_FILE;
    special_char->value.other_value = '$';
    stack_push(&stack, special_char);

    // Get first token
    token_ptr current_token = get_token();

    // Create a token_ptr for the top terminal because in the precedence table
    // we have to compare the current token with the top terminal on the Stack
    token_ptr top_terminal= NULL; // This pointer is used in function Precedence_table_compare


    // To know when to end I have to know if im in a assignment or condition 
    // THIS I HAVE TO FINISH (WHEN THERE IS END OF EXPRESSION)
    switch (recognition_token->type)
    {   // 
        case OPERATOR : 
            switch (recognition_token->value.other_value)
            {
            // Means that Im in assignment
            case EQUAL_SIGN_V:
                
                break;

            }


        break;
        // Means that we are in assignment
        case LEFT_PAR  :
        
        


        break;
   


        // Parse until '{' (temporary end of expression)
        while (current_token->type != LEFT_DOM_PAR) {

            // Skip EOL tokens so they won't affect precedence
            if (current_token->type == EOL) {
              current_token = get_token();
              continue;
         }

            // Perform precedence based analysis
            precedence_table_compare(&stack, current_token, &top_terminal);

        
            current_token = get_token();
        }
 }
    















    //After we hit the end of Expression so we start comparing top_terminal with $ as a current token
    // Create end marker token ($)

    token_ptr end_token = malloc(sizeof(token_t));
    if (!end_token)
        error_exit(ERR_INTERNAL);
    end_token->type = END_OF_FILE;
    end_token->value.other_value = '$';

    token_ptr top_terminal_final = NULL;

    while (true) {
        // aktualizuj top_terminal_final na najvyšší terminál
        StackItem *tmp = stack.head;
        while (tmp) {
            if (tmp->token->type != NONTERMINAL_E && tmp->token->type != MARKER)
                top_terminal_final = tmp->token;
            tmp = tmp->next;
        }

        // Ak sa porovnávajú dve $, ukonči cyklus
        if (top_terminal_final->type == END_OF_FILE &&
            end_token->type == END_OF_FILE) {
            break;
        }

        // vykonaj porovnanie ($ je "current token")
        precedence_table_compare(&stack, end_token, &top_terminal_final);
    }

    // Tests after the final cycle with $ if on the stack is $E 
    if (stack.stack_size == 2 &&
        stack.head &&
        stack.head->token->type == END_OF_FILE &&
        stack.top &&
        stack.top->token->type == NONTERMINAL_E) {
        // If on the stack is $E then free the end_token and the whole stack
        free(end_token);
        stack_free(&stack);
        return true;// Means the syntax is correct
        
    } else {
        // Means something is wrong with the syntax
        free(end_token);
        error_exit(ERR_SYNTACTIC);
    }

}






// nevie rozoznat unarny -
// nevie kedy konci expression 

// pri EOL line musis peakovat aby si vedel co mas a ci mas pokracovat 
// do errorov nedavat cisla ale nazvy z enumu 
// realne asi len reduction a error case musim porobit (asi najlepsie ako samostatne funkcie )
// prerobit  lexikalne automaty reskeptive doplnit != , <= ... 
// vyraz je syntakticky spravny ked nam na stacku ostane vstupny vyraz $ a jedno cislo / id / expression
// koniec expressionu budem riesit tak ze si to rozdelim na situacie ked je to if/while(expresion) a ked je to A = expression ze samo by mi mohol poslat posledny token pred zavolanim expressiony aby som vedel ktora z tych 2 situacii to je lebo keby to je if(exp) tak viem ze sa exp konci ked prite patricne )
// pri precedencnej tabulke sa pozerame na vrchny terminal , cize keby je na vrcholu zasobniku neterminal na ten sa nepozerame. asi som uz spravil ? 


/*
Znak nového řádku je (kromě případů, kde je povinný) možné použít za  tečkami, čárkami, operátory a 
. Sekvence několik znaků nového řádku se
považuje za jeden znak nového řádku.
*/
// . Statický getter je možné použít na místě termu. 



/*
V základním zadání je vždy přítomná i část else. Za ukončovací kulatou závorkou, za koncem
bloku1 ani za klíčovým slovem else nesmí být znak nového řádku.*/