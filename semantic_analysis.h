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

#include "symtable.h"
#include "scope_stack.h"
#include "ast.h"
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
    unsigned args_count;
    enum data_types return_types[2]; // all built in functions have maximum of 2 return types
    enum data_types arg_types[3];    // order of the data types inside the array is the same as the order of args inside the function head

} builtin_function_t;

#define builtin_functions_arr_lenght 10

// macro that determines if an expression has any relational operators
#define IS_REL_OP(op) \
    ((op) == OP_EQ  || \
     (op) == OP_NEQ || \
     (op) == OP_LT  || \
     (op) == OP_LTE || \
     (op) == OP_GT  || \
     (op) == OP_GTE || \
     (op) == OP_IS)

// macro that determines if an expression has >, <, >=, <= operators
#define IS_COMP_OP(op) \
    ((op) == OP_LT  || \
     (op) == OP_LTE || \
     (op) == OP_GT  || \
     (op) == OP_GTE)

// this macro is used when type checking string iteration
#define STR_ITER_INVALID(op1, op2) \
    (((op1) == NODE_INT_LIT && \
    (op2) == NODE_STR_LIT) || \
    ((op1) == NODE_STR_LIT && \
    (op2) == NODE_FLOAT_LIT))

// macro to determine if an ident is a GV
#define IS_GLOB_VAR(name) \
    ((strlen(name)) >= 2 && \
    (name[0]) == '_' && \
    (name[1]) == '_')

// flags signaling that things significant to type prediction and type matching are present inside the expression
// operand flags
extern bool has_string_lit;
extern bool has_null_lit;
extern bool has_num_lit;
// operator flags
extern bool has_arit_op;
extern bool has_minus_or_slash;
extern bool has_only_plus_op; // expression contains only + operators
extern bool has_rel_op;
extern bool has_comp_op; // contains >, <, >=, <=
// illegal operation flags
extern bool zero_divison_detected;

// when equal to zero we can determine we are not in a loop, otherwise we are
// this is used when checking that break and continue keywords are used only inside loops
extern unsigned loop_nesting_tracker;

/**
 * @brief Resets all semantic flags to their default values.
 */
void reset_flags();

/**
 * @brief Called by parser when variable declaration is detected. Verifies if the the passed variable was not already declared.
 *        If the variable already exists inside the current scope, error_exit() is called.
 *
 * @param key Pointer to the key of the symbol.
 * @param symtable Pointer to the root node of a symtable that needs to be searched.
 *
 * @return True if function redec detected, false otherwise.
 */
bool verify_var_redec(Key *key, ST_Node *symtable);

/**
 * @brief Called by the parser when use of a variable is detected. Verifies if an undeclared variable was not
 *        used. If an undeclared variable was used, error_exit() is called.
 *
 * @param key Pointer to the key of the symbol.
 * @param scope_stack Pointer to the scope stack to look for the symbol inside higher level scopes.
 */
bool verify_var_existence(Key *key, Scope_Stack *scope_stack);

/**
 * @brief Called by the parser when function definition is detected. Verifies if a function, getter or a setter
 *        does not already exist inside the function symtable. If it does, error_exit() is called.
 *
 * @param key Pointer to the key of the glob variable.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 */
void verify_func_redef(Key *key, ST_Node *func_symtable);

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
 * @brief Verifies existance of the function, checks it's args count, validates args of the function call
 *
 * @param root Root of the whole AST so it can be freed if needed.
 * @param call_node Node of the function call.
 * @param func_symtable Pointer to the symtable of functions.
 * @param scope_stack Pointer to the scope stack.
 */
void handle_function_call(ASTNode_ptr call_node, ST_Node *func_symtable, Scope_Stack *scope_stack);

/**
 * @brief Verifies whether the args count inside the function call matches the function
 *        definition inside func_symtable.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param key Pointer to a key containing function info.
 *
 * @return True if args count is correct, return false otherwise.
 */
bool args_count_check(ST_Node *func_node, int args_count);

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
 *        it's data type is verified against the defined arg types of built-in fuctions. If a function call or an ident is found
 *        it's existence is checked.
 *
 * @param name Name of the built-in function.
 * @param args_count Num of args inside the function call.
 */
bool builtin_args_type_check(ASTNode_ptr call_node, char *name, unsigned args_count,
                        Scope_Stack *scope_stack, ST_Node *func_symtable);

/**
 * @brief While traversing the expression subtree, differnt expression flags are set. These flags are later used
 *        to determine if type mismatch occurs inside an expression.
 *        Function also handles identification of getters inside an expression or verifying that an ident exists.
 *
 * @param exp_root Root of the expression subtree.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param glob_var_symtable Pointer to the symtable of all glob variables.
 * @param scope_stack Pointer to the scope stack.
 */
void exp_analysis(ASTNode_ptr exp_root, ST_Node *func_symtable, ST_Node *glob_var_symtable, Scope_Stack *scope_stack);

/**
 * @brief Traverses the tree and calls semantic functions based on the current node type.
 *
 * @param root Pointer to the root node of AST, needed so we can free the AST at anytime during the recursion
 * @param node_to_handle Helper pointer that will be used in recursive calls.
 * @param func_symtable Pointer to function symtable.
 * @param glob_var_symtable Pointer to a global variable symtable.
 * @param scope_stack Pointer to the scope_stack.
 */
void semantic_analysis(ASTNode_ptr node_to_handle, ST_Node *func_symtable, ST_Node *glob_var_symtable, Scope_Stack *scope_stack);

#endif