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
#include "include/scope_stack.h"
#include "include/symtable.h"

// other imports
#include <stdlib.h>

#define COMPILATIONS_SUCCESS 0

int main()
{
    parse_program(); // todo : upravit parse program aby vracal AST_strom

    // scope_stack init
    Scope_Stack *scope_stack = malloc(sizeof(Scope_Stack));
    if (!scope_stack)
        error_exit(ERR_INTERNAL);

    scope_stack_init(scope_stack); // if something fails, error_exit() is called inside the function and memory is freed

    ST_Node *func_symtable = NULL;
    ST_Node *glob_var_symtable = NULL;

    // semantic_analysis(); // todo: doplnit ked bude parse program upraveny

    return COMPILATIONS_SUCCESS;
}