/*
 * =============================================================================
 * FISIER: terminal.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Functii pentru controlul terminalului - citirea input-ului de la tastatura
 *     fara a astepta Enter, gestionarea semnalelor (Ctrl+C), etc.
 * 
 * DE CE E COMPLICAT?
 *     In mod normal, terminalul Linux functioneaza in "modul linie":
 *     - Asteapta pana apesi Enter
 *     - Afiseaza ce tastezi (echo)
 *     
 *     Dar noi vrem sa reactionam IMEDIAT cand apesi o tasta (L, S, Q, etc.)
 *     fara sa fie nevoie de Enter. Pentru asta trebuie sa punem terminalul
 *     in "modul raw" (brut).
 * 
 * =============================================================================
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>  /* Pentru size_t */


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: seteaza_terminal_raw
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Pune terminalul in modul "raw" (brut):
 *     - Nu mai asteapta Enter pentru a citi input
 *     - Nu mai afiseaza tastele apasate (echo off)
 *     - Citirea returneaza imediat (sau dupa un timeout mic)
 *     
 *     IMPORTANT: Trebuie sa apelezi seteaza_terminal_normal() inainte de iesire!
 */
void seteaza_terminal_raw(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: seteaza_terminal_normal
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Restaureaza terminalul la modul normal (cum era inainte).
 *     
 *     TREBUIE APELATA:
 *     - Inainte de a iesi din program
 *     - Inainte de a citi o linie intreaga (pentru Search, de exemplu)
 */
void seteaza_terminal_normal(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: citeste_caracter
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Citeste UN SINGUR caracter de la tastatura.
 *     Daca nu e nicio tasta apasata, returneaza 0.
 *     
 *     NOTA: Functioneaza doar in modul raw!
 * 
 * RETURNEAZA:
 *     Caracterul apasat sau 0 daca nu s-a apasat nimic
 */
char citeste_caracter(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: citeste_linie
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Citeste o linie intreaga de text (pana la Enter).
 *     Temporar trece terminalul in modul normal pentru asta.
 * 
 * PARAMETRI:
 *     buffer - unde sa punem textul citit
 *     dimensiune - cat de mare e buffer-ul
 */
void citeste_linie(char* buffer, size_t dimensiune);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: handler_semnal
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Aceasta functie e apelata AUTOMAT de sistem cand primim un semnal
 *     (de exemplu Ctrl+C = SIGINT).
 *     
 *     Seteaza g_server_ruleaza = 0 pentru a opri serverul graceful.
 * 
 * PARAMETRI:
 *     sig - numarul semnalului primit (nefolosit)
 */
void handler_semnal(int sig);


/* Alias-uri pentru compatibilitate */
#define set_terminal_raw    seteaza_terminal_raw
#define set_terminal_normal seteaza_terminal_normal
#define read_char           citeste_caracter
#define read_line           citeste_linie
#define signal_handler      handler_semnal


#endif /* TERMINAL_H */