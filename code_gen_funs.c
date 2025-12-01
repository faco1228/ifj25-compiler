/**
 * @file code_gen_funs.c
 * @author xracekm00, xmezeim00
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
#include "code_gen_funs.h"
#include "error.h"

/* NOTES with useful information

Flags available for each Expression subtree:
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

RUNTIME SEMANTIC:
    zero_division:
    exp_type_check:

EXTENSTION
    cycles
    funexp

NOTE:
Pri kazdej jednej operacii treba robit typove kontroly, jednak kvoli tomu, ze ADD potrebuje 2 int alebo 2 float
ale aj ci to sedi ked niekotra premenna je return value funkcie a niektora moze byt napr read at runtime

NOTE:
Budem musiet uchovavat informaciu o tom, kolko ramcov je na zasobniku resp kolko dat je na datovom zasobniku,
lebo ked chcem citat z prazdneho zasobniku tak dojde k chybe.
*/

// Global varialbe necesary for almost all functions below
name_generator_ptr global_name_gen = NULL;
// Global flag, holds information whether the function contained return node
bool return_occured = false;

/**
 * @brief The main code-gen function - contains switch for all different types of nodes
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

    // Recursivelly processing each node of AST
    switch (node->type)
    {
    case NODE_PROGRAM:
        // Each code in IFJcode25 starts with this line
        printf(".IFJcode25\n");

        // Skipping built in functions defined at the beginning of each program
        printf("JUMP _program_start\n");
        printf("\n");

        // At the beginning of the program, there will be created builtin functions
        // NOTE: kristian, labels should have format "%*Ifj.<name>", just dont use $
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

        // This is where actuall compilation begins
        printf("\n");
        printf("LABEL _program_start\n");

        break;
    case NODE_FUNCTION_DEF:
        break;
    case NODE_BLOCK:
        break;
    case NODE_VAR_DECL:
        break;
    case NODE_ASSIGN:
        break;
    case NODE_IF:
        break;
    case NODE_RETURN:
        break;
    case NODE_WHILE:
        break;
    case NODE_FOR:
        break;
    case NODE_BREAK:
        break;
    case NODE_CONTINUE:
        break;
    case NODE_EXPR_STMNT:
        break;
    case NODE_IDENTIFIER:
        break;
    case NODE_BINARY_OP:
        break;
    case NODE_RANGE:
        break;
    case NODE_INT_LIT:
        break;
    case NODE_FLOAT_LIT:
        break;
    case NODE_STR_LIT:
        break;
    case NODE_NULL_LIT:
        break;
    default:
        break;
    }

    // Iterate throuh all children
    for (unsigned i = 0; i < node->child_count; i++)
    {
        codegen(node->children[i]);
    }

    // TO DO: just wrote down the idea
    /**
     * @brief
     *
     */
    if (node->type == NODE_FUNCTION_DEF)
    {
        gen_func_end();
    }
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
    global_name_gen->label_counter = 0;
    global_name_gen->temp_var_counter = 0;
    global_name_gen->loop_counter = 0;
    global_name_gen->if_counter = 0;
    global_name_gen->bin_op_counter = 0;

    strcpy(global_name_gen->curr_function, node->data.function_def.name);
    memset(global_name_gen->label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_end_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_start_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->temp_var, 0, MAX_LABEL_NAME);
    memset(global_name_gen->if_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->add_label, 0, MAX_LABEL_NAME);

    global_name_gen->curr_param_count = node->data.function_def.arg_count;

    global_name_gen->in_function = true;
    global_name_gen->in_loop = false;

    global_name_gen->stakck_depth = 0;
}

/**
 * @brief Generates code for string iteration (string * num)
 *
 * @param name_gen
 *
 * @note Expects arguments on data stack (Pascal convention)
 *       Returns result-string on stack
 */
