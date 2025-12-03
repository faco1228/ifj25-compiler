/**
 * @file semantic_analysis.c
 * @author Martin Mezei (xmezeim00)
 * @brief Impelements function used during the semantic analysis.
 * @version 0.1
 * @date 2025-11-14
 *
 * @copyright Copyright (c) 2025
 */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "semantic_analysis.h"
#include "scope_stack.h"
#include "symtable.h"
#include "error.h"
#include "ast.h"
#include "global_structures.h"

// an array containing all built in functions
// i implemented it as an array because of the small amount of items which makes linear search suffiecient
builtin_function_t builtin_functions[builtin_functions_arr_lenght] =
    {
        {"read_str", 0, {STR_TYPE, NULL_TYPE}, {UNDEFINED}},
        {"read_num", 0, {NUM_TYPE, NULL_TYPE}, {UNDEFINED}},
        {"write", 1, {NULL_TYPE, UNDEFINED}, {ANY_TYPE}},
        {"floor", 1, {NUM_TYPE, UNDEFINED}, {NUM_TYPE}},
        {"str", 1, {STR_TYPE, UNDEFINED}, {ANY_TYPE}},
        {"length", 1, {NUM_TYPE, UNDEFINED}, {STR_TYPE}},
        {"substring", 3, {STR_TYPE, NULL_TYPE}, {STR_TYPE, NUM_TYPE, NUM_TYPE}},
        {"strcmp", 2, {NUM_TYPE, UNDEFINED}, {STR_TYPE, STR_TYPE}},
        {"ord", 2, {NUM_TYPE, UNDEFINED}, {STR_TYPE, NUM_TYPE}},
        {"chr", 1, {STR_TYPE, UNDEFINED}, {NUM_TYPE}}};

// expression flags init
bool has_only_plus_op = true;
bool has_string_lit = false;
bool has_num_lit = false;
bool has_minus_or_slash = false;
bool has_null_lit = false;
bool has_unary_minus = false;
bool has_arit_op = false;
bool has_rel_op = false;
bool zero_divison_detected = false;
bool has_comp_op = false;

// loop detection helper
unsigned loop_nesting_tracker = 0;
// block counter for local variable name mangling
unsigned block_counter = 0;
// currently searched block tracker
unsigned current_block_id = 0;

/**
 * @brief Mangles a variable name using the id provided
 *
 * @param name Stores the name of the variable.
 * @param block_id Id of the block to which the variable belongs to.
 *
 * @return Mangled variable name.
 */
char *mangle_name(const char *name, unsigned block_id)
{

    // snprinft will return the needed length of the string
    int len = snprintf(NULL, 0, "%s%u", name, block_id);

    char *mangled = malloc(len + 1);

    if (!mangled)
        error_exit(ERR_INTERNAL);

    // now we can actually created the new formatted string
    snprintf(mangled, len + 1, "%s%u", name, block_id);

    return mangled;
}

/**
 * @brief Resets all semantic flags to their default values.
 */
void reset_flags()
{
    has_only_plus_op = true;
    has_string_lit = false;
    has_num_lit = false;
    has_minus_or_slash = false;
    has_null_lit = false;
    has_unary_minus = false;
    has_arit_op = false;
    has_rel_op = false;
    zero_divison_detected = false;
    has_comp_op = false;
}

/**
 * @brief Searches the current scope to verify if the variable was not redeclared.
 *
 * @param key Pointer to the key of the symbol.
 * @param symtable Pointer to the current cope
 *
 * @return True if function redec detected, false otherwise.
 */
bool verify_var_redec(Key *key, ST_Node *symtable)
{
    ST_Node *search_result = st_search(symtable, key);

    return search_result != NULL;
}

/**
 * @brief Searches the current and all higher level scope to verify that a variable exists.
 *
 * @param key Pointer to the key of the symbol.
 */
bool verify_var_existence(Key *key)
{
    ST_Node *search_result = scope_stack_var_lookup(g_scope_stack, key, &current_block_id);

    return search_result != NULL;
}

/**
 * @brief Searches the glob variable symtable to verify that the glob variable exists.
 *
 * @param key Pointer to the key of the variable.
 */
bool verify_glob_var_existence(Key *key)
{
    ST_Node *search_result = st_search(g_global_symtable, key);

    return search_result != NULL;
}

//**FUNCTION ANALYSIS FUNCTIONS**//

/**
 * @brief Verifies existance of the function, checks it's arg count, validates args of the function call.
 *
 * @param call_node Node of the function call.
 */
