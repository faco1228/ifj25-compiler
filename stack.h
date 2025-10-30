
/**
 * @file stack.h
 * @author xcillik00
 * @brief  ADT stack for precedence analysis implemented as Linked List
 */


#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include "scanner.h"   

// Data  structure for Stack

typedef struct StackItem {
    token_ptr token;          
    struct StackItem *next;    
} StackItem;

// Pointer on the top of the stack
typedef struct {
    StackItem *top;  
    int stack_size;          
} Stack;




//************************************** Function prototypes **************************************//
void stack_init(Stack *s);


bool stack_is_empty(Stack *s);


void stack_push(Stack *s, token_ptr token);


void stack_pop(Stack *s);


token_ptr stack_top(Stack *s);


void stack_free(Stack *s);



#endif 