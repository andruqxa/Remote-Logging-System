/*
 * =============================================================================
 * FISIER: afisare.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Functii pentru afisarea datelor in terminal - interfata cu utilizatorul.
 *     Aici e tot ce tine de "cum arata" serverul: tabele, culori, meniuri.
 * 
 * CE GASESTI AICI:
 *     - Curatare ecran
 *     - Afisare header cu status server
 *     - Afisare tabel cu loguri (colorat)
 *     - Afisare meniu comenzi
 *     - Refresh complet display
 * 
 * =============================================================================
 */

#ifndef AFISARE_H
#define AFISARE_H

#include "structuri_date.h"  /* Pentru LogEntry */


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: curata_ecranul
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Sterge tot ce e pe ecran si muta cursorul in coltul stanga-sus.
 *     E ca si cum ai apasa "cls" in Windows sau "clear" in Linux.
 */
void curata_ecranul(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: afiseaza_linie_log
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Afiseaza UN SINGUR LOG pe o linie, cu culori in functie de nivel si status.
 *     
 *     Exemplu output:
 *     [  42] 2024-01-15 14:30:00  1234 chrome.exe   horea    RUNNING  INFO  25.5%  500MB Functional
 * 
 * PARAMETRI:
 *     intrare - log-ul de afisat
 *     index - numarul liniei (pentru afisare)
 */
void afiseaza_linie_log(const LogEntry* intrare, int index);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: afiseaza_antet
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Afiseaza partea de sus a interfetei:
 *     - Titlul serverului
 *     - Statusul (activ, port, numar clienti, numar loguri)
 *     - Lista clientilor conectati
 *     - Filtrele active
 *     - Header-ul tabelului
 */
void afiseaza_antet(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: afiseaza_meniu
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Afiseaza meniul cu comenzile disponibile la baza ecranului.
 *     L=Level, S=Status, F=Search, C=Clear, E=Export, R=Refresh, Q=Quit
 */
void afiseaza_meniu(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: actualizeaza_afisare
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Reface TOATA interfata: antet + loguri filtrate + meniu.
 *     Apeleaza toate functiile de mai sus in ordine.
 *     
 *     Aceasta e functia pe care o apelam cand vrem sa "refresh" ecranul.
 */
void actualizeaza_afisare(void);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: trece_filtrul
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Verifica daca un log trece prin filtrele curente.
 *     
 *     Un log trece daca:
 *     1. Nivelul lui corespunde filtrului de nivel (sau filtrul e "ALL")
 *     2. Statusul lui corespunde filtrului de status (sau filtrul e "ALL")
 *     3. Contine textul cautat (daca e setat vreun text)
 * 
 * PARAMETRI:
 *     intrare - log-ul de verificat
 * 
 * RETURNEAZA:
 *     1 daca trece filtrele, 0 daca nu
 */
int trece_filtrul(const LogEntry* intrare);


/* Alias-uri pentru compatibilitate */
#define clear_screen        curata_ecranul
#define print_log_entry     afiseaza_linie_log
#define print_header        afiseaza_antet
#define print_menu          afiseaza_meniu
#define refresh_display     actualizeaza_afisare
#define passes_filter       trece_filtrul


#endif /* AFISARE_H */