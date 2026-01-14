/* 
 * Activeaza extensii GNU/Linux in bibliotecile standard
 */
#define _GNU_SOURCE

/* Includem header-ele noastre (in ordine logica) */
#include "culori_si_configurari.h"   /* Configurari si culori */
#include "structuri_date.h"          /* Structuri de date */
#include "utilitare.h"               /* Functii utilitare */
#include "parser_json.h"             /* Parser JSON */
#include "afisare.h"                 /* Functii de afisare */
#include "retea.h"                   /* Functii de retea */
#include "export.h"                  /* Functii de export */
#include "terminal.h"                /* Control terminal */
#include "vizualizare_loguri.h"      /* Vizualizare loguri vechi */

/* Biblioteci standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>       /* Pentru signal() */
#include <pthread.h>      /* Pentru pthread_create(), pthread_join() */
#include <unistd.h>       /* Pentru sleep() */
#include <ctype.h>        /* Pentru toupper() */


/*
 * =============================================================================
 * DEFINIREA VARIABILELOR GLOBALE
 * =============================================================================
 */

/* Lista de loguri primite */
LogEntry g_lista_loguri[MAX_LOGURI];
int g_numar_loguri = 0;
pthread_mutex_t g_mutex_loguri = PTHREAD_MUTEX_INITIALIZER;

/* Lista de clienti conectati */
char* g_clienti_conectati[MAX_CLIENTI];
int g_numar_clienti = 0;
pthread_mutex_t g_mutex_clienti = PTHREAD_MUTEX_INITIALIZER;

/* Starea serverului */
volatile sig_atomic_t g_server_ruleaza = 1;
int g_socket_server = -1;

/* Filtre pentru afisare */
char g_filtru_nivel[32] = "ALL";
char g_filtru_status[32] = "ALL";
char g_text_cautat[128] = "";


/*
 * =============================================================================
 * FUNCTIE: afiseaza_meniu_principal
 * =============================================================================
 * Afiseaza meniul principal al aplicatiei cu cele 2 optiuni.
 */