void handle_function_call(ASTNode_ptr call_node)
{
    if (call_node->data.function_call.is_builtin) // built-in function called
    {
        unsigned args_count = call_node->data.function_call.param_count;
        builtin_function_t *builtin_ptr = builtin_exists(call_node->data.function_call.name);

        if (builtin_ptr == NULL) // unknown built-in called
            error_exit(ERR_SEM_UNDEFINED);

        if (builtin_ptr->args_count != args_count) // incorrect args count
            error_exit(ERR_SEM_ARG_COUNT);

        if (!builtin_args_type_check(call_node, builtin_ptr)) // arg data type not correct
        {
            error_exit(ERR_SEM_ARG_COUNT);
        }
    }
    else // user-defined function call
    {
        // this key contains information from the function call not from the actual function definition!!!
        Key *key = st_create_function_key(call_node->data.function_call.name,
                                          call_node->data.function_call.param_count,
                                          FUNCTION);
        ST_Node *search_result = st_search(g_func_symtable, key);

        /* NOTE:
        If the function was called with an incorrect num of args program exits with ERR_SEM_UNDEFINED.
        This is because functions are also identified based on the number of their params, so given that the function was called with an incorrect number of args
        st_search will not be able to find a corresponding function inside the g_func_symtable
        */

        if (!search_result) // function called does not exist
            error_exit(ERR_SEM_UNDEFINED);

        // if an error occurs, error_exit() is called from the inside of the function
        args_exist(call_node);
    }
}

/**
 * @brief Checks if a user-defined function was not redefined somewhere else.
 *
 * @param key Pointer to the key of the glob variable.
 */
void verify_func_redef(Key *key)
{
    ST_Node *search_result = st_search(g_func_symtable, key);

    if (search_result) // function found inside the function symtable
        error_exit(ERR_SEM_REDEFINITION);
}

/**
 * @brief Checks if main function with no args exists inside the class body.
 *
 * @return True if main exists, false otherwise.
 */
bool main_exists()
{
    if (!g_func_symtable) // no function were present inside the class body
        return false;

    // key to search for main with no args
    Key *key = st_create_function_key("main", 0, FUNCTION);

    if (!key)
        error_exit(ERR_INTERNAL);

    ST_Node *search_result = st_search(g_func_symtable, key);
    key_dispose(key);

    return search_result != NULL;
}

/**
 * @brief If a function call or a variable is found inside args, it's existence is checked.
 *
 * @param call_node Node of the function call.
 */
void args_exist(ASTNode_ptr call_node)
{
    unsigned args_count = call_node->data.function_call.param_count;

    for (unsigned idx = 0; idx < args_count; idx++)
        exp_analysis(call_node->children[idx]);
}

/**
 * @brief Verifies that a built-in function exists and that it was called with the correct num of arguments.
 *
 * @param name Name of the built in function.
 *
 * @return Pointer to the found built-in function or NULL if no function was found.
 */
builtin_function_t *builtin_exists(char *name)
{
    for (unsigned idx = 0; idx < builtin_functions_arr_lenght; idx++) // looks for a built-in function with this name
    {
        if (strcmp(name, builtin_functions[idx].name) == 0)
            return &builtin_functions[idx];
    }

    return NULL;
}

/**
 * @brief Loops through all the params inside the function call of a built-in and if a literal is found,
 *        it's data type is verified against the defined arg types of built-in fuctions. If a function call or an ident is found
 *        it's existence is checked.
 *
 * @param name Name of the built-in function.
 * @param builtin_ptr Pointer to a built-in structure.
 */
bool builtin_args_type_check(ASTNode_ptr call_node, builtin_function_t *builtin_ptr)
{
    for (unsigned idx = 0; idx < builtin_ptr->args_count; idx++) // loops through the args of the function call
    {
        ASTNode_ptr arg = call_node->children[idx];

        exp_analysis(arg->children[0]); // all args can be an expression so we call exp_analysis function here

        if (!eval_exp_flags(arg))
        {
            error_exit(ERR_SEM_TYPE_MISMATCH);
        }

        // cannot type predict identifiers
        if (arg->type == NODE_IDENTIFIER || arg->type == NODE_CALL)
        {
            reset_flags();
            continue;
        }

        // there is node need to check arg types or we could not determine the type of the expression passed
        if (builtin_ptr->arg_types[idx] == ANY_TYPE || arg->data.exp_statement.exp_type == TYPE_UNKNOWN)
            continue;

        // if possible check if type mismatch did not occur
        if (arg->data.exp_statement.exp_type != TYPE_STRING && builtin_ptr->arg_types[idx] == STR_TYPE)
            return false;
        else if (arg->data.exp_statement.exp_type != TYPE_NUM && builtin_ptr->arg_types[idx] == NUM_TYPE)
            return false;

        reset_flags();
    }

    return true;
}

