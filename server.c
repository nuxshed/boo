#include "sock.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SERVER_PORT 12345

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
        // Broadcasting the server's message to all clients
        broadcast_to_clients(&server, message, -1);  // -1 ensures no client is skipped
    }
    return NULL;
}

int main() {
    if (!init_server(&server, SERVER_PORT)) {
        printf("Failed to initialize server.\n");
        return 1;
    }

    // Create a thread to handle server broadcasts
    pthread_t server_broadcast_thread;
    pthread_create(&server_broadcast_thread, NULL, server_broadcast_handler, NULL);
    pthread_detach(server_broadcast_thread);

    // Main loop to accept clients and start threads for each one
    while (1) {
        if (accept_client(&server)) {
            int client_index = server.client_count - 1;
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, client_handler, &client_index);
            pthread_detach(client_thread);
        }
    }

    shutdown_server(&server);
    return 0;
}
