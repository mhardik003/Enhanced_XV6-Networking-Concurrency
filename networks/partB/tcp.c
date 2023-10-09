#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

// #include "tcp_utils.c"

#define STRINGSIZE 1024
#define PORT 5000
#define CHUNK_SIZE 5
#define SERVER_IP "127.0.0.1"
#define TIMEOUT 2
#define MAX_RESEND 3



typedef struct {
    uint32_t seq_num;        // Sequence number of the packet
    uint32_t total_chunks;   // Total number of chunks
    char data[CHUNK_SIZE]; // Actual data chunk
} DataPacket;

typedef struct {
    int ack_sequence_number;
} Acknowledgement;


void handle_error(const char *error_message, int client_socket)
{

    if (client_socket != -1)
    {
        close(client_socket);
    }
    perror(error_message);
    exit(EXIT_FAILURE);
}

// function to divide the message into chunks and send back an array of chunks
DataPacket* divide_into_chunks(const char* data, uint32_t* num_chunks) {
    uint32_t data_len = strlen(data);
    *num_chunks = (data_len + CHUNK_SIZE - 1) / CHUNK_SIZE;
    DataPacket* packets = (DataPacket*)malloc(sizeof(DataPacket) * (*num_chunks));

    for(uint32_t i = 0; i < *num_chunks; ++i) {
        packets[i].seq_num = i;
        packets[i].total_chunks = *num_chunks;
        strncpy(packets[i].data, data + i*CHUNK_SIZE, CHUNK_SIZE);
    }

    return packets;
}


char* aggregate_chunks(DataPacket* packets, uint32_t num_chunks) {
    char* aggregated_data = (char*)malloc(num_chunks * CHUNK_SIZE + 1);
    for(uint32_t i = 0; i < num_chunks; ++i) {
        strncat(aggregated_data, packets[i].data, CHUNK_SIZE);
    }
    return aggregated_data;
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


void send_message(const char* message)
{
    int client_socket = create_client_socket();

    struct sockaddr_in server_addr;
    configure_server_address(&server_addr);

    uint32_t num_chunks;
    DataPacket* packets = divide_into_chunks(message, &num_chunks);

    for(uint32_t i = 0; i < num_chunks; ++i) {
        // Send the packet
        if (sendto(client_socket, &packets[i], sizeof(DataPacket), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        {
            handle_error("Error in sending message", client_socket);
        }

        // Receive the acknowledgement
        Acknowledgement ack;
        socklen_t addr_len = sizeof(server_addr);
        if (recvfrom(client_socket, &ack, sizeof(Acknowledgement), 0, (struct sockaddr*)&server_addr, &addr_len) == -1)
        {
            handle_error("Error in receiving acknowledgement", client_socket);
        }

        // change to green color
        printf("\033[0;32m");
        printf("Received acknowledgement for packet with sequence number %d\n", ack.ack_sequence_number);
        printf("\033[0m");
    }

    free(packets);
    close(client_socket);
}

void receive_message()
{
    int client_socket = create_client_socket();

    struct sockaddr_in server_addr;
    configure_server_address(&server_addr);

    // Bind the socket to the client address
    if (bind(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        handle_error("Error in binding socket to client address", client_socket);
    }

    // Receive the message
    DataPacket* packets = (DataPacket*)malloc(sizeof(DataPacket) * STRINGSIZE);
    uint32_t num_chunks = 0;
    while(1) {
        DataPacket packet;
        socklen_t addr_len = sizeof(server_addr);
        if (recvfrom(client_socket, &packet, sizeof(DataPacket), 0, (struct sockaddr*)&server_addr, &addr_len) == -1)
        {
            handle_error("Error in receiving message", client_socket);
        }

        printf("\033[0;32m");
        printf("Received packet with sequence number %d\n", packet.seq_num);
        printf("\033[0m");

        packets[packet.seq_num] = packet;
        num_chunks++;

        // Send the acknowledgement
        Acknowledgement ack;
        ack.ack_sequence_number = packet.seq_num;
        if (sendto(client_socket, &ack, sizeof(Acknowledgement), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        {
            handle_error("Error in sending acknowledgement", client_socket);
        }

        if(packet.seq_num == packet.total_chunks - 1) {
            break;
        }
    }

    char* message = aggregate_chunks(packets, num_chunks);
    // convert the message to yellow
    printf("\033[0;33m");
    printf("Received message: %s\n", message);
    printf("\033[0m");

    free(packets);
    free(message);
    close(client_socket);
}

