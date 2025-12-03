/**
 * @file code_gen_funs.h
 * @author Martin Racek (xracekm00),
 *         Martin Mezei (xmezeim00)
 * @brief Header file for code generating functions
 * @version 0.1
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef _CODE_GEN_
#define _CODE_GEN_

#include <stdbool.h>
#include <stdlib.h>
#include "ast.h"
#include "scope_stack.h"
#include "symtable.h"

#define MAX_LABEL_NAME 512
#define MAX_FUNCTION_NAME 512
#define MAX_STRING_LEN 1024

// Enum representing different options for unique name creation
typedef enum
{
    FUN_LABEL,
    LOOP_START_L,
    LOOP_END_L,
    ITER_START_L,
    ITER_END_L,
    IF_ELSE_L,
    IF_END_L,
    CALL,
    MUL, // labels for expressions start
    STR_ITER,
    MUL_END,
    ADD,
    CONCAT,
    ADD_END,
    SUB,
    EVAL,
    LOG_END,
    LEFT_TO_FLOAT,
    RIGHT_TO_FLOAT,
    BOTH_TO_FLOAT,
    ZERO_DIV_CHECK_FLOAT,
    ZERO_DIV_CHECK_INT,
    ZERO_DIV_CHECK_DONE
} name_option_t;

// Enum representing different options of literals to create
typedef enum
{
    INTEGER,
    FLOAT,
    BOOL,
    STRING,
    NILL
} literal_option_t;

// Data type holding all different kinds of information nececssary for code-gen
typedef struct
{
    /**                           IMPORTANT
     * @note to prevent unnecesary memory allocation and freeing:
     *
     * Module that will call code gen - compiler_main - has to allocate
     * memory for global_name_gen as well its string attributes.
     * Also need to clean this memory after it's finished executing.
     */

    // Name mangeling
    unsigned long long loop_counter;
    unsigned long long if_counter;
    unsigned long long temp_var_counter;
    unsigned long long bin_op_counter;

    // Prevents reading from an empty stack
    unsigned stack_depth;

    // Stores a unique names
    char *fun_label;        // Function def
    char *else_block_label; // When condition is false
    char *end_if_label;     // End of if statement
    char *loop_start_label; // Loop start
    char *loop_end_label;   // Loop end
    char *iter_start_label; // Loop start
    char *iter_end_label;   // Loop end

    // unique names for expression labels
    char *mul;
    char *str_iter;
    char *mul_end;
    char *add;
    char *concat;
    char *add_end;
    char *sub;
    char *eval;
    char *log_end;
    char *left_to_float;
    char *right_to_float;
    char *both_to_float;

    // Location in AST
    char *curr_function;
    unsigned curr_param_count;

    // Variables necessary to generate a call
    char *called_function;
    bool in_function;
    bool in_getter;
    bool in_setter;
    bool in_loop;
    bool in_if;
} name_generator_t, *name_generator_ptr;

// Global flag, holds information whether the function contained return node
extern bool return_occured;
// Global instance of Data type holding all different kinds of information nececssary for code-gen
extern name_generator_ptr global_name_gen;

// Macro, used for string convertion into valid format, to make code more readable

#define is_invalid_char(ch) \
    (((ch) <= 32) || (ch) == 35 || (ch) == 92)

// Macro, determines whether the nodes children should be traversed, to make code more readable
#define is_valid_node_type(node) \
    ((node)->type != NODE_IF && (node)->type != NODE_EXPR_STMNT && (node)->type != NODE_RETURN)

