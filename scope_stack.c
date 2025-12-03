/**
 * @file scope_stack.c
 * @author Martin Mezei (xmezeim00)
 * @brief Implements Stack and its helper functions to manage working with variable
 * scopes according to the memory model of the IFJcode25 programming language.
 *
 * @version 0.1
 * @date 2025-11-20
 * 
 * @copyright Copyright (c) 2025
 *
 */
#include "scope_stack.h"
#include "symtable.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SCOPE_STACK_SIZE 10

/**
 * @brief Initializes a new Scope_Stack
 *
 * @param scope_stack Pointer to uninitialized Scope_Stack.
 */
void scope_stack_init(Scope_Stack *scope_stack)
{
    scope_stack->symtable_array = malloc(sizeof(ST_Node *) * DEFAULT_SCOPE_STACK_SIZE);

    if (!scope_stack->symtable_array)
        error_exit(ERR_INTERNAL);

    scope_stack->stack_top_index = -1; // stackTop value for an empty Stack
    scope_stack->stack_array_size = DEFAULT_SCOPE_STACK_SIZE;
}

/**
 * @brief Handles Scope_Stack clean up.
 *
 * @param scope_stack Pointer to Scope_Stack.
 */
void scope_stack_dispose(Scope_Stack *scope_stack)
{
    if (!scope_stack)
        return;

    // free all symtables
    for (int idx = 0; idx <= scope_stack->stack_top_index; idx++)
    {
        if (scope_stack->symtable_array[idx])
        {
            st_dispose_tree(scope_stack->symtable_array[idx]);
        }
    }

    // deallocation of symtable_array and prevention of dangling pointers
    free(scope_stack->symtable_array);
    scope_stack->symtable_array = NULL;
}

/**
 * @brief Adds Symtable pointer on stack top.
 *
 * @param root_ptr Pointer to the root of a new symtable.
 * @param scope_stack Pointer to Scope_Stack.
 */
void scope_stack_push(Scope_Stack *scope_stack, ST_Node *root_ptr)
{
    // when trying to push to a full Stack, its size is increased before pushing
    if (scope_stack_full(scope_stack))
        scope_stack_increase_size(scope_stack); // if scope stack increase fails, scope_stack_increase_size() exits

    // new Symtable pointer can be added to the Stack
    scope_stack->stack_top_index++;
    scope_stack->symtable_array[scope_stack->stack_top_index] = root_ptr;
}

/**
 * @brief Increases the size of symtable_array to fit one more element.
 * @param scope_stack Pointer to scope_stack.
 */
void scope_stack_increase_size(Scope_Stack *scope_stack)
{
    scope_stack->stack_array_size++;

    scope_stack->symtable_array = realloc(scope_stack->symtable_array, (sizeof(ST_Node *) * scope_stack->stack_array_size));

    if (!scope_stack->symtable_array)
        error_exit(ERR_INTERNAL);
}

/**
 * @brief Removes Symtable pointer from stack top. Calls Disposte_Tree before popping
 *
 * @param scope_stack Pointer to the scope_stack.
 */
void scope_stack_pop(Scope_Stack *scope_stack)
{
    if (scope_stack_empty(scope_stack)) // cannot pop from an empty stack, nothing happens
        return;

    st_dispose_tree(*scope_stack_top(scope_stack)); // tree is freed before popping

    scope_stack->stack_top_index--;
}

/**
 * @brief Checks if Scope_Stack is empty.
 * @param scope_stack Pointer to Scope_Stack.
 */
bool scope_stack_empty(Scope_Stack *scope_stack)
{
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

    return scope_stack->stack_top_index == -1;
}

/**
 * @brief Checks if Scope_Stack if full.
 * @param scope_stack Pointer to Scope_Stack.
 */
bool scope_stack_full(Scope_Stack *scope_stack)
{
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

    return scope_stack->stack_top_index == (int)scope_stack->stack_array_size - 1;
}

/**
 * @brief Looks through all the symtables that are currently on stack and tries to find a specific symbol.
 * 
 * @param scope_stack Pointer to a scope_stack.
 * @param key Key of a symbol we look for.
 * @param block_id Pointer to a helper variable inside the semantic analysis module used for name mangling.
 *
 * @return True if symbol was found, false otherwise.
 */
ST_Node *scope_stack_var_lookup(Scope_Stack *scope_stack, Key *key, unsigned *block_id)
{
    if (scope_stack_empty(scope_stack))
        return NULL;

    ST_Node *symbol;

    for (int idx = scope_stack->stack_top_index; idx >= 0; idx--) // check the whole scope stack for var
    {
        symbol = st_search(scope_stack->symtable_array[idx], key);

        if (symbol)
        {
            *block_id = symbol->block_id;
            return symbol;
        }
    }

    return NULL; // local var not found
}

/**
 * @brief Returns an adress of a Symtable that is currently on top of the stack.
 * @param scope_stack Pointer to Scope_Stack.
 *
 * @return Pointer to an existing Symtable root node or NULL if stack is empty.
 */
ST_Node **scope_stack_top(Scope_Stack *scope_stack)
{
    // cannot return any Symtables from an empty stack
    if (scope_stack_empty(scope_stack))
    {
        return NULL;
    }

    return &scope_stack->symtable_array[scope_stack->stack_top_index];
}
