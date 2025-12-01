/**
 * @file compiler_main.c
 * @authors Martin Mezei (xmezeim00)
 * @brief Implements an executable main to run different modules of the compiler and allocate structure that are needed in multiple modules.
 * @version 0.1
 * @date 2025-11-17
 *
 * @copyright Copyright (c) 2025
 */

// importing compilder modules
#include "parser.h"
#include "semantic_analysis.h"
#include "scope_stack.h"
#include "symtable.h"
#include "code_gen.h"
#include "global_structures.h"
#include "compiler_main.h"

#define COMPILATIONS_SUCCESS 0

ASTNode_ptr g_ast_root = NULL;
ST_Node *g_func_symtable = NULL;
ST_Node *g_global_symtable = NULL;
Scope_Stack *g_scope_stack = NULL;

int main()
{
    // syntactic analysis and creation of ast
    ASTNode_ptr g_ast_root = parse_program();
    
    // print_ast(ast); //! vymazat - volanie pomocnej funkcie pre print ast cez pre order

    // scope_stack init
    g_scope_stack = malloc(sizeof(Scope_Stack));
    if (g_scope_stack == NULL){
        error_exit(ERR_INTERNAL);
    }
    
    // if stack array allocation fails, error_exit() is called inside the function and all memory is freed
    scope_stack_init(g_scope_stack); 
    // performes semantic_analysis
    semantic_analysis(g_ast_root);

    // global_name_gen init
    allocate_global_name_gen(global_name_gen, g_scope_stack, g_ast_root, g_func_symtable, g_global_symtable);

    // traverses AST and generates final code
    codegen(g_ast_root);

    // free all allocated structures
    glob_structs_clean_up(g_scope_stack, g_ast_root, g_func_symtable, g_global_symtable);
    free_global_name_gen(global_name_gen);
    // parser calls scanner_cleanup

    return COMPILATIONS_SUCCESS;
}
