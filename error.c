/**
 * @file error.c
 * @author Kristian Cilling (xcillik00)
 * @brief Handles different errors that might come up during compilation. Warnings are printed to stdout,
 *        error_exit() handles exiting the program with the corresponding error code.
 *
 * @version 0.1
 * @date 2025-11-10
 *
 * @copyright Copyright (c) 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error.h"
#include "scanner.h"
#include "ast.h"
#include "symtable.h"
#include "global_structures.h"

/**
 * @brief Registers the AST root pointer for automatic cleanup on error
 *
 * @param ast_root
 */
void error_set_parser_ast_root(void *ast_root)
{
    g_ast_root = (ASTNode_ptr)ast_root;
}

/**
 * @brief
 *
 * @param func_symtable Registers the function symbol table pointer for automatic cleanup on error
 */
void error_set_parser_func_symtable(void *func_symtable)
{
    g_func_symtable = (ST_Node *)func_symtable;
}

/**
 * @brief Registers the global variable symbol table pointer for automatic cleanup on error
 *
 * @param glob_var_symtable
 */
void error_set_parser_glob_symtable(void *glob_var_symtable)
{
    g_global_symtable = (ST_Node *)glob_var_symtable;
}

/**
 * @brief Internal cleanup only for parser/PSA
 */
static void parser_psa_cleanup(void)
{
    if (g_ast_root != NULL)
    {
        ast_free(g_ast_root);
        g_ast_root = NULL;
    }

    if (g_func_symtable != NULL)
    {
        st_dispose_tree(g_func_symtable);
        g_func_symtable = NULL;
    }

    if (g_global_symtable != NULL)
    {
        st_dispose_tree(g_global_symtable);
        g_global_symtable = NULL;
    }

    if (g_scope_stack != NULL)
    {
        scope_stack_dispose(g_scope_stack);
        g_scope_stack = NULL;
    }

    // scanner (token buffer, pushed_token, ...)
    scanner_cleanup();
}

/**
 * @brief Prints a warning messages based on the provided warning code.
 *
 * @param warning Warning code.
 * @param format Format of the warning message.
 *
 * @note In some cases, other arguments might be provided.
 */
void warnings(int warning, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stderr, "Warning [%d]: ", warning);

    switch (warning)
    {
    case ERR_LEXICAL:
        fprintf(stderr, "Lexikálna chyba - ");
        break;
    case ERR_SYNTACTIC:
        fprintf(stderr, "Syntaktická chyba - ");
        break;
    case ERR_SEM_UNDEFINED:
        fprintf(stderr, "Sémantická chyba - nedefinovaná funkcia/premenná - ");
        break;
    case ERR_SEM_REDEFINITION:
        fprintf(stderr, "Redefinícia funkcie/premennej - ");
        break;
    case ERR_SEM_ARG_COUNT:
        fprintf(stderr, "Neočekávaný počet argumentov / typ parametra - ");
        break;
    case ERR_SEM_TYPE_MISMATCH:
        fprintf(stderr, "Typová nekompatibilita vo výrazoch - ");
        break;
    case ERR_SEM_OTHER:
        fprintf(stderr, "Ostatné sémantické chyby - ");
        break;
    case ERR_RUNTIME_PARAM_TYPE:
        fprintf(stderr, "Behová sémantická chyba - typ parametra - ");
        break;
    case ERR_RUNTIME_TYPE_MISMATCH:
        fprintf(stderr, "Behová sémantická chyba - typová nekompatibilita - ");
        break;
    case ERR_INTERNAL:
        fprintf(stderr, "Interná chyba prekladača - ");
        break;
    default:
        fprintf(stderr, "Neznámy kód chyby - ");
        break;
    }

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/**
 * @brief Handles exiting the program with a corresponding error code
 *
 * @param error Error code to exit with.
 */
void error_exit(int error)
{
    parser_psa_cleanup();

    switch (error)
    {
    case ERR_LEXICAL:
        exit(ERR_LEXICAL);
    case ERR_SYNTACTIC:
        exit(ERR_SYNTACTIC);
    case ERR_SEM_UNDEFINED:
        exit(ERR_SEM_UNDEFINED);
    case ERR_SEM_REDEFINITION:
        exit(ERR_SEM_REDEFINITION);
    case ERR_SEM_ARG_COUNT:
        exit(ERR_SEM_ARG_COUNT);
    case ERR_SEM_TYPE_MISMATCH:
        exit(ERR_SEM_TYPE_MISMATCH);
    case ERR_SEM_OTHER:
        exit(ERR_SEM_OTHER);
    case ERR_RUNTIME_PARAM_TYPE:
        exit(ERR_RUNTIME_PARAM_TYPE);
    case ERR_RUNTIME_TYPE_MISMATCH:
        exit(ERR_RUNTIME_TYPE_MISMATCH);
    case ERR_INTERNAL:
        exit(ERR_INTERNAL);
    default:
        exit(UNKNOWN_ERR_CODE);
    }
}
