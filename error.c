/**
 * @file error.c
 * @author xcillik00
 * @brief Handles different errors that might come up during compilation. Warnings are printed to stdout,
 *        error_exit() handles exiting the program with the corresponding error code.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error.h"


/**
 * @brief Prints a warning messages based on the provided warning code. 
 * 
 * @param warning Warning code.
 * @param format Format of the warning message.
 * 
 * @note In some cases, other arguments might be provided.
 */
void warnings(int warning, const char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("Warning [%d]: ", warning);

    switch (warning) {
        case 1:  printf("Lexikálna chyba - "); break;
        case 2:  printf("Syntaktická chyba - "); break;
        case 3:  printf("Sémantická chyba - nedefinovaná funkcia/premenná - "); break;
        case 4:  printf("Redefinícia funkcie/premennej - "); break;
        case 5:  printf("Neočekávaný počet argumentov / typ parametra - "); break;
        case 6:  printf("Typová nekompatibilita vo výrazoch - "); break;
        case 10: printf("Ostatné sémantické chyby - "); break;
        case 25: printf("Behová sémantická chyba - typ parametra - "); break;
        case 26: printf("Behová sémantická chyba - typová nekompatibilita - "); break;
        case 99: printf("Interná chyba prekladača - "); break;
        default: printf("Neznámy kód chyby - "); break;
    }

    vprintf(format, args);
    printf("\n");

    va_end(args);
}

/**
 * @brief Handles exiting the program with a corresponding error code
 * 
 * @param error Error code to exit with.
 */
void error_exit(int error) {
    switch (error) {
        case ERR_LEXICAL: exit(ERR_LEXICAL);
        case ERR_SYNTACTIC: exit(ERR_SYNTACTIC);
        case ERR_SEM_UNDEFINED: exit(ERR_SEM_UNDEFINED);
        case ERR_SEM_REDEFINITION: exit(ERR_SEM_REDEFINITION);
        case ERR_SEM_ARG_COUNT: exit(ERR_SEM_ARG_COUNT);
        case ERR_SEM_TYPE_MISMATCH: exit(ERR_SEM_TYPE_MISMATCH);
        case ERR_SEM_OTHER: exit(ERR_SEM_OTHER);
        case ERR_RUNTIME_PARAM_TYPE: exit(ERR_RUNTIME_PARAM_TYPE);
        case ERR_RUNTIME_TYPE_MISMATCH: exit(ERR_RUNTIME_TYPE_MISMATCH);
        case ERR_INTERNAL: exit(ERR_INTERNAL);
        default: exit(UNKNOWN_ERR_CODE);
    }
}