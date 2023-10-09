#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


// Handle errors and exit the program
void handle_error(const char *error_message)
{
    fprintf(stderr, "\033[1;31mERROR: %s\n\033[0m", error_message);
    exit(EXIT_FAILURE);
}

char *empty_string(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        str[i] = '\0';
    }
    return str;
}

void send_message(int fd, char *message, struct sockaddr_in *address)
{
    int addr_size = sizeof(*address);

    if (sendto(fd, message, strlen(message), 0, (struct sockaddr *)address, addr_size) == -1)
    {
        handle_error("Couldn't send message to server");
    }
}


char *receive_message(int fd, struct sockaddr_in *address)
{
    char *message = (char *)calloc(1024, sizeof(char));
    int addr_size = sizeof(*address);

    if (recvfrom(fd, message, 1024, 0, (struct sockaddr *)address, &addr_size) < 0)
    {
        handle_error("Nothing was received from server");
    }
    return message;
}