void afiseaza_meniu_principal(void) {
    /* Curatam ecranul */
    printf("\033[2J\033[H");
    
    /* Banner principal */
    printf(FUNDAL_ALBASTRU ALB BOLD);
    printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("                                    PROCESS LOG SYSTEM v1.0                                            \n");
    printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf(RESET);
    
    printf("\n\n");
    printf(CYAN "                              Bine ai venit in sistemul de logare!\n" RESET);
    printf(DIM "                    Acest program permite monitorizarea proceselor in timp real\n");
    printf("                         sau vizualizarea logurilor salvate anterior.\n" RESET);
    printf("\n\n");
    
    /* Optiunile */
    printf("     ╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("     ║                                                                           ║\n");
    printf("     ║  " VERDE BOLD "[1]" RESET "  " BOLD "PORNESTE SERVERUL" RESET "                                                    ║\n");
    printf("     ║       Asculta pe portul %d pentru conexiuni de la clienti.              ║\n", SERVER_PORT);
    printf("     ║       Afiseaza logurile in timp real intr-un tabel colorat.              ║\n");
    printf("     ║                                                                           ║\n");
    printf("     ║  " MAGENTA BOLD "[2]" RESET "  " BOLD "VIZUALIZEAZA LOGURI VECHI" RESET "                                           ║\n");
    printf("     ║       Incarca si cauta in fisierele CSV exportate anterior.              ║\n");
    printf("     ║       Afiseaza statistici si permite filtrare.                           ║\n");
    printf("     ║                                                                           ║\n");
    printf("     ║  " ROSU BOLD "[0]" RESET "  " BOLD "IESIRE" RESET "                                                                ║\n");
    printf("     ║                                                                           ║\n");
    printf("     ╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    printf("\n\n");
    printf("     Introdu optiunea dorita: ");
    fflush(stdout);
}


/*
 * =============================================================================
 * FUNCTIE: ruleaza_mod_server
 * =============================================================================
 * Porneste serverul TCP si intra in modul de ascultare/afisare loguri.
 * Acesta era comportamentul original al programului.
 */
void ruleaza_mod_server(void) {
    /*
     * Configurarea handler-elor pentru semnale
     */
    signal(SIGINT, handler_semnal);   /* Ctrl+C -> opreste serverul */
    signal(SIGTERM, handler_semnal);  /* kill -> opreste serverul */
    signal(SIGPIPE, SIG_IGN);         /* SIG_IGN = ignora semnalul */
    
    /* Resetam starea */
    g_server_ruleaza = 1;
    
    /* Afisarea initiala */
    actualizeaza_afisare();
    
    /* Pornim thread-urile */
    pthread_t id_thread_server;
    pthread_t id_thread_refresh;
    
    pthread_create(&id_thread_server, NULL, thread_server, NULL);
    pthread_create(&id_thread_refresh, NULL, thread_refresh_automat, NULL);
    
    /* Terminal in raw mode */
    seteaza_terminal_raw();
    
    /* Bucla principala - procesarea comenzilor de la tastatura */
    while (g_server_ruleaza) {
        char tasta = citeste_caracter();
        
        if (tasta == 0) {
            continue;
        }
        
        tasta = toupper(tasta);
        
        switch (tasta) {
            
            case 'L': {
                if (strcmp(g_filtru_nivel, "ALL") == 0) {
                    strcpy(g_filtru_nivel, "INFO");
                } 
                else if (strcmp(g_filtru_nivel, "INFO") == 0) {
                    strcpy(g_filtru_nivel, "WARN");
                } 
                else if (strcmp(g_filtru_nivel, "WARN") == 0) {
                    strcpy(g_filtru_nivel, "ERROR");
                } 
                else {
                    strcpy(g_filtru_nivel, "ALL");
                }
                actualizeaza_afisare();
                break;
            }
            
            case 'S': {
                const char* statusuri[] = {
                    "ALL", "RUNNING", "SLEEPING", "STOPPED", 
                    "ZOMBIE", "CRASHED", "STATIC"
                };
                int numar_statusuri = sizeof(statusuri) / sizeof(statusuri[0]);
                
                int index_curent = 0;
                for (int i = 0; i < numar_statusuri; i++) {
                    if (strcmp(g_filtru_status, statusuri[i]) == 0) {
                        index_curent = i;
                        break;
                    }
                }
                
                index_curent = (index_curent + 1) % numar_statusuri;
                strcpy(g_filtru_status, statusuri[index_curent]);
                
                actualizeaza_afisare();
                break;
            }
            
            case 'F': {
                printf("\n  Cauta: ");
                fflush(stdout);
                
                citeste_linie(g_text_cautat, sizeof(g_text_cautat));
                
                actualizeaza_afisare();
                break;
            }
            
            case 'C': {
                printf("\n  Stergi toate logurile? (D/N): ");
                fflush(stdout);
                
                char confirmare = 0;
                while (confirmare == 0) {
                    confirmare = citeste_caracter();
                }
                
                if (toupper(confirmare) == 'D' || toupper(confirmare) == 'Y') {
                    pthread_mutex_lock(&g_mutex_loguri);
                    g_numar_loguri = 0;
                    pthread_mutex_unlock(&g_mutex_loguri);
                }
                
                actualizeaza_afisare();
                break;
            }
            
            case 'E': {
                exporta_loguri_csv();
                sleep(2);
                actualizeaza_afisare();
                break;
            }
            
            case 'R': {
                actualizeaza_afisare();
                break;
            }
            
            case 'Q': {
                printf("\n  Inchizi serverul? (D/N): ");
                fflush(stdout);
                
                char confirmare = 0;
                while (confirmare == 0) {
                    confirmare = citeste_caracter();
                }
                
                if (toupper(confirmare) == 'D' || toupper(confirmare) == 'Y') {
                    g_server_ruleaza = 0;
                } else {
                    actualizeaza_afisare();
                }
                break;
            }
            
            default:
                break;
        }
    }
    
    /* Curatenie la iesire */
    seteaza_terminal_normal();
    
    if (g_socket_server >= 0) {
        close(g_socket_server);
    }
    
    pthread_join(id_thread_server, NULL);
    pthread_join(id_thread_refresh, NULL);
    
    pthread_mutex_lock(&g_mutex_clienti);
    for (int i = 0; i < g_numar_clienti; i++) {
        free(g_clienti_conectati[i]);
    }
    g_numar_clienti = 0;
    pthread_mutex_unlock(&g_mutex_clienti);
    
    printf(GALBEN "\n\n  Server oprit.\n\n" RESET);
}


/*
 * =============================================================================
 * FUNCTIA PRINCIPALA - main()
 * =============================================================================
 */
int main(void) {
    char input[16];
    int ruleaza = 1;
    
    while (ruleaza) {
        /* Afisam meniul principal */
        afiseaza_meniu_principal();
        
        /* Citim optiunea */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        /* Procesam optiunea */
        char optiune = input[0];
        
        switch (optiune) {
            case '1':
                /* Mod Server - porneste serverul TCP */
                printf(VERDE "\n     Pornire server...\n" RESET);
                sleep(1);
                ruleaza_mod_server();
                
                /* Dupa ce serverul se opreste, revenim la meniu */
                printf("\n     Apasa ENTER pentru a reveni la meniu...");
                getchar();
                break;
                
            case '2':
                /* Mod Vizualizare - loguri vechi */
                printf(MAGENTA "\n     Pornire mod vizualizare...\n" RESET);
                sleep(1);
                {
                    int rezultat = meniu_vizualizare_loguri();
                    if (rezultat == 1) {
                        /* Utilizatorul vrea sa porneasca serverul din meniul de vizualizare */
                        printf(VERDE "\n     Pornire server...\n" RESET);
                        sleep(1);
                        ruleaza_mod_server();
                        printf("\n     Apasa ENTER pentru a reveni la meniu...");
                        getchar();
                    }
                }
                break;
                
            case '0':
            case 'q':
            case 'Q':
                /* Iesire */
                ruleaza = 0;
                printf("\n");
                printf(CYAN "     ╔═══════════════════════════════════════════════════════════╗\n");
                printf("     ║                                                             ║\n");
                printf("     ║           Multumim ca ai folosit Process Log System!        ║\n");
                printf("     ║                                                             ║\n");
                printf("     ╚═══════════════════════════════════════════════════════════╝\n" RESET);
                printf("\n");
                break;
                
            default:
                printf(ROSU "\n     Optiune invalida! Introdu 1, 2 sau 0.\n" RESET);
                sleep(1);
                break;
        }
    }
    
    return 0;
}