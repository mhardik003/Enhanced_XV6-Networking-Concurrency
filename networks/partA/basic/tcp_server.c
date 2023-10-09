#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils_tcp.c"

#define PORT 8080
#define BUFFER_SIZE 256
#define RESPONSE_SIZE 1024

// Function Prototypes
char *get_ip_address();
int setup_server(int port);
void handle_client(int server_fd);
void send_receive_messages(int client_fd);
void handle_error(const char *error_message);

// Main Function
int main()
{
    char *ip_address = get_ip_address(); // Obtain IP address
    int server_fd = setup_server(PORT);  // Setup server
    handle_client(server_fd);            // Handle client communication
    close(server_fd);                    // Close the server socket
    return 0;
}

// Function Definitions

// Obtain IP address of the server
char *get_ip_address()
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

// Setup server and return the server file descriptor
int setup_server(int port)
{
    int server_fd;
    struct sockaddr_in server_address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        handle_error("Socket not created");
    }

    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        close(server_fd);
        handle_error("Couldn't bind socket");
    }

    if (listen(server_fd, 5) == -1)
    {
        close(server_fd);
        handle_error("Listening failed");
    }

    return server_fd;
}

// Accept client connection and handle communication
void handle_client(int server_fd)
{
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0)
    {
        close(server_fd);
        handle_error("Couldn't establish connection");
    }

    send_receive_messages(client_fd); // Send and receive messages
    close(client_fd);                 // Close the client socket
}

// Send and receive messages from the client
void send_receive_messages(int client_fd)
{
    char client_response[RESPONSE_SIZE];

    strncpy(client_response, receive_message(client_fd), RESPONSE_SIZE);
    printf("Received from client: %s\n", client_response);

    char *server_message = "Hello! Received your message";
    send_message(client_fd, server_message);
}


