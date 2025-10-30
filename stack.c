#include <stdio.h>
#include <stdlib.h>
#include "stack.h"



void stack_init(Stack*s){
    s->top = NULL; 

}

bool stack_is_empty(Stack *s){
    return (s->top == NULL);

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

}




void stack_pop(Stack *s) {
    if (stack_is_empty(s)) {
        return;
    }
    StackItem *tmp = s->top;
    s->top = s->top->next;

    // Free the aloccated memory 
    if (tmp->token != NULL) {
        free(tmp->token);
    }

    free(tmp);
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
}




