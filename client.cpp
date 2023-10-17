#include <iostream>
#include <cstring>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <sstream>
#include <chrono>
#include <map>

#include <thread>
#include <vector>

#include <random>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 443
#define TIMEOUT_MS 1000
#define WINDOW_SIZE 5
#define MESSAGE_SIZE 1024


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

    // Uncomment this to checksum error
    // return "1234567890";

    return oss.str();
  }
};

class Client {
  private:
    std::mt19937 gen;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    int send_base;
    int next_seq_num;
    std::map<int, std::string> unacknowledgedPackets; 


  public:
    Client() : gen(std::random_device{}()), send_base(0), next_seq_num(0) {
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
    // Client();
    void Connect();
    void Run();
    void Close();
    void sendPacket(const std::string& packet, int seq_num);
    static DWORD WINAPI sendPacketWrapper(LPVOID param);
};

void Client::sendPacket(const std::string& packet, int seq_num) {

    if (send(clientSocket, packet.c_str(), packet.length(), 0) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "Failed to send packet with sequence number " << seq_num << std::endl;
        std::cerr << "Socket Error Code: " << error << std::endl;

        switch (error) {
            case WSAECONNRESET:
                std::cerr << "Connection reset by peer.\n";
                break;
            case WSAETIMEDOUT:
                std::cerr << "Connection timed out.\n";
                break;
            default:
                std::cerr << "An unknown error occurred.\n";
                break;
        }
    }
}

// // Uncomment this to simulate errors
// void Client::sendPacket(const std::string& packet, int seq_num) {
//     int random_number = std::uniform_int_distribution<>(1, 100)(gen);
//     if (random_number <= 10) {
//         std::cout << "Generated random number: " << random_number << std::endl;
//         std::cout << "Simulating error for packet with sequence number " << seq_num << std::endl;
//         return;
//     }

//     if (send(clientSocket, packet.c_str(), packet.length(), 0) == SOCKET_ERROR) {
//         int error = WSAGetLastError();
//         std::cerr << "Failed to send packet with sequence number " << seq_num << std::endl;
//         std::cerr << "Socket Error Code: " << error << std::endl;

//         switch (error) {
//             case WSAECONNRESET:
//                 std::cerr << "Connection reset by peer.\n";
//                 break;
//             case WSAETIMEDOUT:
//                 std::cerr << "Connection timed out.\n";
//                 break;
//             default:
//                 std::cerr << "An unknown error occurred.\n";
//                 break;
//         }
//     } else {
//         std::cout << "Successfully sent packet with sequence number " << seq_num << std::endl;
//     }
// }



DWORD WINAPI Client::sendPacketWrapper(LPVOID param) {
    std::pair<Client*, std::pair<std::string, int>>* args = (std::pair<Client*, std::pair<std::string, int>>*)param;
    Client* client = args->first;
    std::string packet = args->second.first;
    int seq_num = args->second.second;

    client->sendPacket(packet, seq_num);

    delete args; 
    return 0;
}

void Client::Connect() {
  if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    std::cerr << "Failed to connect" << std::endl;
    exit(1);
  }
}

void Client::Run() {
  char message[MESSAGE_SIZE];
  std::string message_with_checksum_and_seq; 
  int count = 0;
  while (true) {
    while (next_seq_num < send_base + WINDOW_SIZE) {
      std::cout << "Send a message or type 'exit' to quit: ";
      std::cin.getline(message, sizeof(message));
      if (strcmp(message, "exit") == 0) {
        break;
      }

      std::string checksum = Utility::Checksum(std::string(message));
      std::cout << "Checksum (MD5): " << checksum << std::endl;

      message_with_checksum_and_seq = std::to_string(next_seq_num) + "|" + std::string(message) + "|" + checksum + "\r\n";
      unacknowledgedPackets[next_seq_num] = message_with_checksum_and_seq;  

      
      DWORD threadId;
      HANDLE hThread = CreateThread(NULL, 0, Client::sendPacketWrapper, new std::pair<Client*, std::pair<std::string, int>>(this, std::make_pair(message_with_checksum_and_seq, next_seq_num)), 0, &threadId);
      
      if (hThread == NULL) {
        std::cout << "CreateThread failed" << std::endl;
        exit(1);
      } else {
        CloseHandle(hThread);
      }

      next_seq_num++;
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

      char buffer[MESSAGE_SIZE] = {0};
      int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
      if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::string serverResponse = std::string(buffer);

        if (serverResponse.find("ACK:") != std::string::npos) {
          size_t start = serverResponse.find("ACK:") + 4;
          size_t end = serverResponse.find(":", start);
          std::string ack_seq_num_str = serverResponse.substr(start, end - start);
          
          try {
            int ack_seq_num = std::stoi(ack_seq_num_str);
            if (ack_seq_num == next_seq_num - 1) {
              std::cout << "Server acknowledged: " << serverResponse << std::endl;
              unacknowledgedPackets.erase(ack_seq_num);
              send_base = ack_seq_num + 1;
              acknowledged = true;
              count++;
            }
          } catch (const std::invalid_argument& ia) {
            std::cerr << "Invalid argument: " << ia.what() << std::endl;
            std::cerr << "Failed to convert sequence number string: '" << ack_seq_num_str << "'" << std::endl;  // Debugging line
            break;
          } catch (const std::out_of_range& oor) {
            std::cerr << "Out of Range error: " << oor.what() << std::endl;
            break;
          }
        } else if (serverResponse.find("NACK") != std::string::npos) {
          std::cout << "Server indicated a problem (NACK). Resending..." << std::endl;
          if (send(clientSocket, message_with_checksum_and_seq.c_str(), message_with_checksum_and_seq.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to resend data to the server" << std::endl;
          }
          send_time = std::chrono::system_clock::now();
        }
      } else if (bytesReceived == 0 || bytesReceived == SOCKET_ERROR) {
        std::cerr << "Recv failed or connection closed" << std::endl;
        Close();
        exit(1);
      }

      if (duration.count() > TIMEOUT_MS) {
        for (auto& entry : unacknowledgedPackets) {
          int seq_num = entry.first;
          std::string packet = entry.second;
          sendPacket(packet, seq_num);
        }
        send_time = std::chrono::system_clock::now();
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
