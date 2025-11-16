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
#include <string.h>
#include "semantic_analysis.h"
#include "include/scope_stack.h"
#include "include/symtable.h"
#include "error.h"
#include "ast.h"

// expression flags
has_only_plus_op = true;
has_string_lit = false;
has_minus_or_slash = false;
has_null_lit = false;
has_unary_minus = false;
has_operator = false;
has_rel_op = false;
zero_divison_detected = false;
has_comp_op = false;

// loop detection helper
loop_nesting_tracker = 0;

//**UNRESOLVED SYMBOL ARRAY FUNCTION DEFINITIONS - START**//

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
 * @brief Handles clean up of the unresolved symbols array.
 *
 * @param unresolved Pointer to the struct of unresolved array.
 */
void unresolved_dispose(Unresolved_Symbols_Array *unresolved)
{
    free(unresolved->array);
    unresolved->array = NULL;

    free(unresolved);
}

//**UNRESOLVED SYMBOL ARRAY FUNCTION DEFINITIONS - END**//

//**SEMANTIC FUNCTIONS USED BY THE PARSER - START**//

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
 * @param unresolved Pointer to the unresolved array struct to store symbols which existance could not be resolved, yet.
 */
void verify_func_existance(Key *key, ST_Node *func_symtable, Unresolved_Symbols_Array *unresolved)
{
    ST_Node *search_result = search(func_symtable, key);

    if (!search_result) // function not found inside the function symtable
    {
        // copy of the key is stored so symbol existance can be resolved later
        add_unresolved_symbol(unresolved, *key);
    }
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
    Key *key = create_function_key("main", 0, FUNCTION);

    if (!key)
        error_exit(ERR_INTERNAL);

    ST_Node *search_result = search(func_symtable, key);
    free(key);

    return search_result != NULL;
}

//**SEMANTIC FUNCTIONS USED BY THE PARSER - END**//

/**
 * @brief Checks if the divider is equal to zero. Works only if the divider is a num literal, otherwise we cannot
 *        detect zero division. If zero division is detected error_exit() is called.
 *
 * @param divider Pointer towards the devider node inside AST
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

/**
 * @brief Verifies whether the args count inside the function call matches the function
 *        definition inside func_symtable.
 *
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 * @param key Pointer to a key containing function info.
 *
 * @return True if args count is correct, return false otherwise.
 */
bool args_count_check(ST_Node *func_symtable, Key *key)
{
    // looks through function symtable to look for a function with a matching key
    ST_Node *search_result = search(func_symtable, key);

    // at this point of compilation we know that no undefined functions can exist
    // so we can determine that if no function was found, it is only because of a wrong arg count
    if (!search_result)
        return false;

    return true;
}

/**
 * @brief Verifies that a built-in function exists and that it was called with the correct num of arguments.
 *
 * @param name Name of the built in function.
 *
 * @return True if a built-in with this name exists, false otherwise.
 */
bool builtin_exists(char *name)
{
    for (unsigned idx = 0; idx < builtin_functions_arr_lenght; idx++) // looks for a built in function with this name
    {
        if (strcmp(name, builtin_functions[idx].name) == 0)
            return true;
    }

    return false;
}

/**
 * @brief Verifies that a built-in function was called with the correct num of arguments.
 *
 * @param name Name of the built-in function.
 * @param args_count Number of passed arguments inside the function call of a built-in function.
 *
 * @return True if args count is correct, false otherwise.
 */
bool builtin_args_count_correct(char *name, unsigned args_count)
{
    bool args_count_correct = false;

    for (unsigned idx = 0; idx < builtin_functions_arr_lenght; idx++) // looks for a built in function with this name
    {
        if (strcmp(name, builtin_functions[idx].name) == 0)
        {
            args_count_correct = builtin_functions[idx].args_count == args_count;
        }
    }

    return args_count_correct;
}

/**
 * @brief Loops through all the params inside the function call of a built-in and if a literal is found,
 *        it's data type is verified against the defined arg types of built-in fuctions.
 *
 * @param name Name of the built-in function.
 * @param args_count Num of args inside the function call.
 */
