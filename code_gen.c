/**
 * @file code_gen_funs.c
 * @author Martin Racek (xracekm00),
 *         Martin Mezei (xmezeim00)
 * @brief Code generating functions for Wren-like programming language
 *
 * @note Code is being printed to stdout.
 *       For testing purposes the stream will be redirected into log.txt file.
 *       We decided to use Pascal convetion for function calls.
 *
 * @version 0.1
 * @date 2025-11-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "symtable.h"
#include "ast.h"
#include "code_gen.h"
#include "error.h"
#include "built_in_funs.h"
#include "global_structures.h"

// Global instance of data type holding different kinds of information nececssary for code-gen
name_generator_ptr global_name_gen = NULL;
// Global flag, holds information whether the function contained return node
bool return_occured = false;

/**
 * @brief The main code generating function - contains switch for all different types of nodes.
 *        Traverses the AST via inorder exept for expression subtrees.
 *        That one is processed by functions handling expression and is being traversed via postorder.
 *
 * @param node
 */
void codegen(ASTNode_ptr node)
{
    // End of reccursion
    if (!node)
    {
        return;
    }

    // based on the node type calls functions generating instructions
    switch (node->type)
    {
    case NODE_PROGRAM:
        // IFJcode25 code starts with this line
        printf(".IFJcode25\n\n");

        // Declares all global variables
        gen_all_glob_vars_dec(g_global_symtable);

        // Skipping built in functions defined at the beginning of each program
        printf("\nJUMP _program_start_\n");

        // At the beginning of the program, there are implementations of builtin functions
        gen_built_in_read_str();
        gen_built_in_read_num();
        gen_built_in_write();
        gen_built_in_floor();
        gen_built_in_str();
        gen_built_in_length();
        gen_built_in_substring();
        gen_built_in_strcmp();
        gen_built_in_ord();
        gen_built_in_chr();

        printf("############# BUILT IN FUNCTIONS END #############\n");

        break;
    case NODE_FUNCTION_DEF:
        // Reset global name_gen_t object
        name_gen_init(node);

        // Set flags
        if (node->data.function_def.type == FUN_F)
        {
            global_name_gen->in_function = true;
        }
        else if (node->data.function_def.type == FUN_G)
        {
            global_name_gen->in_getter = true;
        }
        else
        {
            global_name_gen->in_setter = true;
        }

        // Calls corresponding code generating function
        gen_func_start(node);

        break;
    case NODE_VAR_DECL:
        /**
         * @note TO DO:
         *       When the variable declaration is inside a loop the
         *       code has to make sure that redeclaration wont happen
         */

        // Calls corresponding code generating function
        gen_var_decl(node);

        break;
    case NODE_ASSIGN:
        // Print to make IFJcode25 code more readable
        printf("\n#Expression evaluation follows\n");

        // This function takes care of the right side of the assignment
        eval_exp(node->children[1]->children[0]);
        // Calls corresponding code generating function
        gen_assign(node);

        break;
    case NODE_IF:
        // Print to make IFJcode25 code more readable
        printf("\n#Expression evaluation follows\n");

        // Condition evaluation
        eval_exp(node->children[0]);

        // Variables to make code more readable
        ASTNode_ptr true_block = node->children[1];
        ASTNode_ptr else_block = node->children[2];

        // Calls corresponding code generating function
        gen_if(node);
        // Traverse the true block
        codegen(true_block);

        // Calls corresponding code generating function
        gen_else(node);
        // Traverse the true block
        codegen(else_block);

        // Prints label representing end of if statment
        printf("\nLABEL %s\n", global_name_gen->end_if_label);

        break;
    case NODE_RETURN:
        // Update global flag
        return_occured = true;

        // Print to make IFJcode25 code more readable
        printf("\n#Expression evaluation follows\n");

        // After the node return an expression follows
        eval_exp(node->children[0]->children[0]);
        // Returns from the function
        gen_return();

        break;
    case NODE_WHILE:
        // Updates location flag
        global_name_gen->in_loop = true;
        // Calls corresponding code generating function
        gen_while_start(node);

        break;
    case NODE_FOR:
        // Updates location flag
        global_name_gen->in_loop = true;
        // Calls corresponding code generating function
        gen_for_start(node);

        break;
    case NODE_BREAK:
        // Prints jump to corresponding label
        gen_break();

        break;
    case NODE_CONTINUE:
        // Prints jump to corresponding label
        gen_continue();

        break;
    default:
        /**
         * @brief There is nothing to be done for this type of nodes:
         *        NODE_BINARY_OP, NODE_BLOCK
         *
         * @note These nodes are processed by some other functions and
         *       dont have to be handeled:
         *       NODE_RANGE, NODE_INT_LIT, NODE_FLOAT_LIT, NODE_STR_LIT, NODE_NULL_LIT,
         *       NODE_IDENTIFIER, NODE_CALL
         */

        break;
    }

    /**
     * @brief Iterate throuh all children of the node
     *
     * @note there are nodes with certain types for which we dont want
     *       its children to be traversed (implementation details)
     *
     */
    if (is_valid_node_type(node))
    {
        for (unsigned i = 0; i < node->child_count; i++)
        {
            codegen(node->children[i]);
        }
    }

    /**
     * @brief As the recurrsion returns back to the root these if statements
     *        will be executed.
     */

    // Generates function end instructions
    if (node->type == NODE_FUNCTION_DEF)
    {
        global_name_gen->in_function = false;
        // Makes sure that only one RETURN instruction is generated in each function
        if (!return_occured)
        {
            gen_return();
        }
        return_occured = false;
    }

    // Updates location flag and calls function handeling for loop end
    if (node->type == NODE_FOR)
    {
        global_name_gen->in_loop = false;
        gen_for_end(node);
    }

    // Updates location flag and calls function handeling while loop end
    if (node->type == NODE_WHILE)
    {
        global_name_gen->in_loop = false;
        gen_while_end(node);
    }

    if (node->type == NODE_PROGRAM)
        gen_program_end();
}

