#include "tcp.c"
#define INPUT_SIZE 1024
int main()
{
    while (1)
    {
        char *message = (char *)calloc(INPUT_SIZE, sizeof(char));

        receive_message();

        printf("------------------------------------\n");

        printf("Enter the message to be sent : ");
        fgets(message, INPUT_SIZE, stdin);
        // printf("Input : %s \n", message);

        send_message(message);

        free(message);
    }

    return 0;
}