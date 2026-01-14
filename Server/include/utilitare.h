#ifndef UTILITARE_H
#define UTILITARE_H

#include <stddef.h>  /* Pentru size_t (tip de date pentru dimensiuni) */


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: obtine_timpul_curent
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Pune in buffer data si ora curenta in format frumos.
 *     Exemplu rezultat: "2024-01-15 14:30:45"
 * 
 * PARAMETRI:
 *     buffer - unde sa punem rezultatul (trebuie sa fie destul de mare)
 *     dimensiune - cat de mare e buffer-ul (ca sa nu scriem prea mult)
 * 
 * EXEMPLU FOLOSIRE:
 *     char timp[32];
 *     obtine_timpul_curent(timp, sizeof(timp));
 *     printf("Acum e: %s\n", timp);  // Afiseaza: "Acum e: 2024-01-15 14:30:45"
 */
void obtine_timpul_curent(char* buffer, size_t dimensiune);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: transforma_in_majuscule
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Transforma toate literele din string in MAJUSCULE.
 *     Modifica direct string-ul primit (nu creeaza unul nou).
 * 
 * PARAMETRI:
 *     str - string-ul de transformat
 * 
 * EXEMPLU:
 *     char text[] = "Hello World";
 *     transforma_in_majuscule(text);
 *     // Acum text contine "HELLO WORLD"
 */
void transforma_in_majuscule(char* str);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: transforma_in_minuscule
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Transforma toate literele din string in minuscule.
 *     Modifica direct string-ul primit.
 * 
 * PARAMETRI:
 *     str - string-ul de transformat
 * 
 * EXEMPLU:
 *     char text[] = "Hello World";
 *     transforma_in_minuscule(text);
 *     // Acum text contine "hello world"
 */
void transforma_in_minuscule(char* str);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: elimina_spatii
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Elimina spatiile de la inceputul si sfarsitul unui string.
 *     "  hello world  " devine "hello world"
 * 
 * PARAMETRI:
 *     str - string-ul de curatat
 * 
 * RETURNEAZA:
 *     Pointer catre inceputul textului curatat (poate fi diferit de str!)
 * 
 * ATENTIE:
 *     Nu creeaza un string nou, doar muta pointer-ul si pune '\0' la sfarsit.
 * 
 * EXEMPLU:
 *     char text[] = "  hello  ";
 *     char* curat = elimina_spatii(text);
 *     printf("[%s]\n", curat);  // Afiseaza: [hello]
 */
char* elimina_spatii(char* str);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: contine_text_insensitiv
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Verifica daca un string contine alt string, IGNORAND diferenta
 *     dintre majuscule si minuscule.
 *     
 *     "Hello World" contine "WORLD"? DA!
 *     "Hello World" contine "xyz"? NU!
 * 
 * PARAMETRI:
 *     text_mare - string-ul in care cautam
 *     text_cautat - ce cautam
 * 
 * RETURNEAZA:
 *     1 daca gaseste, 0 daca nu gaseste
 * 
 * EXEMPLU:
 *     if (contine_text_insensitiv("Chrome Browser", "chrome")) {
 *         printf("Gasit!\n");
 *     }
 */
int contine_text_insensitiv(const char* text_mare, const char* text_cautat);


/* 
 * =============================================================================
 * Alias-uri pentru compatibilitate cu codul original
 * =============================================================================
 * Pastram numele vechi ale functiilor ca sa mearga codul existent.
 */
#define get_current_time    obtine_timpul_curent
#define str_to_upper        transforma_in_majuscule
#define str_to_lower        transforma_in_minuscule
#define str_trim            elimina_spatii
#define str_contains_ci     contine_text_insensitiv


#endif /* UTILITARE_H */