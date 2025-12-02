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
#include "error.h"

//! vymazat - funkcia pre print stromu pomocou preorder prechodu
void print_ast(ASTNode_ptr ast_root)
{
    if (!ast_root)
        return;

    printf("NODE TYPE: %d\n", ast_root->type);

    for (size_t i = 0; i < ast_root->child_count; i++)
    {
        print_ast(ast_root->children[i]);
    }
}

void print_code_gen_names(ASTNode_ptr ast_root) // !vymazat pre odovzdanim
{

    if (!ast_root)
        return;

    if (ast_root->type == NODE_IDENTIFIER || ast_root->type == NODE_VAR_DECL)
        printf("CODE_GEN_NAME: %s\n", ast_root->data.identifier.code_gen_name);

    for (size_t i = 0; i < ast_root->child_count; i++)
    {
        print_code_gen_names(ast_root->children[i]);
    }
}

#define COMPILATIONS_SUCCESS 0

ASTNode_ptr g_ast_root = NULL;
ST_Node *g_func_symtable = NULL;
ST_Node *g_global_symtable = NULL;
Scope_Stack *g_scope_stack = NULL;

int main()
{
    // syntactic analysis and creation of ast
    ASTNode_ptr ast = parse_program(); //! bude vobec treba vratit ast ak je globalne? nestaci poslat ten globalny ptr? len na zamyslenie

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

    // print_code_gen_names(ast); // !vymazat

    // free all allocated structures
    glob_structs_clean_up(g_scope_stack, g_ast_root, g_func_symtable, g_global_symtable);
    free_global_name_gen(global_name_gen);
    // parser calls scanner_cleanup

    return COMPILATION_SUCCESS;
}
