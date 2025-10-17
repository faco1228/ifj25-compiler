#include "scope_stack.h"
#include "symtable.h"
#include <stdlib.h>

#define DEFAULT_SCOPE_STACK_SIZE 10
#define INTERNAL_COMPILER_ERROR 99
#define STACK_FUNCTION_SUCCESSES 0;

/**
 * @brief Initializes a new Scope_Stack
 *
 * @param Scope_Stack Pointer to uninitialized Scope_Stack.
 * @return Status code.
 */
int Scope_Stack_Init(Scope_Stack *Scope_Stack)
{
    Scope_Stack->symtable_array = malloc(sizeof(Symtable *) * DEFAULT_SCOPE_STACK_SIZE);

    if (!Scope_Stack->symtable_array)
        return INTERNAL_COMPILER_ERROR;

    Scope_Stack->stack_top_index = -1; // stackTop value for an empty Stack
    Scope_Stack->stack_size = DEFAULT_SCOPE_STACK_SIZE;

    return STACK_FUNCTION_SUCCESSES;
}

/**
 * @brief Handles Scope_Stack clean up.
 *
 * @param Scope_Stack Pointer to Scope_Stack.
 */
void Scope_Stack_Dispose(Scope_Stack *Scope_Stack)
{
    // deallocation of symtable_array and prevention of dangling pointers
    free(Scope_Stack->symtable_array);
    Scope_Stack->symtable_array = NULL;
}

/**
 * @brief Adds Symtable pointer on stack top.
 *
 * @param Symtable Pointer to a new instance of Symtable.
 * @param Scope_Stack Pointer to Scope_Stack.
 *
 * @return Status code.
 */
int Scope_Stack_Push(Scope_Stack *Scope_Stack, Symtable *Symtable)
{
    // when trying to push to a full Stack, its size is increased before pushing
    if (Scope_Stack_Full(Scope_Stack))
        Scope_Stack_Increase_Size(Scope_Stack);

    
    if (!Scope_Stack->symtable_array) // size increase of Stack failed
        return INTERNAL_COMPILER_ERROR;

    // new Symtable pointer can be added to the Stack
    Scope_Stack->stack_top_index++;
    Scope_Stack->symtable_array[Scope_Stack->stack_top_index] = Symtable;

    return STACK_FUNCTION_SUCCESSES;
}

/**
 * @brief Increases the size of symtable_array to fit one more element.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
void Scope_Stack_Increase_Size(Scope_Stack *Scope_Stack)
{
    Scope_Stack->stack_size++;

    // NOTE: might be changed and realloc might not be used
    realloc(Scope_Stack->symtable_array, (sizeof(Symtable *) * Scope_Stack->stack_size));
}

/**
 * @brief Removes Symtable pointer from stack top.
 * @param Scope_Stack Pointer to Scope_Stack.
 * 
 * @return Status code.
 */
int Scope_Stack_Pop(Scope_Stack *Scope_Stack)
{
    /**
     * NOTE: Tento internal error je tu hlavne preto, aby bolo potom jednoduchsie debuggovat keby sa nieco pokazi
     * 
     * Ak niekomu rovno crashne program pri praci so stackom a bude to INTERNAL_COMPILER_ERROR code tak budete lahsi 
     * vediet kde tu chybu hladat
     */
    if (Scope_Stack_Empty(Scope_Stack)) // cannot pop from an empty stack
        return INTERNAL_COMPILER_ERROR;

    Scope_Stack->stack_top_index--;

    return STACK_FUNCTION_SUCCESSES;
}

/**
 * @brief Checks if Scope_Stack is empty.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
bool Scope_Stack_Empty(Scope_Stack *Scope_Stack)
{
    return Scope_Stack->stack_top_index == -1;
}

/**
 * @brief Checks if Scope_Stack if full.
 * @param Scope_Stack Pointer to Scope_Stack.
 */
bool Scope_Stack_Full(Scope_Stack *Scope_Stack)
{
    return Scope_Stack->stack_top_index == Scope_Stack->stack_size - 1;
}

/**
 * @brief Returns an adress of a Symtable that is currently on top of the stack.
 * @param Scope_Stack Pointer to Scope_Stack.
 *
 * @return Pointer to an existing Symtable or NULL if stack is empty.
 */
Symtable *Scope_Stack_Top(Scope_Stack *Scope_Stack)
{
    // cannot return any Symtables from an empty stack
    if (Scope_Stack_Empty(Scope_Stack))
        return NULL;

    return Scope_Stack->symtable_array[Scope_Stack->stack_top_index];
}
