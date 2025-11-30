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

//Global varialbe necesary for almost all functions below
name_generator_ptr global_name_gen = NULL;
//Global flag, holds information whether the function contained return node
bool return_occured = false;

/**
 * @brief The main code-gen function - contains switch for all different types of nodes
 * 
 * @param node 
 */
void codegen(ASTNode_ptr node){
    // End of reccursion
    if (!node){
        return;
    }
    
    // Recursivelly processing each node of AST
    switch (node->type){
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
        // Reset name_gen objects attributes
        name_gen_init(node);

        // Sets flags
        if (node->data.function_def.type == FUN_F){
            global_name_gen->in_function = true;
        }
        else if (node->data.function_def.type == FUN_G){
            global_name_gen->in_getter = true;
        }
        else{
            global_name_gen->in_setter = true;
        }

        // Calls corresponding code generating function
        gen_func_start(node);
        break;
    case NODE_BLOCK:
        // Update block depth counter
        global_name_gen->block_depth_counter++;

        break;
    case NODE_VAR_DECL:
        // Calls corresponding code generating function
        gen_var_decl(node);

        break;
    case NODE_ASSIGN:
        // Calls corresponding code generating function
        gen_assign(node);

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
        // Calls corresponding code generating function based on whether it is variable or getter
        if (node->data.identifier.id_type == VAR){
            gen_variable(node);
        }
        else{
            printf("\n");
            create_unique_name(node, CALL);
            printf("%s\n", global_name_gen->called_function);
        }
        
        break;
    case NODE_BINARY_OP:

        break;
    case NODE_CALL:
        // Based on the function name and whether its builtin or not, corresponding JUMP will be generated
        if (node->data.function_call.is_builtin){
            gen_jmp_builtin(node);
        }
        else{
            printf("\n");
            create_unique_name(node, CALL);
            printf("%s\n", global_name_gen->called_function);
        }

        break;
    case NODE_RANGE:

        break;
    case NODE_INT_LIT:
        // Calls corresponding code generating function
        gen_lit_int(node->data.literal.data.int_val);

        break;
    case NODE_FLOAT_LIT:
        // Calls corresponding code generating function
        gen_lit_int(node->data.literal.data.float_val);

        break;
    case NODE_STR_LIT:
        // Calls corresponding code generating function
        gen_lit_int(node->data.literal.data.str_value);

        break;
    case NODE_NULL_LIT:
        // Calls corresponding code generating function
        gen_lit_null();

        break;
    default:
        break;
    }

    // Iterate throuh all children
    for (unsigned i = 0; i < node->child_count; i++){
        codegen(node->children[i]);
    }

    /**
     * @brief As the recurrsion returns back to the root these if statements
     *        will be executed.
     */
    // Generates function end instructions
    if (node->type == NODE_FUNCTION_DEF){
        gen_func_end();
    }

    // Updates block_depth counter
    if (node->type == NODE_BLOCK){
        global_name_gen->block_depth_counter--;
    }
}

/**
 * @brief sets all name_generator_t attributes to default values
 * 
 * @note used for reset when entering new function_def node
 * 
 * @param node 
 */
void name_gen_init(ASTNode_ptr node){
    // Set counters to default values
    global_name_gen->temp_var_counter = 0;
    global_name_gen->loop_counter = 0;
    global_name_gen->if_counter = 0;
    global_name_gen->block_depth_counter = 1; // 1 by default because this function is called when enetring node_func_def

    // Store current function name and parameter count in global_name_gen
    strcpy(global_name_gen->curr_function, node->data.function_def.name);
    global_name_gen->curr_param_count = node->data.function_def.arg_count;
    // Set all allocated strings to empty
    memset(global_name_gen->fun_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_end_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_start_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->temp_var, 0, MAX_LABEL_NAME);
    memset(global_name_gen->if_true_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->if_false_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->called_function, 0, MAX_LABEL_NAME);

    // Set flags to default values
    global_name_gen->in_function = false;
    global_name_gen->in_getter = false;
    global_name_gen->in_setter = false;
    global_name_gen->in_loop = false;

    // Set stack tracker to zero
    global_name_gen->stack_depth = 0;
}

/**
 * @brief Generates unieque fun_label name using name-mangeling
 * 
 * @note These options can be used LABEL, LOOP_START_L, LOOP_END_L, IF_TRUE, IF_FALSE
 * 
 * @param option
 */
void gen_label(name_option_t option){
    create_unique_name(global_name_gen, option);
    switch (option){
        case FUN_LABEL:
            printf("\n");
            printf("LABEL %s\n", global_name_gen->fun_label);
            break;
        case LOOP_START_L:
            printf("\n");
            printf("LABEL %s\n", global_name_gen->loop_start_label);
            break;
        case LOOP_END_L:
            printf("\n");
            printf("LABEL %s\n", global_name_gen->loop_end_label);
            break;
        case IF_TRUE_L:
            printf("\n");
            printf("LABEL %s\n", global_name_gen->if_true_label);
            break;
        case IF_FALSE_L:
            printf("\n");
            printf("LABEL %s\n", global_name_gen->if_false_label);
            break;
        default:
            break;
    }
}

