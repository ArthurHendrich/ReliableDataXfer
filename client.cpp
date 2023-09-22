#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"  // Endereço IP do servidor
#define SERVER_PORT 443        // Porta do servidor

int main() {
    // Inicialize o WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Erro ao inicializar o WinSock." << std::endl;
        exit(1);
    }

    // Crie um soquete cliente
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Erro ao criar o soquete cliente." << std::endl;

        exit(1);
    }

    // Configure o endereço do servidor
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Conecte-se ao servidor
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Erro ao conectar-se ao servidor." << std::endl;
        exit(1);
    }

while (true) {
    char message[1024];
    std::cout << "Digite uma mensagem (ou 'exit' para sair): ";
    std::cin.getline(message, sizeof(message)); 
    if (strcmp(message, "exit") == 0) {
        break; 
    }
    strcat(message, "\r\n");

    if (send(clientSocket, message, strlen(message), 0) == SOCKET_ERROR) {
        std::cerr << "Erro ao enviar dados para o servidor." << std::endl;
    }
}

    
    // Receba a resposta do servidor
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Erro ao receber dados do servidor." << std::endl;
    } else {
        buffer[bytesReceived] = '\0';
        std::cout << "Resposta do servidor: " << buffer << std::endl;
    }

    // Feche o soquete e limpe o WinSock
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}