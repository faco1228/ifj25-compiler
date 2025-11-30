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
    global_name_gen->add_counter = 0;

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
    case ADD_L:
        snprintf(global_name_gen->add_label, MAX_LABEL_NAME,
                 "&add%llu",
                 global_name_gen->add_counter);

    default:
        break;
    }
}

/**
 * @brief Chooses the operation to generate based on the provided operator.
 *
 * @param operator Pointer to the operator node.
 */
void choose_operation(ASTNode_ptr operator)
{
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the * operator
 *
 * @note Helper variables that start with & and are used in instructions generated in this function are going to
 *       be defined outside these helper functions a will have a separate frame made for them.
 */
void gen_eval_star_op()
{
    // retrieve both operands from the data stack
    printf("POPS LF@op2\n"); // second operand
    printf("POPS LF@op1\n"); // first operand

    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // here we compare the two operands and try to match a valid operation scenario

    // float * float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &mul LF@op_check1 LF@op_check2\n");

    // int * int scenarion
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &mul LF@op_check1 LF@op_check2\n");

    // string * int scenario
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &str_iter LF@op_check1 LF@op_check2\n");

    // float * int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float LF@op_check1 LF@op_check2\n");

    // int * float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float LF@op_check1 LF@op_check2\n");

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float\n"); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &mul\n");

    // int to float conversions - left op
    printf("LABEL &left_to_float\n"); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to jump here

    // multiplication
    printf("LABEL &mul\n"); // lable
    printf("MUL LF@result LF@op1 LF@op2\n");
    printf("PUSHS LF@result\n");
    printf("JUMP &mul_end\n");

    // string iter
    //  todo: sem vlozit kod pre string iter ale este si to chcem prejst s Martinom

    // no need to jump here

    // end of the function that handles the * operator
    printf("LABEL &mul_end\n");
    printf("RETURN\n");
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the / operator.
 *
 * @note Helper variables that start with & and are used in instructions generated in this function are going to
 *       be defined outside these helper functions a will have a separate frame made for them.
 */
void gen_eval_slash_op()
{
    // retrieve both operands from the data stack
    printf("POPS LF@op2\n"); // second operand
    printf("POPS LF@op1\n"); // first operand

    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // float / float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &zero_div_check_float LF@op_check1 LF@op_check2\n");

    // int / int scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &zero_div_check_int LF@op_check1 LF@op_check2\n");

    // float / int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float LF@op_check1 LF@op_check2\n");

    // int / float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float LF@op_check1 LF@op_check2\n");

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float\n"); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &zero_div_check_float\n");

    // int to float conversions - left op
    printf("LABEL &left_to_float\n"); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");
    printf("JUMP &zero_div_check_float\n");

    // zero divison check for floats
    printf("LABEL &zero_div_check_float\n");
    printf("EQ LF@op_check2 LF@op2 float@0x0p+0\n");
    printf("JUMP &zero_check_done\n");
    
    // zero divison check for ints
    printf("LABEL &zero_div_check_int\n");
    printf("EQ LF@op_check2 LF@op2 int@0\n");

    // evaluate zero division check
    printf("LABEL &zero_check_done\n");
    printf("JUMPIFEQ !ERROR_EXP_L LF@op_check2 bool@true\n");

    // division
    printf("LABEL &div\n"); // label
    printf("DIV LF@result LF@op1 LF@op2\n");
    printf("PUSHS LF@result\n");

    printf("RETURN\n");
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the - operator.
 *
 * @note Helper variables that start with & and are used in instructions generated in this function are going to
 *       be defined outside these helper functions a will have a separate frame made for them.
 */
void gen_eval_minus_op()
{
    // retrieve both operands from the data stack
    printf("POPS LF@op2\n"); // second operand
    printf("POPS LF@op1\n"); // first operand

    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // float - float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &sub LF@op_check1 LF@op_check2\n");

    // int - int scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &sub LF@op_check1 LF@op_check2\n");

    // float - int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float LF@op_check1 LF@op_check2\n");

    // int - float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float LF@op_check1 LF@op_check2\n");

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float\n"); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &sub\n");

    // int to float conversions - left op
    printf("LABEL &left_to_float\n"); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to jump here

    // subtraction
    printf("LABEL &sub\n"); // lable
    printf("SUB LF@result LF@op1 LF@op2\n");
    printf("PUSHS LF@result\n");

    // end of the function that handles the - operator
    printf("RETURN\n");
}

/**
 * @brief Generates instructions to type check and evaluate an operation that uses the + operator.
 *
 * @note Helper variables that start with & and are used in instructions generated in this function are going to
 *       be defined outside these helper functions a will have a separate frame made for them.
 */
void gen_eval_plus_op()
{
    // NOTE: printnem to podobne ako built in funckie, potom sa pushne na stack operand lavy, potom pravy a ja si ich uz ziskam
    // v ramci tej "funkcie" pre add a ulozim si ich
    // todo: na zaciatku programu este pred tym nez sa zacne realne generovat kod si len vygenerujem cez defvar tie pomocne premenne
    // todo: vytvorim si aj premennu LF@nill_check pre kontrolu toho ci v aritmentickom vyraze nie je nill hodnota
    // todo: potrebujem premennu pre ulozenie vysledku

    // creates the label
    // todo: vymysliet nejaky label nazov ktory bude vhodny

    // retrieve both operands from the data stack
    printf("POPS LF@op2\n"); // second operand
    printf("POPS LF@op1\n"); // first operand

    // get the data types of both operands
    printf("TYPE LF@type1 LF@op1\n"); // data type of the first operand
    printf("TYPE LF@type2 LF@op2\n"); // data type of the second operand

    // here we compare the two operands and try to match a valid operation scenario

    // float + float scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &add LF@op_check1 LF@op_check2\n");

    // int + int scenarion
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &add LF@op_check1 LF@op_check2\n");

    // string + string scenario
    printf("EQ LF@op_check1 LF@type1 string@string\n");
    printf("EQ LF@op_check2 LF@type2 string@string\n");
    printf("JUMPIFEQ &concat LF@op_check1 LF@op_check2\n");

    // float + int scenario
    printf("EQ LF@op_check1 LF@type1 string@float\n");
    printf("EQ LF@op_check2 LF@type2 string@int\n");
    printf("JUMPIFEQ &right_to_float LF@op_check1 LF@op_check2\n");

    // int + float scenario
    printf("EQ LF@op_check1 LF@type1 string@int\n");
    printf("EQ LF@op_check2 LF@type2 string@float\n");
    printf("JUMPIFEQ &left_to_float LF@op_check1 LF@op_check2\n");

    // none of valid the scenarios was matched, type error occured
    printf("JUMP !ERROR_EXP_L\n");

    // int to float conversions - right op
    printf("LABEL &right_to_float\n"); // label
    printf("INT2FLOAT LF@op2 LF@op2\n");
    printf("JUMP &add\n");

    // int to float conversions - left op
    printf("LABEL &left_to_float\n"); // label
    printf("INT2FLOAT LF@op1 LF@op1\n");

    // no need to use jump here

    // addition
    printf("LABEL &add\n"); // lable
    printf("ADD LF@result LF@op1 LF@op2\n");
    printf("PUSHS LF@result\n");
    printf("JUMP &add_end\n");

    // concat
    printf("LABEL &concat\n"); // lable
    printf("CONCAT LF@result LF@op1 LF@op2\n");
    printf("PUSHS LF@result\n");

    // no need to use jump here

    // end of the function that handles the + operator
    printf("LABEL &add_end\n");
    printf("RETURN\n");
}