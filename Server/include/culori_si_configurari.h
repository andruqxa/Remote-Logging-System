/*
 * =============================================================================
 * FISIER: culori_si_configurari.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Acest fisier contine toate culorile pentru terminal si configurarile
 *     principale ale serverului. Gandeste-te la el ca la un "panou de control"
 *     unde poti schimba setarile fara sa umbli prin tot codul.
 * 
 * CE GASESTI AICI:
 *     1. Coduri de culori ANSI - alea care fac textul colorat in terminal
 *     2. Configurari server - port, numar maxim clienti, etc.
 *     3. Dimensiuni buffere - cat de mare poate fi un mesaj
 * 
 * AUTOR: Echipa de dezvoltare
 * DATA: 2024
 * =============================================================================
 */

#ifndef CULORI_SI_CONFIGURARI_H    /* Verificam daca fisierul a fost deja inclus */
#define CULORI_SI_CONFIGURARI_H    /* Marcam ca l-am inclus (evitam duplicate) */

/* 
 * =============================================================================
 * SECTIUNEA 1: CONFIGURARI SERVER
 * =============================================================================
 * Aici setam parametrii principali ai serverului.
 * Daca vrei sa schimbi portul sau numarul de clienti, aici e locul.
 */

/* Portul pe care serverul asculta conexiuni 
 * Gandeste-te la port ca la un numar de apartament intr-un bloc (IP-ul e blocul)
 * Clientii trebuie sa stie acest numar ca sa se conecteze */
#define SERVER_PORT 8080

/* Cati clienti pot fi conectati in acelasi timp 
 * Daca ai mai multi, restul vor astepta */
#define MAX_CLIENTI 50

/* Dimensiunea buffer-ului pentru primirea datelor
 * Asta e cat de multe date putem primi dintr-o singura "inghititura"
 * 16KB ar trebui sa fie suficient pentru orice JSON */
#define DIMENSIUNE_BUFFER 16384

/* Cate loguri pastram in memorie
 * Cand ajungem la limita, le stergem pe cele vechi (FIFO - first in, first out) */
#define MAX_LOGURI 10000

/* Lungimea maxima a unui camp din JSON (nume proces, user, etc.) */
#define LUNGIME_CAMP 256


/* 
 * =============================================================================
 * SECTIUNEA 2: CODURI CULORI ANSI
 * =============================================================================
 * 
 * Cum functioneaza culorile in terminal?
 * -------------------------------------
 * Terminal-ul intelege coduri speciale care incep cu "\033[" (escape sequence).
 * Cand terminal-ul vede aceste coduri, nu le afiseaza, ci schimba culoarea.
 * 
 * Formatul este: \033[CODm
 *   - \033 = caracterul ESC (escape)
 *   - [ = inceput secventa
 *   - COD = numarul culorii
 *   - m = sfarsit secventa
 * 
 * Exemplu: printf(ROSU "Text rosu" RESET);
 *          Asta va afisa "Text rosu" cu culoare rosie, apoi revine la normal.
 */

/* RESET - Revine la culoarea normala (FOARTE IMPORTANT!)
 * Daca uiti sa pui RESET dupa o culoare, tot textul ramane colorat */
#define RESET       "\033[0m"

/* Culori pentru text (foreground) - numerele 30-37 */
#define NEGRU       "\033[30m"
#define ROSU        "\033[31m"
#define VERDE       "\033[32m"
#define GALBEN      "\033[33m"
#define ALBASTRU    "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define ALB         "\033[37m"

/* Stiluri de text */
#define BOLD        "\033[1m"     /* Text ingrosat (bold) */
#define DIM         "\033[2m"     /* Text estompat (mai putin vizibil) */

/* Culori pentru fundal (background) - numerele 40-47 
 * Functioneaza la fel, dar coloreaza fundalul, nu textul */
#define FUNDAL_NEGRU    "\033[40m"
#define FUNDAL_ROSU     "\033[41m"
#define FUNDAL_VERDE    "\033[42m"
#define FUNDAL_GALBEN   "\033[43m"
#define FUNDAL_ALBASTRU "\033[44m"
#define FUNDAL_MAGENTA  "\033[45m"
#define FUNDAL_CYAN     "\033[46m"
#define FUNDAL_ALB      "\033[47m"
#define FUNDAL_GRI      "\033[100m"


/* 
 * =============================================================================
 * SECTIUNEA 3: MENTINERE COMPATIBILITATE
 * =============================================================================
 * Pastram si denumirile vechi pentru compatibilitate cu codul existent.
 * Asa nu trebuie sa modificam tot codul care folosea denumirile in engleza.
 */

/* Alias-uri pentru compatibilitate cu codul original */
#define BLACK       NEGRU
#define RED         ROSU
#define GREEN       VERDE
#define YELLOW      GALBEN
#define BLUE        ALBASTRU
#define WHITE       ALB

#define BG_BLACK    FUNDAL_NEGRU
#define BG_RED      FUNDAL_ROSU
#define BG_GREEN    FUNDAL_VERDE
#define BG_YELLOW   FUNDAL_GALBEN
#define BG_BLUE     FUNDAL_ALBASTRU
#define BG_MAGENTA  FUNDAL_MAGENTA
#define BG_CYAN     FUNDAL_CYAN
#define BG_WHITE    FUNDAL_ALB
#define BG_GRAY     FUNDAL_GRI

/* Alias-uri pentru configurari (compatibilitate) */
#define BUFFER_SIZE     DIMENSIUNE_BUFFER
#define MAX_CLIENTS     MAX_CLIENTI
#define MAX_LOGS        MAX_LOGURI
#define MAX_FIELD_LEN   LUNGIME_CAMP

#endif /* CULORI_SI_CONFIGURARI_H */