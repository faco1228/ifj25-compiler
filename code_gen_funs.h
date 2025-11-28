/**
 * @file code_gen_funs.h
 * @author xracekm00, xmezei00
 * @brief Header file for code generating functions
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _CODE_GEN_
#define _CODE_GEN_

#include "ast.h"

typedef struct {
    // Name mangeling
    unsigned long long label_counter;
    unsigned long long temp_var_counter;

    // Location in AST
    char *curr_function;
    bool in_function;
    bool in_loop;

    // Will store a unique label name
    char *label;

    // How many thing are on data stack, so reading from an empty stack can be prevented
    unsigned stakck_depth;

} name_generator_t, *name_generator_ptr;

#define MAX_LABEL_NAME 256
#define MAX_FUNCTION_NAME 256


/************************************ FUNCTION PROTOTYPES ************************************/

// The main code-gen function - contains switch for all different types of nodes
void codegen(ASTNode_ptr node, name_generator_ptr ctx);

void name_gen_init(name_generator_ptr name_gen, ASTNode_ptr node);

void string_iter();

void gen_label(name_generator_ptr name_gen);

void gen_temp_var(char* variable, name_generator_ptr name_gen);

void func_start(ASTNode_ptr node);

void gen_return();

void func_end();


void gen_literal_int(long long val);
void gen_literal_float(long double val);  
void gen_literal_string(const char *str);
void gen_literal_null(void);

void gen_identifier(const char *name, bool is_local);

void gen_var_decl(const char *name, bool is_local);
void gen_assign(ASTNode_ptr lhs, ASTNode_ptr rhs, name_generator_ptr ctx);
void gen_return(ASTNode_ptr value_node, name_generator_ptr ctx);
void gen_function_def(ASTNode_ptr func_node);

void gen_binary_op(operator_types op, ASTNode_ptr lhs, ASTNode_ptr rhs, name_generator_ptr ctx);
void gen_if(ASTNode_ptr cond, ASTNode_ptr then_block, ASTNode_ptr else_block, name_generator_ptr ctx);
void gen_while(ASTNode_ptr cond, ASTNode_ptr body, name_generator_ptr ctx);
void gen_call(const char *func_name, ASTNode_ptr *params, unsigned param_count, bool is_builtin);

void gen_for(const char *iter_var, ASTNode_ptr range, ASTNode_ptr body, name_generator_ptr ctx);

// nejako pomocou TYPE
void gen_type_check();

void gen_zero_div_check();

/*

gen_create_variable

gen_clean_up_frames
gen_jump_if_grater      ; Tieto dve funkcie treba preto, aby sme vedeli ci pri loopoch alebo if
gen_jump_if_lowet       ; mame ci nemame previest skok (nejako pomocou LT(S), GT(S), EQ(S))
gen_string_iter         ; CONCAT niekolko krat

funkcie pre generovanie built in funkcii
...
*/

#endif