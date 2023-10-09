#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils_tcp.c"
#include "rpc_utils.c"

#define PORT_A 8896
#define PORT_B 8897
#define BUFFER_SIZE 256
#define RESPONSE_SIZE 1024

// Function Prototypes
int setup_tcp_server(int port);
void handle_clients(int server_fd_A, int server_fd_B);
void communicate_with_clients(int client_fd_A, int client_fd_B);

// Main Function
int main()
{
    int server_fd_A = setup_tcp_server(PORT_A); // Setup TCP server A
    int server_fd_B = setup_tcp_server(PORT_B); // Setup TCP server B
    handle_clients(server_fd_A, server_fd_B);   // Handle client communication
    close(server_fd_A);                         // Close the server socket A
    close(server_fd_B);                         // Close the server socket B
    return 0;
}

// Function Definitions

// Setup TCP server and return the server file descriptor
int setup_tcp_server(int port)
{
    int server_fd;
    struct sockaddr_in server_address;

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (server_fd == -1)
    {
        handle_error("Socket not created");
    }

    // Configure server details
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        close(server_fd);
        handle_error("Couldn't bind socket");
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1)
    {
        close(server_fd);
        handle_error("Listening failed");
    }

    return server_fd;
}

// Handle communication with clients
void handle_clients(int server_fd_A, int server_fd_B)
{
    struct sockaddr_in client_address;
    int addr_size = sizeof(client_address);

    // Accept connections from clients
    int client_fd_A = accept(server_fd_A, (struct sockaddr *)&client_address, &addr_size);
    if (client_fd_A < 0)
    {
        handle_error("Couldn't establish connection with client A");
    }

    int client_fd_B = accept(server_fd_B, (struct sockaddr *)&client_address, &addr_size);
    if (client_fd_B < 0)
    {
        close(client_fd_A);
        handle_error("Couldn't establish connection with client B");
    }

    // Communicate with clients
    communicate_with_clients(client_fd_A, client_fd_B);

    // Close client connections
    close(client_fd_A);
    close(client_fd_B);
}

// Communicate with clients A and B
void communicate_with_clients(int client_fd_A, int client_fd_B)
{

    // Continuous communication loop
    while (1)
    {
        char client_response_A[1024];
        char client_response_B[1024];
        char *server_message_A = (char *)calloc(256, sizeof(char));
        char *server_message_B = (char *)calloc(256, sizeof(char));

        strncpy(client_response_A, receive_message(client_fd_A), 1024);
        strncpy(client_response_B, receive_message(client_fd_B), 1024);

        //  keep only the first character
        client_response_A[1] = '\0';
        client_response_B[1] = '\0';

        printf("Client A: %s\n", client_response_A);
        printf("Client B: %s\n", client_response_B);

        int result = compute_result(atoi(client_response_A), atoi(client_response_B));

        if (result == 3) // Draw
        {
            printf("Result: Draw\n");
            strncpy(server_message_A, "The result of match is: \"Draw\"\nDo you want to play again?\0", 256);
            strncpy(server_message_B, "The result of match is: \"Draw\"\nDo you want to play again?\0", 256);
        }
        if (result == 4) // A wins
        {
            printf("Result: A wins\n");
            strncpy(server_message_A, "You have won the match\nDo you want to play again?\0", 256);
            strncpy(server_message_B, "You have lost the match\nDo you want to play again?\0", 256);
        }
        if (result == 5) // B wins
        {
            printf("Result: B wins\n");
            strncpy(server_message_A, "You have lost the match\nDo you want to play again?\0", 256);
            strncpy(server_message_B, "You have won the match\nDo you want to play again?\0", 256);
        }

        send_message(client_fd_A, server_message_A);
        send_message(client_fd_B, server_message_B);

        client_response_A[0] = '\0';
        client_response_B[0] = '\0';

        strncpy(client_response_A, receive_message(client_fd_A), 1024);
        strncpy(client_response_B, receive_message(client_fd_B), 1024);

        printf("Client A: %s\n", client_response_A);
        printf("Client B: %s\n", client_response_B);

        client_response_A[3] = '\0';
        client_response_B[3] = '\0';

        if (strcmp(client_response_A, "Yes") == 0 && strcmp(client_response_B, "Yes") == 0)
        {

            send_message(client_fd_A, "1");
            send_message(client_fd_B, "1");
        }
        else
        {
            send_message(client_fd_A, "0");
            send_message(client_fd_B, "0");
            break;
        }

        empty_string(client_response_A);
        empty_string(client_response_B);
        empty_string(server_message_A);
        empty_string(server_message_B);
    }
}
