#include <iostream>
#include <cstring>
#include <unistd.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h> // use threads
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#else 

#include <netinet/in.h>
#include <sys/socket.h>
#include <pthreads.h>

#endif

#define PORT 443
#define BUFFER_SIZE 1024 

#ifdef _WIN32
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
};
#endif

class ClientHandler {
  public:
    // LPVOID => void*
    static DWORD WINAPI Handle(LPVOID client_socket_ptr) {
        SOCKET client_socket = (SOCKET) client_socket_ptr;  // Cast back to SOCKET type
        char buffer[BUFFER_SIZE] = {0}; 
        int storage_bytes;
        std::string lineBuffer;

        std::cout << "estamos online" << std::endl;
        while (true) {
          memset(buffer, 0, BUFFER_SIZE);

          if ((storage_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)) == SOCKET_ERROR) {
            std::cout << "Recv failed" << std::endl;
            exit(1);
          }

          if (storage_bytes == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
          }

          lineBuffer += std::string(buffer, storage_bytes);

          if (lineBuffer.find("\r\n") != std::string::npos) {
            std::cout << lineBuffer << std::endl;
            lineBuffer.clear();

            const char *message = "Message received\n";
            if (send(client_socket, message, strlen(message), 0) == SOCKET_ERROR) {
              std::cout << "Send failed" << "\n" << std::endl;
              break;
            }
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

        // std::thread client_thread(ClientHandler::Handle, new_socket);
        // client_thread.detach();
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