void gen_string_iter(ASTNode_ptr node)
{
    // Generates unique variable name
    create_unique_name(node, TEMP_VAR);
    char count[MAX_LABEL_NAME];
    strcpy(count, global_name_gen->temp_var);

    create_unique_name(global_name_gen, TEMP_VAR);
    char string[MAX_LABEL_NAME];
    strcpy(string, global_name_gen->temp_var);

    create_unique_name(global_name_gen, TEMP_VAR);
    char result[MAX_LABEL_NAME];
    strcpy(result, global_name_gen->temp_var);

    create_unique_name(global_name_gen, TEMP_VAR);
    char counter[MAX_LABEL_NAME];
    strcpy(counter, global_name_gen->temp_var);

    // Generates loop
    create_unique_name(global_name_gen, LOOP_START_L);
    char loop_start[MAX_LABEL_NAME];
    strcpy(loop_start, global_name_gen->loop_start_label);

    create_unique_name(global_name_gen, LOOP_END_L);
    char loop_end[MAX_LABEL_NAME];
    strcpy(loop_end, global_name_gen->loop_end_label);

    // Define variables necesary to perform string iteration
    printf("DEFVAR LF@%s\n", count);
    printf("DEFVAR LF@%s\n", string);
    printf("DEFVAR LF@%s\n", result);
    printf("DEFVAR LF@%s\n", counter);

    // Assigns values of the two parameters from data stack to corresponding variables
    printf("POPS LF@%s\n", count);  // count
    printf("POPS LF@%s\n", string); // string

    // Initialize result as empty string
    printf("MOVE LF@%s string@\n", result);

    // Initialize counter to 0
    printf("MOVE LF@%s int@0\n", counter);

    // Loop start
    printf("LABEL %s\n", loop_start);

    // If counter == count, jump to end
    printf("JUMPIFEQ %s LF@%s LF@%s\n", loop_end, counter, count);

    // Concatenate result with string
    printf("CONCAT LF@%s LF@%s LF@%s\n", result, result, string);

    // Increment counter
    printf("ADD LF@%s LF@%s int@1\n", counter, counter);

    // Repeat loop
    printf("JUMP %s\n", loop_start);

    // Loop end
    printf("LABEL %s\n", loop_end);

    // Push result on stack
    printf("PUSHS LF@%s\n", result);
}

/**
 * @brief Generates unieque label name using name-mangeling
 *
 * @note These options can be used LABEL, LOOP_START_L, LOOP_END_L, IF_TRUE, IF_FALSE
 *
 * @param option
 */
void gen_label(name_option_t option)
{
    create_unique_name(global_name_gen, option);
    switch (option)
    {
    case LABEL:
        printf("LABEL %s\n", global_name_gen->label);
        break;
    case TEMP_VAR:
        printf("LABEL %s\n", global_name_gen->temp_var);
        break;
    case LOOP_START_L:
        printf("LABEL %s\n", global_name_gen->loop_start_label);
        break;
    case LOOP_END_L:
        printf("LABEL %s\n", global_name_gen->loop_end_label);
        break;
    case IF_TRUE:
    case IF_FALSE:
        printf("LABEL %s\n", global_name_gen->if_label);
        break;
    default:
        break;
    }
}

/**
 * @brief Generates unieque variable name using name-mangeling
 *
 * @note These options can be used TEMP_VAR
 *
 * @param option
 */
void gen_variable(ASTNode_ptr node)
{
    if (node->data.identifier.is_global)
    {
        printf("PUSHS GF@%s\n", node->data.identifier.name);
    }
    else
    {
        printf("PUSHS LF@%s\n", node->data.identifier.name);
    }

    global_name_gen->stakck_depth++;
}

/**
 * @brief Handles stert of function
 *
 * @note Called after entering NODE_FUNCTION_DEF node
 *
 * @param node
 */
void gen_func_start(ASTNode_ptr node)
{
    // Reset the name generator
    name_gen_init(node);

    // Generate function start

    gen_label(LABEL); // Creates and print unique function label name,
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
}

/**
 * @brief Handles end of function
 *
 * @note Called when childeren array is empty
 *
 */
void gen_func_end()
{
    // Handeling return value via data stack
    if (return_occured)
    {
        gen_return();
    }
    else
    {
        printf("pushs nil@nil\n");
    }

    // Generate function end
    printf("POPFRAME\n");
    printf("RETURN\n");
}

/**
 * @brief Handles return values of a function
 *
 * @note uses data stack to retrun the result
 *
 */
void gen_return()
{
}

/**
 * @brief Calls functions generating literals based on the option
 *
 * @param option
 * @param value
 */
void gen_literal(literal_option_t option, literal_values_t value)
{
    // Different options that can be used as argument
    switch (option)
    {
    case INTEGER:
        gen_lit_int(value.int_value);
        break;
    case FLOAT:
        gen_lit_float(value.float_value);
        break;
    case BOOL:
        gen_lit_bool(value.bool_value);
        break;
    case STRING:
        gen_lit_string(value.string_value);
        break;
    case NILL:
        gen_lit_null();
        break;
    default:
        break;
    }
}

