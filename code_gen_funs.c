/**
 * @file code_gen_funs.c
 * @author xracekm00, xmezeim00, xcillik00
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

/**
 * @brief Budeme musiet uchovavat informaciu o tom, kolko ramcov je na zasobniku resp kolko dat je na 
 *        datovom zasobniku, lebo ked chcem citat z prazdneho zasobniku tak dojde k chybe.
 */

//Global instance of Data type holding all different kinds of information nececssary for code-gen
name_generator_ptr global_name_gen = NULL;
//Global flag, holds information whether the function contained return node
bool return_occured = false;

/**
 * @brief The main code generating function - contains switch for all different types of nodes.
 *        Traverses the AST via inorder and expression subtrees via postorder.
 * 
 * @param node 
 */
void codegen(ASTNode_ptr node){
    // End of reccursion
    if (!node){
        return;
    }
    
    // Recursivelly processes each AST node
    switch (node->type){
        case NODE_PROGRAM:
            // IFJcode25 code starts with this line
            printf(".IFJcode25\n"); 

            // Skipping built in functions defined at the beginning of each program
            printf("JUMP _program_start_\n");
            printf("\n");

            // At the beginning of the program, there will implementations of builtin functions 
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

            // Call corresponding code generating function
            gen_func_start(node);
            break;
        // There is nothing to be done for this type of node
        //
        // case NODE_BLOCK:
        //     break;
        case NODE_VAR_DECL:
            // Call corresponding code generating function
            gen_var_decl(node);

            break;
        case NODE_ASSIGN:
            // Call corresponding code generating function
            gen_assign(node);

            break;
        case NODE_IF:

            break;
        case NODE_RETURN:

            break;
        case NODE_WHILE:
            // Updates location flag and calls corresponding code generating function
            global_name_gen->in_loop = true;
            gen_while_start(node);

            break;
        case NODE_FOR:
            // Updates location flag and calls corresponding code generating function
            global_name_gen->in_loop = true;
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
        case NODE_EXPR_STMNT:

            break;
        case NODE_IDENTIFIER:
            // Call corresponding code generating function based on whether it is variable or getter
            if (node->data.identifier.id_type == VAR){
                gen_variable(node);
            }
            else{
                printf("\n");
                create_unique_name(node, CALL);
                printf("CALL %s\n", global_name_gen->called_function);
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
                printf("CALL %s\n", global_name_gen->called_function);
            }

            break;
        // This node type is already processed when NODE_FOR is reached during traversal
        //   
        // case NODE_RANGE:
        //     break;
        case NODE_INT_LIT:
            // Pushes integer literal on data stack
            gen_lit_int(node->data.literal.data.int_val);

            break;
        case NODE_FLOAT_LIT:
            // Pushes float literal on data stack
            gen_lit_int(node->data.literal.data.float_val);

            break;
        case NODE_STR_LIT:
            // Pushes string literal on data stack
            gen_lit_int(node->data.literal.data.str_value);

            break;
        case NODE_NULL_LIT:
            // Pushes bull literal on data stack
            gen_lit_null();

            break;
        default:
            break;
    }

    // Iterate throuh all children of the node
    for (unsigned i = 0; i < node->child_count; i++){
        codegen(node->children[i]);
    }

    /**
     * @brief As the recurrsion returns back to the root these if statements
     *        will be executed.
     */

    // Generates function end instructions
    if (node->type == NODE_FUNCTION_DEF){
        global_name_gen->in_function = false;
        gen_func_end();
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
    memset(global_name_gen->if_true_label, 0, MAX_LABEL_NAME);
    memset(global_name_gen->if_false_label, 0, MAX_LABEL_NAME);
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
 * @brief Creates a unique label and generetase LABEL <label> instruction
 * 
 * @param option
 */
void gen_label(name_option_t option){
    // Creates a unique name
    create_unique_name(global_name_gen, option);

    // Based on the option generates label
    switch (option){
        case FUN_LABEL:
            printf("\nLABEL %s\n", global_name_gen->fun_label);

            break;
        case LOOP_START_L:
            printf("\nLABEL %s\n", global_name_gen->loop_start_label);

            break;
        case LOOP_END_L:
            printf("\nLABEL %s\n", global_name_gen->loop_end_label);

            break;
        case IF_TRUE_L:
            printf("\nLABEL %s\n", global_name_gen->if_true_label);

            break;
        case IF_FALSE_L:
            printf("\nLABEL %s\n", global_name_gen->if_false_label);

            break;
        default:
            break;
    }
}

/**
 * @brief Pushes variable on data stack, used by expression processing functions
 * 
 * @param node 
 * 
 */
void gen_push_variable(ASTNode_ptr node){
    // Creates uniquq variable name
    create_unique_name(node, VAR);

    // Differentiates between global and local variable
    if (node->data.identifier.is_global){
        printf("PUSHS GF@%s\n", node->data.identifier.name);
    }
    else{
        printf("PUSHS LF@%s\n", node->data.identifier.name);
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
        printf("DEFVAR LF@%s\n", node->data.identifier.name);
    }
}

/**
 * @brief Assigns value to the variable stored in left node child
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
    gen_label(FUN_LABEL);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("\n#Store params into local variables\n");

    // Creates local vaiables
    for (int i = node->data.function_def.arg_count; i >= 0; i--){
        printf("DEFVAR LF@temp_var_param_%d\n", i);
        printf("POPS LF@temp_var_param_%d\n", i);
    }
}

/**
 * @brief Handles end of function
 * 
 * @note Called reccursion reaches NODE_FUNCTION_DEF node while returning
 * 
 */
void gen_func_end(){
    // Returns value via data stack
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
 * @brief Jumps on corresponding built in function
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
    printf("DEFVAR LF@%s\n", node->data.for_statement.name_iter);
    printf("DEFVAR LF@temp_var_until_%d\n", temp_id);

    // Have to read NODE_RANGE children
    ASTNode_ptr range_node = node->children[0];

    // Push iterator value on data stack
    if (range_node->children[0]->type == NODE_INT_LIT){
        gen_lit_int(range_node->children[0]->data.literal.data.int_val);
    }
    else if (range_node->children[0]->type == NODE_IDENTIFIER){
        // Lets assume for now, that ident represents only variable
        gen_push_variable(range_node->children[0]);
    }
    else{
        // IMPORTANT: can be expression, menzi function
    }
    
    // Push end value on data stack
    if (range_node->children[1]->type == NODE_INT_LIT){
        gen_lit_int(range_node->children[1]->data.literal.data.int_val);
    }
    else if (range_node->children[1]->type == NODE_IDENTIFIER){
        // Lets assume for now, that ident represents only variable
        gen_push_variable(range_node->children[1]);
    }
    else{
        // IMPORTANT: can be expression, menzi function
    }
    
    // Initialize variables
    printf("POPS LF@temp_var_until_%d\n", temp_id);
    printf("POPS LF@%s\n", node->data.for_statement.name_iter);

    // Prints label of the beginnning of the loop
    printf("\nLABEL %s\n", global_name_gen->loop_start_label);

    // Push condition arguments on stack    
    printf("\n# Evaluate condition\n");
    printf("PUSHS LF@%s\n", node->data.for_statement.name_iter);
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
    // Increment the iterator variable
    printf("\n# Icrementing iterator\n");
    printf("PUSHS LF@%s\n", node->data.for_statement.name_iter);
    printf("PUSHS int@1\n");
    printf("ADDS\n");
    printf("POPS LF@%s\n", node->data.for_statement.name_iter);
    
    printf("\nJUMP %s\n", global_name_gen->loop_start_label);
    
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
    
    // IMPORTANT: node->children[0] is an expression, that has to be evaluated
    
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
 * @brief Handles if-else statement
 * 
 * @note Called from NODE_IF
 *       children[0] = condition
 *       children[1] = if block
 *       children[2] = else block
 * 
 * @param node 
 */
void gen_if(ASTNode_ptr node) {
    // Creates unique name
    create_unique_name(node, IF_FALSE_L);
    
    // Condition check
    printf("\n# Evaluate if condition\n");
    
    // IMPORTANT: once again expression eval function has to be called
    
    // When the condition is false, jump streight to the else block
    printf("PUSHS bool@false\n");
    printf("JUMPIFEQS %s\n\n", global_name_gen->if_false_label);
    
    // Block for the
    printf("# If true block\n");
    
    // IMPORTANT: somehow codegen has to process the body
    
    // Po then bloku preskoč else (skoč na koniec)
    // Musíme vytvoriť end label
    char if_end_label[MAX_LABEL_NAME];
    sprintf(if_end_label, "__if_end_%llu", global_name_gen->if_counter);
    
    printf("JUMP %s\n\n", if_end_label);
    
    // ===== ELSE BLOK =====
    printf("LABEL %s\n", global_name_gen->if_false_label);
    printf("# Else block\n");
    
    if (node->children[2] != NULL) {
        codegen(node->children[2]);
    }
    // Ak je NULL, else blok je prázdny (ale v základnom zadaní je vždy prítomný)
    
    // ===== KONIEC IF =====
    printf("\nLABEL %s\n", if_end_label);
}