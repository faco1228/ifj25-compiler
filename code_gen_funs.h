/**
 * @file code_gen_funs.h
 * @author xracekm00, xmezeim00
 * @brief Header file for code generating functions
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _CODE_GEN_
#define _CODE_GEN_

#include <stdbool.h>
#include "ast.h"

#define MAX_LABEL_NAME 512
#define MAX_FUNCTION_NAME 512
#define MAX_STRING_LEN 1024

// Enum representing different options for unique name creation via create_unique_name function
typedef enum {
    LABEL, LOOP_START_L, LOOP_END_L, IF_TRUE, IF_FALSE, TEMP_VAR
}name_option_t;

// Enum representing different options of literals to create
typedef enum {
    INTEGER, FLOAT, BOOL, STRING, NILL
}literal_option_t;

// Union representiong different types of value a literal can obtain
typedef union {
    long long int_value;     // To store int
    long double float_value; // To store float
    char *string_value;      // To store string
    bool bool_value;         // To store bool
    // To store nill whatever can be used, since gen_lit_null function doesnt have parameters
}literal_values_t;

// Data type holding all different kinds of information nececssary for code-gen
typedef struct {
    /**                           IMPORTANT
     * @note to prevent unnecesary memory allocation and freeing
     * 
     * The function that will perfrom the AST traversing has to call a malloc
     * at global_name_gen as well its string attributes: global_name_gen.label, ...
     * This functions also need to clear this memory after it's finished.
     */

    // Name mangeling
    unsigned long long label_counter;
    unsigned long long temp_var_counter;
    unsigned long long loop_counter;
    unsigned long long if_counter;

    // Location in AST
    char *curr_function;
    unsigned curr_param_count;

    bool in_function;
    bool in_loop;

    // Will store a unique names
    char* label;            // Function def
    char* temp_var;         // Temp variable
    char* if_label;         // Temp variable
    char* loop_start_label; // Loop start
    char* loop_end_label;   // Loop end

    // How many thing are on data stack, so reading from an empty stack can be prevented
    unsigned stakck_depth;

} name_generator_t, *name_generator_ptr;

//Global varialbe necesary for almost all functions below
extern name_generator_ptr global_name_gen;
//Global flag, holds information whether the function contained return node
extern bool return_occured;

//Macro, used for string convertion into valid format, to make code more readable
#define is_invalid_char(ch) \
    (((ch) >= 0 && (ch) <= 32) || (ch) == 35 || (ch) == 92)

/************************************ FUNCTION PROTOTYPES ************************************/

/**
 * @brief The main code-gen function - contains switch for all different types of nodes
 * 
 * @param node 
 */
void codegen(ASTNode_ptr node);

/*----------------- HELPER FUNCTIONS -----------------*/

/**
 * @brief sets all name_generator_t attributes to default values
 * 
 * @note used for reset when entering new function_def node
 * 
 * @param node 
 */
void name_gen_init(ASTNode_ptr node);

/**
 * @brief Create a unique string which is stored in 
 * 
 * @param node 
 * @param option 
 */
void create_unique_name(ASTNode_ptr node, name_option_t option);

/*----------------- PATTERNS -----------------*/

/**
 * @brief Generates code for string iteration (string * num)
 * 
 * @param node
 * 
 * @note Expects arguments on data stack (Pascal convention)
 *       Returns result-string on stack
 */
void gen_string_iter(ASTNode_ptr node);

/**
 * @brief Generates unieque label name using name-mangeling
 * 
 * @note These options can be used LABEL, LOOP_START_L, LOOP_END_L, IF_TRUE, IF_FALSE
 * 
 * @param option
 */
void gen_label(name_option_t option);

/**
 * @brief Generates unieque variable name using name-mangeling
 * 
 * @note These options can be used TEMP_VAR
 * 
 * @param option
 */
void gen_variable(ASTNode_ptr node);
void gen_var_decl();
void gen_assign();

/**
 * @brief Handles stert of function
 * 
 * @note Called after entering NODE_FUNCTION_DEF node
 * 
 * @param node
 */
void gen_func_start(ASTNode_ptr node);

/**
 * @brief Handles end of function
 * 
 * @note Called when childeren array is empty
 * 
 */
void gen_func_end();

/**
 * @brief Handles return values of a function
 * 
 * @note uses data stack to retrun the result
 * 
 */
void gen_return();

/**
 * @brief Calls functions generating literals based on the option
 * 
 * @param option 
 * @param value 
 */
void gen_literal(literal_option_t option, literal_values_t value);

/**
 * @brief Generates code for integer literal
 * 
 * @param value
 */
void gen_lit_int(long long value);

/**
 * @brief Generates code for float literal
 * 
 * @param value
 */
void gen_lit_float(long double value);

/**
 * @brief Generates code for float literal
 * 
 * @param value The float value to push on stack
 */
void gen_lit_string(char *value);

/**
 * @brief Generates code for float literal
 * 
 * @param value
 */
void gen_lit_bool(bool value);
/**
 * @brief Generates code for float literal
 * 
 * @param value
 */
void gen_lit_null();

/**
 * @brief These will be used to handle if,else statements
 * 
 * @note Implementation:
 *       Use LTS, GTS, EQS instructions
 *       JUMPIFEQ <vysledok>, <1 / 0>
 * 
 */
void gen_jump_if_grater();
void gen_jump_if_lower();

void gen_if();

void gen_while();

void gen_for();

void gen_call();

/**
 * @brief Generates code for built in functions
 * 
 * @note will be called when entered function call node and is_built_in == true
 * 
 * @param node 
 */
void gen_jmp_builtin(ASTNode_ptr node);

/*----------------- BUILTIN FUNCTIONS -----------------*/

void gen_built_in_read_str();

void gen_built_in_read_num();

void gen_built_in_write();

void gen_built_in_floor();

void gen_built_in_str();

void gen_built_in_length();

void gen_built_in_substring();

void gen_built_in_strcmp();

void gen_built_in_ord();

void gen_built_in_chr();

/**
 * @brief MENZI
 *        After entering NODE_EXPRESSION gen_infix_to_postfix and gen_postfix_eval_fun
 *        functions will be called.
 *        Expression will be evaluated using data stack and for each NODE_BINARY_OP
 *        type check will be performed.
 */
void gen_binary_op();

void gen_infix_to_postfix();

void gen_postfix_eval_fun();

/**
 * @brief We will create this one together at the very end.
 * 
 * @note We will use TYPES, ISINTS for implementation.
 *       Has to handle range and is operator as well.
 */
void gen_type_check();

/**
 * @brief Will be handeled in expression evaluation.
 * 
 */
void gen_zero_div_check();

#endif