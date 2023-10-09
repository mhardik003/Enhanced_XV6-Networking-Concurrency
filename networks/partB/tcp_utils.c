#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

#define PORT 8000
#define SERVER_IP "127.0.0.1"


void handle_error(const char *error_message, int client_socket)
{

    if (client_socket != -1)
    {
        close(client_socket);
    }
    perror(error_message);
    exit(EXIT_FAILURE);
}

int create_client_socket()
{
    // Create socket
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1)
    {
        handle_error("Error in creating socket", client_socket);
    }

    return client_socket;
}

// Function to configure server address
void configure_server_address(struct sockaddr_in *server_addr)
{
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr->sin_addr) <= 0)
    {
        handle_error("Error in converting IP address to binary format", -1);
    }
}

// function to divide the message into chunks and send back an array of chunks
char **divide_message(char *message, int chunk_size)
{
    int num_chunks = strlen(message) / chunk_size;
    int remainder = 0;
    if (strlen(message) % chunk_size != 0) // if there is a remainder, add one more chunk
    {
        remainder = 1;
        num_chunks++;
    }
    char **chunks = (char **)malloc(num_chunks * sizeof(char *));
    for (int i = 0; i < num_chunks; i++)
    {
        chunks[i] = (char *)malloc(chunk_size * sizeof(char));
        for (int j = 0; j < chunk_size; j++)
        {
            chunks[i][j] = message[i * chunk_size + j];
        }
    }
    // terminate each chunk with a null character
    for (int i = 0; i < num_chunks - 1; i++)
    {
        chunks[i][chunk_size] = '\0';
    }

    if (remainder == 0)
    {
        chunks[num_chunks - 1][chunk_size] = '\0';
    }
    else
    {
        chunks[num_chunks - 1][strlen(message) % chunk_size] = '\0';
    }
    return chunks;
}