/**
 * @file error.h
 * @author xcillik00
 * @brief Deklarácie funkcií pre warningy a ukončenie programu
 * 
 * warning() vypisuje hlásenia na stdout podľa kódu chyby a program pokračuje.
 * error_exit() ukončí program s príslušným návratovým kódom.
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

// Vypíše warning na stdout podľa kódu chyby, program pokračuje
void warnings(int warning, const char *format, ...);

// Ukončí program s chybovým kódom
void error_exit(int error);

#endif