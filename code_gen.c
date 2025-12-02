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
#include "code_gen.h"
#include "error.h"

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