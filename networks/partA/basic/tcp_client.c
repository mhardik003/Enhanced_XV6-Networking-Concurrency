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
char *obtain_ip_address();
int establish_connection(char *ip_address, int port);
void interact_with_server(int socket_fd);
void handle_error(const char *error_message);



// Main Function
int main()
{
    char *ip_address = obtain_ip_address();                 // Acquiring IP address
    int socket_fd = establish_connection(ip_address, PORT); // Establishing connection
    interact_with_server(socket_fd);                        // Interaction (send/receive messages)
    close(socket_fd);                                       // Closing the socket
    return 0;
}

// Function Definitions


// Obtain the IP address of the client
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

    // Convert the address to a string and return
    return inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
}

// Establish a connection to the server and return the socket file descriptor
int establish_connection(char *ip_address, int port)
{
    int socket_fd;
    struct sockaddr_in server_address;

    // Create a socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        handle_error("Socket not created");
    }

    // Configure server details
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip_address);
    server_address.sin_port = htons(port);

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        close(socket_fd);
        handle_error("Couldn't connect with server");
    }

    return socket_fd;
}

// Send a message to the server and display the received message
void interact_with_server(int socket_fd)
{
    char server_response[RESPONSE_SIZE];
    char *client_message = "Hello! This side Client";

    // Send message to server
    send_message(socket_fd, client_message);

    // Receive message from server
    strncpy(server_response, receive_message(socket_fd), RESPONSE_SIZE);
    printf("Received from server: %s\n", server_response);
}


