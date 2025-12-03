/**
 * @file stack.c
 * @author Kristian Cilling (xcillik00)
 * @brief Linked list implementation of stact for the precedence_analysis
 * @version 0.1
 * @date 2025-11-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include "error.h"

// Function definitions

/**
 * @brief Initializes an empty stack structure.
 *
 * Sets head, top, top_terminal to NULL and size to zero.
 *
 * @param s Pointer to the stack to initialize.
 */
void stack_init(Stack *s)
{
    s->head = NULL;         // beginning of the list and end of the stack
    s->top = NULL;          // top of the stack and end of the list
    s->top_terminal = NULL; // terminal on the top of the stack and end of the list
    s->stack_size = 0;
}

/**
 * @brief Initializes an empty stack structure.
 *
 * Sets head, top, top_terminal to NULL and size to zero.
 *
 * @param s Pointer to the stack to initialize.
 */
bool stack_is_empty(Stack *s)
{
    return (s->head == NULL && s->stack_size == 0);
}

/**
 * @brief Pushes a token onto the top of the stack.
 *
 * Allocates a new StackItem, attaches it at the end of the linked list
 * and updates the top pointer.
 *
 * @param s Pointer to the stack.
 * @param token Token to be stored inside the new stack item.
 */
void stack_push(Stack *s, token_ptr token)
{
    // Create a new item
    StackItem *new_item = malloc(sizeof(StackItem));
    if (!new_item)
        error_exit(ERR_INTERNAL);
    // Set our new item
    new_item->token = token;
    new_item->next = NULL;

    // if its the first item
    if (s->head == NULL)
    {
        s->head = new_item;
        s->top = new_item;
    }
    else
    {
        // Pushes to the end of the list land sets the top on the last item in list
        s->top->next = new_item;
        s->top = new_item;
    }
    s->stack_size++;
}

/**
 * @brief Pops the top item from the stack and frees its token.
 *
 * Removes the last node in the linked list. If the token exists,
 * free_token() is called on it. Adjusts top pointer accordingly.
 *
 * @param s Pointer to the stack.
 */
void stack_pop(Stack *s)
{

    if (stack_is_empty(s))
        return;

    StackItem *tmp = s->head;
    StackItem *prev = NULL;
    // sets the tmp on the last item of the list and prev to the one before the last
    while (tmp->next != NULL)
    {
        prev = tmp;
        tmp = tmp->next;
    }
    // If there are more than 1 item
    if (prev)
    {
        prev->next = NULL;
        s->top = prev;
    }
    // If its the last item
    else
    {
        s->head = NULL;
        s->top = NULL;
    }

    if (tmp->token)
        free_token(tmp->token);
    free(tmp);
    s->stack_size--;
}

/**
 * @brief Returns the token stored at the top of the stack.
 *
 * @param s Pointer to the stack.
 * @return token_ptr Pointer to the top token, or NULL if the stack is empty.
 */
token_ptr stack_top(Stack *s)
{
    if (stack_is_empty(s))
        return NULL;
    return s->top->token;
}

/**
 * @brief Frees all items in the stack.
 *
 * Repeatedly pops elements until empty. If internal size is inconsistent
 * after cleanup, an internal error is raised.
 *
 * @param s Pointer to the stack.
 */
void stack_free(Stack *s)
{
    while (!stack_is_empty(s))
        stack_pop(s);
    if (s->stack_size != 0)
        error_exit(ERR_INTERNAL);
}

/**
 * @brief Sets the pointer to the highest (rightmost) terminal in the stack.
 *
 * Used by precedence analysis to mark where the '<' precedence marker
 * should be inserted.
 *
 * @param s Pointer to the stack.
 * @param item Pointer to the chosen StackItem that represents topmost terminal.
 */
void stack_set_top_terminal_pointer(Stack *s, StackItem *item)
{
    s->top_terminal = item;
}

/**
 * @brief Pushes a token immediately after the top terminal element.
 *
 * Used in precedence analysis for inserting the '<' marker.
 * If no terminal is present, behaves like stack_push().
 *
 * @param s Pointer to the stack.
 * @param token Token to insert after the last terminal.
 */
void stack_push_after(Stack *s, token_ptr token)
{
    // if there is no terminal in the stack then it pushes it on the top of the stack
    if (!s->top_terminal)
    {
        stack_push(s, token);
        return;
    }

    StackItem *new_item = malloc(sizeof(StackItem));
    if (!new_item)
        error_exit(ERR_INTERNAL);

    new_item->token = token;
    new_item->next = s->top_terminal->next;
    s->top_terminal->next = new_item;

    if (s->top_terminal == s->top)
        s->top = new_item; // if we gonna push after top then update the top

    s->stack_size++;
}

/**
 * @brief Pops the top stack item without freeing its token.
 *
 * Used when tokens must remain referenced externally (e.g., in PSA arrays).
 *
 * @param s Pointer to the stack.
 */
void stack_pop_no_free(Stack *s)
{
    if (stack_is_empty(s))
        return;

    StackItem *tmp = s->head;
    StackItem *prev = NULL;

    // Find last item
    while (tmp->next != NULL)
    {
        prev = tmp;
        tmp = tmp->next;
    }

    // Update pointers
    if (prev)
    {
        prev->next = NULL;
        s->top = prev;
    }
    else
    {
        s->head = NULL;
        s->top = NULL;
    }

    free(tmp); // Only free the StackItem structure
    s->stack_size--;
}
