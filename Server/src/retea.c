#define _GNU_SOURCE  /* Necesar pentru unele extensii POSIX */

#include "retea.h"
#include "structuri_date.h"
#include "parser_json.h"
#include "afisare.h"
#include "utilitare.h"
#include "culori_si_configurari.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>         /* Pentru close(), sleep() */
#include <sys/socket.h>     /* Pentru socket(), bind(), listen(), accept(), recv(), send() */
#include <netinet/in.h>     /* Pentru struct sockaddr_in */
#include <arpa/inet.h>      /* Pentru inet_ntoa() */
#include <pthread.h>        /* Pentru pthread_create(), pthread_detach() */
#include <errno.h>          /* Pentru errno */
#include <ctype.h>          /* Pentru isspace() */


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: thread_gestionare_client
 * -----------------------------------------------------------------------------
 * 
 * Aceasta functie e executata de un thread separat pentru FIECARE client.
 * Gandeste-te la ea ca la un "angajat" dedicat unui singur client.
 */
void* thread_gestionare_client(void* arg) {
    /*
     * Pas 1: Extragem informatiile despre client
     * 
     * arg e un pointer generic (void*) pe care il convertim la tipul nostru.
     * Facem asta pentru ca pthread_create() cere void* ca parametru.
     */
    InfoClient* info = (InfoClient*)arg;
    int socket_client = info->socket;
    
    char ip_client[64];
    strncpy(ip_client, info->ip, sizeof(ip_client));
    
    /* Eliberam structura - nu mai avem nevoie de ea
     * Datele importante le-am copiat deja */
    free(info);
    
    /*
     * Pas 2: Adaugam clientul in lista de clienti conectati
     * 
     * IMPORTANT: Folosim mutex pentru ca mai multe thread-uri pot incerca
     * sa modifice lista simultan!
     */
    pthread_mutex_lock(&g_mutex_clienti);  /* Blocam accesul altora */
    
    if (g_numar_clienti < MAX_CLIENTI) {
        /* strdup() creeaza o copie a string-ului (cu malloc intern) */
        g_clienti_conectati[g_numar_clienti] = strdup(ip_client);
        g_numar_clienti++;
    }
    
    pthread_mutex_unlock(&g_mutex_clienti);  /* Deblocam */
    
    /*
     * Pas 3: Trimitem mesaj de confirmare catre client
     * 
     * Asta ii spune clientului ca s-a conectat cu succes.
     */
    char mesaj_bun_venit[512];
    char timestamp[64];
    obtine_timpul_curent(timestamp, sizeof(timestamp));
    
    snprintf(mesaj_bun_venit, sizeof(mesaj_bun_venit), 
             "{\"connection_status\":\"connected\","
             "\"message\":\"Conectat cu succes la server!\","
             "\"server_port\":%d,"
             "\"timestamp\":\"%s\"}\n",
             SERVER_PORT, timestamp);
    
    /* send() trimite date prin socket
     * Parametri: socket, date, lungime, flags (0 = default) */
    send(socket_client, mesaj_bun_venit, strlen(mesaj_bun_venit), 0);
    
    /*
     * Pas 4: Bucla principala - primim date de la client
     */
    char buffer[DIMENSIUNE_BUFFER];
    char buffer_date[DIMENSIUNE_BUFFER * 4] = "";  /* Buffer mai mare pentru JSON-uri fragmentate */
    
    while (g_server_ruleaza) {
        /*
         * recv() citeste date de la client
         * BLOCHEAZA pana primeste ceva sau clientul se deconecteaza
         * 
         * Returneaza:
         * - numar pozitiv = cati octeti am primit
         * - 0 = clientul s-a deconectat normal
         * - -1 = eroare
         */
        ssize_t octeti_primiti = recv(socket_client, buffer, DIMENSIUNE_BUFFER - 1, 0);
        
        if (octeti_primiti <= 0) {
            /* Clientul s-a deconectat sau eroare - iesim din bucla */
            break;
        }
        
        /* Adaugam terminatorul de string */
        buffer[octeti_primiti] = '\0';
        
        /*
         * Pas 5: Adaugam datele primite in buffer-ul nostru
         * 
         * De ce buffer separat? Pentru ca un JSON mare poate veni in mai
         * multe "bucati" (fragmente TCP). Trebuie sa le asamblam.
         */
        size_t lungime_curenta = strlen(buffer_date);
        
        if (lungime_curenta + octeti_primiti < sizeof(buffer_date) - 1) {
            strcat(buffer_date, buffer);
        } else {
            /* Buffer overflow - resetam (nu ar trebui sa se intample) */
            strncpy(buffer_date, buffer, sizeof(buffer_date) - 1);
        }
        
        /*
         * Pas 6: Procesam JSON-urile complete din buffer
         * 
         * Un JSON e complet cand numarul de '{' e egal cu numarul de '}'.
         * Trebuie sa fim atenti la ghilimele - nu numaram acoladele din string-uri.
         */
        char* start = buffer_date;
        
        while (*start) {
            /* Sarim peste spatii */
            while (*start && isspace(*start)) start++;
            
            /* Verificam daca incepe un JSON */
            if (*start != '{') {
                if (*start) start++;
                continue;
            }
            
            /* Numaram acoladele pentru a gasi sfarsitul JSON-ului */
            int adancime = 0;
            char* pozitie = start;
            int in_string = 0;  /* Flag: suntem in interiorul unui string? */
            
            while (*pozitie) {
                /* Detectam intrarea/iesirea din string-uri */
                if (*pozitie == '"' && (pozitie == start || *(pozitie-1) != '\\')) {
                    in_string = !in_string;
                }
                
                /* Numaram acoladele doar daca nu suntem in string */
                if (!in_string) {
                    if (*pozitie == '{') {
                        adancime++;
                    }
                    else if (*pozitie == '}') {
                        adancime--;
                        
                        if (adancime == 0) {
                            /* Am gasit un JSON complet! */
                            pozitie++;  /* Include si ultima '}' */
                            
                            /* Extragem JSON-ul */
                            size_t lungime_json = pozitie - start;
                            char* json = malloc(lungime_json + 1);
                            
                            if (json != NULL) {
                                strncpy(json, start, lungime_json);
                                json[lungime_json] = '\0';
                                
                                /*
                                 * Verificam tipul de JSON:
                                 * - Daca contine "processes" -> e un snapshot (lista de procese)
                                 * - Altfel -> e un singur proces
                                 */
                                if (strstr(json, "\"processes\"") != NULL) {
                                    parseaza_json_snapshot(json, ip_client);
                                } else {
                                    /* Parsam ca proces individual si adaugam in lista */
                                    LogEntry intrare;
                                    if (parseaza_json_proces(json, &intrare, ip_client)) {
                                        pthread_mutex_lock(&g_mutex_loguri);
                                        
                                        if (g_numar_loguri < MAX_LOGURI) {
                                            g_lista_loguri[g_numar_loguri] = intrare;
                                            g_numar_loguri++;
                                        } else {
                                            /* Lista plina - stergem primul (FIFO) */
                                            memmove(&g_lista_loguri[0], &g_lista_loguri[1], 
                                                    (MAX_LOGURI - 1) * sizeof(LogEntry));
                                            g_lista_loguri[MAX_LOGURI - 1] = intrare;
                                        }
                                        
                                        pthread_mutex_unlock(&g_mutex_loguri);
                                    }
                                }
                                
                                free(json);
                            }
                            
                            /* Mutam restul buffer-ului la inceput */
                            memmove(buffer_date, pozitie, strlen(pozitie) + 1);
                            start = buffer_date;
                            continue;  /* Continuam sa cautam alte JSON-uri */
                        }
                    }
                }
                pozitie++;
            }
            
            /* Nu am gasit JSON complet - asteptam mai multe date */
            break;
        }
    }
    
    /*
     * Pas 7: Clientul s-a deconectat - facem curatenie
     */
    
    /* Eliminam clientul din lista */
    pthread_mutex_lock(&g_mutex_clienti);
    
    for (int i = 0; i < g_numar_clienti; i++) {
        if (strcmp(g_clienti_conectati[i], ip_client) == 0) {
            /* Eliberam memoria string-ului */
            free(g_clienti_conectati[i]);
            
            /* Mutam restul elementelor cu o pozitie la stanga */
            for (int j = i; j < g_numar_clienti - 1; j++) {
                g_clienti_conectati[j] = g_clienti_conectati[j + 1];
            }
            
            g_numar_clienti--;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_mutex_clienti);
    
    /* Inchidem socket-ul */
    close(socket_client);
    
    return NULL;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: thread_server
 * -----------------------------------------------------------------------------
 * 
 * Thread-ul principal al serverului - accepta conexiuni noi.
 */
void* thread_server(void* arg) {
    (void)arg;  /* Marcam parametrul ca nefolosit (evita warning) */
    
    /*
     * Pas 1: Cream socket-ul server
     * 
     * socket() creeaza un nou socket
     * AF_INET = familia de adrese IPv4
     * SOCK_STREAM = TCP (stream de date, ordonate si sigure)
     * 0 = protocolul default pentru tipul ales (TCP)
     */
    g_socket_server = socket(AF_INET, SOCK_STREAM, 0);
    
    if (g_socket_server < 0) {
        perror("Eroare la creare socket");  /* perror() afiseaza eroarea sistem */
        return NULL;
    }
    
    /*
     * Pas 2: Setam optiunea SO_REUSEADDR
     * 
     * Asta permite sa refolosim portul imediat dupa ce oprim serverul.
     * Fara asta, daca oprim si repornim rapid, primim eroare "Address already in use"
     * pentru ca sistemul tine portul "rezervat" cateva minute.
     */
    int optiune = 1;
    setsockopt(g_socket_server, SOL_SOCKET, SO_REUSEADDR, &optiune, sizeof(optiune));
    
    /*
     * Pas 3: Configuram adresa serverului
     * 
     * sockaddr_in = structura care tine o adresa de retea
     */
    struct sockaddr_in adresa_server;
    memset(&adresa_server, 0, sizeof(adresa_server));  /* Initializam cu zerouri */
    
    adresa_server.sin_family = AF_INET;              /* IPv4 */
    adresa_server.sin_addr.s_addr = INADDR_ANY;      /* Acceptam conexiuni de pe orice adresa (0.0.0.0) */
    adresa_server.sin_port = htons(SERVER_PORT);     /* Portul nostru */
    
    /* htons() = Host TO Network Short
     * Converteste numarul din formatul calculatorului in formatul retelei
     * (ordinea octetilor poate diferi) */
    
    /*
     * Pas 4: Legam socket-ul de adresa (bind)
     * 
     * Asta "rezerva" portul pentru noi.
     */
    if (bind(g_socket_server, (struct sockaddr*)&adresa_server, sizeof(adresa_server)) < 0) {
        perror("Eroare la bind");
        close(g_socket_server);
        return NULL;
    }
    
    /*
     * Pas 5: Incepem sa ascultam pentru conexiuni (listen)
     * 
     * MAX_CLIENTI = cate conexiuni pot astepta in coada
     */
    if (listen(g_socket_server, MAX_CLIENTI) < 0) {
        perror("Eroare la listen");
        close(g_socket_server);
        return NULL;
    }
    
    /*
     * Pas 6: Setam un timeout pentru accept()
     * 
     * Asta ne permite sa verificam periodic daca serverul trebuie oprit.
     * Fara timeout, accept() ar bloca la infinit.
     */
    struct timeval timeout;
    timeout.tv_sec = 1;   /* 1 secunda */
    timeout.tv_usec = 0;  /* 0 microsecunde */
    setsockopt(g_socket_server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    /*
     * Pas 7: Bucla principala - acceptam clienti
     */
    while (g_server_ruleaza) {
        struct sockaddr_in adresa_client;
        socklen_t lungime_adresa = sizeof(adresa_client);
        
        /*
         * accept() asteapta un client sa se conecteze
         * Returneaza un NOU socket pentru comunicarea cu acel client
         * Socket-ul original continua sa asculte pentru alti clienti
         */
        int socket_client = accept(g_socket_server, 
                                   (struct sockaddr*)&adresa_client, 
                                   &lungime_adresa);
        
        if (socket_client < 0) {
            /* Verificam daca e doar timeout (EAGAIN/EWOULDBLOCK) sau eroare reala */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Timeout - nu e problema, continuam sa asteptam */
                continue;
            }
            
            if (g_server_ruleaza) {
                /* Eroare reala */
                perror("Eroare la accept");
            }
            continue;
        }
        
        /*
         * Pas 8: Cream structura cu informatiile clientului
         */
        InfoClient* info = malloc(sizeof(InfoClient));
        info->socket = socket_client;
        
        /* Formatam IP-ul clientului ca "IP:PORT" */
        snprintf(info->ip, sizeof(info->ip), "%s:%d",
                 inet_ntoa(adresa_client.sin_addr),   /* Converteste IP in string */
                 ntohs(adresa_client.sin_port));      /* Converteste portul din format retea */
        
        /*
         * Pas 9: Cream un thread nou pentru acest client
         * 
         * Fiecare client are propriul thread, asa pot comunica toti simultan.
         */
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, thread_gestionare_client, info);
        
        /* Detach = nu asteptam sa se termine, ruleaza independent */
        pthread_detach(thread_id);
    }
    
    /* Curatenie - inchidem socket-ul server */
    close(g_socket_server);
    
    return NULL;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: thread_refresh_automat
 * -----------------------------------------------------------------------------
 * 
 * Thread care actualizeaza ecranul cand apar loguri noi.
 */
void* thread_refresh_automat(void* arg) {
    (void)arg;  /* Nefolosit */
    
    int numar_anterior = 0;  /* Cate loguri erau la ultima verificare */
    
    while (g_server_ruleaza) {
        /* Asteptam 1 secunda */
        sleep(1);
        
        /* Verificam daca s-a schimbat numarul de loguri */
        pthread_mutex_lock(&g_mutex_loguri);
        int numar_curent = g_numar_loguri;
        pthread_mutex_unlock(&g_mutex_loguri);
        
        if (numar_curent != numar_anterior) {
            /* Au aparut loguri noi - actualizam ecranul */
            numar_anterior = numar_curent;
            actualizeaza_afisare();
        }
    }
    
    return NULL;
}