bool builtin_args_types_correct(ASTNode_ptr call_node, char *name, unsigned args_count)
{
    builtin_function_t *search_result;

    for (unsigned idx = 0; idx < builtin_functions_arr_lenght; idx++) // finds the function based on a name
    {
        if (strcmp(name, builtin_functions[idx].name) == 0)
        {
            search_result = &builtin_functions[idx];
            break;
        }
    }

    for (unsigned idx = 0; idx < args_count; idx++) // loops through the params of the function call
    {
        // types only need to be checked if the current arg has any type restrictions
        if (search_result->arg_types[idx] == ANY_TYPE)
            continue;

        if (call_node->type == NODE_STR_LIT && search_result->arg_types[idx] != STR_TYPE)
        {
            return false;
        }
        else if (call_node->type == NODE_INT_LIT || call_node->type == NODE_FLOAT_LIT)
        {
            if (search_result->arg_types[idx] != NUM_TYPE)
                return false;
        }
    }

    return true;
}

/**
 * @brief While traversing the expression subtree, differnt expression flags are set. These flags are later used
 *        to determine if type mismatch occurs inside an expression.
 *        Function also handles identification of getters inside an expression.
 *
 * @param exp_root Root of the expression subtree.
 * @param func_symtable Pointer to the symtable of all setter, getters and functions.
 */
void exp_analysis(ASTNode_ptr exp_root, ST_Node *func_symtable)
{
    if (!exp_root)
        return;

    switch (exp_root->type) // sets different flags to true based on the current node
    {
    case NODE_IDENTIFIER:
        Key *getter_key = create_function_key(exp_root->data.identifier.name, 0, GETTER); // key needs to be created
        ST_Node *search_result = search(func_symtable, getter_key);

        if (search_result) // id inside the exp is identified as a getter
            exp_root->data.identifier.id_type = GETTER;

        free(getter_key);
        break;
    case NODE_STR_LIT:
        has_string_lit = true;
        break;
    case NODE_UNARY_OP:
        has_unary_minus = true;
        has_operator = true;
        break;
    case NODE_BINARY_OP:
        has_operator = true;

        if (exp_root->data.binary_operator.op_type != OP_PLUS)
            has_only_plus_op = false;

        if (exp_root->data.binary_operator.op_type == OP_DIV || exp_root->data.binary_operator.op_type == OP_MINUS)
            has_minus_or_slash = true;

        if (exp_root->data.binary_operator.op_type == OP_EQ ||
            exp_root->data.binary_operator.op_type == OP_NEQ ||
            exp_root->data.binary_operator.op_type == OP_LT ||
            exp_root->data.binary_operator.op_type == OP_LTE ||
            exp_root->data.binary_operator.op_type == OP_GT ||
            exp_root->data.binary_operator.op_type == OP_GTE ||
            exp_root->data.binary_operator.op_type == OP_IS)
            has_rel_op = true;

        if (exp_root->data.binary_operator.op_type == OP_LT ||
            exp_root->data.binary_operator.op_type == OP_LTE ||
            exp_root->data.binary_operator.op_type == OP_GT ||
            exp_root->data.binary_operator.op_type == OP_GTE)
            has_comp_op = true;

        if (exp_root->data.binary_operator.op_type == OP_DIV && zero_division(exp_root->children[1]))
            zero_divison_detected = true;

        break;
    case NODE_NULL_LIT:
        has_null_lit = true;
        break;
    default:
        break;
    }

    // we agreed on a convention that children[0] is the left child and children[1] the right child inside the exp subtree
    set_allowed_op_types(exp_root->children[0]); // handle left subtree
    set_allowed_op_types(exp_root->children[1]); // handle right subtree
}

/**
 * @brief Checks values of relevant combinations of expression flags and determines if type mismatch occured.
 *        If certain flag combinations are detected, a restriction code can be assigned to different expression nodes.
 *
 * @param exp_root Root of the expression subtree.
 */
