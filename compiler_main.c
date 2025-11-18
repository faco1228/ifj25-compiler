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

// other imports
#include <stdlib.h>

#define COMPILATIONS_SUCCESS 0

int main()
{
    ST_Node *func_symtable = NULL;
    ST_Node *glob_var_symtable = NULL;

    // syntactic analysis and creation of ast
    ASTNode_ptr ast = parse_program(func_symtable, glob_var_symtable);

    // scope_stack init
    Scope_Stack *scope_stack = malloc(sizeof(Scope_Stack));
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

    scope_stack_init(scope_stack); // if something fails, error_exit() is called inside the function and memory is freed

    // performes semantic_analysis and generates code after every successful semantic action
    semantic_analysis(ast, ast, func_symtable, glob_var_symtable, scope_stack);

    // free all allocated structures
    scope_stack_dispose(scope_stack);
    ast_free(ast);
    st_dispose_tree(func_symtable);
    st_dispose_tree(glob_var_symtable);

    return COMPILATIONS_SUCCESS;
}