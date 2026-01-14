#include "terminal.h"
#include "structuri_date.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>   /* Pentru struct termios si functii de control terminal */


/*
 * Variabila globala care salveaza setarile originale ale terminalului.
 * O folosim pentru a restaura terminalul la iesire.
 */
static struct termios g_setari_originale_terminal;


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: seteaza_terminal_raw
 * -----------------------------------------------------------------------------
 */
void seteaza_terminal_raw(void) {
    /*
     * Pas 1: Salvam setarile curente ale terminalului
     * 
     * STDIN_FILENO = 0 = file descriptor pentru standard input (tastatura)
     * tcgetattr() = "Terminal Control GET ATTRibutes"
     */
    tcgetattr(STDIN_FILENO, &g_setari_originale_terminal);
    
    /*
     * Pas 2: Facem o copie pe care o modificam
     */
    struct termios setari_noi = g_setari_originale_terminal;
    
    /*
     * Pas 3: Modificam flag-urile
     * 
     * c_lflag = "local flags" (flag-uri locale)
     * 
     * ICANON = mod canonical (asteapta Enter) - il dezactivam
     * ECHO = afiseaza ce tastezi - il dezactivam
     * 
     * ~(ICANON | ECHO) = inversam bitii acestor flag-uri (le dezactivam)
     * &= inseamna "pastreaza doar bitii care sunt 1 in ambele"
     */
    setari_noi.c_lflag &= ~(ICANON | ECHO);
    
    /*
     * Pas 4: Setam comportamentul pentru read()
     * 
     * c_cc = "control characters" (caractere de control)
     * 
     * VMIN = numar minim de caractere de citit inainte sa returneze
     *        0 = returneaza imediat chiar daca nu e nimic
     * 
     * VTIME = timeout in zecimi de secunda
     *         1 = 0.1 secunde
     */
    setari_noi.c_cc[VMIN] = 0;   /* Nu astepta caractere */
    setari_noi.c_cc[VTIME] = 1;  /* Timeout 0.1 secunde */
    
    /*
     * Pas 5: Aplicam noile setari
     * 
     * TCSANOW = aplica imediat ("NOW")
     * Alternative: TCSADRAIN (dupa ce se goleste buffer-ul de iesire)
     *              TCSAFLUSH (+ ignora input-ul necitit)
     */
    tcsetattr(STDIN_FILENO, TCSANOW, &setari_noi);
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: seteaza_terminal_normal
 * -----------------------------------------------------------------------------
 */
void seteaza_terminal_normal(void) {
    /*
     * Pur si simplu restauram setarile originale salvate
     */
    tcsetattr(STDIN_FILENO, TCSANOW, &g_setari_originale_terminal);
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: citeste_caracter
 * -----------------------------------------------------------------------------
 */
char citeste_caracter(void) {
    char caracter = 0;
    
    /*
     * read() citeste din file descriptor
     * 
     * STDIN_FILENO = standard input (tastatura)
     * &caracter = unde sa puna caracterul citit
     * 1 = cate caractere sa citeasca (maxim)
     * 
     * Returneaza: numarul de caractere citite (0 daca nu e nimic)
     * 
     * In modul raw cu VMIN=0 si VTIME=1:
     * - Daca e un caracter in buffer, il returneaza imediat
     * - Daca nu e nimic, asteapta 0.1 secunde si returneaza 0
     */
    read(STDIN_FILENO, &caracter, 1);
    
    return caracter;
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: citeste_linie
 * -----------------------------------------------------------------------------
 */
void citeste_linie(char* buffer, size_t dimensiune) {
    /*
     * Pentru a citi o linie intreaga (cu echo si tot), trebuie sa
     * trecem temporar in modul normal.
     */
    seteaza_terminal_normal();
    
    /*
     * fgets() citeste o linie (pana la Enter sau EOF)
     * 
     * Parametri:
     * - buffer = unde sa puna textul
     * - dimensiune = cat de mare e buffer-ul (pentru siguranta)
     * - stdin = de unde sa citeasca (standard input)
     * 
     * Returneaza NULL daca e eroare sau EOF
     */
    if (fgets(buffer, dimensiune, stdin) != NULL) {
        /*
         * fgets() include si '\n' la sfarsit - il eliminam
         * 
         * strcspn() returneaza lungimea pana la primul caracter din set
         * Practic gaseste pozitia lui '\n'
         */
        buffer[strcspn(buffer, "\n")] = '\0';
    }
    
    /* Revenim la modul raw */
    seteaza_terminal_raw();
}


/*
 * -----------------------------------------------------------------------------
 * IMPLEMENTARE: handler_semnal
 * -----------------------------------------------------------------------------
 */
void handler_semnal(int sig) {
    /*
     * Marcam parametrul ca nefolosit pentru a evita warning-uri
     * (void)sig e un "cast to void" - spune compilatorului ca stim
     * ca nu folosim variabila si e intentionat
     */
    (void)sig;
    
    /*
     * Setam flag-ul de oprire
     * 
     * g_server_ruleaza e declarat ca volatile sig_atomic_t
     * ceea ce garanteaza ca scrierea e atomica (nu poate fi intrerupta)
     * si compilatorul nu optimizeaza accesul la ea.
     * 
     * Dupa aceasta, toate buclele while(g_server_ruleaza) se vor opri.
     */
    g_server_ruleaza = 0;
}