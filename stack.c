#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include "error.h"

void stack_init(Stack *s) {
    s->head = NULL;   // beginning of the  list and end of the stack
    s->top = NULL;    // top of the stack and end of the list
    s->top_terminal = NULL; // terminal on the top of the stack and end of the list
    s->stack_size = 0;
}

bool stack_is_empty(Stack *s) {
    return (s->head == NULL && s->stack_size == 0);
}

void stack_push(Stack *s, token_ptr token) {
    StackItem *new_item = malloc(sizeof(StackItem));
    if (!new_item) error_exit(99);

    new_item->token = token;
    new_item->next = NULL;
    // if its the first item
    if (s->head == NULL) {
        s->head = new_item;
        s->top = new_item;
    } else {
        // pushes to the end of the list
        s->top->next = new_item;
        s->top = new_item;
    }
    s->stack_size++;
}


void stack_pop(Stack *s) {

    if (stack_is_empty(s)) return;

    StackItem *tmp = s->head;
    StackItem *prev = NULL;
    // sets the tmp on the last item of the list and prev to the one before the last
    while (tmp->next != NULL) {
        prev = tmp;
        tmp = tmp->next;
    }

    if (prev) {
        prev->next = NULL;
        s->top = prev; 

    } else {
        s->head = NULL;
        s->top = NULL;
    }

    if (tmp->token) free_token(tmp->token);
    free(tmp);
    s->stack_size--;
}


// returns the top token of the stack
token_ptr stack_top(Stack *s) {
    if (stack_is_empty(s)) return NULL;
    return s->top->token; 
}

void stack_free(Stack *s) {
    while (!stack_is_empty(s)) stack_pop(s);
    if (s->stack_size != 0) error_exit(99);
}

// sets the pointer on the highest terminal in the stack/ latest terminal in the list 
void stack_set_top_terminal_pointer(Stack *s, StackItem *item) {
    s->top_terminal = item;
}

// pushes marker after the last terminal 
void stack_push_after(Stack *s, token_ptr token) {
    // if there is no terminal in the stack then it pushes it on the top of the stack
    if (!s->top_terminal) {
        stack_push(s, token);
        return;
    }

    StackItem *new_item = malloc(sizeof(StackItem));
    if (!new_item) error_exit(ERR_INTERNAL);

    new_item->token = token;
    new_item->next = s->top_terminal->next;
    s->top_terminal->next = new_item;

    if (s->top_terminal == s->top) s->top = new_item; // if we gonna push after top then update the top

    s->stack_size++;
}









