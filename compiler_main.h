/**
 * @file compiler_main.h
 * @author Martin Racek (xracekm00)
 * @brief Header file implementing macro to make compiler_main's code more readable
 * @version 0.1
 * @date 2025-12-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _COMPILER_MAIN_
#define _COMPILER_MAIN_

#include "ast.h"
#include "symtable.h"
#include "scope_stack.h"

//Macro, used for global structers clean up
#define glob_structs_clean_up(g_scope_stack, ast, g_func_symtable, g_global_symtable) \
    do{ \
        scope_stack_dispose(g_scope_stack); \
        ast_free(ast); \
        st_dispose_tree(g_func_symtable); \
        st_dispose_tree(g_global_symtable); \
    } while (0)

#endif