/**
 * @brief Prints declarations of all global variables from symtable at the
 *        beginning of programe
 *
 */
void gen_all_glob_vars_dec(ST_Node *symtable)
{
    // Reccursion end
    if (!symtable)
        return;
    // Prints variable declaration
    printf("DEFVAR GF@%s\n", symtable->key.name);
    // Recursively traverses symtable
    gen_all_glob_vars_dec(symtable->left);
    gen_all_glob_vars_dec(symtable->right);
}

/**
 * @brief sets all name_generator_t attributes to default values
 *
 * @note used for reset when entering new function_def node
 *
 * @param node
 */
void name_gen_init(ASTNode_ptr node)
{
    // Set counters to default values
    global_name_gen->loop_counter = 0;
    global_name_gen->if_counter = 0;
    global_name_gen->temp_var_counter = 0;
    global_name_gen->bin_op_counter = 0;

    // Set stack tracker to zero
    global_name_gen->stack_depth = 0;

    // Set all allocated strings to empty
    memset(global_name_gen->fun_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->else_block_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->end_if_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_start_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_end_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->called_function, 0, MAX_LABEL_NAME);
    memset(global_name_gen->mul, 0, MAX_LABEL_NAME);
    memset(global_name_gen->str_iter, 0, MAX_LABEL_NAME);
    memset(global_name_gen->mul_end, 0, MAX_LABEL_NAME);
    memset(global_name_gen->add, 0, MAX_LABEL_NAME);
    memset(global_name_gen->concat, 0, MAX_LABEL_NAME);
    memset(global_name_gen->add_end, 0, MAX_LABEL_NAME);
    memset(global_name_gen->sub, 0, MAX_LABEL_NAME);
    memset(global_name_gen->eval, 0, MAX_LABEL_NAME);
    memset(global_name_gen->log_end, 0, MAX_LABEL_NAME);
    memset(global_name_gen->left_to_float, 0, MAX_LABEL_NAME);
    memset(global_name_gen->right_to_float, 0, MAX_LABEL_NAME);
    memset(global_name_gen->both_to_float, 0, MAX_LABEL_NAME);
    memset(global_name_gen->zero_div_check_float, 0, MAX_LABEL_NAME);
    memset(global_name_gen->zero_div_check_int, 0, MAX_LABEL_NAME);
    memset(global_name_gen->zero_div_check_done, 0, MAX_LABEL_NAME);

    // Store current function name and parameter count
    strcpy(global_name_gen->curr_function, node->data.function_def.name);
    global_name_gen->curr_param_count = node->data.function_def.arg_count;

    // Set flags to default values
    global_name_gen->in_function = false;
    global_name_gen->in_getter = false;
    global_name_gen->in_setter = false;
    global_name_gen->in_loop = false;
}

/**
 * @brief Pushes variable on data stack
 *
 * @note used by expression processing functions
 *
 * @param node
 */
void gen_push_variable(ASTNode_ptr node)
{
    // Differentiates between global and local variable
    if (node->data.identifier.is_global)
    {
        printf("PUSHS GF@%s\n", node->data.identifier.name);
    }
    else
    {
        // printf("------------Tu je chyba-----------\n");
        printf("PUSHS LF@%s\n", node->data.identifier.code_gen_name);
    }

    // Updates stack tracker
    global_name_gen->stack_depth++;
}

/**
 * @brief Generates user defined variable declaration
 *
 * @param node
 */
void gen_var_decl(ASTNode_ptr node)
{
    // Differentiates between global and local variable
    if (node->data.identifier.is_global)
    {
        // Here its not necessary to use code_gen_name since global names are unique after scope stack check
        printf("DEFVAR GF@%s\n", node->data.identifier.name);
    }
    else
    {
        printf("DEFVAR LF@%s\n", node->data.identifier.code_gen_name);
    }
}

/**
 * @brief Assigns value to the variable stored in left node child
 *
 * @param node
 */
void gen_assign(ASTNode_ptr node)
{
    // Variable to make code more readable
    ASTNode_ptr lhs = node->children[0];

    // When the lhs is a setter
    if (lhs->type == NODE_IDENTIFIER && lhs->data.identifier.id_type == SETTER)
    {
        // The parameter is already on top of data stack
        gen_jmp_function(lhs);
    }
    else
    { // The lhs has to be variable
        if (lhs->data.identifier.is_global)
        {
            printf("POPS GF@%s\n", lhs->data.identifier.name);
        }
        else
        {
            printf("POPS LF@%s\n", lhs->data.identifier.code_gen_name);
        }
    }
}

/**
 * @brief Handles start of function
 *
 * @note Called after entering NODE_FUNCTION_DEF node
 *
 * @param node
 */
void gen_func_start(ASTNode_ptr node)
{
    // Name generator was already initialized when this function is called

    // Creates and print unique function label name,
    create_unique_name(node, FUN_LABEL);

    if (!strcmp(global_name_gen->fun_label, "_fun_main_0"))
    {
        // This is where actual compilation begins
        printf("\n");
        printf("\nLABEL _program_start_\n");
    }

    printf("\nLABEL %s\n", global_name_gen->fun_label);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    if (node->data.function_def.arg_count != 0)
    {
        printf("\n# Store params into local variables\n");

        // Creates local vaiables
        for (int i = (node->data.function_def.arg_count - 1); i >= 0; i--)
        {
            printf("DEFVAR LF@%s\n", node->children[i]->data.identifier.code_gen_name);
            printf("POPS LF@%s\n", node->children[i]->data.identifier.code_gen_name);
        }
    }
}

