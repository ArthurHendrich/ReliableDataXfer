#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <winsock.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>


#define PORT 443
#define BUFFER_SIZE 1024 // buffer for incoming messages

// The sockaddr_in structure contains an Internet address. It is used by the sockets API functions to specify a destination address for a socket connection.
struct sockaddr_in {
  short   sin_family; // Address family (e.g. AF_INET)
  u_short sin_port;   // Port number
  struct  in_addr sin_addr;   // IP address
  char    sin_zero[8];    // Padding to make the struct the same size as sockaddr
};


int main() {

  // Init socket
  int file_descriptor;
  int new_socket;
  int storage_bytes;
  int reusage = 1; // reuse address if already in use after program termination
  int size_of_address = sizeof(address); // get the real size of client address
  char buffer[BUFFER_SIZE] = {0}; 
  sockaddr_in address;

  // Create socket file descriptor
  if ((file_descriptor = socket(AF_INET, SOCK_STREAM, 0) == 0) {
    std::cout << "Socket creation was not successful" << std::endl;
    return 1;
  })

  // Setsockopt() is used to allow the local address to be reused when the server is restarted before the required wait time expires.
  // Sol Socket => beacuse we are using IPv4
  // Optname => SO_REUSEADDR => Allows a socket to bind to an address and port already in use. The SO_EXCLUSIVEADDRUSE option can prevent this.
  // Optname => SO_REUSEPORT => Allows a socket to bind to a port already in use, but only when all sockets listening on the port have this option enabled. Not supported on Windows.
  // Optval => A pointer to the buffer in which the value for the requested option is specified.
  if (setsockopt(file_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reusage, sizeof(reusage) < 0)) {
    std::cout << "Setsockopt failed" << std::endl;
    return 1;
  }

  // Start to fill the address struct
  address.sin_family = AF_INET; // IPv4
  address.sin_port = htons(PORT); // convert to network byte order
  address.sin_addr.s_addr = INADDR_ANY; // listen to all interfaces
  address.sin_zero = {0}; // fill with zeros

  // Bind socket to address and forcefully attach socket to the port 443
  // S -> socket file descriptor identifying an unbound socket
  // Addr -> A pointer to a sockaddr structure of the local address to assign to the bound socket .
  // namelen -> The length, in bytes, of the value pointed to by addr.
  if (bind(file_descriptor, (struct sockaddr *)&address, sizeof(address) < 0) {
    std::cout << "Bind failed" << std::endl;
    return 1;
  })

  // Listen for incoming connections
  // S -> socket file descriptor
  // backlog -> The maximum length of the queue of pending connections. 

  /* 
    If set to SOMAXCONN, the underlying service provider responsible for socket s will set the backlog to a maximum reasonable value. 
    If set to SOMAXCONN_HINT(N) (where N is a number), the backlog value will be N, adjusted to be within the range (200, 65535).
    Note that SOMAXCONN_HINT can be used to set the backlog to a larger value than possible with SOMAXCONN.
    SOMAXCONN_HINT is only supported by the Microsoft TCP/IP service provider. There is no standard provision to obtain the actual backlog value.
  */

 if (listen(file_descriptor, SOMAXCONN) < 0) {
   std::cout << "Listen failed" << std::endl;
   return 1;
 }

  // Accept incoming connections
  // S -> socket file descriptor
  // addr -> An optional pointer to a buffer that receives the address of the connecting entity, as known to the communications layer.
  // addrlen (in/out) -> An optional pointer to an integer that contains the length of addr.

  if ((new_socket = accept(file_descriptor, (struct sockaddr *)&address, (socklen_t *)&size_of_address)) < 0) {
    std::cout << "Accept failed" << std::endl;
    return 1;
  }

  // Read incoming messages
  // S -> socket file descriptor
  // buf -> A pointer to the buffer to receive the incoming data.
  // len -> The length, in bytes, of the buffer pointed to by the buf parameter.
  // flags -> A set of flags that influences the behavior of this function. The bits in the flags parameter can be combined with the bitwise OR operator (|).

  if ((storage_bytes = recv(new_socket, buffer, BUFFER_SIZE, 0)) < 0) {
    std::cout << "Recv failed" << std::endl;
    return 1;
  }

  // Send message to client
  // S -> socket file descriptor
  // buf -> A pointer to a buffer containing the message to be sent.
  // len -> The length, in bytes, of the message to be sent.
  // flags -> A set of flags that specify the way in which the call is made. This parameter is constructed by using the bitwise OR operator with any of the following values.

  if (send(new_socket, message, strlen(message), 0) < 0) {
    std::cout << "Send failed" << std::endl;
    return 1;
  }  

  // Close socket
  // S -> socket file descriptor
  if (close(file_descriptor) < 0) {
    std::cout << "Close failed" << std::endl;
    return 1;
  }

  if (shutdown(file_descriptor, 2) < 0) {
    std::cout << "Shutdown failed" << std::endl;
    return 1;
  }

  return 0;
}