/**
 * @brief Generates code for integer literal
 *
 * @param value
 */
void gen_lit_int(long long value)
{
    printf("PUSHS int@%lld\n", value);
}

/**
 * @brief Generates code for float literal
 *
 * @param value
 */
void gen_lit_float(long double value)
{
    printf("PUSHS float@%a\n", value);
}

/**
 * @brief Generates code for float literal
 *
 * @param value
 */
void gen_lit_null()
{
    printf("PUSHS nil@nil\n");
}

/**
 * @brief Generates code for float literal
 *
 * @param value The float value to push on stack
 */
void gen_lit_bool(bool value)
{
    if (value)
    {
        printf("PUSHS bool@true\n");
    }
    else
    {
        printf("PUSHS bool@false\n");
    }
}

/**
 * @brief Generates code for float literal
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
        if (is_invalid_char(value[i]))
        {
            new_str_index += sprintf(&correct_value[new_str_index], "\\0%d", value[i]);
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
 * @brief Generates code for built in functions
 *
 * @note will be called when entered function call node and is_built_in == true
 *
 * @param node
 */
void gen_jmp_builtin(ASTNode_ptr node)
{
    // Based on the current node, jump will be performed

    if (!strcmp(node->data.function_call.name, "Ifj.read_str"))
    {
        printf("CALL %%*Ifj.read_str\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.read_num"))
    {
        printf("CALL %%*Ifj.read_num\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.write"))
    {
        printf("CALL %%*Ifj.write\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.floor"))
    {
        printf("CALL %%*Ifj.floor\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.str"))
    {
        printf("CALL %%*Ifj.str\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.length"))
    {
        printf("CALL %%*Ifj.length\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.substring"))
    {
        printf("CALL %%*Ifj.substring\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.strcmp"))
    {
        printf("CALL %%*Ifj.strcmp\n");
    }
    else if (!strcmp(node->data.function_call.name, "Ifj.ord"))
    {
        printf("CALL %%*Ifj.ord\n");
    }
    else
    {
        printf("CALL %%*Ifj.chr\n");
    }
};

/**
 * @brief Create a unique string which is stored in
 *
 * @param node
 * @param option
 */