//**EXPRESSION ANALYSIS FUNCTIONS**//

/**
 * @brief While traversing the expression subtree, differnt expression flags are set. These flags are later used
 *        to determine if type mismatch occurs inside an expression.
 *        Function also handles identification of getters inside an expression or verifying that an variables or functions exist.
 *
 * @param exp_root Root of the expression subtree.
 */
void exp_analysis(ASTNode_ptr exp_root)
{
    if (!exp_root)
        return;

    switch (exp_root->type) // sets different flags to true based on the current node
    {
    case NODE_IDENTIFIER:
    {
        // when ident is found inside an expression we verify whether it is a getter which will later help during code gen
        Key *key = st_create_function_key(exp_root->data.identifier.name, 0, GETTER);
        ST_Node *search_result = st_search(g_func_symtable, key);

        if (search_result) // if a getter is found, ident is assigned the GETTER id_type
        {
            exp_root->data.identifier.id_type = GETTER;
            key_dispose(key);
        }
        else // ident is a variable
        {
            key_dispose(key);                                             // we need to free the setter key
            key = st_create_variable_key(exp_root->data.identifier.name); // new key is created

            if (!IS_GLOB_VAR(key->name)) // we only need to look for local variables
            {
                if (!verify_var_existence(key))
                {
                    key_dispose(key);
                    error_exit(ERR_SEM_UNDEFINED);
                }

                exp_root->data.identifier.code_gen_name = mangle_name(key->name, current_block_id);
            }
            else
            {
                if (!verify_glob_var_existence(key))
                {
                    key_dispose(key);
                    error_exit(ERR_SEM_UNDEFINED);
                }
            }
        }

        key_dispose(key);
        break;
    }
    case NODE_CALL: // function call used as a term inside an expression
        handle_function_call(exp_root);
        break;
    case NODE_BINARY_OP:

        if (exp_root->data.binary_operator.op_type != OP_PLUS)
            has_only_plus_op = false;

        if (exp_root->data.binary_operator.op_type == OP_DIV || exp_root->data.binary_operator.op_type == OP_MINUS)
        {
            has_minus_or_slash = true;
        }

        if (!IS_REL_OP(exp_root->data.binary_operator.op_type)) // has any arit operators
            has_arit_op = true;

        if (IS_REL_OP(exp_root->data.binary_operator.op_type)) // has any rel operators
            has_rel_op = true;

        if (IS_COMP_OP(exp_root->data.binary_operator.op_type)) // exp has <, >, <=, >= specifically
            has_comp_op = true;

        if (exp_root->data.binary_operator.op_type == OP_DIV && zero_division(exp_root->children[1]))
            zero_divison_detected = true;

        if (exp_root->data.binary_operator.op_type == OP_MUL)
        {
            if (STR_ITER_INVALID(exp_root->children[0]->type, exp_root->children[1]->type))
                error_exit(ERR_SEM_TYPE_MISMATCH);

            if (IS_STR_ITER(exp_root->children[0]->type, exp_root->children[1]->type))
            {
                has_string_lit = true;
            }
        }
        break;
    case NODE_NULL_LIT:
        has_null_lit = true;
        break;
    case NODE_INT_LIT:
    case NODE_FLOAT_LIT:
        has_num_lit = true;
        break;
    case NODE_STR_LIT:
        has_string_lit = true;
        break;
    default:
        break;
    }

    // exp subtree has the structure of a binary tree
    // we agreed on a convention that children[0] is the left child and children[1] the right child inside the exp subtree

    if (exp_root->children) // seg fault prevention
        exp_analysis(exp_root->children[0]);

    if (exp_root->children) // seg fault prevention
        exp_analysis(exp_root->children[1]);
}

/**
 * @brief Checks values of relevant combinations of expression flags and determines if type mismatch occured.
 *        If certain flag combinations are detected, a prediction of the expression data type can be made and used for type checking later.
 *
 * @note Works for simple expressions only. Other errors are going to be detected in code gen.
 *
 * @param exp_root Root of the expression subtree.
 */
