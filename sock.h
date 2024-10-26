#ifndef SOCK_H
#define SOCK_H

#include <stdbool.h>
#include <netinet/in.h>  // For sockaddr_in

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

typedef struct {
    int sockfd;
    struct sockaddr_in addr;
} Connection;

typedef struct {
    Connection clients[MAX_CLIENTS];
    int client_count;
    int server_fd;
} Server;

bool init_server(Server *server, int port);
bool accept_client(Server *server);
bool send_to_client(const Connection *client, const char *message);
void broadcast_to_clients(Server *server, const char *message, int sender_index);
int receive_from_client(const Connection *client, char *buffer, int buffer_size);
void close_client(Connection *client);
void shutdown_server(Server *server);

#endif // SOCK_H
