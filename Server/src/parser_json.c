#include "parser_json.h"
#include "utilitare.h"
#include "culori_si_configurari.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>      /* Pentru localtime(), strftime(), time_t */


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: json_extrage_string
 * -----------------------------------------------------------------------------
 */
char* json_extrage_string(const char* json, const char* cheie, 
                          char* valoare, size_t dimensiune_valoare) {
    /*
     * Pas 0: Initializam rezultatul ca string gol
     * Daca nu gasim cheia, macar returnam ceva valid (string gol)
     */
    valoare[0] = '\0';
    
    /*
     * Pas 1: Construim string-ul de cautat
     * Daca cautam cheia "nume", trebuie sa gasim "\"nume\"" in JSON
     * (cheile in JSON sunt inconjurate de ghilimele)
     */
    char pattern_cautat[256];
    snprintf(pattern_cautat, sizeof(pattern_cautat), "\"%s\"", cheie);
    
    /*
     * Pas 2: Cautam cheia in JSON
     * strstr() gaseste prima aparitie a unui substring
     */
    char* pozitie = strstr(json, pattern_cautat);
    if (pozitie == NULL) {
        /* Nu am gasit cheia, returnam string gol */
        return valoare;
    }
    
    /*
     * Pas 3: Sarim peste cheie si cautam valoarea
     * Dupa "cheie" trebuie sa gasim ":" si apoi valoarea
     */
    pozitie += strlen(pattern_cautat);  /* Sarim peste "cheie" */
    
    /* Sarim peste ':' si spatiile de dupa */
    while (*pozitie && (*pozitie == ':' || *pozitie == ' ' || *pozitie == '\t')) {
        pozitie++;
    }
    
    /*
     * Pas 4: Verificam daca valoarea e string (incepe cu ghilimele)
     */
    if (*pozitie == '"') {
        pozitie++;  /* Sarim peste ghilimelele de deschidere */
        
        char* inceput = pozitie;  /* Aici incepe valoarea efectiva */
        
        /*
         * Pas 5: Gasim sfarsitul string-ului (urmatoarele ghilimele)
         * Trebuie sa avem grija la escape sequences: \" nu e sfarsitul!
         */
        while (*pozitie && *pozitie != '"') {
            if (*pozitie == '\\' && *(pozitie + 1)) {
                /* E un escape sequence (ex: \", \\, \n)
                 * Sarim 2 caractere */
                pozitie += 2;
            } else {
                pozitie++;
            }
        }
        
        /*
         * Pas 6: Copiem valoarea in buffer
         */
        size_t lungime = pozitie - inceput;
        
        /* Ne asiguram ca nu depasim buffer-ul */
        if (lungime >= dimensiune_valoare) {
            lungime = dimensiune_valoare - 1;
        }
        
        strncpy(valoare, inceput, lungime);
        valoare[lungime] = '\0';  /* Terminatorul obligatoriu */
    }
    
    return valoare;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: json_extrage_double
 * -----------------------------------------------------------------------------
 */
double json_extrage_double(const char* json, const char* cheie) {
    /* Construim pattern-ul de cautat */
    char pattern_cautat[256];
    snprintf(pattern_cautat, sizeof(pattern_cautat), "\"%s\"", cheie);
    
    char* pozitie = strstr(json, pattern_cautat);
    if (pozitie == NULL) {
        return 0.0;  /* Valoare default daca nu gasim */
    }
    
    /* Sarim la valoare */
    pozitie += strlen(pattern_cautat);
    while (*pozitie && (*pozitie == ':' || *pozitie == ' ' || *pozitie == '\t')) {
        pozitie++;
    }
    
    /*
     * atof() = ASCII to Float
     * Converteste un string in numar cu virgula
     * "25.5" -> 25.5
     * Ignora orice caractere dupa numar
     */
    return atof(pozitie);
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: json_extrage_long
 * -----------------------------------------------------------------------------
 */
long json_extrage_long(const char* json, const char* cheie) {
    char pattern_cautat[256];
    snprintf(pattern_cautat, sizeof(pattern_cautat), "\"%s\"", cheie);
    
    char* pozitie = strstr(json, pattern_cautat);
    if (pozitie == NULL) {
        return 0;
    }
    
    pozitie += strlen(pattern_cautat);
    while (*pozitie && (*pozitie == ':' || *pozitie == ' ' || *pozitie == '\t')) {
        pozitie++;
    }
    
    /*
     * atol() = ASCII to Long
     * Converteste string in numar intreg mare
     */
    return atol(pozitie);
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: json_extrage_int
 * -----------------------------------------------------------------------------
 */
int json_extrage_int(const char* json, const char* cheie) {
    /* Pur si simplu apelam versiunea long si convertim */
    return (int)json_extrage_long(json, cheie);
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: parseaza_json_proces
 * -----------------------------------------------------------------------------
 * 
 * Aceasta functie e "inima" parser-ului. Ia un JSON cu datele unui proces
 * si umple structura LogEntry.
 */
int parseaza_json_proces(const char* json, LogEntry* intrare, const char* ip_client) {
    /*
     * Pas 1: Initializam structura cu zerouri
     * memset() pune valoarea 0 pe toti octetii structurii
     * Asta ne asigura ca toate campurile sunt initializate
     */
    memset(intrare, 0, sizeof(LogEntry));
    
    /*
     * Pas 2: Verificam ca JSON-ul e valid (incepe cu '{')
     */
    const char* json_curatat = json;
    
    /* Sarim peste spatii de la inceput */
    while (*json_curatat && isspace(*json_curatat)) {
        json_curatat++;
    }
    
    /* JSON-ul trebuie sa inceapa cu '{' */
    if (*json_curatat != '{') {
        return 0;  /* JSON invalid */
    }
    
    /*
     * Pas 2.5: Verificam daca e mesaj de control (HELLO, GOODBYE)
     * Clientii trimit aceste mesaje la conectare/deconectare.
     * Nu sunt loguri de procese, deci le ignoram.
     */
    char tip_mesaj[64] = "";
    json_extrage_string(json, "type", tip_mesaj, sizeof(tip_mesaj));
    
    if (strlen(tip_mesaj) > 0) {
        /* E un mesaj de control, nu un log de proces */
        transforma_in_majuscule(tip_mesaj);
        
        if (strcmp(tip_mesaj, "HELLO") == 0 || 
            strcmp(tip_mesaj, "GOODBYE") == 0 ||
            strcmp(tip_mesaj, "PING") == 0 ||
            strcmp(tip_mesaj, "PONG") == 0) {
            /* Ignoram aceste mesaje - nu sunt loguri */
            return 0;
        }
    }
    
    /*
     * Pas 3: Extragem fiecare camp
     * Incercam mai multe variante de nume pentru flexibilitate
     */
    
    /* --- PID --- */
    intrare->pid = json_extrage_int(json, "pid");
    
    /* --- NUME PROCES --- */
    /* Incercam: "name", "app", "process", "client_name" (pentru compatibilitate) */
    json_extrage_string(json, "name", intrare->nume, sizeof(intrare->nume));
    if (strlen(intrare->nume) == 0) {
        json_extrage_string(json, "app", intrare->nume, sizeof(intrare->nume));
    }
    if (strlen(intrare->nume) == 0) {
        json_extrage_string(json, "process", intrare->nume, sizeof(intrare->nume));
    }
    if (strlen(intrare->nume) == 0) {
        json_extrage_string(json, "client_name", intrare->nume, sizeof(intrare->nume));
    }
    if (strlen(intrare->nume) == 0) {
        /* Valoare default daca nu gasim nimic */
        strncpy(intrare->nume, "unknown", sizeof(intrare->nume));
    }
    
    /* --- STATUS --- */
    json_extrage_string(json, "status", intrare->status, sizeof(intrare->status));
    if (strlen(intrare->status) == 0) {
        strncpy(intrare->status, "static", sizeof(intrare->status));
    }
    
    /* --- CPU SI MEMORIE --- */
    intrare->procent_cpu = json_extrage_double(json, "cpu_percent");
    intrare->memorie_kb = json_extrage_long(json, "memory_kb");
    
    /* --- UTILIZATOR --- */
    json_extrage_string(json, "user", intrare->utilizator, sizeof(intrare->utilizator));
    if (strlen(intrare->utilizator) == 0) {
        json_extrage_string(json, "source", intrare->utilizator, sizeof(intrare->utilizator));
    }
    if (strlen(intrare->utilizator) == 0) {
        strncpy(intrare->utilizator, "system", sizeof(intrare->utilizator));
    }
    
    /* --- MESAJ --- */
    json_extrage_string(json, "message", intrare->mesaj, sizeof(intrare->mesaj));
    if (strlen(intrare->mesaj) == 0) {
        strncpy(intrare->mesaj, "Functional", sizeof(intrare->mesaj));
    }
    
    /* --- NIVEL (INFO/WARN/ERROR) --- */
    /* Clientii pot trimite "level" sau "log_level" - acceptam ambele */
    json_extrage_string(json, "level", intrare->nivel, sizeof(intrare->nivel));
    if (strlen(intrare->nivel) == 0) {
        json_extrage_string(json, "log_level", intrare->nivel, sizeof(intrare->nivel));
    }
    
    if (strlen(intrare->nivel) == 0) {
        /*
         * Daca nivelul nu e specificat, il deducem din status si metrici
         */
        char status_mare[32];
        strncpy(status_mare, intrare->status, sizeof(status_mare));
        transforma_in_majuscule(status_mare);
        
        if (strcmp(status_mare, "CRASHED") == 0 || strcmp(status_mare, "ZOMBIE") == 0) {
            /* Proces mort = ERROR */
            strncpy(intrare->nivel, "ERROR", sizeof(intrare->nivel));
        } 
        else if (intrare->procent_cpu > 80.0 || intrare->memorie_kb > 1024 * 1024) {
            /* CPU > 80% sau memorie > 1GB = WARNING */
            strncpy(intrare->nivel, "WARN", sizeof(intrare->nivel));
        } 
        else {
            /* Altfel = INFO */
            strncpy(intrare->nivel, "INFO", sizeof(intrare->nivel));
        }
    }
    
    /* Ne asiguram ca nivelul e uppercase */
    transforma_in_majuscule(intrare->nivel);
    
    /* --- TIMESTAMP --- */
    /* Clientii pot trimite timestamp ca:
     * - String: "2024-01-15 14:30:00"
     * - Numar Unix: 1705324200
     * Acceptam ambele formate */
    json_extrage_string(json, "timestamp", intrare->timestamp, sizeof(intrare->timestamp));
    if (strlen(intrare->timestamp) == 0) {
        json_extrage_string(json, "time", intrare->timestamp, sizeof(intrare->timestamp));
    }
    if (strlen(intrare->timestamp) == 0) {
        /* Verificam daca e timestamp numeric (Unix timestamp) */
        long unix_time = json_extrage_long(json, "timestamp");
        if (unix_time > 0) {
            /* Convertim Unix timestamp in format citibil */
            time_t raw_time = (time_t)unix_time;
            struct tm* timp_local = localtime(&raw_time);
            if (timp_local != NULL) {
                strftime(intrare->timestamp, sizeof(intrare->timestamp), 
                         "%Y-%m-%d %H:%M:%S", timp_local);
            } else {
                /* Daca conversia esueaza, punem timpul curent */
                obtine_timpul_curent(intrare->timestamp, sizeof(intrare->timestamp));
            }
        } else {
            /* Daca nu avem timestamp deloc, punem timpul curent */
            obtine_timpul_curent(intrare->timestamp, sizeof(intrare->timestamp));
        }
    }
    
    /* --- HOSTNAME --- */
    json_extrage_string(json, "hostname", intrare->hostname, sizeof(intrare->hostname));
    
    /* --- IP CLIENT --- */
    strncpy(intrare->ip_client, ip_client, sizeof(intrare->ip_client));
    
    return 1;  /* Succes! */
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: parseaza_json_snapshot
 * -----------------------------------------------------------------------------
 * 
 * Un "snapshot" e un JSON care contine o lista de procese.
 * Aceasta functie parseaza lista si adauga fiecare proces in lista globala.
 */
int parseaza_json_snapshot(const char* json, const char* ip_client) {
    /* 
     * Extragem hostname-ul din snapshot 
     * (toate procesele din snapshot au acelasi hostname)
     */
    char hostname[LUNGIME_CAMP] = "";
    json_extrage_string(json, "hostname", hostname, sizeof(hostname));
    
    /*
     * Gasim inceputul array-ului de procese
     * Cautam: "processes": [
     */
    char* inceput_procese = strstr(json, "\"processes\"");
    if (inceput_procese == NULL) {
        return 0;  /* Nu am gasit array-ul */
    }
    
    /* Cautam paranteza patrata de deschidere '[' */
    char* inceput_array = strchr(inceput_procese, '[');
    if (inceput_array == NULL) {
        return 0;
    }
    
    /*
     * Parcurgem array-ul si extragem fiecare obiect JSON
     */
    char* pozitie = inceput_array + 1;  /* Dupa '[' */
    int numar_procese = 0;
    
    while (*pozitie) {
        /* Sarim peste spatii si virgule */
        while (*pozitie && isspace(*pozitie)) pozitie++;
        
        /* Verificam daca am ajuns la sfarsitul array-ului */
        if (*pozitie == ']') break;
        
        /* Sarim peste virgule dintre elemente */
        if (*pozitie == ',') { 
            pozitie++; 
            continue; 
        }
        
        /* Verificam daca incepe un obiect */
        if (*pozitie == '{') {
            /*
             * Gasim sfarsitul obiectului numarand acoladele
             * Trebuie sa tinem cont de acoladele imbricate
             */
            int adancime = 1;  /* Am intrat intr-o acolada */
            char* inceput_obiect = pozitie;
            pozitie++;  /* Trecem de '{' */
            
            while (*pozitie && adancime > 0) {
                if (*pozitie == '{') {
                    adancime++;  /* Am intrat in alta acolada */
                }
                else if (*pozitie == '}') {
                    adancime--;  /* Am iesit dintr-o acolada */
                }
                pozitie++;
            }
            
            /*
             * Acum avem obiectul JSON intre inceput_obiect si pozitie
             * Il copiem intr-un buffer separat pentru parsare
             */
            size_t lungime_obiect = pozitie - inceput_obiect;
            char* obiect = malloc(lungime_obiect + 1);
            
            if (obiect == NULL) {
                /* Eroare la alocare memorie - continuam cu urmatorul */
                continue;
            }
            
            strncpy(obiect, inceput_obiect, lungime_obiect);
            obiect[lungime_obiect] = '\0';
            
            /*
             * Parsam obiectul JSON ca un proces individual
             */
            LogEntry intrare;
            if (parseaza_json_proces(obiect, &intrare, ip_client)) {
                /* Daca nu are hostname, il punem pe cel din snapshot */
                if (strlen(intrare.hostname) == 0 && strlen(hostname) > 0) {
                    strncpy(intrare.hostname, hostname, sizeof(intrare.hostname));
                }
                
                /*
                 * Adaugam in lista globala de loguri
                 * Trebuie sa folosim mutex pentru thread-safety!
                 */
                pthread_mutex_lock(&g_mutex_loguri);
                
                if (g_numar_loguri < MAX_LOGURI) {
                    /* Avem loc, adaugam la sfarsit */
                    g_lista_loguri[g_numar_loguri] = intrare;
                    g_numar_loguri++;
                } else {
                    /* Lista e plina, stergem primul si adaugam la sfarsit (FIFO) */
                    memmove(&g_lista_loguri[0], &g_lista_loguri[1], 
                            (MAX_LOGURI - 1) * sizeof(LogEntry));
                    g_lista_loguri[MAX_LOGURI - 1] = intrare;
                }
                
                pthread_mutex_unlock(&g_mutex_loguri);
                
                numar_procese++;
            }
            
            /* Eliberam memoria */
            free(obiect);
        } 
        else {
            /* Caracter neasteptat, trecem mai departe */
            pozitie++;
        }
    }
    
    return numar_procese;
}