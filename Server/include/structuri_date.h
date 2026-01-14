/*
 * =============================================================================
 * FISIER: structuri_date.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Aici definim "formele" datelor noastre - cum arata un log, cum arata
 *     informatiile despre un client conectat, etc.
 *     
 *     In C, o structura (struct) e ca un formular cu mai multe campuri.
 *     De exemplu, un LogEntry e ca o fisa cu: timestamp, pid, nume proces, etc.
 * 
 * CE GASESTI AICI:
 *     1. LogEntry - structura pentru un log individual
 *     2. InfoClient - structura pentru un client conectat
 *     3. Variabile globale - liste de loguri si clienti
 * 
 * =============================================================================
 */

#ifndef STRUCTURI_DATE_H
#define STRUCTURI_DATE_H

/* Includem fisierul cu configurari ca sa avem acces la constante */
#include "culori_si_configurari.h"

/* Biblioteca pthread pentru mutex (lacate pentru thread-uri) */
#include <pthread.h>

/* Biblioteca signal pentru gestionarea semnalelor (Ctrl+C, etc.) */
#include <signal.h>


/*
 * =============================================================================
 * STRUCTURA: LogEntry (Intrare Log)
 * =============================================================================
 * 
 * Aceasta structura reprezinta UN SINGUR LOG primit de la un client.
 * Gandeste-te la ea ca la o linie dintr-un tabel Excel cu aceste coloane:
 * 
 * | PID | Nume | Status | CPU% | Memorie | User | Mesaj | Level | Timestamp |
 * |-----|------|--------|------|---------|------|-------|-------|-----------|
 * | 123 | chrome| running| 25.5 | 500MB   | horea| OK    | INFO  | 14:30:00  |
 */
typedef struct {
    /* === DATE DESPRE PROCES === */
    
    /* PID = Process ID, un numar unic pentru fiecare proces in sistem
     * E ca CNP-ul pentru procese - fiecare are unul unic */
    int pid;
    
    /* Numele procesului (ex: "chrome.exe", "firefox", "notepad")
     * Array de caractere = sir de caractere = string */
    char nume[LUNGIME_CAMP];
    
    /* Statusul procesului - ce face acum:
     * - "running" = ruleaza activ
     * - "sleeping" = asteapta ceva (doarme)
     * - "stopped" = oprit temporar
     * - "zombie" = proces mort care n-a fost curatat
     * - "crashed" = s-a prabusit (eroare)
     * - "static" = nu se schimba */
    char status[LUNGIME_CAMP];
    
    /* Cat la suta din procesor foloseste (0.0 - 100.0)
     * double = numar cu virgula, precizie mare */
    double procent_cpu;
    
    /* Cata memorie foloseste, in kilobytes
     * unsigned long = numar pozitiv mare (fara semn) */
    unsigned long memorie_kb;
    
    /* Utilizatorul care a pornit procesul (ex: "horea", "admin", "SYSTEM") */
    char utilizator[LUNGIME_CAMP];
    
    /* Mesajul descriptiv (ex: "Functional", "High CPU usage", "Crashed") */
    char mesaj[LUNGIME_CAMP * 2];  /* Mai lung, mesajele pot fi detaliate */
    
    /* Nivelul de importanta: "INFO", "WARN", "ERROR"
     * - INFO = totul e ok, informatie normala
     * - WARN = atentie, ceva nu e tocmai ok
     * - ERROR = problema serioasa */
    char nivel[LUNGIME_CAMP];
    
    /* Cand s-a intamplat (ex: "2024-01-15 14:30:00") */
    char timestamp[LUNGIME_CAMP];
    
    /* === METADATE CONEXIUNE === */
    /* Informatii despre de unde a venit acest log */
    
    /* IP-ul clientului care a trimis log-ul (ex: "192.168.1.100:5432") */
    char ip_client[64];
    
    /* Numele calculatorului client (ex: "DESKTOP-ABC123") */
    char hostname[LUNGIME_CAMP];
    
} LogEntry;


