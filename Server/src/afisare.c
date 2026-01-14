#include "afisare.h"
#include "utilitare.h"
#include "culori_si_configurari.h"

#include <stdio.h>
#include <string.h>


void curata_ecranul(void) {
    /*
     * Folosim coduri ANSI pentru a curata ecranul:
     * \033[2J = sterge tot ecranul
     * \033[H = muta cursorul la pozitia (1,1) - coltul stanga-sus
     * 
     * Alternativ puteam folosi system("clear") dar e mai lent si mai putin portabil
     */
    printf("\033[2J\033[H");
}


int trece_filtrul(const LogEntry* intrare) {
    /*
     * Verificam filtrul de NIVEL (INFO/WARN/ERROR)
     */
    if (strcmp(g_filtru_nivel, "ALL") != 0) {
        /* Facem copii pentru a compara case-insensitive */
        char nivel_log[32];
        char nivel_filtru[32];
        
        strncpy(nivel_log, intrare->nivel, sizeof(nivel_log));
        strncpy(nivel_filtru, g_filtru_nivel, sizeof(nivel_filtru));
        
        transforma_in_majuscule(nivel_log);
        transforma_in_majuscule(nivel_filtru);
        
        if (strcmp(nivel_log, nivel_filtru) != 0) {
            return 0;  /* Nu trece filtrul */
        }
    }
    
    /*
     * Verificam filtrul de STATUS (running/crashed/etc)
     */
    if (strcmp(g_filtru_status, "ALL") != 0) {
        char status_log[32];
        char status_filtru[32];
        
        strncpy(status_log, intrare->status, sizeof(status_log));
        strncpy(status_filtru, g_filtru_status, sizeof(status_filtru));
        
        transforma_in_majuscule(status_log);
        transforma_in_majuscule(status_filtru);
        
        if (strcmp(status_log, status_filtru) != 0) {
            return 0;  /* Nu trece filtrul */
        }
    }
    
    /*
     * Verificam textul cautat (search)
     */
    if (strlen(g_text_cautat) > 0) {
        /* Cautam in mai multe campuri */
        if (!contine_text_insensitiv(intrare->nume, g_text_cautat) &&
            !contine_text_insensitiv(intrare->utilizator, g_text_cautat) &&
            !contine_text_insensitiv(intrare->mesaj, g_text_cautat) &&
            !contine_text_insensitiv(intrare->status, g_text_cautat) &&
            !contine_text_insensitiv(intrare->hostname, g_text_cautat)) {
            return 0;  /* Nu am gasit textul nicaieri */
        }
    }
    
    return 1;  /* Trece toate filtrele! */
}

