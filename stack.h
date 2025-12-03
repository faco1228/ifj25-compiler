/**
 * @file stack.h
 * @author Kristian Cilling (xcillik00)
 * @brief  ADT stack for precedence analysis implemented as Linked List
 */

#ifndef _STACK_H_
#define _STACK_H_

#include <stdbool.h>
#include "scanner.h"

// Data  structure for Stack

typedef struct StackItem
{
    token_ptr token;
    struct StackItem *next;
} StackItem;

// Pointer on the top of the stack
typedef struct
{
    StackItem *top;
    StackItem *head;
    StackItem *top_terminal; // Pointer to the top terminal in Stack
    int stack_size;
} Stack;

//************************************** Function prototypes **************************************//

/**
 * @brief Initializes an empty stack structure.
 *
 * Sets head, top, top_terminal to NULL and size to zero.
 *
 * @param s Pointer to the stack to initialize.
 */
void stack_init(Stack *s);

/**
 * @brief Checks whether the stack is empty.
 *
 * @param s Pointer to the stack.
 * @return true If the stack contains no elements.
 * @return false Otherwise.
 */
bool stack_is_empty(Stack *s);

/**
 * @brief Pushes a token onto the top of the stack.
 *
 * Allocates a new StackItem, attaches it at the end of the linked list
 * and updates the top pointer.
 *
 * @param s Pointer to the stack.
 * @param token Token to be stored inside the new stack item.
 */
void stack_push(Stack *s, token_ptr token);

/**
 * @brief Pops the top item from the stack and frees its token.
 *
 * Removes the last node in the linked list. If the token exists,
 * free_token() is called on it. Adjusts top pointer accordingly.
 *
 * @param s Pointer to the stack.
 */
void stack_pop(Stack *s);

/**
 * @brief Returns the token stored at the top of the stack.
 *
 * @param s Pointer to the stack.
 * @return token_ptr Pointer to the top token, or NULL if the stack is empty.
 */
token_ptr stack_top(Stack *s);

/**
 * @brief Frees all items in the stack.
 *
 * Repeatedly pops elements until empty. If internal size is inconsistent
 * after cleanup, an internal error is raised.
 *
 * @param s Pointer to the stack.
 */
void stack_free(Stack *s);

/**
 * @brief Sets the pointer to the highest (rightmost) terminal in the stack.
 *
 * Used by precedence analysis to mark where the '<' precedence marker
 * should be inserted.
 *
 * @param s Pointer to the stack.
 * @param item Pointer to the chosen StackItem that represents topmost terminal.
 */
void stack_set_top_terminal_pointer(Stack *s, StackItem *item);

/**
 * @brief Pushes a token immediately after the top terminal element.
 *
 * Used in precedence analysis for inserting the '<' marker.
 * If no terminal is present, behaves like stack_push().
 *
 * @param s Pointer to the stack.
 * @param token Token to insert after the last terminal.
 */
void stack_push_after(Stack *s, token_ptr token);

/**
 * @brief Pops the top stack item without freeing its token.
 *
 * Used when tokens must remain referenced externally (e.g., in PSA arrays).
 *
 * @param s Pointer to the stack.
 */
void stack_pop_no_free(Stack *s);

#endif
