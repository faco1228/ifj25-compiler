#include "scope_stack.h"
#include "symtable.h"
#include "../error.h"
#include <stdlib.h>

#define DEFAULT_SCOPE_STACK_SIZE 10
#define STACK_FUNCTION_SUCCESSES 0

/**
 * @brief Initializes a new Scope_Stack
 *
 * @param scope_stack Pointer to uninitialized Scope_Stack.
 */
void Scope_Stack_Init(Scope_Stack *scope_stack)
{
    // Null pointer to Scope_Stack passed
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

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
void Scope_Stack_Dispose(Scope_Stack *scope_stack)
{
    if (!scope_stack)
        return;

    // free all symtables
    for (int idx = 0; idx <= scope_stack->stack_top_index; idx++)
    {
        if (scope_stack->symtable_array[idx])
        {
            Dispose_Tree(scope_stack->symtable_array[idx]);
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
void Scope_Stack_Push(Scope_Stack *scope_stack, ST_Node *root_ptr)
{
    // when trying to push to a full Stack, its size is increased before pushing
    if (Scope_Stack_Full(scope_stack))
        Scope_Stack_Increase_Size(scope_stack); // if scope stack increase fails, Scope_Stack_Increase_Size() exits

    // new Symtable pointer can be added to the Stack
    scope_stack->stack_top_index++;
    scope_stack->symtable_array[scope_stack->stack_top_index] = root_ptr;
}

/**
 * @brief Increases the size of symtable_array to fit one more element.
 * @param scope_stack Pointer to Scope_Stack.
 */
void Scope_Stack_Increase_Size(Scope_Stack *scope_stack)
{
    scope_stack->stack_array_size++;

    scope_stack->symtable_array = realloc(scope_stack->symtable_array, (sizeof(ST_Node *) * scope_stack->stack_array_size));

    if (!scope_stack->symtable_array)
        error_exit(ERR_INTERNAL);
}

/**
 * @brief Removes Symtable pointer from stack top.
 * 
 * @param Scope_Stack Pointer to Scope_Stack.
 */
void Scope_Stack_Pop(Scope_Stack *scope_stack)
{
    if (Scope_Stack_Empty(scope_stack)) // cannot pop from an empty stack, nothing happens
        return;

    scope_stack->stack_top_index--;
}

/**
 * @brief Checks if Scope_Stack is empty.
 * @param scope_stack Pointer to Scope_Stack.
 */
bool Scope_Stack_Empty(Scope_Stack *scope_stack)
{
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

    return scope_stack->stack_top_index == -1;
}

/**
 * @brief Checks if Scope_Stack if full.
 * @param scope_stack Pointer to Scope_Stack.
 */
bool Scope_Stack_Full(Scope_Stack *scope_stack)
{
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

    return scope_stack->stack_top_index == (int) scope_stack->stack_array_size - 1;
}

/**
 * @brief Looks through all the symtables that are currently on stack and tries to find a specific symbol.
 * @param scope_stack Pointer to a scope_stack.
 * @param key Key of a symbol we look for.
 * 
 * @return Pointer on a symbol 
 */
ST_Node *Scope_Stack_Lookup(Scope_Stack *scope_stack, Key *key)
{
    for (int idx = scope_stack->stack_top_index; idx >= 0; idx--) // loop through the stack all the way to global frame
    {
        ST_Node *symbol = Search(scope_stack->symtable_array[idx], key); 

        if (symbol)
            return symbol;
    }

    return NULL; // symbol not found
    
}

/**
 * @brief Returns an adress of a Symtable that is currently on top of the stack.
 * @param scope_stack Pointer to Scope_Stack.
 *
 * @return Pointer to an existing Symtable root node or NULL if stack is empty.
 */
ST_Node *Scope_Stack_Top(Scope_Stack *scope_stack)
{
    // cannot return any Symtables from an empty stack
    if (Scope_Stack_Empty(scope_stack))
        return NULL;

    return scope_stack->symtable_array[scope_stack->stack_top_index];
}
