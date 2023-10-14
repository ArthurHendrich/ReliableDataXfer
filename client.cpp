#include <iostream>
#include <cstring>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <sstream>
#include <chrono>

#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 443
#define TIMEOUT_MS 1000



class Utility {
public:
  static std::string Checksum(const std::string &data) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD cbHashSize = 16;
    DWORD dwCount = cbHashSize;
    BYTE RGBBufferHash[16] = {0};
    std::ostringstream oss;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
      std::cout << "CryptAcquireContext failed" << std::endl;
      exit(1);
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
      std::cout << "CryptCreateHash failed" << std::endl;
      exit(1);
    }

    if (!CryptHashData(hHash, (BYTE *)data.c_str(), data.length(), 0)) {
      std::cout << "CryptHashData failed" << std::endl;
      exit(1);
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, RGBBufferHash, &dwCount, 0)) {
      DWORD dwError = GetLastError();
      std::cout << "CryptGetHashParam (for hash value) failed with error: " << dwError << std::endl;
      exit(1);
    }

    for (int i = 0; i < cbHashSize; i++) {
      oss << std::hex << (int)RGBBufferHash[i];
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return oss.str();
  }
};

class Client {
  private:
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    int sequence_number;

  public:
    Client();
    void Connect();
    void Run();
    void Close();
  
  static CRITICAL_SECTION CriticalSection;
  static std::vector<std::string> messages;
};

Client::Client() : sequence_number(0) {
  WSADATA wsaData;
  int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (err != 0) {
    std::cerr << "WSAStartup failed with error: " << err << std::endl;
    exit(1);
  }

  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == INVALID_SOCKET) {
    std::cerr << "Failed to create client socket" << std::endl;
    exit(1);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SERVER_PORT);
  serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
}

void Client::Connect() {
  if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    std::cerr << "Failed to connect" << std::endl;
    exit(1);
  }
}

void Client::Run() {
  while (true) {
    char message[1024];
    std::cout << "Send a message or type 'exit' to quit: ";
    std::cin.getline(message, sizeof(message));
    if (strcmp(message, "exit") == 0) {
      break;
    }

    std::string checksum = Utility::Checksum(std::string(message));
    std::cout << "Checksum (MD5): " << checksum << std::endl;
    // strcat(message, "\r\n");

    // std::string message_with_checksum = std::string(message) + "|" + checksum + "\r\n";

    std::string message_with_checksum_and_seq = std::to_string(sequence_number) + "|" + std::string(message) + "|" + checksum + "\r\n";

    if (send(clientSocket, message_with_checksum_and_seq.c_str(), message_with_checksum_and_seq.length(), 0) == SOCKET_ERROR) {
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
        // std::cout << "Server answer: " << buffer << std::endl;
        // acknowledged = true;
        std::string serverResponse = std::string(buffer);

        if (serverResponse.find("ACK") != std::string::npos) {
          std::cout << "Server acknowledged: " << buffer << std::endl;
          acknowledged = true;
          sequence_number++;
        }
        else if (serverResponse.find("NACK") != std::string::npos) {
          std::cout << "Server indicated a problem (NACK). Resending..." << std::endl;

          if (send(clientSocket, message_with_checksum_and_seq.c_str(), message_with_checksum_and_seq.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to resend data to the server" << std::endl;
          }
          send_time = std::chrono::system_clock::now();
        }
      }
      else if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR) {
        std::cerr << "Recv failed or connection closed" << std::endl;
        break;
      }
    }
  }
}

void Client::Close() {
    closesocket(clientSocket);
    WSACleanup();
}


int main() {
   Client client;
    client.Connect();
    client.Run();
    client.Close();
    return 0;
}