/**
 * @brief This function is called at the end of function or when return node is encountered
 *
 */
void gen_return()
{
    /**
     * @note Returns nill when function didnt contain return, but only when the 
     *       function isn't main. Since main can not be calle by user, this function
     *       doesnt return anything though there exist a version with parameters which
     *       returns.
     */
    if (!return_occured && strcmp(global_name_gen->fun_label, "_fun_main_0"))
    {
        printf("PUSHS nil@nil\n");
    }

    // Generate function end
    printf("POPFRAME\n");
    // Prints return only when the current function isnt main
    if (strcmp(global_name_gen->fun_label, "_fun_main_0"))
    {
        printf("RETURN\n");
    }
    else
        printf("JUMP _program_end_\n");
}

/**
 * @brief Converts string literal into corresponding value and pushes this value on a stack
 *
 * @param value The float value to push on stack
 */
void gen_lit_string(char *value)
{
    // First, lets make sure the string format is valid

    char *correct_value = calloc(MAX_STRING_LEN, sizeof(char));
    if (correct_value == NULL)
    {
        // NOTE: handle freeing later
        error_exit(ERR_INTERNAL);
    }

    // Iterating throgh the string
    unsigned new_str_index = 0;

    for (int i = 0; value[i] != '\0'; i++)
    {
        unsigned char ch = (unsigned char)value[i];

        if (is_invalid_char(ch))
        {
            new_str_index += sprintf(&correct_value[new_str_index], "\\%03d", ch);
        }
        else
        {
            correct_value[new_str_index] = value[i];
            new_str_index++;
        }
    }

    // Strings must be null terminated
    correct_value[new_str_index] = '\0';

    printf("PUSHS string@%s\n", correct_value);

    free(correct_value);
}

/**
 * @brief Generates jump on corresponding function
 *
 * @note works  on both built in and user defined functions
 *
 * @param node
 */
void gen_jmp_function(ASTNode_ptr node)
{
    /**
     * @brief When this function is called for a setter, the argument is already
     *        on top of the data stack.
     *        When this function is called for a getter, there is no argument.
     */

    if (node->type == NODE_CALL)
    {
        // Print to make IFJcode25 code more readable
        printf("\n#Expression evaluation of parameters follows\n");
        // First the arguments are pushed on data strack (left to right) but
        // has to be treated as potential expression
        for (unsigned i = 0; i < node->data.function_call.param_count; i++)
        {
            eval_exp(node->children[i]);
        }
    }

    // The function is built in
    if (node->data.function_call.is_builtin)
    {
        if (!strcmp(node->data.function_call.name, "read_str"))
        {
            printf("CALL IFJ_read_str\n");
        }
        else if (!strcmp(node->data.function_call.name, "read_num"))
        {
            printf("CALL IFJ_read_num\n");
        }
        else if (!strcmp(node->data.function_call.name, "write"))
        {
            printf("CALL IFJ_write\n");
        }
        else if (!strcmp(node->data.function_call.name, "floor"))
        {
            printf("CALL IFJ_floor\n");
        }
        else if (!strcmp(node->data.function_call.name, "str"))
        {
            printf("CALL IFJ_str\n");
        }
        else if (!strcmp(node->data.function_call.name, "length"))
        {
            printf("CALL IFJ_length\n");
        }
        else if (!strcmp(node->data.function_call.name, "substring"))
        {
            printf("CALL IFJ_substring\n");
        }
        else if (!strcmp(node->data.function_call.name, "strcmp"))
        {
            printf("CALL IFJ_strcmp\n");
        }
        else if (!strcmp(node->data.function_call.name, "ord"))
        {
            printf("CALL IFJ_ord\n");
        }
        else
        {
            printf("CALL IFJ_chr\n");
        }
    }
    else
    {
        // The function is user defined
        create_unique_name(node, CALL);
        printf("\nCALL %s\n", global_name_gen->called_function);
    }
}

/**
 * @brief Handles start of a for loop
 *
 * @note called from NODE_FOR
 *
 * @param node
 */
void gen_for_start(ASTNode_ptr node)
{
    // Creates unique label name
    create_unique_name(node, LOOP_START_L);
    create_unique_name(node, LOOP_END_L);

    // Store current temp_var_counter value for later usage and increment the counter inside global_name_gen
    unsigned temp_id = global_name_gen->temp_var_counter++;

    // Variable to make code more readable
    char *iter_name = node->children[0]->data.identifier.code_gen_name;

    // Variable used as "index" in for loop - iterator
    printf("DEFVAR LF@%s\n", iter_name);
    printf("DEFVAR LF@temp_var_until_%d\n", temp_id);

    // Have to read NODE_RANGE children
    ASTNode_ptr range_node = node->children[1]->children[0];

    // Print to make IFJcode25 code more readable
    printf("\n#Expression evaluation of range follows1\n");
    // This function will return start and end of the range on stack data
    eval_exp(range_node);

    // Initialize variables
    printf("POPS LF@temp_var_until_%d\n", temp_id);
    printf("POPS LF@%s\n", iter_name);

    // Prints label of the beginnning of the loop
    printf("\nLABEL %s\n", global_name_gen->loop_start_label);

    // Push condition arguments on stack
    printf("\n# Evaluate condition\n");
    printf("PUSHS LF@%s\n", iter_name);
    printf("PUSHS LF@temp_var_until_%d\n", temp_id);

    // Evaluates condition based on whether the range is inclusive(double dot) or not(triple dot)
    // Continue until i <= end
    if (range_node->data.range.inclusive)
    {
        // Jump when i > end
        printf("GTS\n");
        printf("PUSHS bool@true\n");
        printf("JUMPIFEQS %s\n\n", global_name_gen->loop_end_label);
    }
    // Continue until i < end
    else
    {
        // jump when i !< end
        printf("LTS\n");
        printf("PUSHS bool@false\n");
        printf("JUMPIFEQS %s\n\n", global_name_gen->loop_end_label);
    }

    // for loop body follows
}

