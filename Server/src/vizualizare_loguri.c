/*
 * =============================================================================
 * FISIER: vizualizare_loguri.c
 * =============================================================================
 * 
 * DESCRIERE:
 *     Implementarea functiilor pentru vizualizarea logurilor vechi din CSV.
 * 
 * =============================================================================
 */

#include "vizualizare_loguri.h"
#include "structuri_date.h"
#include "afisare.h"
#include "utilitare.h"
#include "culori_si_configurari.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>      /* Pentru scanarea directoarelor (opendir, readdir) */
#include <sys/stat.h>    /* Pentru stat() - informatii despre fisiere */
#include <ctype.h>


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE HELPER: extrage_camp_csv
 * -----------------------------------------------------------------------------
 * Extrage un camp dintr-o linie CSV, tinand cont de ghilimele.
 * 
 * CSV-ul nostru foloseste ghilimele pentru valori: "valoare1","valoare2",...
 * Aceasta functie extrage corect valorile chiar daca contin virgule.
 */
static char* extrage_camp_csv(char** pozitie_curenta, char* buffer, size_t dim_buffer) {
    char* src = *pozitie_curenta;
    char* dest = buffer;
    size_t count = 0;
    
    /* Initializam buffer-ul gol */
    buffer[0] = '\0';
    
    if (src == NULL || *src == '\0') {
        return buffer;
    }
    
    /* Sarim peste spatii de la inceput */
    while (*src == ' ' || *src == '\t') src++;
    
    /* Verificam daca campul e intre ghilimele */
    if (*src == '"') {
        src++;  /* Sarim ghilimelele de deschidere */
        
        /* Copiem pana la ghilimelele de inchidere */
        while (*src && count < dim_buffer - 1) {
            if (*src == '"') {
                if (*(src + 1) == '"') {
                    /* Ghilimele duble = o ghilimea in text */
                    *dest++ = '"';
                    count++;
                    src += 2;
                } else {
                    /* Sfarsitul campului */
                    src++;
                    break;
                }
            } else {
                *dest++ = *src++;
                count++;
            }
        }
        
        /* Sarim virgula de dupa */
        if (*src == ',') src++;
    } else {
        /* Camp fara ghilimele - copiem pana la virgula */
        while (*src && *src != ',' && *src != '\n' && *src != '\r' && count < dim_buffer - 1) {
            *dest++ = *src++;
            count++;
        }
        
        /* Sarim virgula */
        if (*src == ',') src++;
    }
    
    *dest = '\0';
    *pozitie_curenta = src;
    
    return buffer;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: parseaza_linie_csv
 * -----------------------------------------------------------------------------
 * Formatul CSV generat de export:
 * Timestamp,PID,Process,User,Status,Level,CPU%,MemoryKB,Message,Hostname,ClientIP
 */
int parseaza_linie_csv(const char* linie, LogEntry* intrare) {
    /* Initializam structura */
    memset(intrare, 0, sizeof(LogEntry));
    
    /* Facem o copie a liniei pentru ca extrage_camp_csv modifica pointer-ul */
    char* linie_copie = strdup(linie);
    if (linie_copie == NULL) return 0;
    
    char* pozitie = linie_copie;
    char buffer[512];
    
    /* Campul 1: Timestamp */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->timestamp, buffer, sizeof(intrare->timestamp) - 1);
    
    /* Campul 2: PID */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    intrare->pid = atoi(buffer);
    
    /* Campul 3: Process (nume) */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->nume, buffer, sizeof(intrare->nume) - 1);
    
    /* Campul 4: User */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->utilizator, buffer, sizeof(intrare->utilizator) - 1);
    
    /* Campul 5: Status */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->status, buffer, sizeof(intrare->status) - 1);
    
    /* Campul 6: Level */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->nivel, buffer, sizeof(intrare->nivel) - 1);
    
    /* Campul 7: CPU% */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    intrare->procent_cpu = atof(buffer);
    
    /* Campul 8: MemoryKB */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    intrare->memorie_kb = atol(buffer);
    
    /* Campul 9: Message */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->mesaj, buffer, sizeof(intrare->mesaj) - 1);
    
    /* Campul 10: Hostname */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->hostname, buffer, sizeof(intrare->hostname) - 1);
    
    /* Campul 11: ClientIP */
    extrage_camp_csv(&pozitie, buffer, sizeof(buffer));
    strncpy(intrare->ip_client, buffer, sizeof(intrare->ip_client) - 1);
    
    free(linie_copie);
    
    /* Verificam ca am citit cel putin numele procesului */
    return (strlen(intrare->nume) > 0) ? 1 : 0;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: listeaza_fisiere_csv
 * -----------------------------------------------------------------------------
 */
