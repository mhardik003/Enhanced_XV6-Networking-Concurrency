#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils_udp.c"

#define PORT 8855
#define BUFFER_SIZE 256
#define RESPONSE_SIZE 1024

// Function Prototypes
char *obtain_ip_address();
int setup_udp_client(char *ip_address, struct sockaddr_in *server_address);
void communicate_with_server(int client_fd, struct sockaddr_in *server_address);
void handle_error(const char *error_message);

// Main Function
int main()
{
    char *ip_address = obtain_ip_address(); // Obtain IP address
    struct sockaddr_in server_address;
    int client_fd = setup_udp_client(ip_address, &server_address); // Setup UDP client
    communicate_with_server(client_fd, &server_address);           // Communicate with server
    close(client_fd);                                              // Close the client socket
    return 0;
}

// Function Definitions

// Obtain IP address of the client
char *obtain_ip_address()
{
    char hostbuffer[BUFFER_SIZE];
    struct hostent *host_entry;

    if (gethostname(hostbuffer, sizeof(hostbuffer)) != 0)
    {
        handle_error("Error in obtaining IP address");
    }

    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL)
    {
        handle_error("Error in obtaining IP address");
    }

    return inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
}

// Setup UDP client and return the client file descriptor
int setup_udp_client(char *ip_address, struct sockaddr_in *server_address)
{
    int client_fd;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd == -1)
    {
        handle_error("Socket not created");
    }

    memset(server_address, '\0', sizeof(*server_address));
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(PORT);
    server_address->sin_addr.s_addr = inet_addr(ip_address);

    return client_fd;
}

// Communicate with the server (send and receive messages)
void communicate_with_server(int client_fd, struct sockaddr_in *server_address)
{
    char server_response[RESPONSE_SIZE];
    char *client_message = "Hello! This side Client";
    int addr_size = sizeof(*server_address);

    send_message(client_fd, client_message, server_address);

    strncpy(server_response, receive_message(client_fd, server_address), RESPONSE_SIZE);

    printf("Received from server: %s\n", server_response);
}
