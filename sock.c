#include "sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

bool init_server(Server *server, const char *ip_address, int port) {
    server->client_count = 0;
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd == -1) {
        perror("Socket creation failed");
        return false;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    server_addr.sin_port = htons(port);

    if (bind(server->server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return false;
    }
    if (listen(server->server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return false;
    }

    printf("Server initialized on IP %s, port %d\n", ip_address, port);
    return true;
}

bool accept_client(Server *server) {
    if (server->client_count >= MAX_CLIENTS) {
        printf("Max clients reached.\n");
        return false;
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_socket = accept(server->server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (new_socket < 0) {
        perror("Accept failed");
        return false;
    }

    server->clients[server->client_count].sockfd = new_socket;
    server->clients[server->client_count].addr = client_addr;
    server->client_count++;

    printf("New client connected. Total clients: %d\n", server->client_count);
    return true;
}

bool send_to_client(const Connection *client, const char *message) {
    return send(client->sockfd, message, strlen(message), 0) != -1;
}

void broadcast_to_clients(Server *server, const char *message, int sender_index) {
    for (int i = 0; i < server->client_count; i++) {
        if (i != sender_index) {  // Skip the sender
            send_to_client(&server->clients[i], message);
        }
    }
}

int receive_from_client(const Connection *client, char *buffer, int buffer_size) {
    memset(buffer, 0, buffer_size);
    return recv(client->sockfd, buffer, buffer_size, 0);
}

void close_client(Connection *client) {
    close(client->sockfd);
    client->sockfd = -1;
}

void shutdown_server(Server *server) {
    for (int i = 0; i < server->client_count; i++) {
        close_client(&server->clients[i]);
    }
    close(server->server_fd);
    printf("Server shut down.\n");
}
