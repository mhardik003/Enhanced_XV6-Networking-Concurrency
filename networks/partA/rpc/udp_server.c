#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils_udp.c"
#include "rpc_utils.c"

#define PORT_A 8800
#define PORT_B 8801
#define BUFFER_SIZE 1024

// Function Prototypes
int setup_udp_server(int port);
void handle_clients(int server_fd_A, int server_fd_B);
void communicate_with_clients(int server_fd_A, int server_fd_B, struct sockaddr_in client_A, struct sockaddr_in client_B);

// Main Function
int main()
{
    int server_fd_A = setup_udp_server(PORT_A); // Setup UDP server A
    int server_fd_B = setup_udp_server(PORT_B); // Setup UDP server B
    handle_clients(server_fd_A, server_fd_B);   // Handle client communication
    close(server_fd_A);                         // Close the server socket A
    close(server_fd_B);                         // Close the server socket B
    return 0;
}

// Function Definitions

// Setup UDP server and return the server file descriptor
int setup_udp_server(int port)
{
    int server_fd;
    struct sockaddr_in server_address;

    // Create a socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    return server_fd;
}

// Handle communication with clients
void handle_clients(int server_fd_A, int server_fd_B)
{
    struct sockaddr_in client_A, client_B;
    socklen_t addr_size_A = sizeof(client_A);
    socklen_t addr_size_B = sizeof(client_B);

    // Communicate with clients
    communicate_with_clients(server_fd_A, server_fd_B, client_A, client_B);
}

// Communicate with clients A and B
void communicate_with_clients(int server_fd_A, int server_fd_B, struct sockaddr_in client_A, struct sockaddr_in client_B)
{

    // Continuous communication loop
    while (1)
    {
        char client_response_A[1024];
        char client_response_B[1024];
        char *server_message_A = (char *)calloc(256, sizeof(char));
        char *server_message_B = (char *)calloc(256, sizeof(char));

        strncpy(client_response_A, receive_message(server_fd_A, &client_A), BUFFER_SIZE);
        strncpy(client_response_B, receive_message(server_fd_B, &client_B), BUFFER_SIZE);

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

        send_message(server_fd_A, server_message_A, &client_A);
        send_message(server_fd_B, server_message_B, &client_B);

        client_response_A[0] = '\0';
        client_response_B[0] = '\0';

        strncpy(client_response_A, receive_message(server_fd_A, &client_A), BUFFER_SIZE);
        strncpy(client_response_B, receive_message(server_fd_B, &client_B), BUFFER_SIZE);

        printf("Client A: %s\n", client_response_A);
        printf("Client B: %s\n", client_response_B);

        client_response_A[3] = '\0';
        client_response_B[3] = '\0';

        if (strcmp(client_response_A, "Yes") == 0 && strcmp(client_response_B, "Yes") == 0)
        {
            send_message(server_fd_A, "1", &client_A);
            send_message(server_fd_B, "1", &client_B);
            continue;
        }
        else
        {
            send_message(server_fd_A, "0", &client_A);
            send_message(server_fd_B, "0", &client_B);
            break;
        }

        empty_string(client_response_A);
        empty_string(client_response_B);
        empty_string(server_message_A);
        empty_string(server_message_B);
    }
}


