/*
 * =============================================================================
 * FISIER: vizualizare_loguri.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Functii pentru vizualizarea si cautarea in fisierele de loguri exportate
 *     anterior (fisierele CSV generate cu optiunea Export).
 * 
 * =============================================================================
 */

#ifndef VIZUALIZARE_LOGURI_H
#define VIZUALIZARE_LOGURI_H

#include "structuri_date.h"


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: listeaza_fisiere_csv
 * -----------------------------------------------------------------------------
 * Scaneaza directorul curent si afiseaza toate fisierele CSV disponibile.
 * Returneaza numarul de fisiere gasite.
 */
int listeaza_fisiere_csv(char fisiere[][256], int max_fisiere);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: incarca_fisier_csv
 * -----------------------------------------------------------------------------
 * Incarca un fisier CSV in lista globala de loguri.
 * Returneaza numarul de loguri incarcate, sau -1 la eroare.
 */
int incarca_fisier_csv(const char* nume_fisier);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: parseaza_linie_csv
 * -----------------------------------------------------------------------------
 * Parseaza o linie CSV si extrage datele intr-un LogEntry.
 */
int parseaza_linie_csv(const char* linie, LogEntry* intrare);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: meniu_vizualizare_loguri
 * -----------------------------------------------------------------------------
 * Meniu interactiv pentru vizualizarea logurilor vechi.
 * Returneaza: 0 = iesire, 1 = porneste serverul
 */
int meniu_vizualizare_loguri(void);


#endif /* VIZUALIZARE_LOGURI_H */