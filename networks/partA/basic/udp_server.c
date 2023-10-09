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
char* obtain_ip_address();
int setup_udp_server(int port);
void handle_client(int server_fd);
void send_receive_messages(int server_fd, struct sockaddr_in* client_address, socklen_t addr_size);
void error_exit(const char* error_message);

// Main Function
int main() {
    char* ip_address = obtain_ip_address();  // Obtain IP address
    int server_fd = setup_udp_server(PORT);  // Setup UDP server
    handle_client(server_fd);  // Handle client communication
    close(server_fd);  // Close the server socket
    return 0;
}

// Function Definitions

// Obtain IP address of the server
char* obtain_ip_address() {
    char hostbuffer[BUFFER_SIZE];
    struct hostent *host_entry;

    if(gethostname(hostbuffer, sizeof(hostbuffer)) != 0) {
        error_exit("Error in obtaining IP address");
    }

    host_entry = gethostbyname(hostbuffer);
    if(host_entry == NULL) {
        error_exit("Error in obtaining IP address");
    }

    return inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
}

// Setup UDP server and return the server file descriptor
int setup_udp_server(int port) {
    int server_fd;
    struct sockaddr_in server_address;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == -1) {
        error_exit("Socket not created");
    }

    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        close(server_fd);
        error_exit("Couldn't bind socket");
    }

    return server_fd;
}

// Handle client communication
void handle_client(int server_fd) {
    struct sockaddr_in client_address;
    socklen_t addr_size = sizeof(client_address);

    send_receive_messages(server_fd, &client_address, addr_size);  // Send and receive messages
}

// Send and receive messages from the client
void send_receive_messages(int server_fd, struct sockaddr_in* client_address, socklen_t addr_size) {
    char client_response[RESPONSE_SIZE];

    strncpy(client_response, receive_message(server_fd, client_address), RESPONSE_SIZE);
    printf("Received from client: %s\n", client_response);

    char* server_message = "Hello! Received your message";
    send_message(server_fd, server_message, client_address);
}

// Handle errors and exit the program
void error_exit(const char* error_message) {
    fprintf(stderr, "\033[1;31mERROR: %s\n\033[0m", error_message);
    exit(EXIT_FAILURE);
}
