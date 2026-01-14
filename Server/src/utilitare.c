/* Includem header-ul nostru cu declaratiile functiilor */
#include "utilitare.h"

/* Biblioteci standard necesare */
#include <stdio.h>      /* Pentru sprintf si alte functii de I/O */
#include <stdlib.h>     /* Pentru malloc, free, etc. */
#include <string.h>     /* Pentru strlen, strcpy, strstr, etc. */
#include <time.h>       /* Pentru time(), localtime(), strftime() */
#include <ctype.h>      /* Pentru toupper(), tolower(), isspace() */


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: obtine_timpul_curent
 * -----------------------------------------------------------------------------
 */
void obtine_timpul_curent(char* buffer, size_t dimensiune) {
    /*
     * Pas 1: Obtinem timpul curent
     * time(NULL) returneaza numarul de secunde de la 1 ianuarie 1970
     * (asa numitul "Unix timestamp" sau "epoch time")
     */
    time_t acum = time(NULL);
    
    /*
     * Pas 2: Convertim in structura usor de citit
     * localtime() transforma timestamp-ul intr-o structura cu:
     * - tm_year = anul (de la 1900)
     * - tm_mon = luna (0-11)
     * - tm_mday = ziua (1-31)
     * - tm_hour, tm_min, tm_sec = ora, minut, secunda
     */
    struct tm* timp_local = localtime(&acum);
    
    /*
     * Pas 3: Formatam frumos in string
     * strftime() e ca printf dar pentru timp
     * %Y = an cu 4 cifre (2024)
     * %m = luna cu 2 cifre (01-12)
     * %d = ziua cu 2 cifre (01-31)
     * %H = ora cu 2 cifre, format 24h (00-23)
     * %M = minute (00-59)
     * %S = secunde (00-59)
     */
    strftime(buffer, dimensiune, "%Y-%m-%d %H:%M:%S", timp_local);
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: transforma_in_majuscule
 * -----------------------------------------------------------------------------
 */
void transforma_in_majuscule(char* str) {
    /*
     * Parcurgem fiecare caracter din string pana la '\0' (terminatorul)
     * str[i] acceseaza caracterul de pe pozitia i
     */
    for (int i = 0; str[i] != '\0'; i++) {
        /*
         * toupper() transforma 'a' -> 'A', 'b' -> 'B', etc.
         * Daca caracterul nu e litera mica, il lasa neschimbat.
         */
        str[i] = toupper(str[i]);
    }
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: transforma_in_minuscule
 * -----------------------------------------------------------------------------
 */
void transforma_in_minuscule(char* str) {
    /* Acelasi principiu ca mai sus, dar cu tolower() */
    for (int i = 0; str[i] != '\0'; i++) {
        /*
         * tolower() transforma 'A' -> 'a', 'B' -> 'b', etc.
         */
        str[i] = tolower(str[i]);
    }
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: elimina_spatii
 * -----------------------------------------------------------------------------
 */
char* elimina_spatii(char* str) {
    /*
     * Pas 1: Sarim peste spatiile de la inceput
     * isspace() verifica daca e spatiu, tab, newline, etc.
     * (unsigned char) e necesar pentru a evita probleme cu caractere speciale
     */
    while (isspace((unsigned char)*str)) {
        str++;  /* Mutam pointer-ul la urmatorul caracter */
    }
    
    /*
     * Pas 2: Verificam daca am ajuns la sfarsit (string gol sau doar spatii)
     * '\0' e caracterul null care marcheaza sfarsitul oricarui string in C
     */
    if (*str == '\0') {
        return str;  /* Returnam string gol */
    }
    
    /*
     * Pas 3: Gasim sfarsitul string-ului si mergem inapoi cat sunt spatii
     * strlen() returneaza lungimea string-ului (fara '\0')
     */
    char* sfarsit = str + strlen(str) - 1;  /* Pointer la ultimul caracter */
    
    /* Mergem inapoi cat timp gasim spatii */
    while (sfarsit > str && isspace((unsigned char)*sfarsit)) {
        sfarsit--;
    }
    
    /*
     * Pas 4: Punem terminatorul dupa ultimul caracter non-spatiu
     * Asta "taie" spatiile de la sfarsit
     */
    sfarsit[1] = '\0';
    
    return str;  /* Returnam pointer-ul la inceputul curatat */
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: contine_text_insensitiv
 * -----------------------------------------------------------------------------
 */
int contine_text_insensitiv(const char* text_mare, const char* text_cautat) {
    /*
     * Pas 1: Verificam cazurile speciale
     */
    
    /* Daca nu avem ce cauta sau cautam string gol, consideram ca "am gasit" */
    if (text_cautat == NULL || *text_cautat == '\0') {
        return 1;
    }
    
    /* Daca textul in care cautam e NULL, clar nu gasim nimic */
    if (text_mare == NULL) {
        return 0;
    }
    
    /*
     * Pas 2: Facem copii ale string-urilor pe care le transformam in minuscule
     * 
     * strdup() = string duplicate, creeaza o COPIE a string-ului
     * E important sa facem copii pentru ca:
     * - Nu vrem sa modificam string-urile originale
     * - transforma_in_minuscule() modifica string-ul primit
     * 
     * ATENTIE: strdup() foloseste malloc(), deci trebuie sa facem free()!
     */
    char* copie_mare = strdup(text_mare);
    char* copie_cautat = strdup(text_cautat);
    
    /* Transformam ambele in minuscule pentru comparatie corecta */
    transforma_in_minuscule(copie_mare);
    transforma_in_minuscule(copie_cautat);
    
    /*
     * Pas 3: Cautam substring-ul
     * strstr() cauta un string in alt string
     * Returneaza pointer la prima aparitie sau NULL daca nu gaseste
     */
    int rezultat = (strstr(copie_mare, copie_cautat) != NULL);
    
    /*
     * Pas 4: Eliberam memoria alocata de strdup()
     * FOARTE IMPORTANT! Fara free() am avea memory leak.
     * Memory leak = memorie alocata dar niciodata eliberata
     */
    free(copie_mare);
    free(copie_cautat);
    
    return rezultat;  /* 1 daca am gasit, 0 daca nu */
}