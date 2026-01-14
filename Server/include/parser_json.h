/*
 * =============================================================================
 * FISIER: parser_json.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Functii pentru a "citi" si intelege mesajele JSON primite de la clienti.
 *     
 *     JSON (JavaScript Object Notation) e un format de date foarte popular.
 *     Arata cam asa:
 *     {
 *         "nume": "Chrome",
 *         "pid": 1234,
 *         "cpu_percent": 25.5
 *     }
 *     
 *     Acest parser e SIMPLU - nu e o biblioteca completa de JSON,
 *     dar e suficient pentru nevoile noastre.
 * 
 * =============================================================================
 */

#ifndef PARSER_JSON_H
#define PARSER_JSON_H

#include "structuri_date.h"  /* Pentru LogEntry */
#include <stddef.h>          /* Pentru size_t */


/*
 * =============================================================================
 * FUNCTII PENTRU EXTRAGERE VALORI DIN JSON
 * =============================================================================
 * Aceste functii "scot" valori din JSON dupa cheia lor.
 */

/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: json_extrage_string
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Cauta o cheie in JSON si extrage valoarea ei (de tip string/text).
 *     
 *     Din {"nume": "Chrome", "pid": 1234}
 *     cu cheia "nume" -> extrage "Chrome"
 * 
 * PARAMETRI:
 *     json - textul JSON complet
 *     cheie - ce cautam (ex: "nume", "status")
 *     valoare - unde punem rezultatul
 *     dimensiune_valoare - cat de mare e buffer-ul pentru rezultat
 * 
 * RETURNEAZA:
 *     Pointer la valoare (acelasi cu parametrul 'valoare')
 * 
 * EXEMPLU:
 *     char rezultat[100];
 *     json_extrage_string(json, "nume", rezultat, sizeof(rezultat));
 *     printf("Numele e: %s\n", rezultat);
 */
char* json_extrage_string(const char* json, const char* cheie, 
                          char* valoare, size_t dimensiune_valoare);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: json_extrage_double
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Extrage o valoare numerica cu virgula (ex: 25.5, 99.99)
 * 
 * PARAMETRI:
 *     json - textul JSON
 *     cheie - cheia valorii dorite
 * 
 * RETURNEAZA:
 *     Valoarea numerica sau 0.0 daca nu gaseste
 */
double json_extrage_double(const char* json, const char* cheie);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: json_extrage_long
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Extrage un numar intreg mare (ex: 1024000 pentru memorie in KB)
 */
long json_extrage_long(const char* json, const char* cheie);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: json_extrage_int
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Extrage un numar intreg simplu (ex: PID-ul unui proces)
 */
int json_extrage_int(const char* json, const char* cheie);


/*
 * =============================================================================
 * FUNCTII PENTRU PARSARE COMPLETA
 * =============================================================================
 * Aceste functii parseaza un JSON intreg si umplu structurile noastre.
 */

/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: parseaza_json_proces
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Ia un JSON cu informatii despre un singur proces si umple o structura
 *     LogEntry cu datele gasite.
 *     
 *     Daca anumite campuri lipsesc din JSON, pune valori default:
 *     - nume lipsa -> "unknown"
 *     - status lipsa -> "static"
 *     - mesaj lipsa -> "Functional"
 *     - nivel lipsa -> deduce din status/cpu
 * 
 * PARAMETRI:
 *     json - textul JSON de parsat
 *     intrare - structura LogEntry unde punem rezultatul
 *     ip_client - IP-ul clientului care a trimis (pentru metadata)
 * 
 * RETURNEAZA:
 *     1 daca parsarea a reusit, 0 daca JSON-ul e invalid
 */
int parseaza_json_proces(const char* json, LogEntry* intrare, const char* ip_client);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: parseaza_json_snapshot
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Parseaza un "snapshot" - un JSON care contine o LISTA de procese.
 *     
 *     Formatul asteptat:
 *     {
 *         "hostname": "PC-HOREA",
 *         "processes": [
 *             { proces 1 },
 *             { proces 2 },
 *             ...
 *         ]
 *     }
 *     
 *     Fiecare proces din lista e parsat si adaugat in lista globala de loguri.
 * 
 * PARAMETRI:
 *     json - textul JSON cu snapshot-ul
 *     ip_client - IP-ul clientului
 * 
 * RETURNEAZA:
 *     Numarul de procese parsate cu succes
 */
int parseaza_json_snapshot(const char* json, const char* ip_client);


/* Alias-uri pentru compatibilitate cu codul original */
#define json_get_string     json_extrage_string
#define json_get_double     json_extrage_double
#define json_get_long       json_extrage_long
#define json_get_int        json_extrage_int
#define parse_process_json  parseaza_json_proces
#define parse_snapshot_json parseaza_json_snapshot


#endif /* PARSER_JSON_H */