bool eval_exp_flags(ASTNode_ptr exp_root)
{
    if (has_arit_op) // type checks that are specific for arit operators
    {
        // handle error flag combinations
        if (has_string_lit && has_minus_or_slash) // minus and division operators cannot be used with strings
            return false;
        else if (has_null_lit && has_arit_op) // null literal inside
            return false;
        else if (has_num_lit && has_string_lit && has_only_plus_op) //
            return false;
    }
    else if (has_rel_op) // has rel operators
    {
        if (has_string_lit && has_comp_op) // comparison operators cannot be used with string values
            return false;

        if (has_null_lit && has_comp_op) // null literal can only be used with ==, != rel operators
            return false;

        if (has_null_lit && (has_num_lit || has_string_lit)) // cannot compare null with other data types
            return false;
    }

    // handle prediction of exp operand restrictions
    if (has_minus_or_slash) // these operands cannot be used with strings
        exp_root->data.exp_statement.exp_type = TYPE_NUM;
    else if (has_only_plus_op && has_num_lit) // when number literal is present here, + operator can only be used as addition
        exp_root->data.exp_statement.exp_type = TYPE_NUM;
    else if (has_string_lit) // when string literal is present here, + operator can only be used as concat
    {
        exp_root->data.exp_statement.exp_type = TYPE_STRING;
    }
    else // could not predict any restrictions
        exp_root->data.exp_statement.exp_type = TYPE_UNKNOWN;

    return true;
}

/**
 * @brief Checks if the divider is equal to zero. Works only if the divider is a num literal, otherwise we cannot
 *        detect zero division. If zero division is detected error_exit() is called.
 *
 * @param divider Pointer towards the devider node inside AST.
 *
 * @return True if zero division detected, false otherwise.
 */
bool zero_division(ASTNode_ptr divider)
{
    if (divider->type == NODE_FLOAT_LIT && divider->data.literal.data.float_val == 0.0)
        return true;

    if (divider->type == NODE_INT_LIT && divider->data.literal.data.int_val == 0)
        return true;

    return false;
}

//**MAIN SEMANTIC FUNCTION**//

/**
 * @brief Traverses the tree and calls semantic functions based on the current node type.
 *
 * @param root Pointer to the root node of AST, needed so we can free the AST at anytime during the recursion
 * @param node_to_handle Helper pointer that will be used in recursive calls.
 * @param scope_stack Pointer to the scope_stack.
 */
