#pragma once
#include <string>
#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "client_config.hpp"

#pragma comment(lib, "ws2_32.lib")

class NetworkClient {
private:
    SOCKET sock_fd;
    bool connected;
    bool wsa_initialized;
    char send_buffer[CLIENT_BUFFER_SIZE];
    char recv_buffer[CLIENT_BUFFER_SIZE];

public:
    NetworkClient() : sock_fd(INVALID_SOCKET), connected(false), wsa_initialized(false) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
        wsa_initialized = true;
    }

    ~NetworkClient() {
        disconnect();
        if (wsa_initialized) {
            WSACleanup();
        }
    }

    bool connect_to_server(const std::string& server_ip, int port) {
        if (sock_fd != INVALID_SOCKET) {
            closesocket(sock_fd);
            sock_fd = INVALID_SOCKET;
            connected = false;
        }

        sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_fd == INVALID_SOCKET) {
            int error = WSAGetLastError();
            std::cerr << "EROARE: socket() failed cu codul: " << error << std::endl;
            return false;
        }

       
        DWORD timeout = CONNECTION_TIMEOUT_MS;
        setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

        char flag = 1;
        setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        sockaddr_in server_addr;
        ZeroMemory(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<u_short>(port));

        int result = inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
        if (result <= 0) {
            std::cerr << "EROARE: IP invalid: " << server_ip << std::endl;
            closesocket(sock_fd);
            sock_fd = INVALID_SOCKET;
            return false;
        }

        std::cout << "Incercare de conectare la " << server_ip << ":" << port << "..." << std::endl;

        if (connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
            int error = WSAGetLastError();
            std::cerr << "EROARE: connect() failed cu codul: " << error << std::endl;

            switch (error) {
            case WSAECONNREFUSED:
                std::cerr << "  -> Conexiunea a fost refuzata (serverul nu asculta?)" << std::endl;
                break;
            case WSAETIMEDOUT:
                std::cerr << "  -> Timeout la conectare (serverul nu raspunde?)" << std::endl;
                break;
            case WSAEHOSTUNREACH:
                std::cerr << "  -> Host-ul nu poate fi atins (retea/routing?)" << std::endl;
                break;
            case WSAENETUNREACH:
                std::cerr << "  -> Reteaua nu este accesibila" << std::endl;
                break;
            default:
                std::cerr << "  -> Eroare necunoscuta: " << error << std::endl;
            }

            closesocket(sock_fd);
            sock_fd = INVALID_SOCKET;
            return false;
        }

        connected = true;
        std::cout << "Conectat cu succes!" << std::endl;
        return true;
    }

    bool send_message(const std::string& message) {
        if (!connected || sock_fd == INVALID_SOCKET) {
            std::cerr << "EROARE: Nu exista conexiune activa" << std::endl;
            return false;
        }

        if (message.length() >= CLIENT_BUFFER_SIZE) {
            std::cerr << "EROARE: Mesaj prea mare (" << message.length()
                << " >= " << CLIENT_BUFFER_SIZE << ")" << std::endl;
            return false;
        }

        if (message.empty()) {
            std::cerr << "EROARE: Mesaj gol" << std::endl;
            return false;
        }

        uint32_t msg_len = static_cast<uint32_t>(message.length());
        uint32_t msg_len_network = htonl(msg_len);

        int sent = send(sock_fd, reinterpret_cast<const char*>(&msg_len_network),
            sizeof(msg_len_network), 0);

        if (sent != sizeof(msg_len_network)) {
            int error = WSAGetLastError();
            std::cerr << "EROARE la trimiterea lungimii: " << error << std::endl;
            connected = false;
            return false;
        }

        size_t total_sent = 0;
        while (total_sent < message.length()) {
            int chunk = send(sock_fd, message.c_str() + total_sent,
                static_cast<int>(message.length() - total_sent), 0);

            if (chunk == SOCKET_ERROR) {
                int error = WSAGetLastError();
                std::cerr << "EROARE la trimiterea mesajului: " << error << std::endl;
                connected = false;
                return false;
            }

            total_sent += chunk;
        }

        return true;
    }

    bool receive_message(std::string& message, int timeout_ms = RECEIVE_TIMEOUT_MS) {
        if (!connected || sock_fd == INVALID_SOCKET) {
            std::cerr << "EROARE: Nu exista conexiune activa" << std::endl;
            return false;
        }

       
        DWORD timeout = timeout_ms;
        setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        uint32_t msg_len_network;
        int received = recv(sock_fd, reinterpret_cast<char*>(&msg_len_network),
            sizeof(msg_len_network), 0);

        if (received != sizeof(msg_len_network)) {
            if (received == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT) {
                    return false;
                }
                else {
                    std::cerr << "EROARE recv length: " << error << std::endl;
                    connected = false;
                }
            }
            else if (received == 0) {
                std::cerr << "Conexiunea s-a inchis de catre server" << std::endl;
                connected = false;
            }
            else {
                std::cerr << "Date incomplete la primirea lungimii (primit "
                    << received << " bytes in loc de 4)" << std::endl;
                connected = false;
            }
            return false;
        }

        uint32_t msg_len = ntohl(msg_len_network);

        if (msg_len == 0) {
            std::cerr << "Mesaj cu lungime 0 primit" << std::endl;
            return false;
        }

        if (msg_len > CLIENT_BUFFER_SIZE) {
            std::cerr << "EROARE CRITICA: Lungime mesaj invalida: " << msg_len
                << " (max permis: " << CLIENT_BUFFER_SIZE << ")" << std::endl;
            std::cerr << "  -> Bytes primiti (raw): "
                << "0x" << std::hex << msg_len_network << std::dec << std::endl;
            std::cerr << "  -> Aceasta indica o problema de protocol sau date corupte" << std::endl;
            std::cerr << "  -> Inchidem conexiunea pentru siguranta..." << std::endl;

            connected = false;
            disconnect();
            return false;
        }
        if (msg_len > 8192) {
            std::cerr << "AVERTISMENT: Mesaj mare primit: " << msg_len << " bytes" << std::endl;
        }

        message.clear();
        message.resize(msg_len);
        size_t total_received = 0;

        while (total_received < msg_len) {
            int chunk = recv(sock_fd, &message[total_received],
                static_cast<int>(msg_len - total_received), 0);

            if (chunk == SOCKET_ERROR) {
                int error = WSAGetLastError();
                std::cerr << "EROARE recv message: " << error << std::endl;
                connected = false;
                disconnect();
                return false;
            }

            if (chunk == 0) {
                std::cerr << "Conexiunea s-a inchis in timpul primirii (primit "
                    << total_received << "/" << msg_len << " bytes)" << std::endl;
                connected = false;
                disconnect();
                return false;
            }

            total_received += chunk;
        }

        return true;
    }

    void disconnect() {
        if (sock_fd != INVALID_SOCKET) {
            shutdown(sock_fd, SD_BOTH);
            closesocket(sock_fd);
            sock_fd = INVALID_SOCKET;
            connected = false;
        }
    }

    bool is_connected() const {
        return connected && sock_fd != INVALID_SOCKET;
    }

    bool check_connection() {
        if (!connected || sock_fd == INVALID_SOCKET) {
            return false;
        }

        char test_byte;
        int result = recv(sock_fd, &test_byte, 1, MSG_PEEK);

        if (result == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                connected = false;
                return false;
            }
        }
        else if (result == 0) {
            connected = false;
            return false;
        }

        return true;
    }
};