void afiseaza_linie_log(const LogEntry* intrare, int index) {
    /*
     * Afisam fiecare camp pe rand, cu formatare si culori
     */
    
    /* INDEX - numar de ordine, dim (estompat) */
    printf(DIM "[%4d] " RESET, index);
    
    /* TIMESTAMP - cand s-a intamplat, cyan */
    printf(CYAN "%-19.19s " RESET, intrare->timestamp);
    /* %-19.19s inseamna: 
     * - aliniat la stanga (-)
     * - minim 19 caractere latime
     * - maxim 19 caractere (.19) - trunchiaza daca e mai lung */
    
    /* PID - numarul procesului, dim */
    printf(DIM "%-6d " RESET, intrare->pid);
    
    /* NUME PROCES - bold alb pentru vizibilitate */
    printf(BOLD ALB "%-14.14s " RESET, intrare->nume);
    
    /* UTILIZATOR - cine a pornit procesul */
    printf(DIM CYAN "%-10.10s " RESET, intrare->utilizator);
    
    /*
     * STATUS - colorat diferit in functie de stare
     */
    char status_mare[32];
    strncpy(status_mare, intrare->status, sizeof(status_mare));
    transforma_in_majuscule(status_mare);
    
    /* Alegem culorile in functie de status */
    if (strcmp(status_mare, "RUNNING") == 0) {
        /* Running = verde (totul e ok) */
        printf(FUNDAL_VERDE NEGRU " %-8.8s " RESET, status_mare);
    } 
    else if (strcmp(status_mare, "SLEEPING") == 0) {
        /* Sleeping = cyan (asteapta) */
        printf(FUNDAL_CYAN NEGRU " %-8.8s " RESET, status_mare);
    } 
    else if (strcmp(status_mare, "STATIC") == 0) {
        /* Static = albastru */
        printf(FUNDAL_ALBASTRU ALB " %-8.8s " RESET, status_mare);
    } 
    else if (strcmp(status_mare, "CRASHED") == 0 || strcmp(status_mare, "ZOMBIE") == 0) {
        /* Crashed/Zombie = ROSU (problema!) */
        printf(FUNDAL_ROSU ALB " %-8.8s " RESET, status_mare);
    } 
    else if (strcmp(status_mare, "STOPPED") == 0) {
        /* Stopped = gri */
        printf(FUNDAL_GRI NEGRU " %-8.8s " RESET, status_mare);
    } 
    else {
        /* Altceva = magenta */
        printf(FUNDAL_MAGENTA ALB " %-8.8s " RESET, status_mare);
    }
    
    /*
     * LEVEL - nivelul de importanta, colorat
     */
    char nivel_mare[32];
    strncpy(nivel_mare, intrare->nivel, sizeof(nivel_mare));
    transforma_in_majuscule(nivel_mare);
    
    if (strcmp(nivel_mare, "INFO") == 0) {
        printf(FUNDAL_VERDE NEGRU " %-5s " RESET, nivel_mare);
    } 
    else if (strcmp(nivel_mare, "WARN") == 0) {
        printf(FUNDAL_GALBEN NEGRU " %-5s " RESET, nivel_mare);
    } 
    else if (strcmp(nivel_mare, "ERROR") == 0) {
        printf(FUNDAL_ROSU ALB " %-5s " RESET, nivel_mare);
    } 
    else {
        printf(" %-5s ", nivel_mare);
    }
    
    /*
     * CPU si MEMORIE - afisate doar daca au valori
     */
    if (intrare->procent_cpu > 0.0 || intrare->memorie_kb > 0) {
        /* CPU - rosu daca e mare */
        if (intrare->procent_cpu > 80.0) {
            printf(ROSU);
        } else if (intrare->procent_cpu > 50.0) {
            printf(GALBEN);
        } else {
            printf(DIM);
        }
        printf(" %5.1f%% ", intrare->procent_cpu);
        /* %5.1f = 5 caractere total, 1 dupa virgula */
        
        /* Memorie - rosu daca e peste 1GB */
        if (intrare->memorie_kb > 1024 * 1024) {
            printf(ROSU);
        } else {
            printf(DIM);
        }
        
        /* Afisam in MB daca e peste 1024 KB */
        if (intrare->memorie_kb > 1024) {
            printf("%6luMB ", intrare->memorie_kb / 1024);
        } else {
            printf("%6luKB ", intrare->memorie_kb);
        }
        printf(RESET);
    } else {
        /* Spatii goale daca nu avem date */
        printf("                ");
    }
    
    /*
     * MESAJ - trunchiat daca e prea lung
     */
    char mesaj_trunchiat[40];
    strncpy(mesaj_trunchiat, intrare->mesaj, sizeof(mesaj_trunchiat) - 4);
    mesaj_trunchiat[sizeof(mesaj_trunchiat) - 4] = '\0';
    
    if (strlen(intrare->mesaj) > sizeof(mesaj_trunchiat) - 4) {
        strcat(mesaj_trunchiat, "...");  /* Adaugam "..." la sfarsit */
    }
    
    printf(" %s\n", mesaj_trunchiat);
}


