/**
 * @file global_structures.h
 * @author Samuel Facka (xfackas00)
 * @brief Contains extern daclarations of global structers used in multiple modules
 * @version 0.1
 * @date 2025-12-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _GLOBALS_H_GUARD_
#define _GLOBALS_H_GUARD_

#include <stdarg.h>
#include "ast.h"
#include "symtable.h"
#include "scope_stack.h"

extern ASTNode_ptr g_ast_root;
extern ST_Node *g_func_symtable;
extern ST_Node *g_global_symtable;
extern Scope_Stack *g_scope_stack;

#endif