/**
 * @brief Pushes variable on data stack
 * 
 * @note Used by expression processing functions.
 *       These options can be used VAR, TEMP_VAR.
 * 
 * @param option
 */
void gen_variable(ASTNode_ptr node){
    // Creates uniquq variable name
    create_unique_name(node, VAR);

    if (node->data.identifier.is_global){
        printf("PUSHS GF@%s\n", node->data.identifier.name);
    }
    else{
        printf("PUSHS LF@%s\n", node->data.identifier.name);
    }
    
    global_name_gen->stack_depth++;
}

/**
 * @brief Generates variable declaration
 * 
 * @param node 
 */
void gen_var_decl(ASTNode_ptr node){
    // Creates uniquq variable name
    create_unique_name(node, VAR);

    if (node->data.identifier.is_global) {
        printf("DEFVAR GF@%s\n", node->data.identifier.name);
    } 
    else{
        printf("DEFVAR LF@%s\n", node->data.identifier.name);
    }
}

/**
 * @brief Assigns value to the variable 
 * 
 * @param node 
 */
void gen_assign(ASTNode_ptr node){
    
    /**
     * @brief IMPORTANT = expression result has to be pushed on data stack
     */
    
    // POPS the value from the data stack
    ASTNode_ptr lhs = node->children[0];

    if (lhs->data.identifier.is_global) {
        printf("POPS GF@%s\n", lhs->data.identifier.name);
    } else {
        printf("POPS LF@%s\n", lhs->data.identifier.name);
    }
}

/**
 * @brief Handles stert of function
 * 
 * @note Called after entering NODE_FUNCTION_DEF node
 * 
 * @param node
 */
void gen_func_start(ASTNode_ptr node){
    // Reset the name generator
    name_gen_init(node);

    // Creates and print unique function label name, 
    gen_label(FUN_LABEL);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("\n#Storing params into local variables\n");

    // Creates local vaiables
    for (int i = node->data.function_def.arg_count; i >= 0; i--){
        create_unique_name(node->children[i], TEMP_VAR);
        printf("DEFVAR LF@%s\n", global_name_gen->temp_var);
        printf("POPS LF@%s\n", global_name_gen->temp_var);
    }
    
}

/**
 * @brief Handles end of function
 * 
 * @note Called when childeren array is empty
 * 
 */
