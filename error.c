/**
 * @file error.c
 * @author xcillik00
 * @brief Warningy vypisuje na stdout, error_exit ukončuje program s kódom
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error.h"

// Funkcia pre vypis warningov na stdout
// Funkcia pre vypis warningov na stdout s interpretáciou kódu chyby
void warnings(int warning, const char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("Warning [%d]: ", warning);

    // Switch pre popis typu chyby podľa kódu
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

// Funkcia pre ukoncenie programu s chybovym kodom
void error_exit(int error) {
    // Ukončenie programu s prislusnym kodom
    switch (error) {
        case 1:  exit(1);   // Lexikálna chyba
        case 2:  exit(2);   // Syntaktická chyba
        case 3:  exit(3);   // Sémantická chyba - nedefinovaná funkcia/premenná
        case 4:  exit(4);   // Redefinícia funkcie/premennej
        case 5:  exit(5);   // Neočakávaný počet argumentov / typ parametra
        case 6:  exit(6);   // Typová nekompatibilita vo výrazoch
        case 10: exit(10);  // Ostatné sémantické chyby
        case 25: exit(25);  // Behová sémantická chyba - typ parametra
        case 26: exit(26);  // Behová sémantická chyba - typová nekompatibilita
        case 99: exit(99);  // Interná chyba prekladača - napr. chybná alokácia
        default: exit(EXIT_FAILURE); // Neznámy kód chyby
    }
}