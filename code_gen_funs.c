/**
 * @file code_gen_funs.c
 * @author xracekm00, xmezeim00
 * @brief Code generator for Wren-like programming language
 * 
 * @note Code is being printed to stdout
 *       We decided to use Pascal convetion for function calls
 * 
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "symtable.h"
#include "ast.h"
#include "code_gen_funs.h"
#include "error.h"


/*

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

void codegen(ASTNode_ptr node, name_generator_ptr name_gen){
    // Recursion end
    if (!node){
        return;
    }
    
    // Recursivelly processing nodes
    switch (node->type){
    case NODE_PROGRAM:
        // Each code in IFJcode25 starts with this line
        printf(".IFJcode25\n"); 
        
        // Iterate throuh all children
        for (unsigned i = 0; i < node->child_count; i++){
            codegen(node->children[i], name_gen);
        }
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

}

//Global varialbe - its content will be alterred when entering a new function node
name_generator_t global_name_gen;

/**
 * @note to prevent unnecesary memory allocation and freeing
 * 
 * The function that will perfrom the AST traversing has to call malloc
 * at global_name_gen.label and global_name_gen.curr_function.
 * This functions also need to clear this memory after it's finished.
 */


/**
 * @brief Initializes/Resets name generator attributes
 * 
 * @note This function is called every time when entering new function subtree
 *       during seconf AST traversal
 * 
 * @param mnglr 
 */
void name_gen_init(name_generator_ptr name_gen, ASTNode_ptr node){
    name_gen->label_counter = 0;
    name_gen->temp_var_counter = 0;

    strcpy(name_gen->curr_function, node->data.function_def.name);
    name_gen->label = "\0";

    name_gen->in_function = true;
    name_gen->in_loop = false;

    name_gen->stakck_depth = 0;
}


/**
 * @brief Function simulating string iteration using IFJcode25 instructions
 * 
 */
void string_iter(){
}

/**
 * @brief Generates unieque label name using name-mangeling
 * 
 * @param label
 * @param name_gen
 */
void gen_label(name_generator_ptr name_gen){

}

/**
 * @brief Generates unieque variable name using name-mangeling
 * 
 * @param variable 
 * @param mnglr 
 */
void gen_temp_var(char* variable, name_generator_ptr name_gen){

}


/**
 * @brief Handles stert of function
 * 
 * @note Called after entering NODE_FUNCTION_DEF node
 */
void func_start(ASTNode_ptr node){
    // Reset the name generator
    name_gen_init(&global_name_gen, node);

    // Creates unique function label name
    gen_label(&global_name_gen);

    // Instructions for function start
    printf("LABEL %s\n", global_name_gen.label);
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
}

/**
 * @brief Handles return values of a function
 * 
 * @note uses data stack to store returne value
 * 
 */
void gen_return(){

}

/**
 * @brief Handles end of function
 * 
 * @note Called when childeren array is empty
 * 
 */
void func_end(){
    // This function will handle return value
    gen_return();

    // Instructions for function end
    printf("%s\n", "POPFRAME");
    printf("%s\n", "RETURN");
}

