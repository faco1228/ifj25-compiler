/**
 * @file scope_stack.h
 * @author xmezeim00
 *
 * @brief Implements Stack and its helper functions to manage working with variable
 * scopes according to the memory model of the IFJcode25 programming language.
 *
 */

#ifndef SCOPE_STACK_H
#define SCOPE_STACK_H

#include <symtable.h>
#include <stdbool.h>

// Scope_Stack stores pointers to different symtable instances
typedef struct Scope_Stack
{
    Symtable **symtable_array;

    // index of the top of the stack
    // stackTop is set to -1 if the Stack is empty
    unsigned stack_top_index;
    unsigned stack_size;
} Scope_Stack;

/**
 * @brief Initializes a new Scope_Stack
 *
 * @param Scope_Stack Pointer to uninitialized Scope_Stack.
 */
int Scope_Stack_Init(Scope_Stack *Scope_Stack);

/**
 * @brief Handles Scope_Stack clean up.
 *
 * @param Scope_Stack Pointer to Scope_Stack.
 */
void Scope_Stack_Dispose(Scope_Stack *Scope_Stack);

/**
 * @brief Adds Symtable pointer on stack top.
 *
 * @param Symtable Pointer to a new instance of Symtable.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
int Scope_Stack_Push(Scope_Stack *Scope_Stack, Symtable *Symtable);

/**
 * @brief Increases the size of symtable_array to fit one more element.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
void Scope_Stack_Increase_Size(Scope_Stack *Scope_Stack);

/**
 * @brief Removes Symtable pointer from stack top.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
int Scope_Stack_Pop(Scope_Stack *Scope_Stack);

/**
 * @brief Checks if Scope_Stack is empty.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
bool Scope_Stack_Empty(Scope_Stack *Scope_Stack);

/**
 * @brief Checks if Scope_Stack if full.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
bool Scope_Stack_Full(Scope_Stack *Scope_Stack);

/**
 * @brief Returns an adress of a Symtable that is currently on top of the stack.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
Symtable *Scope_Stack_Top(Scope_Stack *Scope_Stack);

#endif
