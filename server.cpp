#include <iostream>
#include <cstring>

#include <winsock2.h>
#include <windows.h> 
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <sstream>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

#define PORT 443
#define BUFFER_SIZE 1024 
#define TIMEOUT_MS 1000

class Utility {
  public:
    static void InitializeWinSock() {
      WORD wVersionRequested;
      WSADATA wsa_data;
      int err;

      wVersionRequested = MAKEWORD(2, 2); 
      err = WSAStartup(wVersionRequested, &wsa_data);
      
      if (err != 0) {
        std::cout << "WSAStartup failed with error: " << err << std::endl;
        exit(1);
      }
    }

    // MD5 checksum
    static std::string Checksum(const std::string& data) {
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
      
      for (int  i = 0; i < cbHashSize; i++) {
        oss << std::hex << (int)RGBBufferHash[i];
      }


      CryptDestroyHash(hHash);
      CryptReleaseContext(hProv, 0);

      return oss.str();
    }

};

class ClientHandler {
public:
    static void HandleMessage(const std::string& message, const std::string& client_id, SOCKET client_socket) {
        std::cout << client_id << ": " << message << std::endl;

        std::string checksum = Utility::Checksum(message);
        std::cout << "Calculated Checksum (MD5): " << checksum << std::endl;

        std::string response = "Checksum (MD5): " + checksum + "\n";
        if (send(client_socket, response.c_str(), response.length(), 0) == SOCKET_ERROR) {
            std::cout << "Send failed" << "\n" << std::endl;
        }
    }

    static DWORD WINAPI Handle(LPVOID client_socket_ptr) {
        SOCKET client_socket = (SOCKET)client_socket_ptr;
        char buffer[BUFFER_SIZE] = {0};
        int storage_bytes;

        sockaddr_in client_info;
        int client_info_size = sizeof(client_info);
        getpeername(client_socket, (struct sockaddr*)&client_info, &client_info_size);
        std::string client_id = inet_ntoa(client_info.sin_addr);
        client_id += ":" + std::to_string(ntohs(client_info.sin_port));

        std::cout << "We are online. Client ID: " << client_id << std::endl;

        std::string lineBuffer;
        auto last_received_time = std::chrono::system_clock::now();

        while (true) {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_received_time);
            if (duration.count() > TIMEOUT_MS) {
                std::cout << "Client timed out." << std::endl;
                break;
            }

            memset(buffer, 0, BUFFER_SIZE);

            storage_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (storage_bytes <= 0) {
                std::cout << "Client " << client_id << " disconnected or failed" << std::endl;
                closesocket(client_socket);
                break;
            }

            last_received_time = std::chrono::system_clock::now();

            lineBuffer += std::string(buffer, storage_bytes);

            size_t pos;
            while ((pos = lineBuffer.find("\r\n")) != std::string::npos) {
                std::string token = lineBuffer.substr(0, pos);
                if (!token.empty()) {
                    HandleMessage(token, client_id, client_socket);
                }

                lineBuffer.erase(0, pos + 2);
            }
        }

        return 0;
    }
};


class Server {
  private:
    int file_descriptor;
    sockaddr_in address;
    int new_socket;
    int reusage = 1;

  public:
    Server() {
      Utility::InitializeWinSock();
      if ((file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cout << "Socket creation was not successful" << std::endl;
        exit(1);
      }

      if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reusage, sizeof(reusage)) == SOCKET_ERROR) {
        std::cout << "Setsockopt failed" << std::endl;
        exit(1);
      }

      address.sin_family = AF_INET;
      address.sin_port = htons(PORT); 
      address.sin_addr.s_addr = INADDR_ANY; 
      memset((&address.sin_zero), 0, 8);

      if (bind(file_descriptor, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cout << "Bind failed" << std::endl;
        exit(1);
      }

      if (listen(file_descriptor, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed" << std::endl;
        exit(1);
      }
    }

    void Start() {
      while (true) {
        int size_of_address = sizeof(address); 
        new_socket = accept(file_descriptor, (struct sockaddr *)&address, &size_of_address);

        if (new_socket == INVALID_SOCKET) {
          std::cout << "Accept failed" << std::endl;
          exit(1);
        }

        HANDLE client_thred;

        if((client_thred = CreateThread(NULL, 0, ClientHandler::Handle, (LPVOID)new_socket, 0, NULL)) == NULL) {
          std::cout << "Faile to creat thread" << std::endl;
          exit(1);
        }

        CloseHandle(client_thred);
      }
      Stop();
    }

    void Stop() {
      if (shutdown(file_descriptor, SD_BOTH) == SOCKET_ERROR) {
        std::cout << "Shutdown failed" << std::endl;
        exit(1);
      }

      if (closesocket(file_descriptor) == SOCKET_ERROR) {
        std::cout << "Close failed" << std::endl;
        exit(1);
      }

      if (closesocket(new_socket) == SOCKET_ERROR) {
        std::cout << "Close failed" << std::endl;
        exit(1);
      }
    }
};

int main() {
  Server server;
  server.Start();

  WSACleanup(); 

  return 0;
}
