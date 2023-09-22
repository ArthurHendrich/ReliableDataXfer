# TCP Server in C++

This is a simple TCP server implemented in C++ using Winsock for Windows. It listens on port 443 and accepts incoming connections, receiving and responding to messages from clients.

## Prerequisites

Before running this code, make sure you have the following:

- Visual Studio or a C++ development environment for Windows.
- Winsock library installed (linked via `#pragma comment(lib, "Ws2_32.lib")`).

## Usage

1. Clone the repository or download the source code.
2. Open the project in your C++ development environment.
3. Build (g++ server.cpp -o test.exe -lws2_32 -std=c++11) and run the code.
4. The server will start listening on port 443.

## Functionality

- The server binds to the local IP address and port 443.
- It accepts incoming connections from clients.
- When a client connects, the server receives messages from the client.
- Messages are expected to end with "\r\n".
- Upon receiving a complete message, the server sends back a "Message received" response.

## Troubleshooting

If you encounter any issues, consider checking the following:

- Ensure that the Winsock library is properly linked.
- Make sure no other application is using port 443 on your system.

## Disclaimer

This code is for educational purposes only and should be used responsibly and ethically. It demonstrates basic socket programming concepts and should not be used for malicious purposes.
