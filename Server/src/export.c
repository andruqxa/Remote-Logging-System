#include "export.h"
#include "structuri_date.h"
#include "afisare.h"
#include "utilitare.h"
#include "culori_si_configurari.h"

#include <stdio.h>
#include <string.h>

void exporta_loguri_csv(void) {
    /*
     * Pas 1: Generam numele fisierului cu timestamp
     * 
     * Formatul: logs_export_2024-01-15_14_30_00.csv
     * Folosim underscore in loc de spatii si doua puncte pentru compatibilitate
     * cu sistemele de fisiere.
     */
    char nume_fisier[128];
    char timestamp[32];
    obtine_timpul_curent(timestamp, sizeof(timestamp));
    
    /* Inlocuim caracterele problematice */
    for (int i = 0; timestamp[i]; i++) {
        if (timestamp[i] == ':' || timestamp[i] == ' ') {
            timestamp[i] = '_';
        }
    }
    
    snprintf(nume_fisier, sizeof(nume_fisier), "logs_export_%s.csv", timestamp);
    
    /*
     * Pas 2: Deschidem fisierul pentru scriere
     * 
     * fopen() cu "w" = write mode
     * - Daca fisierul exista, il sterge si creeaza unul nou
     * - Daca nu exista, il creeaza
     */
    FILE* fisier = fopen(nume_fisier, "w");
    
    if (fisier == NULL) {
        /* Nu am putut deschide/crea fisierul */
        printf(ROSU "\n  [EROARE] Nu s-a putut crea fisierul: %s\n" RESET, nume_fisier);
        return;
    }
    
    /*
     * Pas 3: Scriem header-ul CSV (numele coloanelor)
     */
    fprintf(fisier, "Timestamp,PID,Process,User,Status,Level,CPU%%,MemoryKB,Message,Hostname,ClientIP\n");
    
    /*
     * Pas 4: Scriem fiecare log care trece filtrul
     * 
     * Folosim mutex pentru ca lista de loguri poate fi modificata
     * de alte thread-uri in timp ce scriem.
     */
    pthread_mutex_lock(&g_mutex_loguri);
    
    int numar_exportate = 0;
    
    for (int i = 0; i < g_numar_loguri; i++) {
        /* Verificam daca acest log trece prin filtrele curente */
        if (trece_filtrul(&g_lista_loguri[i])) {
            /*
             * Scriem linia in format CSV
             * 
             * Punem valorile intre ghilimele pentru a gestiona corect
             * cazurile cand valoarea contine virgule sau ghilimele.
             * 
             * fprintf() e ca printf() dar scrie in fisier in loc de ecran
             */
            fprintf(fisier, "\"%s\",%d,\"%s\",\"%s\",\"%s\",\"%s\",%.2f,%lu,\"%s\",\"%s\",\"%s\"\n",
                    g_lista_loguri[i].timestamp,
                    g_lista_loguri[i].pid,
                    g_lista_loguri[i].nume,
                    g_lista_loguri[i].utilizator,
                    g_lista_loguri[i].status,
                    g_lista_loguri[i].nivel,
                    g_lista_loguri[i].procent_cpu,
                    g_lista_loguri[i].memorie_kb,
                    g_lista_loguri[i].mesaj,
                    g_lista_loguri[i].hostname,
                    g_lista_loguri[i].ip_client);
            
            numar_exportate++;
        }
    }
    
    pthread_mutex_unlock(&g_mutex_loguri);
    
    /*
     * Pas 5: Inchidem fisierul
     * 
     * IMPORTANT: Trebuie sa inchidem fisierul pentru ca:
     * - Datele sa fie scrise efectiv pe disk (flush)
     * - Sa eliberam resursa sistemului de operare
     */
    fclose(fisier);
    
    /*
     * Pas 6: Afisam mesaj de confirmare
     */
    printf(VERDE "\n  [OK] Exportate %d loguri in: %s\n" RESET, numar_exportate, nume_fisier);
}