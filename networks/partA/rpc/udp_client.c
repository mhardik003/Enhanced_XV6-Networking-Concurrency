#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils_udp.c"

#define BUFFER_SIZE 1024

// Function Prototypes
int setup_udp_client(char *ip_address);
void communicate_with_server(int client_fd, struct sockaddr_in serverAddress);
void handle_error(const char *error_message);
char *get_ip_address();

// Function Definitions

// Get IP address and return it
char *get_ip_address()
{
    char *ip_address;
    struct hostent *host_entry;
    char hostbuffer[256];
    int hostname;

    // Get the hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname != 0)
    {
        handle_error("Error in obtaining IP address");
    }

    // Get the host entry
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL)
    {
        handle_error("Error in obtaining IP address");
    }

    // Convert an Internet network address into ASCII string
    ip_address = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));

    return ip_address;
}

// Setup UDP client and return the client file descriptor
int setup_udp_client(char *ip_address)
{
    int client_fd;

    // Create a socket
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd == -1)
    {
        handle_error("Socket not created");
    }

    return client_fd;
}

// Communicate with the server
void communicate_with_server(int client_fd, struct sockaddr_in serverAddress)
{

    printf("0 for rock \n1 for paper \n2 for scissors\n");

    while (1)
    {
        char server_response[BUFFER_SIZE];
        char input[256];
        socklen_t addr_size = sizeof(serverAddress);

        printf("Enter your move: ");
        scanf("%s", input);

        send_message(client_fd, input, &serverAddress);
        strncpy(server_response, receive_message(client_fd, &serverAddress), BUFFER_SIZE);
        printf("%s\n", server_response);

        input[0] = '\0';
        scanf("%s", input);
        send_message(client_fd, input, &serverAddress);
        server_response[0] = '\0';
        strncpy(server_response, receive_message(client_fd, &serverAddress), BUFFER_SIZE);

        if (strcmp(input, "No") == 0 || atoi(server_response) == 0)
        {
            printf("Game Over\n");
            break;
        }
    }
}
