#include "sock.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

Server server;

void *client_handler(void *arg) {
    int client_index = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = receive_from_client(&server.clients[client_index], buffer, BUFFER_SIZE);
        if (bytes_received <= 0) {
            printf("Client %d disconnected.\n", client_index);
            close_client(&server.clients[client_index]);
            break;
        }

        printf("Received from client %d: %s", client_index, buffer);
        broadcast_to_clients(&server, buffer, client_index);  // Broadcast to all except sender
    }
    return NULL;
}

void *server_broadcast_handler(void *arg) {
    char message[BUFFER_SIZE];
    while (1) {
        printf("Server message: ");
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            perror("Failed to read server message");
            break;
        }
        broadcast_to_clients(&server, message, -1);  // Broadcast to all clients
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    if (!init_server(&server, server_ip, server_port)) {
        printf("Failed to initialize server.\n");
        return 1;
    }

    printf("Server running on %s:%d\n", server_ip, server_port);

    pthread_t server_broadcast_thread;
    pthread_create(&server_broadcast_thread, NULL, server_broadcast_handler, NULL);
    pthread_detach(server_broadcast_thread);

    while (1) {
        if (accept_client(&server)) {
            printf("Accepted new client: %d\n", server.client_count - 1);
            int client_index = server.client_count - 1;
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, client_handler, &client_index);
            pthread_detach(client_thread);
        }
    }

    shutdown_server(&server);
    return 0;
}
