/**
 * @file semantic_analysis.h
 * @author Martin Mezei (xmezeim00)
 * @brief Contains function prototypes of the semantic analysis module.
 * @version 0.2
 * @date 2025-11-14
 *
 * @copyright Copyright (c) 2025
 */

#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "symtable.h"
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
#define IS_REL_OP(op)  \
    ((op) == OP_EQ ||  \
     (op) == OP_NEQ || \
     (op) == OP_LT ||  \
     (op) == OP_LTE || \
     (op) == OP_GT ||  \
     (op) == OP_GTE || \
     (op) == OP_IS)

// macro that determines if an expression has >, <, >=, <= operators
#define IS_COMP_OP(op) \
    ((op) == OP_LT ||  \
     (op) == OP_LTE || \
     (op) == OP_GT ||  \
     (op) == OP_GTE)

// this macro is used when type checking string iteration
#define STR_ITER_INVALID(op1, op2)                         \
    (((op1) == NODE_INT_LIT && (op2) == NODE_STR_LIT) ||   \
     ((op1) == NODE_STR_LIT && (op2) == NODE_FLOAT_LIT) || \
     (op2) == NODE_STR_LIT)

// this macro determines if a string iteration is present
#define IS_STR_ITER(op1, op2) \
    ((op1) == NODE_STR_LIT && (op2) == NODE_INT_LIT)

// macro to determine if an ident is a GV
#define IS_GLOB_VAR(name) \
    ((name) != NULL &&    \
     (name)[0] == '_' &&  \
     (name)[1] == '_' &&  \
     (name)[2] != '\0')

// this macro checks if right operand of an expression that uses the is operator is valid
#define IS_VALID_RIGHT_OP(op) \
    ((op))

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
 * @brief Searches the current scope to verify if the variable was not redeclared.
 *
 * @param key Pointer to the key of the symbol.
 * @param symtable Pointer to the current cope
 *
 * @return True if function redec detected, false otherwise.
 */
bool verify_var_redec(Key *key, ST_Node *symtable);

/**
 * @brief Searches the current and all higher level scope to verify that a variable exists.
 *
 * @param key Pointer to the key of the symbol.
 * @param scope_stack Pointer to the scope stack.
 */
bool verify_var_existence(Key *key);

/**
 * @brief Checks if a user-defined function was not redefined somewhere else.
 *
 * @param key Pointer to the key of the glob variable.
 */
void verify_func_redef(Key *key);

/**
 * @brief Checks if main function with no args exists inside the class body.
 *
 * @return True if main exists, false otherwise.
 */
bool main_exists();

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
 */
void handle_function_call(ASTNode_ptr call_node);

/**
 * @brief Verifies that a built-in function exists and that it was called with the correct num of arguments.
 *
 * @param name Name of the built in function.
 *
 * @return Pointer to the found built-in function or NULL if no function was found.
 */
builtin_function_t *builtin_exists(char *name);

/**
 * @brief Loops through all the params inside the function call of a built-in and if a literal is found,
 *        it's data type is verified against the defined arg types of built-in fuctions. If a function call or an ident is found
 *        it's existence is checked.
 *
 * @param name Name of the built-in function.
 * @param builtin_ptr Pointer to a built-in structure.
 */
bool builtin_args_type_check(ASTNode_ptr call_node, builtin_function_t *builtin_ptr);

/**
 * @brief If a function call or a variable is found inside args, it's existence is checked.
 *
 * @param call_node Node of the function call.
 */
void args_exist(ASTNode_ptr call_node);

/**
 * @brief Checks values of relevant combinations of expression flags and determines if type mismatch occured.
 *        If certain flag combinations are detected, a prediction of the expression data type can be made and used for type checking later.
 *
 * @note Works for simple expressions only. Other errors are going to be detected in code gen.
 *
 * @param exp_root Root of the expression subtree.
 */
void exp_analysis(ASTNode_ptr exp_root);

/**
 * @brief Checks values of relevant combinations of expression flags and determines if type mismatch occured.
 *        If certain flag combinations are detected, a restriction code can be assigned to different expression nodes.
 *
 * @param exp_root Root of the expression subtree.
 */
bool eval_exp_flags(ASTNode_ptr exp_root);

/**
 * @brief Traverses the tree and calls semantic functions based on the current node type.
 *
 * @param node_to_handle Helper pointer that will be used in recursive calls.
 */
void semantic_analysis(ASTNode_ptr node_to_handle);

#endif