/**
 * @brief Handles end of a for loop
 *
 * @note called when recrusion returns back to the node
 *
 * @param node
 */
void gen_for_end(ASTNode_ptr node)
{
    // Variable to make code more readable
    char *iter_name = node->children[0]->data.identifier.code_gen_name;

    // Increment the iterator variable
    printf("\n# Icrementing iterator\n");
    printf("PUSHS LF@%s\n", iter_name);
    printf("PUSHS int@1\n");
    printf("ADDS\n");
    printf("POPS LF@%s\n", iter_name);

    // Loops to the beginning
    printf("\nJUMP %s\n", global_name_gen->loop_start_label);
    // Prints label representing loops end
    printf("\nLABEL %s\n", global_name_gen->loop_end_label);
}

/**
 * @brief Handles start of a while loop
 *
 * @note called from NODE_WHILE
 *
 * @param node
 */
void gen_while_start(ASTNode_ptr node)
{
    // Creates unique label names
    create_unique_name(node, LOOP_START_L);
    create_unique_name(node, LOOP_END_L);

    // Prints loop start label
    printf("\nLABEL %s\n", global_name_gen->loop_start_label);

    // Condition check
    printf("# Evaluate while condition\n");

    // Print to make IFJcode25 code more readable
    printf("\n#Expression evaluation follows\n");

    // This function will evaluate the condition and leave the result at data stack top
    eval_exp(node->children[0]);

    // When condition is false, while loop will end
    printf("PUSHS bool@false\n");
    printf("JUMPIFEQS %s\n\n", global_name_gen->loop_end_label);

    // while loop body follows
}

/**
 * @brief Handles end of a while loop
 *
 * @note called when recursion returns back to NODE_WHILE
 *
 * @param node
 */
void gen_while_end()
{
    // Loop back to the beginning
    printf("\nJUMP %s\n", global_name_gen->loop_start_label);

    // Prints loop end label
    printf("\nLABEL %s\n", global_name_gen->loop_end_label);
}

/**
 * @brief Terminates correspondig while loop
 */
void gen_break()
{
    printf("JUMP %s\n", global_name_gen->loop_end_label);
}

/**
 * @brief Skips one iteration in correspondig while loop
 */
void gen_continue()
{
    printf("JUMP %s\n", global_name_gen->loop_start_label);
}

/**
 * @brief Handles start of if statement
 *
 * @param node
 */
void gen_if(ASTNode_ptr node)
{
    // Unique labely
    create_unique_name(node, IF_ELSE_L);
    create_unique_name(node, IF_END_L);

    // Condition check
    printf("\n# Decides where to continue based on the condition result\n");

    // When the condition is false jump on else block label
    printf("PUSHS bool@false\n");
    printf("JUMPIFEQS %s\n\n", global_name_gen->else_block_label);

    // Generate body
}

/**
 * @brief Generates error labels at the end of the program.
 */
void gen_program_end()
{
    // here program exits with err code 25 - invalid arg type
    printf("\nLABEL !ERROR_ARG_L\n");
    printf("EXIT int@25\n");

    // here program exits with err code 25 - invalid data type in an expression
    printf("\nLABEL !ERROR_EXP_L\n");
    printf("EXIT int@26\n");

    printf("\nLABEL _program_end_\n");
    printf("EXIT int@0\n");
}

/**
 * @brief Handles beginning of else block of if statement
 *
 * @param node
 */
void gen_else()
{
    // In case the if block would reach this instructions jump at the end of
    // the whole if statement needs to be executed
    printf("JUMP %s\n\n", global_name_gen->end_if_label);

    // Prints label for else block
    printf("\nLABEL %s\n", global_name_gen->else_block_label);

    // Generate body
}

/**
 * @brief Creates a unique label name
 *
 * @param node
 * @param option
 */
