#include "tcp_client.c"

#define PORT 8897

int main()
{
    char *ip_address = obtain_ip_address();
    int client_fd = setup_tcp_client(ip_address, PORT);
    communicate_with_server(client_fd);
    close(client_fd);
    return 0;
}