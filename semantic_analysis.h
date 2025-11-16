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

// enum of number codes for all possible data types
enum data_types
{
    NUM_TYPE,
    STR_TYPE,
    NULL_TYPE,
    ANY_TYPE,
    UNDEFINED,
};

// stores info about a specific built-in fuction
typedef struct Builtin_Function
{
    char *name;
    int args_count;
    enum data_types return_types[2]; // all built in functions have maximum of 2 return types
    enum data_types arg_types[3];    // order of the data types inside the array is the same as the order of args inside the function head

} builtin_function_t;

#define builtin_functions_arr_lenght 10

// an array containing all built in functions
extern builtin_function_t builtin_functions[builtin_functions_arr_lenght] =
    {
        {"read_str", 0, {STR_TYPE, NULL_TYPE}, {}},
        {"read_num", 0, {NUM_TYPE, NULL_TYPE}, {}},
        {"write", 1, {NULL_TYPE, UNDEFINED}, {ANY_TYPE}},
        {"floor", 1, {NUM_TYPE, UNDEFINED}, {NUM_TYPE}},
        {"str", 1, {STR_TYPE, UNDEFINED}, {ANY_TYPE}},
        {"length", 1, {NUM_TYPE, UNDEFINED}, {STR_TYPE}},
        {"substring", 3, {STR_TYPE, NULL_TYPE}, {STR_TYPE, NUM_TYPE, NUM_TYPE}},
        {"strcmp", 2, {NUM_TYPE, UNDEFINED}, {STR_TYPE, STR_TYPE}},
        {"ord", 2, {NUM_TYPE, UNDEFINED}, {STR_TYPE, NUM_TYPE}},
        {"chr", 1, {STR_TYPE, UNDEFINED}, {NUM_TYPE}}};

// flags signaling that things significant to type prediction are present in an expression
extern bool has_string_lit;
extern bool has_minus_or_slash; // expression contains
extern bool has_null_lit;
extern bool has_unary_minus;
extern bool has_operator;
extern bool has_only_plus_op; // expression contains only + operators
extern bool has_rel_op;
extern bool has_comp_op; // contains >, <, >=, <=
extern bool zero_divison_detected;

// when equal to zero we can determine we are not in a loop, otherwise we are
// this is used when checking that break and continue keywords are used only inside loops
extern unsigned loop_nesting_tracker;

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

//! po tieto funkcie su tie, ktore samo zavola este v parser

/**
 * @brief Checks if the divider is equal to zero. Works only if the divider is a num literal, otherwise we cannot
 *        detect zero division. If zero division is detected error_exit() is called.
 *
 * @param divider Pointer towards the devider node inside AST
 *
 * @return True if zero division detected, false otherwise.
 */
bool zero_division(ASTNode_ptr divider);

/**
 * @brief Verifies whether the args count inside the function call matches the function
 *        definition inside func_symtable.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param key Pointer to a key containing function info.
 *
 * @return True if args count is correct, return false otherwise.
 */
bool args_count_check(ST_Node *func_symtable, Key *key);

/**
 * @brief Verifies that a built-in function exists and that it was called with the correct num of arguments.
 *
 * @param name Name of the built in function.
 *
 * @return True if a built-in with this name exists, false otherwise.
 */
bool builtin_exists(char *name);

/**
 * @brief Verifies that a built-in function was called with the correct num of arguments.
 *
 * @param name Name of the built-in function.
 * @param args_count Number of passed arguments inside the function call of a built-in function.
 *
 * @return True if args count is correct, false otherwise.
 */
bool builtin_args_count_correct(char *name, unsigned args_count);

/**
 * @brief Loops through all the params inside the function call of a built-in and if a literal is found,
 *        it's data type is verified against the defined arg types of built-in fuctions.
 *
 * @param name Name of the built-in function.
 * @param args_count Num of args inside the function call.
 */
bool builtin_args_types_correct(ASTNode_ptr call_node, char *name, unsigned args_count);

/**
 * @brief While traversing the expression subtree, differnt expression flags are set. These flags are later used
 *        to determine if type mismatch occurs inside an expression.
 *        Function also handles identification of getters inside an expression.
 *
 * @param exp_root Root of the expression subtree.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 */
void exp_analysis(ASTNode_ptr exp_root, ST_Node *func_symtable);

#endif