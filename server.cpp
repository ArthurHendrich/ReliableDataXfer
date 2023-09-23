#include <iostream>
#include <cstring>
#include <unistd.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h> // use threads
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

#else 

#include <netinet/in.h>
#include <sys/socket.h>
#include <pthreads.h>
#include <openssl/md5.h>

#endif

#define PORT 443
#define BUFFER_SIZE 1024 

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

    // Checksum with md5
    static std::string Checksum(const std::string& data) {
      HCRYPTPROV hProv = 0; // Init a handle to a cryptographic service provider (CSP)
      HCRYPTHASH hHash = 0; // Init a handle to a hash object. This object will eventually hold the MD5 hash of the input data.
      DWORD cbHashSize = 16; // Sets the size of the hash (in bytes). MD5 produces a 128-bit (16-byte) hash.
      DWORD dwCount = cbHashSize; // Stores the size of a DWORD, which will be used to get hash parameters.
      BYTE RGBBufferHash[16] = {0}; // Initializes a byte array to hold the MD5 hash.
      std::ostringstream oss; // Creates an output string stream to convert the hash bytes into a hexadecimal string representation.


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

      // Retrieve the hash value into RGBBufferHash
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
    static DWORD WINAPI Handle(LPVOID client_socket_ptr) {
        SOCKET client_socket = (SOCKET) client_socket_ptr;  
        char buffer[BUFFER_SIZE] = {0}; 
        int storage_bytes;
        std::string lineBuffer;

        std::cout << "We are online" << std::endl;
        while (true) {
          memset(buffer, 0, BUFFER_SIZE);

          if ((storage_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)) == SOCKET_ERROR) {
            std::cout << "Recv failed with error code: " << WSAGetLastError() << std::endl;
            exit(1);
          }

          if (storage_bytes == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
          }

          lineBuffer += std::string(buffer, storage_bytes);

          size_t pos = 0;
          std::string token;
          while ((pos = lineBuffer.find("\r\n")) != std::string::npos) {
              token = lineBuffer.substr(0, pos);
              std::cout << token << std::endl;

              std::string checksumStr = "Checksum (MD5): " + Utility::Checksum(token) + "\n";
              if (send(client_socket, checksumStr.c_str(), checksumStr.length(), 0) == SOCKET_ERROR) {
                  std::cout << "Send failed" << "\n" << std::endl;
                  break;
              }
              lineBuffer.erase(0, pos + 2);
          }
        }
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
