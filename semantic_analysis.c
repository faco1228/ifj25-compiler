/**
 * @file semantic_analysis.c
 * @authors xmezeim00, xracekm00
 * @brief Impelements function used during the semantic analysis.
 * @version 0.1
 * @date 2025-11-14
 *
 * @copyright Copyright (c) 2025
 */

#include "semantic_analysis.h"
#include "include/scope_stack.h"
#include "include/symtable.h"
#include "error.h"

//**SEM. ONLY FUNCTION PROTOTYPES**//

//**HELPER PROTOTYPES**//

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
        return; // todo : pridat logiku pre pridanie do zoznamu nevyriesenych symbolov
}
