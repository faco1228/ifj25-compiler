/**
 * @file code_gen_funs.c
 * @author xracekm00, xmezeim00
 * @brief Code generator for Wren-like programming language
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <symtable.h>

/*
KOD sa printuje na stdout

Flagy, ktore mam aktualne pre jednotlive expression subtrees:

bool has_only_plus_op = true;
bool has_string_lit = false;
bool has_num_lit = false;
bool has_minus_or_slash = false;
bool has_null_lit = false;
bool has_unary_minus = false;
bool has_arit_op = false;
bool has_rel_op = false;
bool zero_divison_detected = false;
bool has_comp_op = false;

RUNTIME SEMANTIC:
zero_division: menzi odhali iba ked tam je vyslovene 0 ale ked tam je co i len 1/(2-2) tak to nedohali.
exp_type_check:...

EXTENSTION
cycles a funexp

NOTE:
Pri kazdej jednej operacii treba robit typove kontroly, jednak kvoli tomu, ze ADD potrebuje 2 int alebo 2 float
ale aj ci to sedi ked niekotra premenna je return value funkcie a niektora moze byt napr read at runtime

NOTE:
Pravdepocobne budem musiet uchovavat informaciu o tom, kolko ramcov je na zasobniku
lebo ked chcem POPFRAME pouzit na prazdnom zasobniku tak dojde k chybe.

NOTE: 
Nie je problem, ze sa k hexadecimalnym cislam spravame ako ku floatom?
V ukazke pre zapis premennej je deklaracia hexa premennej, ale nam sa to potom
nebude v codegene zhodovat s ich kodom, kedze to prevadzam na float.

*/



/*
FUNS TO IMPLEMENT
gen_type_check             ; nejako pomocou TYPE
gen_zero_division_check
gen_create_literal  -str
                    -int
                    -float
                    -hexa
gen_create_variable

gen_clean_up_frames
gen_jump_if_grater      ; Tieto dve funkcie treba preto, aby sme vedeli ci pri loopoch alebo if
gen_jump_if_lowet       ; mame ci nemame previest skok (nejako pomocou LT(S), GT(S), EQ(S))
gen_string_iter         ; CONCAT niekolko krat

funkcie pre generovanie built in funkcii
...

NODY pre ktore treba este vymysliet funkcie:
NODE_BLOCK
NODE_ASSIGN
NODE_BINARY_OP
NODE_IF
NODE_RETURN
NODE_WHILE
NODE_FOR
NODE_RANGE
NODE_BREAK
NODE_CONTINUE
*/

typedef struct {
    unsigned long long label_counter;
    unsigned long long temp_var_counter;
} name_mnglr_t, *name_mnglr_t_ptr;

//Tato premenna je sice globalna ale jej obsah sa bude menit podla toho v akej sa nachazdas funkcii
name_mnglr_t global_mnglr;

//Pre "a" * 3 (iterácia):
void string_iter(){
    // Tu sa bude z nejakeho ramcu alebo z niekadial cerpat premenna, ktora bude ako 
    // druhy argument printu, zatial pre ukazku to nechavam takto nech to neskor chapem

    printf("%s %s", "MOVE GF@result string@\n");
    printf("%s %s", "MOVE GF@counter int@0\n");
    printf("%s %s", "LABEL $loop\n");
    printf("%s %s", "JUMPIFEQ $end GF@counter int@3\n");
    printf("%s %s", "CONCAT GF@result GF@result string@a\n");
    printf("%s %s", "ADD GF@counter GF@counter int@1\n");
    printf("%s %s", "JUMP $loop\n");
    printf("%s %s", "LABEL $end\n");
}

void nmg_label(char *label, name_mnglr_t_ptr mnglr){
    // Bude robit name mangeling labelu
    // Prida predponu podla toho v akej je funkcii, to zistime z symtable
    // zakomponuje tam cislo z objektu typu name_mnglr_t
}

void nmg_variable(char* variable, name_mnglr_t_ptr mnglr){
    // Bude robit name mangeling premennej
    // Prida predponu podla toho v akej je funkcii, to zistime z symtable
    // zakomponuje tam cislo z objektu typu name_mnglr_t
}

// Will be called when entered new function
void nmg_init(name_mnglr_t_ptr mnglr){
    mnglr->label_counter = 0;
    mnglr->temp_var_counter = 0;
}

// Ked sa dostaneme do node function def
void func_start(){

    char *label = NULL;
    nmg_init(&global_mnglr);

    generate_label(&label, &global_mnglr);



    printf("%s %s\n", "LABEL", label);
    printf("%s\n", "CREATEFRAME");
    printf("%s\n", "PUSHFRAME");
}

//Called after the return node was processed and child array is empty
void func_end(){

    //Tuto treba este ale poriesit to, ze return moze mat nejaku hodnotu
    //Ak tam bdue tak sa zavolafunkcia eval expression
    //Vysledok sa priradi do nejakej temporary premennej
    //To sa pushne na datovy zasobnik
    //Az potom sa vykonaju tieto 2 instrukcie

    printf("%s\n", "POPFRAME");
    printf("%s\n", "RETURN");
}


void func_return(){

}

/*
POZNAMKY K EXPRESSION
prechadzam binarny strom postorederom, co mi simuluje postfix
vzdy mam na konci prechodu na stack-top vysledok
pocas prechodu su prve 2 veci na stacku op1 a op2
Ked narazim na node operator popnem zo stacku operandy a pomocou switcha urcim co sa ma vykonat
na to by mohla byt dobra pomocna funkcia riesiaca switch operatoru


*/