void create_unique_name(ASTNode_ptr node, name_option_t option)
{
    // Name mangeling variable to differentiate between function a gett/setter
    char fun_type;

    // Different options that can be used as argument
    switch (option)
    {
    case LABEL:

        if (node->data.function_def.type == FUN_F)
        {
            fun_type = 'f';
        }
        else if (node->data.function_def.type == FUN_G)
        {
            fun_type = 'g';
        }
        else
        {
            fun_type = 's';
        }

        snprintf(global_name_gen->label, MAX_LABEL_NAME,
                 "%%*%s_%u_%s_%llu",
                 global_name_gen->curr_function,
                 global_name_gen->curr_param_count,
                 fun_type,
                 global_name_gen->label_counter);

        global_name_gen->label_counter++;

        break;
    case TEMP_VAR:
        snprintf(global_name_gen->temp_var, MAX_LABEL_NAME,
                 "__temp_var_%s_%u_%llu",
                 global_name_gen->curr_function,
                 global_name_gen->curr_param_count,
                 global_name_gen->temp_var_counter);

        global_name_gen->temp_var_counter++;

        break;
    case LOOP_START_L:
        snprintf(global_name_gen->loop_start_label, MAX_LABEL_NAME,
                 "#_loop_start_%s_%u_%llu",
                 global_name_gen->curr_function,
                 global_name_gen->curr_param_count,
                 global_name_gen->loop_counter);

        global_name_gen->loop_counter++;

        break;
    case LOOP_END_L:
        snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME,
                 "#_loop_end_%s_%u_%llu",
                 global_name_gen->curr_function,
                 global_name_gen->curr_param_count,
                 global_name_gen->loop_counter);

        // dont have to increment since loop_start already incremented the counter
        // global_name_gen->label_counter++;

        break;
    case IF_TRUE:
        snprintf(global_name_gen->if_label, MAX_LABEL_NAME,
                 "#_if_true_%s_%u_%llu",
                 global_name_gen->curr_function,
                 global_name_gen->curr_param_count,
                 global_name_gen->if_counter);

        global_name_gen->if_counter++;

        break;

    case IF_FALSE:
        snprintf(global_name_gen->if_label, MAX_LABEL_NAME,
                 "#_else_%s_%u_%llu",
                 global_name_gen->curr_function,
                 global_name_gen->curr_param_count,
                 global_name_gen->if_counter);

        // dont have to increment since if_true already incremented the counter
        // name_gen->if_counter++;

        break;
    default:
        break;
    }
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
    if (exp_node->child_count == 0)
        return;

    eval_exp(exp_node->children[0]); // left subtree
    eval_exp(exp_node->children[1]); // right subtree

    if (exp_node->type == NODE_BINARY_OP)
    {
        global_name_gen->bin_op_counter++;
        eval_bin_op(exp_node);
    }
    else if (exp_node->type == NODE_IDENTIFIER) // name mangled idents are already inside the ast nodes
    {
        if (exp_node->data.identifier.id_type == GETTER)
            gen_jmp_function(exp_node);
        else
            gen_variable(exp_node);
    }
    else if (exp_node->type == NODE_STR_LIT)
        printf("PUSHS string@%s", exp_node->data.literal.data.str_value);
    else if (exp_node->type == NODE_INT_LIT)
        printf("PUSHS int@%d", exp_node->data.literal.data.int_val);
    else if (exp_node->type == NODE_FLOAT_LIT)
        printf("PUSHS float@%a", exp_node->data.literal.data.float_val);
    else if (exp_node->type == NODE_NULL_LIT)
        printf("PUSHS nil@nil");
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
    printf("DEFVAR LF@type_check1\n");
    printf("DEFVAR LF@type_check2\n");
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

    if (operator->type == NODE_RANGE) // special case for a range operator
    {
        gen_eval_range_op();
        return;
    }

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

    // result of the expression evaluation
    printf("PUSHS LF@result\n");

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
        gen_eval_equal_not_equal(operator->data.binary_operator.op_type);
        break;
    case OP_GT:
    case OP_GTE:
    case OP_LT:
    case OP_LTE:
        gen_eval_greater_lower(operator->data.binary_operator.op_type);
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
    printf("EQ LF@type_check1 LF@type1 string@string\n");
    printf("EQ LF@type_check2 LF@type2 string@string\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // int op int
    printf("EQ LF@type_check1 LF@type1 string@int\n");
    printf("EQ LF@type_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // float op float
    printf("EQ LF@type_check1 LF@type1 string@float\n");
    printf("EQ LF@type_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // bool op bool
    printf("EQ LF@type_check1 LF@type1 string@bool\n");
    printf("EQ LF@type_check2 LF@type2 string@bool\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // nil op nil
    printf("EQ LF@type_check1 LF@type1 string@nil\n");
    printf("EQ LF@type_check2 LF@type2 string@nil\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // operands are of different types so we can just return false
    printf("MOVE LF@result bool@false\n");
    printf("JUMP &logical_end%d\n", global_name_gen->bin_op_counter);

    // evaluate expressions
    printf("LABEL eval%d\n");
    printf("EQ LF@result LF@op1 LF@op2\n");

    // push the result to the data stack and clean up
    printf("LABEL &logical_end%d\n" ,global_name_gen->bin_op_counter);

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
    printf("EQ LF@type_check1 LF@type1 string@int\n");
    printf("EQ LF@type_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // float op float
    printf("EQ LF@type_check1 LF@type1 string@float\n");
    printf("EQ LF@type_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &eval%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // float op int
    printf("EQ LF@type_check1 LF@type1 string@float\n");
    printf("EQ LF@type_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // int op float
    printf("EQ LF@type_check1 LF@type1 string@int\n");
    printf("EQ LF@type_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float%d LF@type_check1 LF@type_check2\n", global_name_gen->bin_op_counter);

    // int to float conversion - right op
    printf("LABEL &right_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &eval%d\n", global_name_gen->bin_op_counter);

    // int to float conversion - left op
    printf("LABEL &left_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    printf("LABEL &eval%d\n", global_name_gen->bin_op_counter); // label

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
        printf("EQ LF@op_check1 LF@op1 LF@op2\n");          // num1 == num2
        printf("OR LF@result LF@op_check1 LF@op_check2\n"); // (num1 < num2 || num1 == num2)
    }
    else if (*op_type == OP_GTE)
    {
        // here i use op_check1 and op_check2 as helper variables to store both bool values of num1 < num2, num1 == num2
        printf("GT LF@op_check1 LF@op1 LF@op2\n");          // num1 > num2
        printf("EQ LF@op_check1 LF@op1 LF@op2\n");          // num1 == num2
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
    printf("JUMPIFEQ &mul%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // int * int scenarion
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &mul%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // string * int scenario
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &str_iter%d LF@op_check1 LF@op_check2\n" ,global_name_gen->bin_op_counter);

    // float * int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float%d LF@op_check1 LF@op_check2\n" ,global_name_gen->bin_op_counter);

    // int * float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &mul%d\n", global_name_gen->bin_op_counter);

    // int to float conversions - left op
    printf("LABEL &left_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // multiplication
    printf("LABEL &mul%d\n", global_name_gen->bin_op_counter); // lable
    printf("MUL LF@result LF@op1 LF@op2\n");
    printf("JUMP &mul_end%d\n", global_name_gen->bin_op_counter);

    // string iter
    //  todo: sem vlozit kod pre string iter ale este si to chcem prejst s Martinom

    // no need to jump here

    // end of the function that handles the * operator
    printf("LABEL &mul_end%d\n", global_name_gen->bin_op_counter);
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
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &zero_div_check_float%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // int / int scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &zero_div_check_int%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // float / int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // int / float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &zero_div_check_float%d\n", global_name_gen->bin_op_counter);

    // int to float conversions - left op
    printf("LABEL &left_to_float%d\n" ,global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");
    printf("JUMP &zero_div_check_float%d\n", global_name_gen->bin_op_counter);

    // zero divison check for floats
    printf("LABEL &zero_div_check_float%d\n", global_name_gen->bin_op_counter);
    printf("EQ LF@op_check2 LF@op2 float@0x0p+0\n");
    printf("JUMP &zero_check_done%d\n", global_name_gen->bin_op_counter);

    // zero divison check for ints
    printf("LABEL &zero_div_check_int%d\n", global_name_gen->bin_op_counter);
    printf("EQ LF@op_check2 LF@op2 int@0\n");

    // evaluate zero division check
    printf("LABEL &zero_check_done%d\n", global_name_gen->bin_op_counter);
    printf("JUMPIFEQ !ERROR_EXP_L LF@op_check2 bool@true\n");

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
    printf("JUMPIFEQ &sub%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // int - int scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &sub%d LF@op_check1 LF@op_check2\n" ,global_name_gen->bin_op_counter);

    // float - int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // int - float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float%d LF@op_check1 LF@op_check2\n" ,global_name_gen->bin_op_counter);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float%d\n", global_name_gen->bin_op_counter);
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &sub%d\n" , global_name_gen->bin_op_counter);

    // int to float conversions - left op
    printf("LABEL &left_to_float%d\n" ,global_name_gen->bin_op_counter);
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to jump here

    // subtraction
    printf("LABEL &sub%d\n" ,global_name_gen->bin_op_counter);
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
    printf("JUMPIFEQ &add%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // int + int scenarion
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &add%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // string + string scenario
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@string\n");
    printf("JUMPIFEQ &concat%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // float + int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float%d LF@op_check1 LF@op_check2\n" , global_name_gen->bin_op_counter);

    // int + float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float%d LF@op_check1 LF@op_check2\n", global_name_gen->bin_op_counter);

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &add%d\n", global_name_gen->bin_op_counter);

    // int to float conversions - left op
    printf("LABEL &left_to_float%d\n", global_name_gen->bin_op_counter); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to use jump here

    // addition
    printf("LABEL &add%d\n", global_name_gen->bin_op_counter); // lable
    printf("ADD LF@result LF@op1 LF@op2\n");
    printf("JUMP &add_end%d\n", global_name_gen->bin_op_counter);

    // concat
    printf("LABEL &concat%d\n", global_name_gen->bin_op_counter); // lable
    printf("CONCAT LF@result LF@op1 LF@op2\n");

    // end of the function that handles the + operator
    printf("LABEL &add_end%d\n", global_name_gen->bin_op_counter);
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

    // now i can just push the starting and ending iterator values back to the scope stack
    printf("PUSHS LF@op1\n");
    printf("PUSHS LF@op2\n");
}