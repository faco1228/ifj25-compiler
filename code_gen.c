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
#include "code_gen.h"
#include "ast.h"
#include "global_structures.h"
#include "built_in_funs.h"

//Global instance of data type holding different kinds of information nececssary for code-gen
name_generator_ptr global_name_gen = NULL;
//Global flag, holds information whether the function contained return node
bool return_occured = false;

/**
 * @brief The main code generating function - contains switch for all different types of nodes.
 *        Traverses the AST via inorder exept for expression subtrees.
 *        That one is processed by functions handling expression and is being traversed via postorder.
 * 
 * @param node 
 */
void codegen(ASTNode_ptr node){
    // End of reccursion
    if (!node){
        return;
    }
    
    // based on the node type calls functions generating instructions
    switch (node->type){
        case NODE_PROGRAM:
            // IFJcode25 code starts with this line
            printf(".IFJcode25\n"); 

            // Skipping built in functions defined at the beginning of each program
            printf("JUMP _program_start_\n");
            printf("\n");

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
            
            // This is where actual compilation begins
            printf("\n");
            printf("\nLABEL _program_start_\n");

            break;
        case NODE_FUNCTION_DEF:
            // Reset global name_gen_t object
            name_gen_init(node);

            // Set flags
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
            // Calls corresponding code generating function
            gen_assign(node);

            break;
        case NODE_IF:
            // Condition evaluation
            eval_exp(node->children[0]);
            
            // Variables to make code more readable
            ASTNode_ptr true_block = node->children[0];
            ASTNode_ptr else_block = node->children[1];
            
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
            // After the node return an expression follows
            eval_exp(node->children[0]);
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
        //case NODE_EXPR_STMNT:
        //    // Expression will be evaluated and the result will be left at the top of data stack
        //    eval_exp(node);

        case NODE_IDENTIFIER:
            // Call corresponding code generating function, differentiates between variable and getter
            if (node->data.identifier.id_type == VAR){
                gen_push_variable(node);
            }
            else{
                printf("\n");
                create_unique_name(node, CALL);
                printf("CALL %s\n", global_name_gen->called_function);
            }

            break;
        case NODE_CALL:
            // Call corresponding code generating function
            gen_jmp_function(node);

            break;
        default:
        /**
         * @brief There is nothing to be done for this type of nodes:
         *        NODE_BINARY_OP, NODE_BLOCK
         * 
         * @note These nodes are processed by some other functions and 
         *       dont have to be handeled:
         *       NODE_RANGE, NODE_INT_LIT, NODE_FLOAT_LIT, NODE_STR_LIT, NODE_NULL_LIT
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
    if (is_valid_node_type(node)){
        for (unsigned i = 0; i < node->child_count; i++){
            codegen(node->children[i]);
        }
    }
    
    /**
     * @brief As the recurrsion returns back to the root these if statements
     *        will be executed.
     */

    // Generates function end instructions
    if (node->type == NODE_FUNCTION_DEF){
        global_name_gen->in_function = false;
        // Makes sure that only one RETURN instruction is generated in each function
        if (!return_occured){
            gen_return();
        }
        return_occured = false;
    }

    // Updates location flag and calls function handeling for loop end
    if (node->type == NODE_FOR){
        global_name_gen->in_loop = false;
        gen_for_end(node);
    }

    // Updates location flag and calls function handeling while loop end
    if (node->type == NODE_WHILE){
        global_name_gen->in_loop = false;
        gen_while_end(node);
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
    global_name_gen->loop_counter = 0;
    global_name_gen->if_counter = 0;
    global_name_gen->temp_var_counter = 0;
    
    // Set stack tracker to zero
    global_name_gen->stack_depth = 0;

    // Set all allocated strings to empty
    memset(global_name_gen->fun_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->else_block_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->end_if_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_start_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->loop_end_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->called_function, 0, MAX_LABEL_NAME);
    
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
 * 
 */
void gen_push_variable(ASTNode_ptr node){
    // Differentiates between global and local variable
    if (node->data.identifier.is_global){
        printf("PUSHS GF@%s\n", node->data.identifier.code_gen_name);
    }
    else{
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
void gen_var_decl(ASTNode_ptr node){
    // Differentiates between global and local variable
    if (node->data.identifier.is_global) {
        printf("DEFVAR GF@%s\n", node->data.identifier.name);
    }
    else{
        printf("DEFVAR LF@%s\n", node->data.identifier.code_gen_name);
    }
}

/**
 * @brief Assigns value to the variable stored in left node child
 * 
 * @param node 
 */
void gen_assign(ASTNode_ptr node){
    // Variable to make code more readable
    ASTNode_ptr lhs = node->children[0];

    // When the lhs is a setter
    if (lhs->type == SETTER){
        gen_jmp_function(node);
    }
    else{ // The lhs needs to be a variable
        if (lhs->data.identifier.is_global) {
            printf("POPS GF@%s\n", lhs->data.identifier.name);
        } else {
            printf("POPS LF@%s\n", lhs->data.identifier.name);
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
void gen_func_start(ASTNode_ptr node){
    // Reset the name generator
    name_gen_init(node);

    // Creates and print unique function label name, 
    create_unique_name(node, FUN_LABEL);
    printf("\nLABEL %s\n", global_name_gen->fun_label);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("\n# Store params into local variables\n");

    // Creates local vaiables
    for (int i = node->data.function_def.arg_count; i >= 0; i--){
        printf("DEFVAR LF@%s\n", node->children[i]->data.identifier.code_gen_name);
        printf("POPS LF@%s\n", node->children[i]->data.identifier.code_gen_name);
    }
}

// TO DO: add a comment
void gen_return(){
    // Returns value via data stack
    if (!return_occured){
        printf("pushs nil@nil\n");
    }

    // Generate function end
    printf("POPFRAME\n");
    printf("RETURN\n");
}

/**
 * @brief Converts string literal into corresponding value and pushes this value on a stack
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
 * @brief Generates jump on corresponding function
 * 
 * @note works  on both built in and user defined functions
 * 
 * @param node 
 */
void gen_jmp_function(ASTNode_ptr node){
    // Checks whether the node is getter/setter/function
    if (node->type = SETTER){
        eval_exp(node->children[1]);
    }
    else if (node->type = GETTER){
        ; // Nothing will be pushed
    }
    else{
        // First the arguments are pushed on data strack (left to right) but
        // has to be treated as potential expression
        for (int i = 0; i < node->data.function_call.param_count; i++){
            eval_exp(node->children[i]);
        }
    }

    // The function is built in
    if (node->data.function_call.is_builtin){
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
    }
    else{
        // The function is user defined
        create_unique_name(node, CALL);
        printf("\nCALL %s\n", global_name_gen->called_function);
    }
}

/**
 * @brief Creates a unique label name
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
        case IF_ELSE_L:
            // Have to differentiate between the location of loop: function, setter and getter
            if (global_name_gen->in_function){
                // Creates unique name
                snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "_if_felse_fun_%s_%u_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count,
                global_name_gen->loop_counter);
            }
            else if (global_name_gen->in_getter){
                // Creates unique name
                snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "_if_else_getter_%s_0_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }
            else{
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
            if (global_name_gen->in_function){
                // Creates unique name
                snprintf(global_name_gen->end_if_label, MAX_LABEL_NAME, "_if_end_fun_%s_%u_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->curr_param_count,
                global_name_gen->loop_counter);
            }
            else if (global_name_gen->in_getter){
                // Creates unique name
                snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "_if_end_getter_%s_0_%llu", 
                global_name_gen->curr_function, 
                global_name_gen->loop_counter);
            }
            else{
                // Creates unique name
                snprintf(global_name_gen->else_block_label, MAX_LABEL_NAME, "if_end_setter_%s_1_%llu", 
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

            if (node->data.identifier.id_type == FUNCTION){
                snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_fun_%s_%u", 
                node->data.function_call.name,
                node->data.function_call.param_count);
            }
            else if (node->data.identifier.id_type == SETTER){
                snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_setter_%s", 
                node->data.identifier.code_gen_name);
            }
            else{
                snprintf(global_name_gen->called_function, MAX_FUNCTION_NAME, "_getter_%s", 
                node->data.identifier.code_gen_name);
            }

            break;
        default:
            break;
    }
}

/**
 * @brief Handles start of a for loop
 * 
 * @note called from NODE_FOR
 * 
 * @param node 
 */
void gen_for_start(ASTNode_ptr node){
    // Creates unique label name
    create_unique_name(node, LOOP_START_L);
    create_unique_name(node, LOOP_END_L);
    
    // Store current temp_var_counter value for later usage and increment the counter inside global_name_gen
    unsigned temp_id = global_name_gen->temp_var_counter++;

    // Variable used as "index" in for loop - iterator
    printf("DEFVAR LF@%s\n", node->children[0]->data.identifier.code_gen_name);
    printf("DEFVAR LF@temp_var_until_%d\n", temp_id);

    // Have to read NODE_RANGE children
    ASTNode_ptr range_node = node->children[1]->children[0];

    // This function will return start and end of the range on stack data
    eval_exp(range_node);
    
    // Initialize variables
    printf("POPS LF@temp_var_until_%d\n", temp_id);
    printf("POPS LF@%s\n", node->children[0]->data.identifier.code_gen_name);

    // Prints label of the beginnning of the loop
    printf("\nLABEL %s\n", global_name_gen->loop_start_label);

    // Push condition arguments on stack    
    printf("\n# Evaluate condition\n");
    printf("PUSHS LF@%s\n", node->children[0]->data.identifier.code_gen_name);
    printf("PUSHS LF@temp_var_until_%d\n", temp_id);
    
    // Evaluates condition based on whether the range is inclusive(double dot) or not(triple dot)
    if (range_node->data.range.inclusive) {
        // Continue until i <= end
        printf("GTS\n");
        printf("PUSHS bool@true\n");
        printf("JUMPIFEQS %s\n\n", global_name_gen->loop_end_label);
    } else {
        // Continue until i < end
        printf("LTS\n");
        printf("PUSHS bool@false\n");
        printf("JUMPIFEQS %s\n\n", global_name_gen->loop_end_label);
    }

    // Increment the iterator variable
    printf("\n# Icrementing iterator\n");
    printf("PUSHS LF@%s\n", node->data.for_statement.name_iter);
    printf("PUSHS int@1\n");
    printf("ADDS\n");
    printf("POPS LF@%s\n", node->data.for_statement.name_iter);

    // for loop body follows
}

/**
 * @brief Handles end of a for loop
 * 
 * @note called when recrusion returns back to the node
 * 
 * @param node 
 */
void gen_for_end(ASTNode_ptr node) {
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
void gen_while_start(ASTNode_ptr node) {
    // Creates unique label names
    create_unique_name(node, LOOP_START_L);
    create_unique_name(node, LOOP_END_L);
    
    // Prints loop start label
    printf("\nLABEL %s\n", global_name_gen->loop_start_label);
    
    // Condition check
    printf("# Evaluate while condition\n");
    
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
void gen_while_end(ASTNode_ptr node) {
    // Loop back to the beginning
    printf("\nJUMP %s\n", global_name_gen->loop_start_label);
    
    // Prints loop end label
    printf("\nLABEL %s\n", global_name_gen->loop_end_label);
}

/**
 * @brief Terminates correspondig while loop
 */
void gen_break(){
    printf("JUMP %s\n", global_name_gen->loop_end_label);
};

/**
 * @brief Skips one iteration in correspondig while loop
 */
void gen_continue(){
    printf("JUMP %s\n", global_name_gen->loop_start_label);
};

/**
 * @brief Memory clean up for global instance of name_generator_ptr object
 * 
 * @param global_name_gen 
 */
void free_global_name_gen(name_generator_ptr global_name_gen){
    // Free memory used by its strings
    free(global_name_gen->fun_label);
    free(global_name_gen->else_block_label);
    free(global_name_gen->end_if_label);
    free(global_name_gen->loop_start_label);
    free(global_name_gen->loop_end_label);
    free(global_name_gen->curr_function);
    free(global_name_gen->called_function);

    // Make sure this address wont be derreferenced again
    global_name_gen->fun_label = NULL;
    global_name_gen->else_block_label = NULL;
    global_name_gen->end_if_label = NULL;
    global_name_gen->loop_start_label = NULL;
    global_name_gen->loop_end_label = NULL;
    global_name_gen->curr_function = NULL;
    global_name_gen->called_function = NULL;

    // Free the vole oject
    free(global_name_gen);
}

/**
 * @brief Handles start of if statement
 * 
 * @param node 
 */
void gen_if(ASTNode_ptr node) {
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
 * @brief Handles beginning of else block of if statement
 * 
 * @param node 
 */
void gen_else(ASTNode_ptr node) {
    // In case the if block would reach this instructions jump at the end of 
    // the whole if statement needs to be executed
    printf("JUMP %s\n\n", global_name_gen->end_if_label);
    
    // Prints label for else block
    printf("\nLABEL %s\n", global_name_gen->else_block_label);
    
    // Generate body
}