int listeaza_fisiere_csv(char fisiere[][256], int max_fisiere) {
    DIR* director;
    struct dirent* intrare_dir;
    int numar_fisiere = 0;
    
    /* Deschidem directorul curent */
    director = opendir(".");
    if (director == NULL) {
        printf(ROSU "  Eroare: Nu se poate deschide directorul curent!\n" RESET);
        return 0;
    }
    
    /* Parcurgem fisierele */
    while ((intrare_dir = readdir(director)) != NULL && numar_fisiere < max_fisiere) {
        /* Verificam daca e fisier CSV care incepe cu "logs_export" */
        if (strstr(intrare_dir->d_name, "logs_export") != NULL &&
            strstr(intrare_dir->d_name, ".csv") != NULL) {
            
            strncpy(fisiere[numar_fisiere], intrare_dir->d_name, 255);
            fisiere[numar_fisiere][255] = '\0';
            numar_fisiere++;
        }
    }
    
    closedir(director);
    return numar_fisiere;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: incarca_fisier_csv
 * -----------------------------------------------------------------------------
 */
int incarca_fisier_csv(const char* nume_fisier) {
    FILE* fisier = fopen(nume_fisier, "r");
    if (fisier == NULL) {
        printf(ROSU "  Eroare: Nu se poate deschide fisierul: %s\n" RESET, nume_fisier);
        return -1;
    }
    
    char linie[2048];
    int numar_incarcate = 0;
    int linie_curenta = 0;
    
    /* Blocam mutex-ul pentru a modifica lista de loguri */
    pthread_mutex_lock(&g_mutex_loguri);
    
    /* Golim lista existenta */
    g_numar_loguri = 0;
    
    while (fgets(linie, sizeof(linie), fisier) != NULL) {
        linie_curenta++;
        
        /* Sarim header-ul (prima linie) */
        if (linie_curenta == 1) continue;
        
        /* Sarim liniile goale */
        if (strlen(linie) < 5) continue;
        
        /* Parsam linia */
        LogEntry intrare;
        if (parseaza_linie_csv(linie, &intrare)) {
            if (g_numar_loguri < MAX_LOGURI) {
                g_lista_loguri[g_numar_loguri] = intrare;
                g_numar_loguri++;
                numar_incarcate++;
            } else {
                printf(GALBEN "  Avertisment: Lista plina, unele loguri nu au fost incarcate\n" RESET);
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&g_mutex_loguri);
    
    fclose(fisier);
    return numar_incarcate;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: meniu_vizualizare_loguri
 * -----------------------------------------------------------------------------
 * Meniu interactiv complet pentru vizualizarea logurilor vechi
 */
int meniu_vizualizare_loguri(void) {
    char fisiere[100][256];  /* Maxim 100 fisiere */
    int numar_fisiere = 0;
    char input[256];
    int fisier_incarcat = 0;
    char fisier_curent[256] = "";
    int mod_afisare = 0;  /* 0 = meniu, 1 = afisare loguri live */
    
    while (1) {
        /* Curatam ecranul - REFRESH CURAT */
        printf("\033[2J\033[H");
        fflush(stdout);
        
        if (mod_afisare == 1 && fisier_incarcat) {
            /*
             * MOD AFISARE LOGURI - similar cu serverul
             */
            
            /* Header */
            printf(FUNDAL_MAGENTA ALB BOLD);
            printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
            printf("                         VIZUALIZARE LOGURI - %s                                    \n", fisier_curent);
            printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
            printf(RESET);
            
            /* Status */
            printf(VERDE " [FISIER] " RESET "%s", fisier_curent);
            printf(" | Loguri: " GALBEN "%d" RESET "\n", g_numar_loguri);
            
            /* Filtre active */
            printf(MAGENTA " [FILTRE] " RESET);
            printf("Nivel: " GALBEN "%s" RESET " | ", g_filtru_nivel);
            printf("Status: " GALBEN "%s" RESET " | ", g_filtru_status);
            printf("Cautare: " GALBEN "%s\n" RESET, 
                   strlen(g_text_cautat) ? g_text_cautat : "(nimic)");
            
            /* Separator */
            printf(DIM "───────────────────────────────────────────────────────────────────────────────────────────────────────\n" RESET);
            
            /* Header tabel */
            printf(FUNDAL_ALB NEGRU BOLD);
            printf(" %-6s %-19s %-6s %-14s %-10s %-10s %-7s %-7s %-8s %s\n", 
                   "INDEX", "TIMESTAMP", "PID", "PROCES", "USER", "STATUS", "NIVEL", "CPU", "MEMORIE", "MESAJ");
            printf(RESET);
            printf(DIM "───────────────────────────────────────────────────────────────────────────────────────────────────────\n" RESET);
            
            /* Afisam logurile */
            pthread_mutex_lock(&g_mutex_loguri);
            
            int afisate = 0;
            int total_filtrate = 0;
            
            /* Mai intai numaram cate trec filtrul */
            for (int i = 0; i < g_numar_loguri; i++) {
                if (trece_filtrul(&g_lista_loguri[i])) {
                    total_filtrate++;
                }
            }
            
            /* Afisam ultimele 20 care trec filtrul */
            int de_sarit = (total_filtrate > 20) ? (total_filtrate - 20) : 0;
            int sarite = 0;
            
            for (int i = 0; i < g_numar_loguri; i++) {
                if (trece_filtrul(&g_lista_loguri[i])) {
                    if (sarite < de_sarit) {
                        sarite++;
                        continue;
                    }
                    afiseaza_linie_log(&g_lista_loguri[i], i + 1);
                    afisate++;
                }
            }
            
            pthread_mutex_unlock(&g_mutex_loguri);
            
            if (afisate == 0) {
                printf(GALBEN "\n  (Niciun log nu corespunde filtrelor active)\n" RESET);
            }
            
            /* Statistici */
            printf(DIM "\n  Afisate: %d din %d filtrate (total: %d)\n" RESET, 
                   afisate, total_filtrate, g_numar_loguri);
            
            /* Meniu comenzi */
            printf(DIM "───────────────────────────────────────────────────────────────────────────────────────────────────────\n" RESET);
            printf(BOLD " [COMENZI] " RESET);
            printf("L=Nivel | S=Status | F=Cautare | C=Reseteaza | M=Meniu | Q=Iesire\n");
            printf(" > ");
            fflush(stdout);
            
            /* Citim comanda */
            if (fgets(input, sizeof(input), stdin) == NULL) {
                mod_afisare = 0;
                continue;
            }
            input[strcspn(input, "\n")] = '\0';
            
            char cmd = toupper(input[0]);
            
            switch (cmd) {
                case 'L': {
                    /* Cicleaza nivel */
                    if (strcmp(g_filtru_nivel, "ALL") == 0) {
                        strcpy(g_filtru_nivel, "INFO");
                    } else if (strcmp(g_filtru_nivel, "INFO") == 0) {
                        strcpy(g_filtru_nivel, "WARN");
                    } else if (strcmp(g_filtru_nivel, "WARN") == 0) {
                        strcpy(g_filtru_nivel, "ERROR");
                    } else {
                        strcpy(g_filtru_nivel, "ALL");
                    }
                    break;
                }
                case 'S': {
                    /* Cicleaza status */
                    const char* statusuri[] = {"ALL", "RUNNING", "SLEEPING", "STOPPED", "ZOMBIE", "CRASHED"};
                    int n = sizeof(statusuri) / sizeof(statusuri[0]);
                    int idx = 0;
                    for (int i = 0; i < n; i++) {
                        if (strcmp(g_filtru_status, statusuri[i]) == 0) {
                            idx = i;
                            break;
                        }
                    }
                    idx = (idx + 1) % n;
                    strcpy(g_filtru_status, statusuri[idx]);
                    break;
                }
                case 'F': {
                    printf("\n  Introdu textul de cautat: ");
                    fflush(stdout);
                    if (fgets(input, sizeof(input), stdin) != NULL) {
                        input[strcspn(input, "\n")] = '\0';
                        strncpy(g_text_cautat, input, sizeof(g_text_cautat) - 1);
                    }
                    break;
                }
                case 'C': {
                    /* Reseteaza filtrele */
                    strcpy(g_filtru_nivel, "ALL");
                    strcpy(g_filtru_status, "ALL");
                    g_text_cautat[0] = '\0';
                    break;
                }
                case 'M': {
                    /* Inapoi la meniu */
                    mod_afisare = 0;
                    break;
                }
                case 'Q':
                case '0': {
                    return 0;
                }
                default:
                    /* Orice alta tasta = refresh */
                    break;
            }
            
            continue;  /* Refresh automat */
        }
        
        /*
         * MOD MENIU PRINCIPAL VIZUALIZARE
         */
        
        /* Header */
        printf(FUNDAL_MAGENTA ALB BOLD);
        printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
        printf("                              VIZUALIZARE LOGURI VECHI - Din fisiere CSV                               \n");
        printf("═══════════════════════════════════════════════════════════════════════════════════════════════════════\n");
        printf(RESET);
        
        /* Status fisier */
        if (fisier_incarcat) {
            printf(VERDE "\n  [INCARCAT] " RESET "Fisier: " CYAN "%s" RESET, fisier_curent);
            printf(" | Loguri: " GALBEN "%d" RESET "\n", g_numar_loguri);
        } else {
            printf(GALBEN "\n  [!] Niciun fisier incarcat\n" RESET);
        }
        
        /* Meniu */
        printf("\n");
        printf("     ╔═══════════════════════════════════════════════════════════════════╗\n");
        printf("     ║                                                                   ║\n");
        printf("     ║  " CYAN "[1]" RESET " Listeaza fisierele CSV disponibile                          ║\n");
        printf("     ║  " CYAN "[2]" RESET " Incarca un fisier CSV                                       ║\n");
        if (fisier_incarcat) {
        printf("     ║  " VERDE "[3]" RESET " " BOLD "Afiseaza logurile" RESET " (mod interactiv)                         ║\n");
        } else {
        printf("     ║  " DIM "[3] Afiseaza logurile (incarca intai un fisier)" RESET "              ║\n");
        }
        printf("     ║                                                                   ║\n");
        printf("     ║  " VERDE "[S]" RESET " Porneste SERVERUL (asculta conexiuni noi)                  ║\n");
        printf("     ║  " ROSU  "[Q]" RESET " Inapoi la meniul principal                                 ║\n");
        printf("     ║                                                                   ║\n");
        printf("     ╚═══════════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("     Alege optiunea: ");
        fflush(stdout);
        
        /* Citim inputul */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;
        }
        input[strcspn(input, "\n")] = '\0';
        
        char cmd = toupper(input[0]);
        
        switch (cmd) {
            case '1': {
                /* Listeaza fisierele CSV */
                printf("\033[2J\033[H");
                printf(BOLD "\n  ═══ FISIERE CSV DISPONIBILE ═══\n\n" RESET);
                
                numar_fisiere = listeaza_fisiere_csv(fisiere, 100);
                
                if (numar_fisiere == 0) {
                    printf(GALBEN "  Nu s-au gasit fisiere logs_export_*.csv\n" RESET);
                    printf("  Exporta intai niste loguri cu optiunea 'E' din server.\n");
                } else {
                    for (int i = 0; i < numar_fisiere; i++) {
                        struct stat st;
                        stat(fisiere[i], &st);
                        printf("  " CYAN "[%d]" RESET " %-40s " DIM "(%ld KB)\n" RESET, 
                               i + 1, fisiere[i], st.st_size / 1024);
                    }
                }
                printf("\n  Apasa ENTER pentru a continua...");
                getchar();
                break;
            }
            
            case '2': {
                /* Incarca fisier */
                printf("\033[2J\033[H");
                printf(BOLD "\n  ═══ SELECTEAZA FISIER DE INCARCAT ═══\n\n" RESET);
                
                numar_fisiere = listeaza_fisiere_csv(fisiere, 100);
                
                if (numar_fisiere == 0) {
                    printf(GALBEN "  Nu exista fisiere CSV disponibile!\n" RESET);
                    printf("\n  Apasa ENTER pentru a continua...");
                    getchar();
                    break;
                }
                
                for (int i = 0; i < numar_fisiere; i++) {
                    printf("  " CYAN "[%d]" RESET " %s\n", i + 1, fisiere[i]);
                }
                printf("\n  Introdu numarul fisierului (0 = anulare): ");
                fflush(stdout);
                
                if (fgets(input, sizeof(input), stdin) == NULL) break;
                int selectie = atoi(input);
                
                if (selectie > 0 && selectie <= numar_fisiere) {
                    printf("\n  Se incarca " CYAN "%s" RESET "...\n", fisiere[selectie - 1]);
                    
                    /* Resetam filtrele */
                    strcpy(g_filtru_nivel, "ALL");
                    strcpy(g_filtru_status, "ALL");
                    g_text_cautat[0] = '\0';
                    
                    int incarcate = incarca_fisier_csv(fisiere[selectie - 1]);
                    
                    if (incarcate >= 0) {
                        printf(VERDE "\n  ✓ S-au incarcat %d loguri!\n" RESET, incarcate);
                        fisier_incarcat = 1;
                        strncpy(fisier_curent, fisiere[selectie - 1], sizeof(fisier_curent) - 1);
                    } else {
                        printf(ROSU "\n  ✗ Eroare la incarcarea fisierului!\n" RESET);
                    }
                    printf("\n  Apasa ENTER pentru a continua...");
                    getchar();
                }
                break;
            }
            
            case '3': {
                /* Afiseaza logurile */
                if (!fisier_incarcat || g_numar_loguri == 0) {
                    printf(GALBEN "\n  Nu sunt loguri incarcate! Incarca intai un fisier (optiunea 2).\n" RESET);
                    printf("\n  Apasa ENTER pentru a continua...");
                    getchar();
                } else {
                    mod_afisare = 1;  /* Trecem in modul afisare */
                }
                break;
            }
            
            case 'S': {
                return 1;  /* Porneste serverul */
            }
            
            case 'Q':
            case '0': {
                return 0;  /* Iesire */
            }
            
            default: {
                /* Ignora - refresh automat */
                break;
            }
        }
    }
    
    return 0;
}