void create_unique_name(ASTNode_ptr node, name_option_t option)
{
    // Different options that can be used as argument
    switch (option)
    {
    case FUN_LABEL:
        // Have to differentiate between the location of loop: function, setter and getter
        if (node->data.function_def.type == FUN_F)
        {
            // Creates unique name
            snprintf(global_name_gen->fun_label, MAX_LABEL_NAME, "_fun_%s_%u",
                     global_name_gen->curr_function,
                     global_name_gen->curr_param_count);
        }
        else if (node->data.function_def.type == FUN_G)
        {
            // Creates unique name
            snprintf(global_name_gen->fun_label, MAX_LABEL_NAME, "_getter_%s",
                     global_name_gen->curr_function);
        }
        else
        {
            // Creates unique name
            snprintf(global_name_gen->fun_label, MAX_LABEL_NAME, "_setter_%s",
                     global_name_gen->curr_function);
        }

        break;
    case LOOP_START_L:
        // Have to differentiate between the location of loop: function, setter and getter
        if (global_name_gen->in_function)
        {
            // Creates unique name
            snprintf(global_name_gen->loop_start_label, MAX_LABEL_NAME, "_loop_start_fun_%s_%u_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->curr_param_count,
                     global_name_gen->loop_counter);
        }
        else if (global_name_gen->in_getter)
        {
            // Creates unique name
            snprintf(global_name_gen->loop_start_label, MAX_LABEL_NAME, "_loop_start_getter_%s_0_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }
        else
        {
            // Creates unique name
            snprintf(global_name_gen->loop_start_label, MAX_LABEL_NAME, "_loop_start_setter_%s_1_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }

        // Increments loop counter
        global_name_gen->loop_counter++;

        break;
    case LOOP_END_L:
        // Have to differentiate between the location of loop: function, setter and getter
        if (global_name_gen->in_function)
        {
            // Creates unique name
            snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME, "_loop_end_fun_%s_%u_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->curr_param_count,
                     global_name_gen->loop_counter);
        }
        else if (global_name_gen->in_getter)
        {
            // Creates unique name
            snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME, "_loop_end_getter_%s_0_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }
        else
        {
            // Creates unique name
            snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME, "_loop_end_setter_%s_1_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }

        // dont have to increment since loop_start already incremented the counter

        break;
    case IF_ELSE_L:
        // Have to differentiate between the location of loop: function, setter and getter
        if (global_name_gen->in_function)
        {
            // Creates unique name
            snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "_if_else_fun_%s_%u_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->curr_param_count,
                     global_name_gen->loop_counter);
        }
        else if (global_name_gen->in_getter)
        {
            // Creates unique name
            snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "_if_else_getter_%s_0_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }
        else
        {
            // Creates unique name
            snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "if_else_setter_%s_1_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }

        // Increments if counter
        global_name_gen->if_counter++;

        break;
    case IF_END_L:
        // Have to differentiate between the location of loop: function, setter and getter
        if (global_name_gen->in_function)
        {
            // Creates unique name
            snprintf(global_name_gen->end_if_label, MAX_LABEL_NAME, "_if_end_fun_%s_%u_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->curr_param_count,
                     global_name_gen->loop_counter);
        }
        else if (global_name_gen->in_getter)
        {
            // Creates unique name
            snprintf(global_name_gen->end_if_label, MAX_LABEL_NAME, "_if_end_getter_%s_0_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }
        else
        {
            // Creates unique name
            snprintf(global_name_gen->end_if_label, MAX_LABEL_NAME, "if_end_setter_%s_1_%llu",
                     global_name_gen->curr_function,
                     global_name_gen->loop_counter);
        }

        break;
    case CALL:
        /**
         *                       IMPORTANT
         * This option/case isnt really used the same way as the previous ones.
         * When encountered fun call or getter/setter call in code, we have to
         * know/generate the very same label name as is used for the function definition.
         * This name will be stored in global_name_gen->called_function.
         */

        if (node->type == NODE_CALL)
        {
            snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_fun_%s_%u",
                     node->data.function_call.name,
                     node->data.function_call.param_count);
        }
        else if (node->type == NODE_IDENTIFIER && node->data.identifier.id_type == SETTER)
        {
            snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_setter_%s",
                     node->data.identifier.name);
        }
        else
        { // node->type == NODE_IDENTIFIER && node->data.identifier.id_type == GETTER
            snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_getter_%s",
                     node->data.identifier.name);
        }
        break;
    case MUL:
        snprintf(global_name_gen->mul, MAX_FUNCTION_NAME, "_mul_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case STR_ITER:
        snprintf(global_name_gen->str_iter, MAX_FUNCTION_NAME, "_str_iter_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case MUL_END:
        snprintf(global_name_gen->mul_end, MAX_FUNCTION_NAME, "_mul_end_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case ADD:
        snprintf(global_name_gen->add, MAX_FUNCTION_NAME, "_add_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case CONCAT:
        snprintf(global_name_gen->concat, MAX_FUNCTION_NAME, "_concat_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case ADD_END:
        snprintf(global_name_gen->add_end, MAX_FUNCTION_NAME, "_add_end_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case SUB:
        snprintf(global_name_gen->sub, MAX_FUNCTION_NAME, "_sub_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case EVAL:
        snprintf(global_name_gen->eval, MAX_FUNCTION_NAME, "_eval_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case LOG_END:
        snprintf(global_name_gen->log_end, MAX_FUNCTION_NAME, "_log_end_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case LEFT_TO_FLOAT:
        snprintf(global_name_gen->left_to_float, MAX_FUNCTION_NAME, "_left_to_float_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case RIGHT_TO_FLOAT:
        snprintf(global_name_gen->right_to_float, MAX_FUNCTION_NAME, "_right_to_float_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case BOTH_TO_FLOAT:
        snprintf(global_name_gen->both_to_float, MAX_FUNCTION_NAME, "_both_to_float_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case ZERO_DIV_CHECK_FLOAT:
        snprintf(global_name_gen->zero_div_check_float, MAX_FUNCTION_NAME, "_zero_div_check_float_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case ZERO_DIV_CHECK_INT:
        snprintf(global_name_gen->zero_div_check_int, MAX_FUNCTION_NAME, "_zero_div_check_int_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;
    case ZERO_DIV_CHECK_DONE:
        snprintf(global_name_gen->zero_div_check_done, MAX_FUNCTION_NAME, "_zero_div_check_done_%s_%u_%lld",
                 global_name_gen->curr_function, global_name_gen->curr_param_count, global_name_gen->bin_op_counter);
        break;

    default:
        break;
    }
}

/**
 * @brief Helper function that generates all label names that are needed inside expression evaluation codes.
 */
void create_label_names(ASTNode_ptr exp_node)
{
    create_unique_name(exp_node, MUL);
    create_unique_name(exp_node, STR_ITER);
    create_unique_name(exp_node, MUL_END);
    create_unique_name(exp_node, ADD);
    create_unique_name(exp_node, CONCAT);
    create_unique_name(exp_node, ADD_END);
    create_unique_name(exp_node, SUB);
    create_unique_name(exp_node, EVAL);
    create_unique_name(exp_node, LOG_END);
    create_unique_name(exp_node, LEFT_TO_FLOAT);
    create_unique_name(exp_node, RIGHT_TO_FLOAT);
    create_unique_name(exp_node, BOTH_TO_FLOAT);
    create_unique_name(exp_node, ZERO_DIV_CHECK_FLOAT);
    create_unique_name(exp_node, ZERO_DIV_CHECK_INT);
    create_unique_name(exp_node, ZERO_DIV_CHECK_DONE);
}

/**
 * @brief Traverses the expression AST subtree using the postorder traversal and evaluates each binary operation of the expression.
 *        The postorder traversal simulates the postfix notation. All results of evaluations are pushed to the data stack.
 *
 * @note Pushing the results to the data stack is handled by the eval_bin_op function.
 *
 * @param exp_node Root of the expression subtree.
 */
void eval_exp(ASTNode_ptr exp_node)
{
    for (unsigned idx = 0; idx < exp_node->child_count; idx++)
    {
        /*
            NOTE: Tree traversal is ended so fun call args are not pushed to the data stack twice.
            If break is not called, eval_exp will find the args of the function call, identify them as
            an identifier or a literal and push them to the data stack. Then function call is generated
            which is going to push these args again.
        */
        if (exp_node->type == NODE_CALL)
            break;

        eval_exp(exp_node->children[idx]);
    }

    if (exp_node->type == NODE_BINARY_OP || exp_node->type == NODE_RANGE)
    {
        global_name_gen->bin_op_counter++;
        create_label_names(exp_node);
        eval_bin_op(exp_node);
    }
    else if (exp_node->type == NODE_IDENTIFIER) // name mangled idents are already inside the ast nodes
    {
        if (exp_node->data.identifier.id_type == GETTER)
            gen_jmp_function(exp_node);
        else
        {
            gen_push_variable(exp_node);
        }
    }
    else if (exp_node->type == NODE_STR_LIT)
        gen_lit_string(exp_node->data.literal.data.str_value); // Calls the function on escape
    else if (exp_node->type == NODE_INT_LIT)
        printf("PUSHS int@%lld\n", exp_node->data.literal.data.int_val);
    else if (exp_node->type == NODE_FLOAT_LIT)
        printf("PUSHS float@%La\n", exp_node->data.literal.data.float_val);
    else if (exp_node->type == NODE_NULL_LIT)
        printf("PUSHS nil@nil\n");
    else if (exp_node->type == NODE_CALL)
        gen_jmp_function(exp_node);

    // NOTE: nothing has to be done for NODE_TYPE_LIT because I don't actually need it when evaluating IS
}

/**
 * @brief Generates code that defines helper variables used for expression evaluation.
 */
void gen_exp_helpers()
{
    printf("DEFVAR LF@op1\n");
    printf("DEFVAR LF@op2\n");
    printf("DEFVAR LF@type1\n");
    printf("DEFVAR LF@type2\n");
    printf("DEFVAR LF@op_check1\n");
    printf("DEFVAR LF@op_check2\n");
    printf("DEFVAR LF@type_check_res\n");
    printf("DEFVAR LF@result\n");
}

/**
 * @brief Decides what binary op eval function to call based on the provided operator.
 *
 * @param operator Pointer to the operator node.
 */
void eval_bin_op(ASTNode_ptr operator)
{

    // creates a local frame for the exp evaluation
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    gen_exp_helpers(); // defines all helper variables that are needed

    if (operator->data.binary_operator.op_type == OP_IS)
    {
        /*
            IS eval only needs the left side of the expression. Deciding whether the keyword was Num, String or Null is done in compile time so less
            instructions are needed during evaluation. Because of this there is no need to push any information about the keyword itself to the data stack.
            If IS operator is detected I will only pop once to access the left side of the expression. No other data stack pops are performed to prevent popping
            an empty data stack.
        */
        printf("POPS LF@op1\n"); // left side of the IS expression
    }
    else
    {
        // retrieve both operands from the data stack
        printf("POPS LF@op2\n"); // second operand
        printf("POPS LF@op1\n"); // first operand
    }

    if (operator->type == NODE_RANGE)
    {
        gen_eval_range_op();
    }
    else // other binary operators
    {
        // based on the operator type a different version of binary op eval is generated
        switch (operator->data.binary_operator.op_type)
        {
        case OP_PLUS:
            gen_eval_plus_op();
            break;
        case OP_MINUS:
            gen_eval_minus_op();
            break;
        case OP_MUL:
            gen_eval_star_op();
            break;
        case OP_DIV:
            gen_eval_slash_op();
            break;
        case OP_EQ:
        case OP_NEQ:
        case OP_GT:
        case OP_GTE:
        case OP_LT:
        case OP_LTE:
        case OP_IS:
            gen_eval_logical_op(operator);
            break;
        default: // not a valid operator
            error_exit(ERR_INTERNAL);
            break;
        }

        printf("PUSHS LF@result\n"); // result of the expression evaluation
    }

    // NOTE: PUSHS does not have to be generated for the eval of node range

    // cleanup after evaluating the expression
    printf("POPFRAME\n");
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses logical operators.
 *        Based on the provided type of the logical operator, different versions of this function can be generated
 *        that are specific for the current logical operator.
 *
 * @param operator Pointer to the node that holds the binary operator of the expression.
 *
 * @note Helper variables that are used in instructions generated in this function are going to
 *       be defined outside these helper functions a will have a separate frame made for them.
 */
void gen_eval_logical_op(ASTNode_ptr operator)
{
    switch (operator->data.binary_operator.op_type)
    {
    case OP_EQ:
    case OP_NEQ:
        gen_eval_equal_not_equal(&operator->data.binary_operator.op_type);
        break;
    case OP_GT:
    case OP_GTE:
    case OP_LT:
    case OP_LTE:
        gen_eval_greater_lower(&operator->data.binary_operator.op_type);
        break;
    case OP_IS:
        gen_eval_is(operator);
        break;
    default:
        break;
    }
}

/**
 * @brief Generates instructions to evaluate an operation that uses the is operator.
 *
 * @param operator Pointer to the node that holds the is operator.
 *
 * @note variables that are used inside this function were defined inside the gen_eval_logical_op
 */
void gen_eval_is(ASTNode_ptr operator)
{
    // NOTE: based on the ast structure convetion that we agreed on I always know that children[1] is the right side of the expression

    printf("TYPE LF@type1 LF@op1\n"); // I always have to aquire the data type of the left operand

    // I need to access the type keyword inside the expression and generate evaluation based on the keyword
    if (strcmp(operator->children[1]->data.literal.data.str_value, "Num") == 0)
    {
        // i use op_check1 and op_check2 as helper variables to store both bool values of the first two comparisons
        printf("EQ LF@op_check1 LF@type1 string@int\n");
        printf("EQ LF@op_check2 LF@type1 string@float\n");

        printf("OR LF@result LF@op_check1 LF@op_check2\n"); // is it int OR float?
    }
    else if (strcmp(operator->children[1]->data.literal.data.str_value, "String") == 0)
    {
        printf("EQ LF@result LF@type1 string@string\n");
    }
    else if (strcmp(operator->children[1]->data.literal.data.str_value, "Null") == 0)
    {
        printf("EQ LF@result LF@type1 string@nil\n");
    }
}

/**
 * @brief Generates instructions to evaluate an operation that uses ==, != operators.
 *
 * @param op_type Based on this value I will either generate EQ or NEQ instruction at the end of the evaluation.
 *
 * @note variables that are used inside this function were defined inside the gen_eval_logical_op
 */
void gen_eval_equal_not_equal(operator_types *op_type)
{
    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // checks different op type combinations and evaluates based on the current combination

    // string op string
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@string\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // int op int
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // float op float
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // bool op bool
    printf("EQ LF@op_check1 LF@type1 string@bool\n");
    printf("EQ LF@op_check2 LF@type2 string@bool\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // nil op nil
    printf("EQ LF@op_check1 LF@type1 string@nil\n");
    printf("EQ LF@op_check2 LF@type2 string@nil\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // operands are of different types so we can just return false
    printf("MOVE LF@result bool@false\n");
    printf("JUMP %s\n", global_name_gen->log_end);

    // evaluate expressions
    printf("\nLABEL %s\n", global_name_gen->eval);
    printf("EQ LF@result LF@op1 LF@op2\n");

    // push the result to the data stack and clean up
    printf("\nLABEL %s\n", global_name_gen->log_end);

    if (*op_type == OP_NEQ) // i can just negate the current result if needed
        printf("NOT LF@result LF@result\n");
}

/**
 * @brief Generates instructions to evaluate an operation that uses ==, != operators.
 *
 * @param op_type Based on this value I will either generate EQ or NEQ instruction at the end of the evaluation.
 *
 * @note variables that are used inside this function were defined inside the gen_eval_logical_op
 */
void gen_eval_greater_lower(operator_types *op_type)
{
    // operand data was aquired already

    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // compare different valid operand combinations

    // int op int
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // float op float
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // float op int
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->right_to_float);

    // int op float
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->left_to_float);

    // no valid operand combination detected
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversion - right op
    printf("\nLABEL %s\n", global_name_gen->right_to_float); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP %s\n", global_name_gen->eval);

    // int to float conversion - left op
    printf("\nLABEL %s\n", global_name_gen->left_to_float); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    printf("\nLABEL %s\n", global_name_gen->eval); // label

    // based on different types of operators, different variant of the eval code block will be generated

    if (*op_type == OP_LT)
    {
        printf("LT LF@result LF@op1 LF@op2\n"); // num1 < num2
    }
    else if (*op_type == OP_GT)
    {
        printf("GT LF@result LF@op1 LF@op2\n"); // num1 > num2
    }
    // for LTE and GTE I have to evaluate it like so :  LTE (num1 < num2 || num1 == num2) GTE(num1 > num2 || num1 == num2)
    else if (*op_type == OP_LTE)
    {
        // here i use op_check1 and op_check2 as helper variables to store both bool values of num1 < num2, num1 == num2
        printf("LT LF@op_check1 LF@op1 LF@op2\n");          // num1 < num2
        printf("EQ LF@op_check2 LF@op1 LF@op2\n");          // num1 == num2
        printf("OR LF@result LF@op_check1 LF@op_check2\n"); // (num1 < num2 || num1 == num2)
    }
    else if (*op_type == OP_GTE)
    {
        // here i use op_check1 and op_check2 as helper variables to store both bool values of num1 < num2, num1 == num2
        printf("GT LF@op_check1 LF@op1 LF@op2\n");          // num1 > num2
        printf("EQ LF@op_check2 LF@op1 LF@op2\n");          // num1 == num2
        printf("OR LF@result LF@op_check1 LF@op_check2\n"); // (num1 > num2 || num1 == num2)
    }
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the * operator
 */
void gen_eval_star_op()
{
    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // here we compare the two operands and try to match a valid operation scenario

    // float * float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->mul);

    // int * int scenarion
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->mul);

    // string * int scenario
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->str_iter);

    // float * int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->right_to_float);

    // int * float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->left_to_float);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("\nLABEL %s\n", global_name_gen->right_to_float); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP %s\n", global_name_gen->mul);

    // int to float conversions - left op
    printf("\nLABEL %s\n", global_name_gen->left_to_float); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // multiplication
    printf("\nLABEL %s\n", global_name_gen->mul);
    printf("MUL LF@result LF@op1 LF@op2\n");
    printf("JUMP %s\n", global_name_gen->mul_end);

    // string iter
    printf("\nLABEL %s\n", global_name_gen->str_iter);
    // todo : najst v historii commitov string iter

    // no need to jump here

    // end of the function that handles the * operator
    printf("\nLABEL %s\n", global_name_gen->mul_end);
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the / operator.
 */
void gen_eval_slash_op()
{
    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // float / float scenario
    printf("\n");
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->eval);

    // int / int scenario
    printf("\n");
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->both_to_float);

    // float / int scenario
    printf("\n");
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->right_to_float);

    // int / float scenario
    printf("\n");
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->left_to_float);

    // none of valid the scenarios was matched, type error occured
    printf("\n");
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("\n");
    printf("LABEL %s\n", global_name_gen->right_to_float);
    printf("INT2FLOAT LF@op2 LF@op2\n");

    // int to float conversions - left op
    printf("\n");
    printf("LABEL %s\n", global_name_gen->left_to_float);
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // int to float conversions - both ops
    printf("\n");
    printf("LABEL %s\n", global_name_gen->both_to_float);
    printf("INT2FLOAT LF@op1 LF@op1\n");
    printf("INT2FLOAT LF@op2 LF@op2\n");

    printf("\n");
    printf("LABEL %s\n", global_name_gen->eval);

    // division
    printf("DIV LF@result LF@op1 LF@op2\n");
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the - operator.
 */
void gen_eval_minus_op()
{
    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // float - float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->sub);

    // int - int scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->sub);

    // float - int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->right_to_float);

    // int - float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->left_to_float);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("\nLABEL %s\n", global_name_gen->right_to_float);
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP %s\n", global_name_gen->sub);

    // int to float conversions - left op
    printf("\nLABEL %s\n", global_name_gen->left_to_float);
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to jump here

    // subtraction
    printf("\nLABEL %s\n", global_name_gen->sub);
    printf("SUB LF@result LF@op1 LF@op2\n");
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the + operator.
 *
 * @note Helper variables that are used in instructions generated in this function are going to
 *       be defined outside these helper functions a will have a separate frame made for them.
 */
void gen_eval_plus_op()
{
    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // here we compare the two operands and try to match a valid operation scenario

    // float + float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->add);

    // int + int scenarion
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->add);

    // string + string scenario
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@string\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->concat);

    // float + int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->right_to_float);

    // int + float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("AND LF@type_check_res LF@op_check1 LF@op_check2\n");
    printf("JUMPIFEQ %s LF@type_check_res bool@true\n", global_name_gen->left_to_float);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("\nLABEL %s\n", global_name_gen->right_to_float); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP %s\n", global_name_gen->add);

    // int to float conversions - left op
    printf("\nLABEL %s\n", global_name_gen->left_to_float); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to use jump here

    // addition
    printf("\nLABEL %s\n", global_name_gen->add); // lable
    printf("ADD LF@result LF@op1 LF@op2\n");
    printf("JUMP %s\n", global_name_gen->add_end);

    // concat
    printf("\nLABEL %s\n", global_name_gen->concat); // lable
    printf("CONCAT LF@result LF@op1 LF@op2\n");

    // end of the function that handles the + operator
    printf("\nLABEL %s\n", global_name_gen->add_end);
}

/**
 * @brief Generates instructions to type check an operation that uses the range operator.
 */
void gen_eval_range_op()
{
    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // check if both sides of the expression are an int value
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("JUMPIFEQ !ERROR_EXP_L LF@op_check1 bool@false\n");

    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ !ERROR_EXP_L LF@op_check2 bool@false\n");

    // check if left side is not the bigger number
    printf("GT LF@op_check1 LF@op1 LF@op2\n");
    printf("JUMPIFEQ !ERROR_EXP_L LF@op_check1 bool@true\n");

    // now i can just push the starting and ending iterator values back to the scope stack
    printf("PUSHS LF@op1\n");
    printf("PUSHS LF@op2\n");
}

/**
 * @brief Memory clean up for global instance of name_generator_ptr object
 *
 * @param global_name_gen
 */
void free_global_name_gen(name_generator_ptr global_name_gen)
{
    // Free memory used by its strings
    free(global_name_gen->fun_label);
    free(global_name_gen->else_block_label);
    free(global_name_gen->end_if_label);
    free(global_name_gen->loop_start_label);
    free(global_name_gen->loop_end_label);
    free(global_name_gen->curr_function);
    free(global_name_gen->called_function);
    free(global_name_gen->mul);
    free(global_name_gen->str_iter);
    free(global_name_gen->mul_end);
    free(global_name_gen->add);
    free(global_name_gen->concat);
    free(global_name_gen->add_end);
    free(global_name_gen->sub);
    free(global_name_gen->eval);
    free(global_name_gen->log_end);
    free(global_name_gen->left_to_float);
    free(global_name_gen->right_to_float);
    free(global_name_gen->both_to_float);
    free(global_name_gen->zero_div_check_float);
    free(global_name_gen->zero_div_check_int);
    free(global_name_gen->zero_div_check_done);

    // Make sure this address wont be derreferenced again
    global_name_gen->fun_label = NULL;
    global_name_gen->else_block_label = NULL;
    global_name_gen->end_if_label = NULL;
    global_name_gen->loop_start_label = NULL;
    global_name_gen->loop_end_label = NULL;
    global_name_gen->curr_function = NULL;
    global_name_gen->called_function = NULL;
    global_name_gen->mul = NULL;
    global_name_gen->str_iter = NULL;
    global_name_gen->mul_end = NULL;
    global_name_gen->add = NULL;
    global_name_gen->concat = NULL;
    global_name_gen->add_end = NULL;
    global_name_gen->sub = NULL;
    global_name_gen->eval = NULL;
    global_name_gen->log_end = NULL;
    global_name_gen->left_to_float = NULL;
    global_name_gen->right_to_float = NULL;
    global_name_gen->both_to_float = NULL;
    global_name_gen->zero_div_check_float = NULL;
    global_name_gen->zero_div_check_int = NULL;
    global_name_gen->zero_div_check_done = NULL;

    // Free the vole oject
    free(global_name_gen);
}