void afiseaza_antet(void) {
    curata_ecranul();
    
    /*
     * TITLU - banner mare cu fundal albastru
     */
    printf(FUNDAL_ALBASTRU ALB BOLD);
    printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("                              PROCESS LOG SERVER - Port %d (POSIX)                                     \n", SERVER_PORT);
    printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf(RESET);
    
    /*
     * STATUS SERVER - port, clienti, loguri
     */
    printf(VERDE BOLD " [SERVER] " RESET);
    printf("Status: " VERDE "ACTIV" RESET " | Port: %d | ", SERVER_PORT);
    printf(CYAN "Clienti: %d" RESET " | ", g_numar_clienti);
    printf(GALBEN "Loguri: %d\n" RESET, g_numar_loguri);
    
    /*
     * LISTA CLIENTI CONECTATI
     */
    pthread_mutex_lock(&g_mutex_clienti);
    if (g_numar_clienti > 0) {
        printf(DIM CYAN " [CLIENTI] " RESET);
        
        /* Afisam primii 5 clienti */
        for (int i = 0; i < g_numar_clienti && i < 5; i++) {
            printf("%s ", g_clienti_conectati[i]);
        }
        
        /* Daca sunt mai multi, aratam cati */
        if (g_numar_clienti > 5) {
            printf("(+%d more)", g_numar_clienti - 5);
        }
        printf("\n");
    }
    pthread_mutex_unlock(&g_mutex_clienti);
    
    /*
     * FILTRE ACTIVE
     */
    printf(MAGENTA BOLD " [FILTRE] " RESET);
    printf("Nivel: " GALBEN "%s" RESET " | ", g_filtru_nivel);
    printf("Status: " GALBEN "%s" RESET " | ", g_filtru_status);
    printf("Cautare: " GALBEN "%s\n" RESET, 
           strlen(g_text_cautat) ? g_text_cautat : "(nimic)");
    
    /*
     * LINIE SEPARATOR
     */
    printf(DIM "───────────────────────────────────────────────────────────────────────────────────────────────────────\n" RESET);
    
    /*
     * HEADER TABEL - cu fundal alb
     */
    printf(FUNDAL_ALB NEGRU BOLD);
    printf(" %-6s %-19s %-6s %-14s %-10s %-10s %-7s %-7s %-8s %s\n", 
           "INDEX", "TIMESTAMP", "PID", "PROCES", "USER", "STATUS", "NIVEL", "CPU", "MEMORIE", "MESAJ");
    printf(RESET);
    
    printf(DIM "───────────────────────────────────────────────────────────────────────────────────────────────────────\n" RESET);
}


void afiseaza_meniu(void) {
    printf(DIM "───────────────────────────────────────────────────────────────────────────────────────────────────────\n" RESET);
    
    printf(BOLD ALB " [COMENZI] " RESET);
    printf("L=Level | S=Status | F=Cautare | C=Sterge | E=Export | R=Refresh | Q=Iesire\n");
    printf(" [STATUS] running | sleeping | stopped | zombie | crashed | static\n");
}

void actualizeaza_afisare(void) {
    /* Afisam header-ul */
    afiseaza_antet();
    
    /* Blocam mutex-ul pentru a accesa lista de loguri in siguranta */
    pthread_mutex_lock(&g_mutex_loguri);
    
    /*
     * Pas 1: Gasim toate logurile care trec filtrul
     */
    int indici_filtrate[MAX_LOGURI];
    int numar_filtrate = 0;
    
    for (int i = 0; i < g_numar_loguri; i++) {
        if (trece_filtrul(&g_lista_loguri[i])) {
            indici_filtrate[numar_filtrate] = i;
            numar_filtrate++;
        }
    }
    
    /*
     * Pas 2: Afisam ultimele N loguri (sa incapa pe ecran)
     */
    int max_afisare = 18;  /* Cam atatea incap pe un ecran normal */
    int start = (numar_filtrate > max_afisare) ? (numar_filtrate - max_afisare) : 0;
    
    for (int i = start; i < numar_filtrate; i++) {
        afiseaza_linie_log(&g_lista_loguri[indici_filtrate[i]], indici_filtrate[i] + 1);
    }
    
    /*
     * Pas 3: Mesaj daca nu sunt loguri
     */
    if (numar_filtrate == 0) {
        printf(DIM "\n  (Nu exista loguri care sa corespunda filtrelor)\n" RESET);
    }
    
    /*
     * Pas 4: Statistici
     */
    printf(DIM "\n  Afisate: %d / %d (total: %d)\n" RESET, 
           numar_filtrate > max_afisare ? max_afisare : numar_filtrate, 
           numar_filtrate, 
           g_numar_loguri);
    
    pthread_mutex_unlock(&g_mutex_loguri);
    
    /* Afisam meniul */
    afiseaza_meniu();
    
    /* Ne asiguram ca totul e afisat imediat */
    fflush(stdout);
}