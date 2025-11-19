/**
 * @file error.h
 * @author xcillik00
 * @brief Declaration of functions used to handle exiting with corresponding error codes and 
 *        printing warnings to stdout.
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

typedef enum
{
    ERR_LEXICAL = 1,
    ERR_SYNTACTIC,
    ERR_SEM_UNDEFINED,
    ERR_SEM_REDEFINITION,
    ERR_SEM_ARG_COUNT,
    ERR_SEM_TYPE_MISMATCH,
    ERR_SEM_OTHER = 10,
    ERR_RUNTIME_PARAM_TYPE = 25,
    ERR_RUNTIME_TYPE_MISMATCH = 26,
    ERR_INTERNAL = 99,
    UNKNOWN_ERR_CODE = 100
} ERROR_CODES;

// /**
//  * @brief Prints a warning messages based on the provided warning code. 
//  * 
//  * @param warning Warning code.
//  * @param format Format of the warning message.
//  * 
//  * @note In some cases, other arguments might be provided.
//  */
// void warnings(int warning, const char *format, ...);

/**
 * @brief Handles exiting the program with a corresponding error code
 * 
 * @param error Error code to exit with.
 */
void error_exit(int error);

/**
 * @brief Register parser/PSA resources for automatic cleanup on error.
 * 
 * Tieto funkcie si bude volať parser (alebo main pred volaním parsera)
 * a `error_exit` ich pri chybe uvoľní.
 */
void error_set_parser_ast_root(void *ast_root);
void error_set_parser_func_symtable(void *func_symtable);
void error_set_parser_glob_symtable(void *glob_var_symtable);

#endif