void semantic_analysis(ASTNode_ptr node_to_handle)
{
    switch (node_to_handle->type)
    {
    case NODE_PROGRAM:
    {
        if (!main_exists())
        {
            error_exit(ERR_SEM_UNDEFINED);
        }
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        // every time we enter a new function block counter can be set to 0 again because every function is going to have it's own local frame
        block_counter = 0;

        // creates a separate symtable for the function arguments
        // this symtable is always going to be on the bottom of the stack, so all args will be visible to lower level scopes
        scope_stack_push(g_scope_stack, NULL);
        ST_Node **arg_symtable = scope_stack_top(g_scope_stack);

        if (node_to_handle->child_count == 0)
            break;

        for (unsigned idx = 0; idx < node_to_handle->data.function_def.arg_count; idx++) // we loop through all of the args and add them to the symtable of args
        {
            // new symbol is created
            Key *key = st_create_variable_key(node_to_handle->children[idx]->data.identifier.name);
            ST_Node *arg_node = st_create_node(key, block_counter); // function args are stored in a block with id == 0

            if (!arg_node)
                error_exit(ERR_INTERNAL);

            // symbol is inserted
            *arg_symtable = st_insert_node(*arg_symtable, arg_node);
            free(key);
        }
        break;
    }
    case NODE_ASSIGN: // when assignment node is found, we need to verify whether the assignment target is not a setter
    {
        // first we try to find a setter with idents name
        ASTNode_ptr assign_target = node_to_handle->children[0]; // first child inside NODE_ASSINGN is always the assign target

        Key *key = st_create_function_key(assign_target->data.identifier.name, 1, SETTER);
        ST_Node *search_result = st_search(g_func_symtable, key);

        if (search_result) // setter was found, so idents type is set to SETTER
            assign_target->data.identifier.id_type = SETTER;

        key_dispose(key);

        // we check if user did not assign into a getter
        key = st_create_function_key(assign_target->data.identifier.name, 0, GETTER);
        search_result = st_search(g_func_symtable, key);

        if (search_result) // assign target was a getter
            error_exit(ERR_SEM_OTHER);

        key_dispose(key);

        // now we can check if a new global variable was not defined
        if (IS_GLOB_VAR(assign_target->data.identifier.name))
        {
            key = st_create_variable_key(assign_target->data.identifier.name);
            search_result = st_search(g_global_symtable, key);

            if (!search_result) // new global variable defined
            {
                ST_Node *new_glob_var = st_create_node(key, block_counter);

                if (!new_glob_var)
                    error_exit(ERR_INTERNAL);

                g_global_symtable = st_insert_node(g_global_symtable, new_glob_var);
            }

            free(key);
        }
        break;
    }
    case NODE_VAR_DECL:
    {
        Key *key = st_create_variable_key(node_to_handle->data.identifier.name);
        ST_Node **current_scope = scope_stack_top(g_scope_stack);

        if (verify_var_redec(key, *current_scope)) // redec detected
        {
            free(key);
            error_exit(ERR_SEM_REDEFINITION);
        }
        else // new local var needs to be added to current_scope
        {
            ST_Node *new_node = st_create_node(key, block_counter);

            if (!new_node)
                error_exit(ERR_INTERNAL);

            *current_scope = st_insert_node(*current_scope, new_node);

            // assign the newly create mangled variable name
            node_to_handle->data.identifier.code_gen_name = mangle_name(key->name, new_node->block_id);

            free(key);
        }

        break;
    }
    case NODE_BLOCK: // creates new empty scope
    {
        block_counter++; // we entered a new block so we need to increment the block_counter here

        scope_stack_push(g_scope_stack, NULL); // new empty scope is created
        break;
    }
    case NODE_IDENTIFIER: // can only be a local or global var, because function nodes have a separate node type
    {
        if (node_to_handle->data.identifier.id_type == SETTER || node_to_handle->data.identifier.id_type == GETTER)
            break; // node was already checked because id_type is assigned

        Key *var_key = st_create_variable_key(node_to_handle->data.identifier.name);

        if (!IS_GLOB_VAR(var_key->name)) // we only need to search if the variable is not global
        {
            if (!verify_var_existence(var_key))
            {
                free(var_key);
                error_exit(ERR_SEM_UNDEFINED);
            }

            node_to_handle->data.identifier.code_gen_name = mangle_name(var_key->name, current_block_id);
        }

        break;
    }
    case NODE_EXPR_STMNT:
    {
        exp_analysis(node_to_handle->children[0]);

        if (!eval_exp_flags(node_to_handle))
            error_exit(ERR_SEM_TYPE_MISMATCH);

        if (zero_divison_detected)
            error_exit(ERR_SEM_OTHER);

        reset_flags();
        break;
    }
    case NODE_FOR:
    {
        // we have push the iterator to a symtable so we can access it inside the for cycle
        ASTNode_ptr iterator = node_to_handle->children[0];

        Key *iter_key = st_create_variable_key(iterator->data.identifier.name);

        block_counter++;                                              // block counter needs to be incremented
        ST_Node *iter_node = st_create_node(iter_key, block_counter); // iterator is stored in it's own block

        if (!iter_node)
            error_exit(ERR_INTERNAL);

        scope_stack_push(g_scope_stack, iter_node);

        /* NOTE:
        node_to_handle->children[1] == expression
        node_to_handle->children[1]->children[0] == range operator
        */
        ASTNode_ptr range_operator = node_to_handle->children[1]->children[0];

        // we check that the expression inside the for cycle definition is really a range expression
        if (range_operator->type != NODE_RANGE)
            error_exit(ERR_SEM_OTHER);

        loop_nesting_tracker++; // gets incremented each time a for loop is entered
        break;
    }
    case NODE_WHILE:
        loop_nesting_tracker++; // gets incremented each time a while loop is entered
        break;
    case NODE_BREAK: // break and continue keyword usage check
    case NODE_CONTINUE:
    {
        if (loop_nesting_tracker == 0) // break or continue keyword used outside of a loop
            error_exit(ERR_SEM_OTHER);
        break;
    }
    case NODE_CALL:
        handle_function_call(node_to_handle);
        break;
    default:
        break;
    }

    for (unsigned idx = 0; idx < node_to_handle->child_count; idx++)
    {
        // we can skip all children of the exp node because they were all analysed inside the exp_analysis function
        if (node_to_handle->type == NODE_EXPR_STMNT)
            continue;

        semantic_analysis(node_to_handle->children[idx]);
    }

    // block and all its statements processed - safe to pop scope from stack
    if (node_to_handle->type == NODE_BLOCK)
        scope_stack_pop(g_scope_stack);

    // loops processed - we can decrement loop nesting tracker
    if (node_to_handle->type == NODE_FOR || node_to_handle->type == NODE_WHILE)
        loop_nesting_tracker--;
}
