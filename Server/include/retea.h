/*
 * =============================================================================
 * FISIER: retea.h
 * =============================================================================
 * 
 * DESCRIERE:
 *     Functii pentru partea de retea - serverul TCP care primeste conexiuni
 *     si mesaje de la clienti.
 * 
 * CONCEPTE DE BAZA NETWORKING:
 * 
 *     1. SOCKET = "priza" de retea, punctul prin care trimiti/primesti date
 *     
 *     2. TCP = protocol de comunicare care garanteaza ca datele ajung
 *              in ordine si fara erori (spre deosebire de UDP)
 *     
 *     3. PORT = numar care identifica un serviciu pe un calculator
 *               (ex: 80 = web, 22 = SSH, noi folosim 8080)
 *     
 *     4. IP = adresa calculatorului in retea (ex: 192.168.1.100)
 * 
 * CUM FUNCTIONEAZA UN SERVER TCP:
 * 
 *     1. Creaza un socket (socket())
 *     2. Il leaga de un port (bind())
 *     3. Incepe sa asculte (listen())
 *     4. Asteapta clienti (accept()) - BLOCHEAZA pana vine cineva
 *     5. Pentru fiecare client, creaza un thread separat
 *     6. Thread-ul citeste date de la client (recv())
 *     7. Cand clientul se deconecteaza, thread-ul se termina
 * 
 * =============================================================================
 */

#ifndef RETEA_H
#define RETEA_H


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: thread_gestionare_client
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Aceasta functie ruleaza intr-un THREAD SEPARAT pentru fiecare client.
 *     Se ocupa de:
 *     - Trimiterea mesajului de bun venit
 *     - Primirea datelor JSON de la client
 *     - Parsarea si adaugarea log-urilor
 *     - Curatarea cand clientul se deconecteaza
 * 
 * PARAMETRI:
 *     arg - pointer la structura InfoClient (socket + IP)
 * 
 * RETURNEAZA:
 *     NULL (cerut de interfata pthread)
 * 
 * NOTA:
 *     Aceasta functie este "entry point"-ul pentru pthread_create()
 */
void* thread_gestionare_client(void* arg);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: thread_server
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Thread-ul principal al serverului. Se ocupa de:
 *     - Crearea socket-ului server
 *     - Legarea la port (bind)
 *     - Ascultarea pentru conexiuni (listen)
 *     - Acceptarea clientilor noi (accept)
 *     - Crearea thread-urilor pentru fiecare client
 * 
 * PARAMETRI:
 *     arg - nefolosit (NULL)
 * 
 * RETURNEAZA:
 *     NULL
 */
void* thread_server(void* arg);


/*
 * -----------------------------------------------------------------------------
 * FUNCTIE: thread_refresh_automat
 * -----------------------------------------------------------------------------
 * CE FACE:
 *     Thread care actualizeaza automat display-ul cand apar loguri noi.
 *     Verifica la fiecare secunda daca s-a schimbat numarul de loguri.
 * 
 * PARAMETRI:
 *     arg - nefolosit (NULL)
 * 
 * RETURNEAZA:
 *     NULL
 */
void* thread_refresh_automat(void* arg);


/* Alias-uri pentru compatibilitate */
#define client_handler  thread_gestionare_client
#define server_thread   thread_server
#define refresh_thread  thread_refresh_automat


#endif /* RETEA_H */