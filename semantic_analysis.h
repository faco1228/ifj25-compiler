/**
 * @file semantic_analysis.h
 * @authors xmezeim00, xracekm00
 * @brief Contains function prototypes of semantic analysis used by the parser. // todo : upravit podla potreby
 * @version 0.1
 * @date 2025-11-14
 *
 * @copyright Copyright (c) 2025
 */

#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "include/symtable.h"
#include "include/scope_stack.h"
#include <stdbool.h>

// stores num encoding of different Ifj built-in function
// these value will be used to directly index an array containg info about different built in function
enum builtin_type 
{
    READ_STR,
    READ_NUM,
    WRITE,
    FLOOR,
    STR,
    LENGTH,
    SUBTRING,
    STRCMP,
    ORD,
    CHR
};

// used to store keys of symbols that could not be verified during the synt. analysis
typedef struct
{
    Key *array;
    unsigned array_size;     // default size of the array is 20
    unsigned first_free_idx; // used for direct indexing of the array when adding new keys
} Unresolved_Symbols_Array;

/**
 * @brief Allocates space for 20 keys and inits unresolved symbols array attributes.
 *        If allocation fails, function exits with ERR_INTERNAL.
 *
 * @param array_ptr Pointer to the Unresolved_Symbols_Array struct.
 */
Unresolved_Symbols_Array *unresolved_array_init();

/**
 * @brief Adds a new symbol to unresolved symbola array so their existance can be verified later.
 *
 * @param unresolved Pointer to an array of unresolved symbols.
 * @param key Key of the unresolved symbol.
 */
Unresolved_Symbols_Array *add_unresolved_symbol(Unresolved_Symbols_Array *unresolved, Key key);

/**
 * @brief Handles clean up of the unresolved symbols array.
 *
 * @param unresolved Pointer to the struct of unresolved array.
 */
void unresolved_dispose(Unresolved_Symbols_Array *unresolved);

/**
 * @brief Called by parser when declaration is detected. Verifies if the the passed variable was not already declared.
 *        If the variable already exists inside the current scope, error_exit() is called.
 *
 * @param key Pointer to the key of the symbol.
 * @param symtable Pointer to the root node of a symtable that needs to be searched.
 *
 */
void verify_redec(Key *key, ST_Node *symtable);

/**
 * @brief Called by the parser when use of a variable is detected. Verifies if an undeclared variable was not
 *        used. If an undeclared variable was used, error_exit() is called.
 *
 * @param key Pointer to the key of the symbol.
 * @param scope_stack Pointer to the scope stack to look for the symbol inside higher level scopes.
 * @param glob_var_symtable Pointer to the symtable of all global variables.
 */
void verify_var_existence(Key *key, Scope_Stack *scope_stack, ST_Node *glob_var_symtable);

/**
 * @brief Called by the parser when function definition is detected. Verifies if a function, getter or a setter
 *        does not already exist inside the function symtable. If it does, error_exit() is called.
 *
 * @param key Pointer to the key of the glob variable.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 */
void verify_func_redef(Key *key, ST_Node *func_symtable);

/**
 * @brief Called by the parser when function call is detected. Verifies if the function, getter or a setter exists.
 *        If it does not, error_exit() is called.
 *
 * @param key Pointer to the key of the glob variable.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param unresolved Pointer to the unresolved array struct to store symbols which existance could not be resolved, yet.
 */
void verify_func_existance(Key *key, ST_Node *func_symtable, Unresolved_Symbols_Array *unresolved);

/**
 * @brief Checks if main function with no args exists inside the programs body.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 *
 * @return True if main exists, false otherwise.
 */
bool main_exists(ST_Node *func_symtable);

/**
 * @brief Checks if the divider is equal to zero. Works only if the divider is a num literal, otherwise we cannot
 *        detect zero division. If zero division is detected error_exit() is called.
 *
 * @param divider Pointer towards the devider node inside AST
 */
void zero_division(ASTNode_ptr divider);

/**
 * @brief Checks if break keyword was used inside a loop. If not error_exit() is called.
 *
 * @param in_loop Signals that we are currently in a loop.
 */
void break_usage_check(bool in_loop);

/**
 * @brief Checks if continue keyword was used inside a loop. If not error_exit() is called.
 *
 * @param in_loop Signals that we are currently in a loop.
 */
void continue_usage_check(bool in_loop);

/**
 * @brief Checks if a bool expression is not assigned to a variable.
 *
 * @param in_assignment Signals that we are currently inside assignment.
 */
void bool_value_assignment_check(bool in_assignment);

/**
 * @brief Verifies whether the args count inside the function call matches the function
 *        definition inside func_symtable.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param key Pointer to a key.
 */
void args_count_check(ST_Node *func_symtable, Key *key);

#endif