void gen_func_end(){
    // Handeling return value via data stack
    if (return_occured){
        gen_return();   
    }
    else{
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
void gen_return(){
    /**
     * @brief IMPORTANT = expression result has to be pushed on data stack
     */
}

/**
 * @brief Pushes integer literal on data stack
 * 
 * @note Used by expression processing functions
 * 
 * @param value
 */
void gen_lit_int(long long value) {
    printf("PUSHS int@%lld\n", value);
}

/**
 * @brief Pushes float literal on data stack
 * 
 * @note Used by expression processing functions
 * 
 * @param value
 */
void gen_lit_float(long double value) {
    printf("PUSHS float@%a\n", value);
}

/**
 * @brief Pushes null literal on data stack
 * 
 * @note Used by expression processing functions
 * 
 */
void gen_lit_null(){
    printf("PUSHS nil@nil\n");
}

/**
 * @brief Pushes bool literal on data stack
 * 
 * @note Used by expression processing functions
 * 
 * @param value
 */
void gen_lit_bool(bool value){
    if (value){
        printf("PUSHS bool@true\n");
    }
    else{
        printf("PUSHS bool@false\n");
    }
}

/**
 * @brief Generates code for float literal
 * 
 * @param value The float value to push on stack
 */
void gen_lit_string(char* value){
    // First, lets make sure the string format is valid

    char *correct_value = calloc(MAX_STRING_LEN, sizeof(char));
    if (correct_value == NULL){
        // NOTE: handle freeing later
        error_exit(ERR_INTERNAL);
    }
    
    // Iterating throgh the string
    unsigned new_str_index = 0;

    for (int i = 0; value[i] != '\0'; i++){
        if (is_invalid_char(value[i])){
            new_str_index += sprintf(&correct_value[new_str_index], "\\0%d", value[i]);
            
        }
        else{
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
void gen_jmp_builtin(ASTNode_ptr node){
    // Based on the current node, jump will be performed

    if (!strcmp(node->data.function_call.name, "read_str")){
        printf("CALL %%*Ifj.read_str\n");
    }
    else if (!strcmp(node->data.function_call.name, "read_num")){
        printf("CALL %%*Ifj.read_num\n");
    }
    else if (!strcmp(node->data.function_call.name, "write")){
        printf("CALL %%*Ifj.write\n");
    }
    else if (!strcmp(node->data.function_call.name, "floor")){
        printf("CALL %%*Ifj.floor\n");
    }
    else if (!strcmp(node->data.function_call.name, "str")){
        printf("CALL %%*Ifj.str\n");
    }
    else if (!strcmp(node->data.function_call.name, "length")){
        printf("CALL %%*Ifj.length\n");
    }
    else if (!strcmp(node->data.function_call.name, "substring")){
       printf("CALL %%*Ifj.substring\n");
    }
    else if (!strcmp(node->data.function_call.name, "strcmp")){
        printf("CALL %%*Ifj.strcmp\n");
    }
    else if (!strcmp(node->data.function_call.name, "ord")){
        printf("CALL %%*Ifj.ord\n");
    }
    else{
        printf("CALL %%*Ifj.chr\n");
    }
};

/**
 * @brief Create a unique string which is stored in 
 * 
 * @param node 
 * @param option 
 */
void create_unique_name(ASTNode_ptr node, name_option_t option) {
    // Different options that can be used as argument
    switch (option){
        case FUN_LABEL:
            // Have to differentiate between the location of loop: function, setter and getter
            if (node->data.function_def.type == FUN_F){
                // Creates unique name
                snprintf(global_name_gen->fun_label, MAX_LABEL_NAME, "_fun_%s_%u", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count);

            }else if (node->data.function_def.type == FUN_G){
                // Creates unique name
                snprintf(global_name_gen->fun_label, MAX_LABEL_NAME, "_getter_%s", 
                global_name_gen->curr_function);
            }else{
                // Creates unique name
                snprintf(global_name_gen->fun_label, MAX_LABEL_NAME, "_setter_%s", 
                global_name_gen->curr_function);
            }

            break;
        case LOOP_START_L:
            // Have to differentiate between the location of loop: function, setter and getter
            if (global_name_gen->in_function){
                // Creates unique name
                snprintf(global_name_gen->loop_start_label, MAX_LABEL_NAME, "_loop_start_fun_%s_%u_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count,
                global_name_gen->loop_counter);
            }
            else if (global_name_gen->in_getter){
                // Creates unique name
                snprintf(global_name_gen->loop_start_label, MAX_LABEL_NAME, "_loop_start_getter_%s_0_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }
            else{
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
            if (global_name_gen->in_function){
                // Creates unique name
                snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME, "_loop_end_fun_%s_%u_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count,
                global_name_gen->loop_counter);
            }
            else if (global_name_gen->in_getter){
                // Creates unique name
                snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME, "_loop_end_getter_%s_0_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }
            else{
                // Creates unique name
                snprintf(global_name_gen->loop_end_label, MAX_LABEL_NAME, "_loop_end_setter_%s_1_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }

            // dont have to increment since loop_start already incremented the counter

            break;
        case IF_TRUE_L:
            // Have to differentiate between the location of loop: function, setter and getter
            if (global_name_gen->in_function){
                // Creates unique name
                snprintf(global_name_gen->if_true_label, MAX_LABEL_NAME, "_if_true_fun_%s_%u_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count,
                global_name_gen->loop_counter);
            }
            else if (global_name_gen->in_getter){
                // Creates unique name
                snprintf(global_name_gen->if_true_label, MAX_LABEL_NAME, "_if_true_getter_%s_0_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }
            else{
                // Creates unique name
                snprintf(global_name_gen->if_true_label, MAX_LABEL_NAME, "if_true_setter_%s_1_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }

            // Increments if counter
            global_name_gen->if_counter++;
            
            break;
        case IF_FALSE_L:
            // Have to differentiate between the location of loop: function, setter and getter
            if (global_name_gen->in_function){
                // Creates unique name
                snprintf(global_name_gen->if_false_label, MAX_LABEL_NAME, "_if_false_fun_%s_%u_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count,
                global_name_gen->loop_counter);
            }
            else if (global_name_gen->in_getter){
                // Creates unique name
                snprintf(global_name_gen->if_false_label, MAX_LABEL_NAME, "_if_false_getter_%s_0_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }
            else{
                // Creates unique name
                snprintf(global_name_gen->if_false_label, MAX_LABEL_NAME, "if_false_setter_%s_1_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }

            // dont have to increment since if_true already incremented the counter

            break;
        case TEMP_VAR:
            // Creates unique name
            snprintf(global_name_gen->temp_var, MAX_LABEL_NAME, "temp_var_%llu", 
            global_name_gen->temp_var_counter);

            // Increments temp_var counter
            global_name_gen->temp_var_counter++;
            
            break;
        case VAR:
            // Creates unique name
            snprintf(global_name_gen->temp_var, MAX_LABEL_NAME, "%s_%llu", 
            node->data.identifier.name,
            global_name_gen->block_depth_counter);
            
            /**
             * @brief Block depth is incremented/decremented in the main code-gen function.
             *        These variables can have 2 same names but they will still be unique for
             *        current FRAME.
             */
            break;

        case CALL:
            /**
             *                       IMPORTANT
             * This option/case isnt really used the same way as the previous ones.
             * When encountered fun call or getter/setter call in code, we have to 
             * know/generate the very same label name as is used for the function definition.
             * This name will be stored in global_name_gen->called_function.
             */

            if (node->data.identifier.id_type == FUNCTION){
                snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_fun_%s_%u", 
                node->data.function_call.name,
                node->data.function_call.param_count);
            }
            else if (node->data.identifier.id_type == SETTER){
                snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_setter_%s", 
                node->data.identifier.name);
            }
            else{
                snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_getter_%s", 
                node->data.identifier.name);
            }

            break;
        default:
            break;
    }
}