// Macro for safe memory allocation of name_generator_ptr instance, called by compiler_main
#define allocate_global_name_gen(global_name_gen, g_scope_stack, ast, g_func_symtable, g_global_symtable) \
    do                                                                                                    \
    {                                                                                                     \
        /* Allocates memory for the whole object instance and checks if the operation succeded*/          \
        (global_name_gen) = malloc(sizeof(name_generator_t));                                             \
        if (global_name_gen == NULL)                                                                      \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->fun_label) = calloc(MAX_LABEL_NAME, sizeof(char));                              \
        if (global_name_gen->fun_label == NULL)                                                           \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->else_block_label) = calloc(MAX_LABEL_NAME, sizeof(char));                       \
        if (global_name_gen->else_block_label == NULL)                                                    \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->end_if_label) = calloc(MAX_LABEL_NAME, sizeof(char));                           \
        if (global_name_gen->end_if_label == NULL)                                                        \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->loop_start_label) = calloc(MAX_LABEL_NAME, sizeof(char));                       \
        if (global_name_gen->loop_start_label == NULL)                                                    \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->loop_end_label) = calloc(MAX_LABEL_NAME, sizeof(char));                         \
        if (global_name_gen->loop_end_label == NULL)                                                      \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->iter_start_label) = calloc(MAX_LABEL_NAME, sizeof(char));                       \
        if (global_name_gen->loop_end_label == NULL)                                                      \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->iter_end_label) = calloc(MAX_LABEL_NAME, sizeof(char));                         \
        if (global_name_gen->loop_end_label == NULL)                                                      \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->curr_function) = calloc(MAX_LABEL_NAME, sizeof(char));                          \
        if (global_name_gen->curr_function == NULL)                                                       \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
                                                                                                          \
        (global_name_gen->called_function) = calloc(MAX_LABEL_NAME, sizeof(char));                        \
        if (global_name_gen->called_function == NULL)                                                     \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->mul) = calloc(MAX_LABEL_NAME, sizeof(char));                                    \
        if (global_name_gen->mul == NULL)                                                                 \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->str_iter) = calloc(MAX_LABEL_NAME, sizeof(char));                               \
        if (global_name_gen->str_iter == NULL)                                                            \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->mul_end) = calloc(MAX_LABEL_NAME, sizeof(char));                                \
        if (global_name_gen->mul_end == NULL)                                                             \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->add) = calloc(MAX_LABEL_NAME, sizeof(char));                                    \
        if (global_name_gen->add == NULL)                                                                 \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->concat) = calloc(MAX_LABEL_NAME, sizeof(char));                                 \
        if (global_name_gen->concat == NULL)                                                              \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->add_end) = calloc(MAX_LABEL_NAME, sizeof(char));                                \
        if (global_name_gen->add_end == NULL)                                                             \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->sub) = calloc(MAX_LABEL_NAME, sizeof(char));                                    \
        if (global_name_gen->sub == NULL)                                                                 \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->eval) = calloc(MAX_LABEL_NAME, sizeof(char));                                   \
        if (global_name_gen->eval == NULL)                                                                \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->log_end) = calloc(MAX_LABEL_NAME, sizeof(char));                                \
        if (global_name_gen->log_end == NULL)                                                             \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->left_to_float) = calloc(MAX_LABEL_NAME, sizeof(char));                          \
        if (global_name_gen->left_to_float == NULL)                                                       \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->right_to_float) = calloc(MAX_LABEL_NAME, sizeof(char));                         \
        if (global_name_gen->right_to_float == NULL)                                                      \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
        (global_name_gen->both_to_float) = calloc(MAX_LABEL_NAME, sizeof(char));                          \
        if (global_name_gen->both_to_float == NULL)                                                       \
        {                                                                                                 \
            glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable);                \
        }                                                                                                 \
    } while (0)

/************************************ FUNCTION PROTOTYPES ************************************/

/**
 * @brief The main code generating function - contains switch for all different types of nodes.
 *        Traverses the AST via inorder exept for expression subtrees.
 *        That one is processed by functions handling expression and is being traversed via postorder.
 *
 * @param node
 */
void codegen(ASTNode_ptr node);

/**
 * @brief Prints declarations of all global variables from symtable at the
 *        beginning of programe
 *
 * @param symtable Symtable that contains all global variables.
 */
void gen_all_glob_vars_dec(ST_Node *symtable);

/**
 * @brief sets all name_generator_t attributes to default values
 *
 * @note used for reset when entering new function_def node
 *
 * @param node
 */
void name_gen_init(ASTNode_ptr node);

