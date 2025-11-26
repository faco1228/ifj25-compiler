/**
 * @file compiler_main.c
 * @authors xmezeim00
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
#include "global_structures.h"


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

#define COMPILATIONS_SUCCESS 0

ASTNode_ptr g_ast_root = NULL;
ST_Node *g_func_symtable = NULL;
ST_Node *g_global_symtable = NULL;
Scope_Stack *g_scope_stack = NULL;

int main()
{
    // syntactic analysis and creation of ast
    ASTNode_ptr ast = parse_program(); //! bude vobec treba vratit ast ak je globalne? nestaci poslat ten globalny ptr? len na zamyslenie
    
    print_ast(ast); //! vymazat - volanie pomocnej funkcie pre print ast cez pre order

    // scope_stack init
    g_scope_stack = malloc(sizeof(Scope_Stack));
    if (!g_scope_stack)
        error_exit(ERR_INTERNAL);

    scope_stack_init(g_scope_stack); // if stack array allocation fails, error_exit() is called inside the function and all memory is freed

    // performes semantic_analysis and generates code after every successful semantic action
    semantic_analysis(ast, g_func_symtable, g_scope_stack);

    // free all allocated structures
    scope_stack_dispose(g_scope_stack);
    ast_free(ast);
    st_dispose_tree(g_func_symtable);
    st_dispose_tree(g_global_symtable);
    // parser calls scanner_cleanup

    return COMPILATIONS_SUCCESS;
}
