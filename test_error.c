
/**
 * @file test_error.c
 * @author xcillik00/ChatGPT
 * @brief Jednoduchy testovaci program pre error.c
 * 
 * warning() vypisuje hlásenia na stdout podľa kódu chyby a program pokračuje.
 * error_exit() ukončí program s príslušným návratovým kódom.
 */



#include <stdio.h>
#include "error.h"

int main() {
    int line = 15;

    // Testovanie warningov
    warnings(2, "Nespravna syntax v riadku %d", line);
    warnings(3, "Pouzitie nedefinovanej premennej '%s'", "x");

    printf("Program pokracuje po warningoch...\n");

    // Testovanie error_exit
    printf("Teraz sa ukonci program s kodom 2\n");
    error_exit(2);

    // Tento riadok sa uz nevykona
    printf("Tento text sa uz neukaze\n");

    return 0;
}