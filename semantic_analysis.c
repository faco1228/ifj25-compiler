/**
 * @file semantic_analysis.c
 * @authors xmezeim00, xracekm00
 * @brief Impelements function used during the semantic analysis.
 * @version 0.1
 * @date 2025-11-14
 *
 * @copyright Copyright (c) 2025
 */
#include <stdlib.h>
#include <stdbool.h>
#include "semantic_analysis.h"
#include "include/scope_stack.h"
#include "include/symtable.h"
#include "error.h"
#include "ast.h"

//**SEM. ONLY FUNCTION PROTOTYPES**//

//**HELPER PROTOTYPES**//

//**FUNCTION DEFINITIONS**//

/**
 * @brief Allocates space for 20 keys and inits unresolved symbols array attributes.
 *
 * @return Pointer to the allocated struct or NULL ptr if allocation fails.
 */
Unresolved_Symbols_Array *unresolved_array_init()
{
    Unresolved_Symbols_Array *new_arr = malloc(sizeof(Unresolved_Symbols_Array));

    if (!new_arr) // struct allocation failed
        return NULL;

    new_arr->array_size = 20; // default size of the array
    new_arr->first_free_idx = 0;

    new_arr->array = malloc(sizeof(Key) * new_arr->array_size);

    if (!new_arr->array) // array allocation failed
    {
        free(new_arr);
        return NULL;
    }

    return new_arr;
}

/**
 * @brief Adds a new symbol to unresolved symbola array so their existance can be verified later.
 *
 * @param unresolved Pointer to an array of unresolved symbols.
 * @param key Key of the unresolved symbol.
 *
 * @return Pointer to the unresolved symbols struct in case reallocation was needed.
 */
Unresolved_Symbols_Array *add_unresolved_symbol(Unresolved_Symbols_Array *unresolved, Key key)
{
    if (!unresolved) // mainly for debugging purposes
        return NULL;

    if (unresolved->first_free_idx == unresolved->array_size) // array is full and needs to be reallocated
        unresolved = realloc(unresolved, unresolved->array_size * 2 * sizeof(Key));

    if (!unresolved) // realloc successes check
        return NULL;

    unresolved->array[unresolved->first_free_idx] = key;

    return unresolved;
}

/**
 * @brief Called by parser when variable declaration is detected. Verifies if the the passed variable was not already declared.
 *        If the variable already exists inside the current scope, error_exit() is called.
 *
 * @param key Pointer to the key of the symbol.
 * @param symtable Pointer to the root node of a symtable that needs to be searched.
 *
 */
void verify_redec(Key *key, ST_Node *symtable)
{
    ST_Node *search_result = search(symtable, key);

    if (search_result) // variable found inside the current scope
        error_exit(ERR_SEM_REDEFINITION);
}

/**
 * @brief Called by the parser when use of a variable is detected. Verifies if an undeclared variable was not
 *        used. If an undeclared variable was used, error_exit() is called.
 *
 * @param key Pointer to the key of the symbol.
 * @param scope_stack Pointer to the scope stack to look for the symbol inside higher level scopes.
 * @param glob_var_symtable Pointer to the symtable of all global variables.
 */
void verify_var_existence(Key *key, Scope_Stack *scope_stack, ST_Node *glob_var_symtable)
{
    bool is_glob_var = false;

    if (strlen(key->name) >= 2 && key->name[0] == '_' && key->name[1] == '_')
        is_glob_var = true;

    ST_Node *search_result;

    if (is_glob_var)
        search_result = search(glob_var_symtable, key);
    else
        search_result = scope_stack_lookup(scope_stack, key);

    if (!search_result)
        error_exit(ERR_SEM_UNDEFINED);
}

/**
 * @brief Called by the parser when function definition is detected. Verifies if a function, getter or a setter
 *        does not already exist inside the function symtable. If it does, error_exit() is called.
 *
 * @param key Pointer to the key of the glob variable.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 */
void verify_func_redef(Key *key, ST_Node *func_symtable)
{
    ST_Node *search_result = search(func_symtable, key);

    if (search_result) // function found inside the function symtable
        error_exit(ERR_SEM_REDEFINITION);
}

/**
 * @brief Called by the parser when function call is detected. Verifies if the function, getter or a setter exists.
 *        If it does not, error_exit() is called.
 *
 * @param key Pointer to the key of the glob variable.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 */
void verify_func_existance(Key *key, ST_Node *func_symtable)
{
    ST_Node *search_result = search(func_symtable, key);

    if (!search_result) // function not found inside the function symtable
        return;         // todo : pridat logiku pre pridanie do zoznamu nevyriesenych symbolov
}

/**
 * @brief Checks if main function with no args exists inside the programs body.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 *
 * @return True if main exists, false otherwise.
 */
bool main_exists(ST_Node *func_symtable)
{
    if (!func_symtable) // mainly for debugging
        error_exit(ERR_INTERNAL);

    // key init to search for main with no args
    Key key;
    key.name = "main";
    key.args_count = 0;
    key.id_type = FUNCTION;

    ST_Node *search_result = search(func_symtable, &key);

    return search_result != NULL;
}

/**
 * @brief Checks if the divider is equal to zero. Works only if the divider is a num literal, otherwise we cannot
 *        detect zero division. If zero division is detected error_exit() is called.
 *
 * @param divider Pointer towards the devider node inside AST
 */
void zero_division(ASTNode_ptr divider)
{
    if (divider->type == NODE_FLOAT_LIT && divider->data.literal.float_val == 0.0)
        error_exit(ERR_SEM_OTHER);

    if (divider->type == NODE_INT_LIT && divider->data.literal.int_val == 0)
        error_exit(ERR_SEM_OTHER);
}

/**
 * @brief Checks if break keyword was used inside a loop. If not error_exit() is called.
 *
 * @param in_loop Signals that we are currently in a loop.
 */
void break_usage_check(bool in_loop)
{
    if (!in_loop)
        error_exit(ERR_SEM_OTHER);
}

/**
 * @brief Checks if continue keyword was used inside a loop. If not error_exit() is called.
 *
 * @param in_loop Signals that we are currently in a loop.
 */
void continue_usage_check(bool in_loop)
{
    if (!in_loop)
        error_exit(ERR_SEM_OTHER);
}

/**
 * @brief Checks if a bool expression is not assigned to a variable.
 *
 * @param in_assignment Signals that we are currently inside assignment.
 */
void bool_value_assignment_check(bool in_assignment)
{
    if (!in_assignment)
        error_exit(ERR_SEM_OTHER);
}

/**
 * @brief Verifies whether the args count inside the function call matches the function
 *        definition inside func_symtable.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param key Pointer to a key containing function info.
 */
void args_count_check(ST_Node *func_symtable, Key *key)
{
    // looks through function symtable to look for a function with a matching key
    ST_Node *search_result = search(func_symtable, key);

    // at this point of compilation we know that no undefined functions can exist
    // so we can determine that if no function was found, it is only because of a wrong arg count
    if (!search_result)
        error_exit(ERR_SEM_ARG_COUNT);
}