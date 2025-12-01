/**
 * @file code_gen_funs.h
 * @author xracekm00, xmezeim00, xcillik00
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

// Enum representing different options for unique name creation
typedef enum {
    FUN_LABEL, LOOP_START_L, LOOP_END_L, IF_TRUE_L, IF_FALSE_L, CALL
}name_option_t;

// Enum representing different options of literals to create
typedef enum {
    INTEGER, FLOAT, BOOL, STRING, NILL
}literal_option_t;

// Data type holding all different kinds of information nececssary for code-gen
typedef struct {
    /**                           IMPORTANT
     * @note to prevent unnecesary memory allocation and freeing:
     * 
     * The function that will perfrom the AST traversing (probably main) has to allocate
     * memory for global_name_gen as well its string attributes.
     * This functions also need to clean this memory after it's finished executing.
     */

    // Name mangeling
    unsigned long long loop_counter;
    unsigned long long if_counter;
    unsigned long long temp_var_counter;

    // Prevents reading from an empty stack
    unsigned stack_depth;
    
    // Stores a unique names
    char* fun_label;        // Function def
    char* if_true_label;    // When condition is true
    char* if_false_label;   // When condition is false
    char* loop_start_label; // Loop start
    char* loop_end_label;   // Loop end

    // Location in AST
    char *curr_function;
    unsigned curr_param_count;

    // Variables necessary to generate a call
    char *called_function;
    bool in_function;
    bool in_getter;
    bool in_setter;
    bool in_loop;
} name_generator_t, *name_generator_ptr;

//Global instance of Data type holding all different kinds of information nececssary for code-gen
extern name_generator_ptr global_name_gen;
//Global flag, holds information whether the function contained return node
extern bool return_occured;

//Macro, used for string convertion into valid format, to make code more readable
#define is_invalid_char(ch) \
    (((ch) >= 0 && (ch) <= 32) || (ch) == 35 || (ch) == 92)

/************************************ FUNCTION PROTOTYPES ************************************/

/**
 * @brief The main code generating function - contains switch for all different types of nodes.
 *        Traverses the AST via inorder and expression subtrees via postorder.
 * 
 * @param node 
 */
void codegen(ASTNode_ptr node);

/**
 * @brief sets all attributes of global instance of name_gen_t to default values
 * 
 * @note used for reset when entering new function_def node
 * 
 * @param node 
 */
void name_gen_init(ASTNode_ptr node);

/**
 * @brief Generates unieque label name using name-mangeling
 * 
 * @note These options can be used LABEL, LOOP_START_L, LOOP_END_L, IF_TRUE, IF_FALSE
 * 
 * @param option
 */
void gen_label(name_option_t option);

/**
 * @brief Pushes variable on data stack, used by expression processing functions
 * 
 * @param node 
 * 
 */
void gen_push_variable(ASTNode_ptr node);

/**
 * @brief Generates user defined variable declaration
 * 
 * @param node 
 */
void gen_var_decl(ASTNode_ptr node);

/**
 * @brief Assigns value to the variable stored in left node child
 * 
 * @param node 
 */
void gen_assign(ASTNode_ptr node);

/**
 * @brief Handles start of function
 * 
 * @note Called after entering NODE_FUNCTION_DEF node
 * 
 * @param node
 */
void gen_func_start(ASTNode_ptr node);

/**
 * @brief Handles end of function
 * 
 * @note Called reccursion reaches NODE_FUNCTION_DEF node while returning
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
 * @param value
 */
void gen_lit_null();

/**
 * @brief Generates code for float literal
 * 
 * @param value
 */
void gen_lit_bool(bool value);

/**
 * @brief Generates code for float literal
 * 
 * @param value The float value to push on stack
 */
void gen_lit_string(char *value);


void gen_if();

/**
 * @brief Handles start of a while loop
 * 
 * @note called from NODE_WHILE
 * 
 * @param node 
 */
void gen_while_start();

/**
 * @brief Handles end of a while loop
 * 
 * @note called when recursion returns back to NODE_WHILE
 * 
 * @param node 
 */
void gen_while_end();

/**
 * @brief Handles start of a for loop
 * 
 * @note called from NODE_FOR
 * 
 * @param node 
 */
void gen_for_start(ASTNode_ptr node);

/**
 * @brief Handles end of a for loop
 * 
 * @note called when recrusion returns back to the node
 * 
 * @param node 
 */
void gen_for_end(ASTNode_ptr node);

/**
 * @brief Jumps on corresponding built in function
 * 
 * @param node 
 */
void gen_jmp_builtin(ASTNode_ptr node);

/**
 * @brief Creates a unique label name
 * 
 * @param node 
 * @param option 
 */
void create_unique_name(ASTNode_ptr node, name_option_t option);

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
 * @brief Terminates correspondig while loop
 */
void gen_break();

/**
 * @brief Skips one iteration in correspondig while loop
 */
void gen_continue();

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