bool eval_exp_flags(ASTNode_ptr exp_root)
{
    if (!has_rel_op)
    {
        // handle error flag combinations
        if (has_string_lit && has_minus_or_slash) // minus and division operators cannot be used with strings
            return false;
        else if (has_unary_minus && has_string_lit)
            return false;
        else if (has_null_lit && has_operator) // null literal inside
            return false;

        // handle prediction of exp operand restrictions
        if (has_minus_or_slash || has_unary_minus)
            // these operands cannot be used with strings
            exp_root->data.exp_statement.restriction = ONLY_NUM;
        else if (has_operator && has_only_plus_op && has_string_lit)
            // when string literal is present here, + operator can only be used as concat
            exp_root->data.exp_statement.restriction = ONLY_STR;
        else // could not predict any restrictions
            exp_root->data.exp_statement.restriction = UNDETERMINED;
    }
    else // has rel operators
    {
        if (has_string_lit && has_comp_op) // comparison operators cannot be used with string values
            return false;
    }

    return true;
}

/**
 * @brief Traverses the tree and calls semantic functions based on the current node type.
 *
 * @param root Pointer to the root node of AST.
 * @param node_to_handle Helper pointer that will be used in recursive calls.
 * @param func_symtable Pointer to function symtable that will be used inside args_count_check() for user defined functions.
 */
void semantic_analysis(ASTNode_ptr root, ASTNode_ptr node_to_handle, ST_Node *func_symtable)
{
    if (!node_to_handle->child_count)
        return;

    switch (node_to_handle->type)
    {
    case NODE_ASSIGN:
        Key *setter_key = create_function_key(node_to_handle->data.identifier.name, 1, SETTER); // key needs to be created
        ST_Node *search_result = search(func_symtable, setter_key);

        if (search_result) // id inside the exp is identified as a setter
            node_to_handle->data.identifier.id_type = SETTER;

        free(setter_key);

    case NODE_EXPR_STMNT:
        set_exp_flags(node_to_handle);

        if (!eval_exp_flags(node_to_handle))
        {
            ast_free(root);
            error_exit(ERR_SEM_TYPE_MISMATCH);
        }

        if (zero_divison_detected)
        {
            ast_free(root);
            error_exit(ERR_SEM_OTHER);
        }
        break;

    case NODE_FOR:
    case NODE_WHILE:
        loop_nesting_tracker++; // gets incremented each time a loop is entered
        break;

    case NODE_BREAK: // break and continue keyword usage check
    case NODE_CONTINUE:
        if (loop_nesting_tracker == 0) // break or continue keyword used outside of a loop
        {
            ast_free(root);
            error_exit(ERR_SEM_OTHER);
        }

    case NODE_CALL:
        if (node_to_handle->data.function_call.is_builtin) // function called is a Ifj built-in function
        {
            // get important info
            char *name = node_to_handle->data.function_call.name;
            unsigned args_count = node_to_handle->data.function_call.param_count;

            if (!builtin_exists(name)) // incorrect built-in ident used
            {
                ast_free(root);
                error_exit(ERR_SEM_UNDEFINED);
            }

            if (!builtin_args_count_correct(name, args_count))
            {
                ast_free(root);
                error_exit(ERR_SEM_ARG_COUNT);
            }

            if (!builtin_args_types_correct(node_to_handle, name, args_count))
            {
                ast_free(root);
                error_exit(ERR_SEM_TYPE_MISMATCH);
            }
        }
        else // not a built-in function
        {
            // creates a key to search the func_symtable
            Key *key = create_function_key(node_to_handle->data.function_call.name,
                                           node_to_handle->data.function_call.param_count,
                                           FUNCTION);

            if (!args_count_check(func_symtable, key))
            {
                ast_free(root);
                error_exit(ERR_SEM_ARG_COUNT);
            }
        }

    default:
        break;
    }

    // todo : pridat na toto miesto resetovanie flagov

    for (unsigned idx = 0; idx < node_to_handle->child_count; idx++)
        semantic_analysis(root, node_to_handle->children[idx], func_symtable);
}