/**
 * @file scope_stack.h
 * @author Martin Mezei (xmezeim00)
 *
 * @brief Implements Stack and its helper functions to manage working with variable
 * scopes according to the memory model of the IFJcode25 programming language.
 *
 * @version 0.1
 * @date 2025-11-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _SCOPE_STACK_H_
#define _SCOPE_STACK_H_

#include "symtable.h"
#include <stdbool.h>

// Scope_Stack stores pointers to different symtable instances
typedef struct Scope_Stack
{
    ST_Node **symtable_array;

    // index of the top of the stack
    // stackTop is set to -1 if the Stack is empty
    int stack_top_index;
    unsigned stack_array_size;
} Scope_Stack;

/**
 * @brief Initializes a new Scope_Stack
 *
 * @param scope_stack Pointer to uninitialized Scope_Stack.
 */
void scope_stack_init(Scope_Stack *scope_stack);

/**
 * @brief Handles Scope_Stack clean up.
 *
 * @param scope_stack Pointer to Scope_Stack.
 */
void scope_stack_dispose(Scope_Stack *scope_stack);

/**
 * @brief Adds Symtable pointer on stack top.
 *
 * @param root_ptr Pointer to the root of a new symtable.
 * @param scope_stack Pointer to Scope_Stack.
 */
void scope_stack_push(Scope_Stack *scope_stack, ST_Node *root_ptr);

/**
 * @brief Increases the size of symtable_array to fit one more element.
 * 
 * @param scope_stack Pointer to Scope_Stack.
 */
void scope_stack_increase_size(Scope_Stack *scope_stack);

/**
 * @brief Removes Symtable pointer from stack top.
 * 
 * @param scope_stack Pointer to Scope_Stack.
 */
void scope_stack_pop(Scope_Stack *scope_stack);

/**
 * @brief Checks if Scope_Stack is empty.
 * 
 * @param scope_stack Pointer to Scope_Stack.
 */
bool scope_stack_empty(Scope_Stack *scope_stack);

/**
 * @brief Checks if Scope_Stack if full.
 *
 * @param scope_stack Pointer to Scope_Stack.
 */
bool scope_stack_full(Scope_Stack *scope_stack);

/**
 * @brief Looks through all the symtables that are currently on stack and tries to find a specific symbol.
 *
 * @param scope_stack Pointer to a scope_stack.
 * @param key Key of a symbol we look for.
 * @param block_id Pointer to a helper variable inside the semantic analysis module used for name mangling.
 *
 * @return True if symbol was found, false otherwise.
 */
ST_Node *scope_stack_var_lookup(Scope_Stack *scope_stack, Key *key, unsigned *block_id);

/**
 * @brief Returns an adress of a Symtable that is currently on top of the stack.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
ST_Node **scope_stack_top(Scope_Stack *scope_stack);

#endif
