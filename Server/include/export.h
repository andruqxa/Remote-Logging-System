/*
 * =============================================================================
 * FISIER: export.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Functii pentru exportarea log-urilor in diferite formate.
 *     Momentan suportam doar CSV, dar structura permite adaugarea altor formate.
 * 
 * CE E CSV?
 *     CSV = Comma-Separated Values (Valori Separate prin Virgula)
 *     E un format simplu de tabel, unde:
 *     - Fiecare linie e un rand din tabel
 *     - Coloanele sunt separate prin virgula
 *     - Prima linie e de obicei header-ul (numele coloanelor)
 *     
 *     Exemplu:
 *     Nume,Varsta,Oras
 *     Ion,25,Bucuresti
 *     Maria,30,Cluj
 *     
 *     Poate fi deschis in Excel, LibreOffice Calc, etc.
 * 
 * =============================================================================
 */

#ifndef EXPORT_H
#define EXPORT_H


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: exporta_loguri_csv
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Exporta toate log-urile (care trec filtrele curente) intr-un fisier CSV.
 *     
 *     Numele fisierului e generat automat:
 *     logs_export_2024-01-15_14_30_00.csv
 *     
 *     Coloanele exportate:
 *     Timestamp, PID, Process, User, Status, Level, CPU%, MemoryKB, Message, Hostname, ClientIP
 * 
 * RETURNEAZA:
 *     Nimic (void), dar afiseaza mesaj de succes/eroare pe ecran
 */
void exporta_loguri_csv(void);


/* Alias pentru compatibilitate */
#define export_to_csv exporta_loguri_csv


#endif /* EXPORT_H */