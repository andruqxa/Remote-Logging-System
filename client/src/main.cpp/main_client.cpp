#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "process_collector.hpp"
#include "network_client.hpp"
#include "log_types.hpp"
#include "client_config.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

// flag global pentru oprire gracioasa
std::atomic<bool> running(true);
std::atomic<bool> shutdown_in_progress(false);

// handler pentru Ctrl+C 
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        if (!shutdown_in_progress.exchange(true)) {  // atomic check-and-set
            std::cout << "\n\n=== Primire semnal de oprire ===" << std::endl;
            std::cout << "Se opreste aplicatia..." << std::endl;
            running.store(false);
        }
        return TRUE;  // am gestionat semnalul
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    // configurare pentru a preveni crash-uri
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
    SetConsoleOutputCP(CP_UTF8);

    std::string server_ip = "192.168.0.213";  // MODIFICA IP !!!!!
    int server_port = 8080;
    int interval_seconds = 5;

    if (argc >= 2) server_ip = argv[1];
    if (argc >= 3) server_port = std::stoi(argv[2]);
    if (argc >= 4) interval_seconds = std::stoi(argv[3]);

    std::cout << "=== Client de Monitorizare Procese (Doar Apps) ===" << std::endl;
    std::cout << "Server: " << server_ip << ":" << server_port << std::endl;
    std::cout << "Interval: " << interval_seconds << " secunde" << std::endl;
    std::cout << "Mod: Trimitere doar aplicatii cu fereastra (Task Manager -> Apps)" << std::endl;
    std::cout << "Foloseste Ctrl+C pentru a opri" << std::endl << std::endl;

    // inregistreaza handler-ul pentru Ctrl+C
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        std::cerr << "EROARE: Nu se poate inregistra handler-ul pentru Ctrl+C" << std::endl;
        return 1;
    }

    NetworkClient client;
    std::cout << "Conectare la server..." << std::endl;

    // incercare de conectare cu protectie
    try {
        if (!client.connect_to_server(server_ip, server_port)) {
            std::cerr << "\n=== EROARE: Nu se poate conecta la server! ===" << std::endl;
            std::cerr << "Detalii:" << std::endl;
            std::cerr << "  - Server IP: " << server_ip << std::endl;
            std::cerr << "  - Port: " << server_port << std::endl;
            std::cerr << "\nVerificari necesare:" << std::endl;
            std::cerr << "  1. Serverul ruleaza?" << std::endl;
            std::cerr << "  2. Firewall-ul permite conexiunea?" << std::endl;
            std::cerr << "  3. IP-ul este accesibil? (ping " << server_ip << ")" << std::endl;

            // cleanup inainte de iesire
            SetConsoleCtrlHandler(ConsoleHandler, FALSE);
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "EROARE la initializare: " << e.what() << std::endl;
        SetConsoleCtrlHandler(ConsoleHandler, FALSE);
        return 1;
    }

    std::cout << "Socket conectat cu succes!" << std::endl;
    std::cout << "Trimitere mesaj de handshake..." << std::endl;

    // Handshake initial
    std::string hostname = ProcessCollector::get_hostname();
    std::string hello_msg = "{\"type\":\"HELLO\",\"client_name\":\"" +
        hostname + "\",\"version\":\"1.0\"}";

    if (!client.send_message(hello_msg)) {
        std::cerr << "EROARE: Nu se poate trimite mesaj de handshake!" << std::endl;
        client.disconnect();
        SetConsoleCtrlHandler(ConsoleHandler, FALSE);
        return 1;
    }

    // primeste welcome DOAR o data la inceput
    std::string welcome_msg;
    if (client.receive_message(welcome_msg, 5000)) {
        std::cout << "Raspuns de la server: " << welcome_msg << std::endl;
        std::cout << "Conectat si autentificat cu succes!" << std::endl << std::endl;
    }
    else {
        std::cout << "AVERTISMENT: Nu s-a primit raspuns de la server (continuam oricum...)" << std::endl << std::endl;
    }

    int total_processes_sent = 0;
    int cycle_count = 0;

    // bucla principala cu PROTECTIE COMPLETA
    while (running.load()) {
        try {
            auto start_time = std::chrono::steady_clock::now();
            cycle_count++;

            // verificare rapida daca trebuie sa oprim
            if (!running.load()) {
                std::cout << "Oprire detectata la inceputul ciclului" << std::endl;
                break;
            }

            std::cout << "\n=== Ciclu #" << cycle_count << " ===" << std::endl;

            // colectam DOAR aplicatiile (apps din task manager)
            std::vector<ProcessInfo> app_processes;

            try {
                app_processes = ProcessCollector::collect_app_processes();
            }
            catch (const std::exception& e) {
                std::cerr << "EROARE la colectarea proceselor: " << e.what() << std::endl;

                if (!running.load()) break;

                // continua cu lista vida
                app_processes.clear();
            }

            std::cout << "Aplicatii colectate (Apps): " << app_processes.size() << std::endl;

            if (app_processes.empty()) {
                std::cout << "  -> Nu s-au gasit aplicatii active cu ferestre" << std::endl;
            }
            else {
                // statistici
                int info_count = 0, warn_count = 0, error_count = 0;
                for (const auto& proc : app_processes) {
                    switch (proc.log_level) {
                    case LogLevel::INFO:  info_count++;  break;
                    case LogLevel::WARN:  warn_count++;  break;
                    case LogLevel::ERR:   error_count++; break;
                    }
                }

                std::cout << "Distributie: INFO=" << info_count
                    << ", WARN=" << warn_count
                    << ", ERROR=" << error_count << std::endl;

                // afisam lista de aplicatii gasite
                std::cout << "\nAplicatii detectate:" << std::endl;
                for (const auto& proc : app_processes) {
                    std::cout << "  - " << proc.name
                        << " (PID=" << proc.pid
                        << ", CPU=" << proc.cpu_percent << "%"
                        << ", MEM=" << (proc.memory_kb / 1024) << "MB)"
                        << std::endl;

                    // verificare Ctrl+C in timpul afisarii
                    if (!running.load()) {
                        std::cout << "Oprire detectata in timpul afisarii" << std::endl;
                        break;
                    }
                }

                if (!running.load()) break;

                std::cout << "\nTrimiteri individuale:" << std::endl;

                // trimitem fiecare proces individual
                int sent_in_cycle = 0;
                for (const auto& proc : app_processes) {
                    // verificare la fiecare iteratie
                    if (!running.load()) {
                        std::cout << "Oprire detectata - abandonam trimiterea" << std::endl;
                        break;
                    }

                    std::string json_data;
                    try {
                        json_data = proc.to_json(hostname);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "EROARE la serializarea procesului " << proc.name
                            << ": " << e.what() << std::endl;
                        continue;
                    }

                    // VALIDARE CRITICA: verifica dimensiunea mesajului
                    if (json_data.length() > MAX_JSON_MESSAGE_SIZE) {
                        std::cerr << "  -> AVERTISMENT: Mesaj prea mare pentru " << proc.name
                            << " (" << json_data.length() << " bytes > "
                            << MAX_JSON_MESSAGE_SIZE << " bytes) - OMIS" << std::endl;
                        continue;
                    }

                    if (json_data.length() > CLIENT_BUFFER_SIZE) {
                        std::cerr << "  -> EROARE: Mesaj depaseste buffer-ul pentru " << proc.name
                            << " - OMIS" << std::endl;
                        continue;
                    }

                    // verificare inainte de trimitere
                    if (!running.load()) break;

                    bool send_success = false;
                    try {
                        send_success = client.send_message(json_data);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "EXCEPTIE la trimitere: " << e.what() << std::endl;
                        send_success = false;
                    }

                    if (send_success) {
                        sent_in_cycle++;
                        total_processes_sent++;
                        std::cout << "  -> Trimis: " << proc.name << " [" << sent_in_cycle
                            << "/" << app_processes.size() << "] ("
                            << json_data.length() << " bytes)" << std::endl;

                        // pauză intre trimiteri - cu verificare
                        for (int i = 0; i < DELAY_BETWEEN_SENDS_MS / 10 && running.load(); ++i) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                    }
                    else {
                        std::cerr << "  -> EROARE la trimiterea procesului PID="
                            << proc.pid << " (" << proc.name << ")" << std::endl;

                        // verificare inainte de reconectare
                        if (!running.load()) break;

                        std::cout << "  -> Incercare de reconectare..." << std::endl;

                        try {
                            client.disconnect();
                            std::this_thread::sleep_for(std::chrono::seconds(1));

                            if (!running.load()) break;

                            if (client.connect_to_server(server_ip, server_port)) {
                                std::cout << "  -> Reconectat cu succes!" << std::endl;

                                // trimite doar handshake, NU asteapta raspuns
                                if (client.send_message(hello_msg)) {
                                    std::cout << "  -> Handshake trimis" << std::endl;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                }

                                if (!running.load()) break;

                                // reincercam sa trimitem procesul curent
                                if (client.send_message(json_data)) {
                                    sent_in_cycle++;
                                    total_processes_sent++;
                                    std::cout << "  -> Proces retrimis cu succes dupa reconectare" << std::endl;
                                }
                            }
                            else {
                                std::cerr << "  -> Reconectare esuata!" << std::endl;
                            }
                        }
                        catch (const std::exception& e) {
                            std::cerr << "  -> EXCEPTIE la reconectare: " << e.what() << std::endl;
                        }
                    }

                    // verificare finala
                    if (!running.load()) break;
                }

                std::cout << "\nTotal trimis in acest ciclu: " << sent_in_cycle << " aplicatii" << std::endl;
            }

            // verificare inainte de cleanup
            if (!running.load()) {
                std::cout << "Oprire detectata - abandonam cleanup-ul" << std::endl;
                break;
            }

            // curatam cache-ul CPU doar pentru procesele colectate
            try {
                ProcessCollector::cleanup_cpu_cache(app_processes);
            }
            catch (const std::exception& e) {
                std::cerr << "EROARE la cleanup CPU cache: " << e.what() << std::endl;
            }

            // verificare finala inainte de sleep
            if (!running.load()) break;

            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
            auto sleep_time = interval_seconds - elapsed.count();

            if (sleep_time > 0) {
                std::cout << "\nAsteptare " << sleep_time << " secunde pana la urmatorul ciclu..." << std::endl;

                // sleep cu verificari la fiecare secunda
                for (int i = 0; i < sleep_time && running.load(); ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

        }
        catch (const std::exception& e) {
            std::cerr << "\n!!! EXCEPTIE IN BUCLA PRINCIPALA: " << e.what() << " !!!" << std::endl;

            // daca e Ctrl+C, iesim imediat
            if (!running.load()) {
                std::cout << "Exceptie detectata in timpul opririi - iesim" << std::endl;
                break;
            }

            // altfel, asteptam si continuam
            std::cerr << "Reluam peste " << interval_seconds << " secunde..." << std::endl;

            for (int i = 0; i < interval_seconds && running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        catch (...) {
            std::cerr << "\n!!! EXCEPTIE NECUNOSCUTA !!!" << std::endl;

            if (!running.load()) {
                std::cout << "Exceptie necunoscuta in timpul opririi - iesim" << std::endl;
                break;
            }
        }
    }

   
    std::cout << "\n\n=== Inchidere aplicatie ===" << std::endl;

    // trimite goodbye DOAR daca suntem conectati
    try {
        if (client.is_connected()) {
            std::string goodbye_msg = "{\"type\":\"GOODBYE\",\"client_name\":\"" +
                hostname + "\"}";

            std::cout << "Trimitere mesaj goodbye..." << std::endl;
            client.send_message(goodbye_msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Eroare la goodbye (ignorata): " << e.what() << std::endl;
    }
    catch (...) {
    }

    // deconectare
    try {
        std::cout << "Deconectare..." << std::endl;
        client.disconnect();
    }
    catch (...) {
        // ignoram erori la deconectare
    }

    // dezinregistreaza handler-ul
    SetConsoleCtrlHandler(ConsoleHandler, FALSE);

    std::cout << "\n=== Client oprit cu succes! ===" << std::endl;
    std::cout << "Total cicluri: " << cycle_count << std::endl;
    std::cout << "Total aplicatii trimise: " << total_processes_sent << std::endl;

    return 0;
}