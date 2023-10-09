#include <stdio.h>
#include <stdlib.h>


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

//  Function to send message
void send_message(int fd, char *message)
{
    if (send(fd, message, strlen(message), 0) == -1)
    {
        handle_error("Couldn't send message to client");
    }
}

// Receive message from client
char *receive_message(int fd)
{
    char *message = (char *)calloc(1024, sizeof(char));
    if (recv(fd, message, 1024, 0) < 0)
    {
        handle_error("Nothing was received from client");
    }
    return message;
}
