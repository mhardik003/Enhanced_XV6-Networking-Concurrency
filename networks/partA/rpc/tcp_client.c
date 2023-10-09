#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils_tcp.c"

#define BUFFER_SIZE 256
#define RESPONSE_SIZE 1024

// Function Prototypes
char *obtain_ip_address();
int setup_tcp_client(char *ip_address, int port);
void communicate_with_server(int client_fd);
void handle_error(const char *error_message);

// // Main Function
// int main() {
//     char* ip_address = obtain_ip_address();  // Obtain IP address
//     int client_fd = setup_tcp_client(ip_address, PORT);  // Setup TCP client
//     communicate_with_server(client_fd);  // Communicate with server
//     close(client_fd);  // Close the client socket
//     return 0;
// }

// Function Definitions

// Obtain IP address of the client
char *obtain_ip_address()
{
    char hostbuffer[BUFFER_SIZE];
    struct hostent *host_entry;

    // Get the hostname
    if (gethostname(hostbuffer, sizeof(hostbuffer)) != 0)
    {
        handle_error("Error in obtaining IP address");
    }

    // Get the host entry corresponding to the hostname
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL)
    {
        handle_error("Error in obtaining IP address");
    }

    // Convert the address to a string and return the IP address
    return inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
}

// Setup TCP client and return the client file descriptor
int setup_tcp_client(char *ip_address, int port)
{
    int client_fd;
    struct sockaddr_in server_address;

    // Create a socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        handle_error("Socket not created");
    }

    // Configure server details
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip_address);
    server_address.sin_port = htons(port);

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        close(client_fd);
        handle_error("Couldn't connect with server");
    }

    return client_fd;
}

// Communicate with the server (send and receive messages)
void communicate_with_server(int client_fd)
{
    printf("0 for rock \n1 for paper \n2 for scissors\n");
    // Continuous communication loop
    while (1)
    {
        // Get user input and send to server
        char server_response[RESPONSE_SIZE];
        char input[BUFFER_SIZE];
        printf("Enter your move: ");
        scanf("%s", input);
        send_message(client_fd, input);

        // Receive and print server response
        strncpy(server_response, receive_message(client_fd), RESPONSE_SIZE);
        printf("%s ", server_response);

        // Additional communication based on server response
        input[0] = '\0';
        scanf("%s", input);
        send_message(client_fd, input);

        // Receive and print additional server response
        server_response[0] = '\0';
        strncpy(server_response, receive_message(client_fd), RESPONSE_SIZE);

        // Break loop if user input is "No" or server response is 0
        if (strcmp(input, "No") == 0 || atoi(server_response) == 0)
        {
            break;
        }
        empty_string(input);
        empty_string(server_response);
    }
}

