#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "udp_client.c"

#define PORT 8800


int main() {
    char* ip_address = get_ip_address();  // Get IP address
    int client_fd = setup_udp_client(ip_address);  // Setup UDP client
    struct sockaddr_in serverAddress;  // Server address structure

    // Configure server details
    memset(&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(ip_address);

    communicate_with_server(client_fd, serverAddress);  // Communicate with server
    close(client_fd);  // Close the client socket
    return 0;
}