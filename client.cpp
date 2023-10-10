#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 443
#define TIMEOUT_MS 1000

int main() {
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        std::cerr << "WSAStartup failed with error: " << err << std::endl;
        exit(1);
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create client socket" << std::endl;
        exit(1);
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect" << std::endl;
        exit(1);
    }

    while (true) {
        char message[1024];
        std::cout << "Send a message or type 'exit' to quit: ";
        std::cin.getline(message, sizeof(message));
        if (strcmp(message, "exit") == 0) {
            break;
        }
        strcat(message, "\r\n");

        if (send(clientSocket, message, strlen(message), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send data to the server" << std::endl;
        }

        auto send_time = std::chrono::system_clock::now();
        bool acknowledged = false;

        while (!acknowledged) {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - send_time);

            if (duration.count() > TIMEOUT_MS) {
                std::cout << "Timeout. Resending data..." << std::endl;
                if (send(clientSocket, message, strlen(message), 0) == SOCKET_ERROR) {
                    std::cerr << "Failed to send data to the server" << std::endl;
                }
                send_time = std::chrono::system_clock::now();
            }

            char buffer[1024] = {0};
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::cout << "Server answer: " << buffer << std::endl;
                acknowledged = true;
            } else if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR) {
                std::cerr << "Recv failed or connection closed" << std::endl;
                break;  // Break the inner loop upon an error or closure
            }
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
