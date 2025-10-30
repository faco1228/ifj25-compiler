#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

 

void stack_init(Stack*s){
    s->top = NULL; 
    s->stack_size = 0; 
}

bool stack_is_empty(Stack *s){
    return (s->top == NULL && s->stack_size == 0);

}

void stack_push(Stack*s , token_ptr token ){
    // Allocation to new node/token
    StackItem *new_item = malloc(sizeof(StackItem));
    if (new_item == NULL) {
        
        warnings(99);
        error_exit(99);
    }
    // Set this node/token to be the first
    new_item->token = token;
    new_item->next = s->top;
    s->top = new_item;
    s->stack_size++ ; 

}




void stack_pop(Stack *s) {
    if (stack_is_empty(s)) {
        return;
    }
    StackItem *tmp = s->top;
    s->top = s->top->next;

    // Free the aloccated memory 
    if (tmp->token != NULL) {
        free_token(tmp->token);
    }

    free_token(tmp);
    s->stack_size-- ;
    
}





token_ptr stack_top(Stack *s) {
    if (stack_is_empty(s)) {
        return NULL;
    }
    // returns the top token
    return s->top->token;
}


void stack_free(Stack *s) {
    while (!stack_is_empty(s)) {
        stack_pop(s);
    }
    if(s->stack_size != 0)
        error_exit(99);
}




