#ifndef GLOBALS_H_GUARD
#define GLOBALS_H_GUARD

#include <stdarg.h>
#include "ast.h"
#include "symtable.h"
#include "scope_stack.h"

extern ASTNode_ptr g_ast_root;
extern ST_Node *g_func_symtable;
extern ST_Node *g_global_symtable;
extern Scope_Stack *g_scope_stack;

#endif