/**
 * @brief Pushes variable on data stack
 *
 * @note used by expression processing functions
 *
 * @param node
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
 * @brief This function is called at the end of function or when return node is encountered
 */
void gen_return();

/**
 * @brief Converts string literal into corresponding value and pushes this value on a stack
 *
 * @param value The float value to push on stack
 */
void gen_lit_string(char *value);

/**
 * @brief Generates jump on corresponding function
 *
 * @note works on both built in and user defined functions
 *
 * @param node
 */
void gen_jmp_function(ASTNode_ptr node);

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
 * @brief Handles start of a while loop
 *
 * @note called from NODE_WHILE
 *
 * @param node
 */
void gen_while_start(ASTNode_ptr node);

/**
 * @brief Handles end of a while loop
 *
 * @note called when recursion returns back to NODE_WHILE
 */
void gen_while_end();

/**
 * @brief Terminates correspondig while loop
 */
void gen_break();

/**
 * @brief Skips one iteration in correspondig while loop
 */
void gen_continue();

/**
 * @brief Handles start of if statement
 *
 * @param node
 */
void gen_if(ASTNode_ptr node);

/**
 * @brief Handles beginning of else block of if statement
 */
void gen_else();

/**
 * @brief Generates error labels at the end of the program.
 */
void gen_program_end();

/**
 * @brief Creates a unique label name
 *
 * @param node
 * @param option
 */
void create_unique_name(ASTNode_ptr node, name_option_t option);

/**
 * @brief Memory clean up for global instance of name_generator_ptr object
 *
 * @param global_name_gen
 */
void free_global_name_gen(name_generator_ptr global_name_gen);

/**
 * @brief Traverses the expression AST subtree using the postorder traversal and evaluates each binary operation of the expression.
 *        The postorder traversal simulates the postfix notation. All results of evaluations are pushed to the data stack.
 *
 * @note Pushing the results to the data stack is handled by the eval_bin_op function.
 *
 * @param exp_node Root of the expression subtree.
 */
void eval_exp(ASTNode_ptr exp_node);

/**
 * @brief Generates code that defines helper variables used for expression evaluation.
 */
void gen_exp_helpers();

/**
 * @brief Decides what binary op eval function to call based on the provided operator.
 *
 * @param operator Pointer to the operator node.
 */
void eval_bin_op(ASTNode_ptr operator);

/**
 * @brief Generates instructions to type check and evaluate an operation that uses logical operators.
 *        Based on the provided type of the logical operator, different versions of this function can be generated
 *        that are specific for the current logical operator.
 *
 * @param operator Pointer to the node that holds the binary operator of the expression.
 */
void gen_eval_logical_op(ASTNode_ptr operator);

/**
 * @brief Generates instructions to evaluate an operation that uses the is operator.
 *
 * @param operator Pointer to the node that holds the is operator.
 *
 * @note variables that are used inside this function were defined inside the gen_eval_logical_op
 */
void gen_eval_is(ASTNode_ptr operator);

/**
 * @brief Generates instructions to evaluate an operation that uses ==, != operators.
 *
 * @param op_type Based on this value I will either generate EQ or NEQ instruction at the end of the evaluation.
 *
 * @note variables that are used inside this function were defined inside the gen_eval_logical_op
 */
void gen_eval_equal_not_equal(operator_types *op_type);

/**
 * @brief Generates instructions to evaluate an operation that uses ==, != operators.
 *
 * @param op_type Based on this value I will either generate EQ or NEQ instruction at the end of the evaluation.
 *
 * @note variables that are used inside this function were defined inside the gen_eval_logical_op
 */
void gen_eval_greater_lower(operator_types *op_type);

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the * operator
 */
void gen_eval_star_op();

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the / operator.
 */
void gen_eval_slash_op();

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the - operator.
 */
void gen_eval_minus_op();

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the + operator.
 */
void gen_eval_plus_op();

/**
 * @brief Generates instructions to type check an operation that uses the range operator.
 */
void gen_eval_range_op();

/**
 * @brief Helper function that generates all label names that are needed inside expression evaluation codes.
 */
void create_label_names(ASTNode_ptr exp_node);

#endif
