# TCP Client-Server Application with Sliding Window Algorithm in C++

This is an advanced TCP client-server application implemented in C++ using Winsock for Windows. The application provides reliable data transport at the application layer, considering a channel with data loss and errors. It listens on port 443 for the server and uses the same port for the client to connect.

## Prerequisites

Before running this code, make sure you have the following:

- Visual Studio or a C++ development environment for Windows.
- Winsock library installed (linked via `#pragma comment(lib, "Ws2_32.lib")`).
- Cryptographic API library (linked via `-lcrypt32` for the server).

## Compilation

To compile the server and client, use the following commands:

For the server:
`g++ server.cpp -o server.exe -lws2_32 -lcrypt32 -std=c++11P`

For the client:
`g++ client.cpp -o client.exe -lws2_32 -std=c++11`


## Usage

1. Clone the repository or download the source code.
2. Open the project in your C++ development environment.
3. Build and run the code using the compilation commands above.
4. The server will start listening on port 443, and the client can connect to it.

## Functionality

- The server binds to the local IP address and port 443.
- It accepts incoming connections from clients.
- Both the client and server use a sliding window algorithm for reliable data transport.
- Messages are expected to end with "\r\n".
- The application handles:
  - Checksums for data integrity
  - Timers for timeouts
  - Sequence numbers for ordered data transfer
  - Acknowledgments (ACK) and Negative Acknowledgments (NACK)
- The server responds with either an "ACK" or "NACK" based on the received message's integrity and sequence number.

## Troubleshooting

If you encounter any issues, consider checking the following:

- Ensure that the Winsock and Cryptographic API libraries are properly linked.
- Make sure no other application is using port 443 on your system.

## Disclaimer

This code is for educational purposes only and should be used responsibly and ethically. It demonstrates advanced socket programming concepts and should not be used for malicious purposes.