/*
 * =============================================================================
 * STRUCTURA: InfoClient
 * =============================================================================
 * 
 * Tine minte informatiile despre un client conectat.
 * Cand cineva se conecteaza, cream o astfel de structura pentru el.
 */
typedef struct {
    /* Socket-ul = "telefonul" prin care vorbim cu clientul
     * E un numar pe care sistemul il foloseste sa identifice conexiunea */
    int socket;
    
    /* Adresa IP a clientului (ex: "192.168.1.100:5432") */
    char ip[64];
    
} InfoClient;


/*
 * =============================================================================
 * VARIABILE GLOBALE
 * =============================================================================
 * 
 * Variabilele globale sunt accesibile din orice fisier (dupa ce le declaram
 * cu 'extern'). Le definim efectiv in fisierul principal (main).
 * 
 * 'extern' inseamna "aceasta variabila exista undeva, nu o crea aici, 
 * doar spune-mi ca pot s-o folosesc"
 */

/* === LISTA DE LOGURI === */

/* Array-ul care tine toate log-urile primite
 * E ca un tabel cu MAX_LOGURI randuri */
extern LogEntry g_lista_loguri[MAX_LOGURI];

/* Cate loguri avem in lista (0 la inceput, creste cand primim loguri) */
extern int g_numar_loguri;

/* Mutex (lacat) pentru lista de loguri
 * 
 * DE CE AVEM NEVOIE DE MUTEX?
 * Imaginea-ti ca 2 persoane incearca sa scrie in acelasi caiet simultan.
 * Rezultatul ar fi haos - textele s-ar amesteca.
 * 
 * Mutex-ul e ca un stilou magic: doar cine il tine poate scrie.
 * Ceilalti trebuie sa astepte pana il primesc.
 * 
 * pthread_mutex_lock(&g_mutex_loguri);   // "Ia stiloul"
 * // ... scrie in lista ...
 * pthread_mutex_unlock(&g_mutex_loguri); // "Da stiloul inapoi"
 */
extern pthread_mutex_t g_mutex_loguri;


/* === LISTA DE CLIENTI CONECTATI === */

/* Array de pointeri la string-uri cu IP-urile clientilor conectati */
extern char* g_clienti_conectati[MAX_CLIENTI];

/* Cati clienti sunt conectati acum */
extern int g_numar_clienti;

/* Mutex pentru lista de clienti (acelasi principiu ca mai sus) */
extern pthread_mutex_t g_mutex_clienti;


/* === STAREA SERVERULUI === */

/* Flag care indica daca serverul ruleaza (1) sau trebuie sa se opreasca (0)
 * 
 * 'volatile' ii spune compilatorului: "Aceasta variabila poate fi schimbata
 * din alta parte (alt thread, signal handler), nu optimiza accesul la ea!"
 * 
 * 'sig_atomic_t' e un tip special garantat sa fie citit/scris atomic
 * (adica nu poate fi intrerupt la mijloc) */
extern volatile sig_atomic_t g_server_ruleaza;

/* Socket-ul serverului (conexiunea principala pe care ascultam) */
extern int g_socket_server;


/* === FILTRE PENTRU AFISARE === */

/* Filtrul curent pentru nivel (INFO/WARN/ERROR sau ALL) */
extern char g_filtru_nivel[32];

/* Filtrul curent pentru status (running/crashed/etc sau ALL) */
extern char g_filtru_status[32];

/* Textul cautat (pentru functia de search) */
extern char g_text_cautat[128];


/* Compatibilitate cu denumirile vechi din cod */
#define g_logs          g_lista_loguri
#define g_log_count     g_numar_loguri
#define g_logs_mutex    g_mutex_loguri
#define g_connected_clients g_clienti_conectati
#define g_client_count  g_numar_clienti
#define g_clients_mutex g_mutex_clienti
#define g_running       g_server_ruleaza
#define g_server_socket g_socket_server
#define g_filter_level  g_filtru_nivel
#define g_filter_status g_filtru_status
#define g_search_text   g_text_cautat

/* Alias pentru structuri */
typedef InfoClient ClientInfo;

#endif /* STRUCTURI_DATE_H */