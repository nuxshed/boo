// chat_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 5000

// Structure to hold client information
typedef struct {
    int socket;
    int id;
} client_t;

static client_t clients[MAX_CLIENTS];
static int num_clients = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to broadcast server message to all clients
void broadcast_message(const char* message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != -1) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Handle client connection
void* handle_client(void* arg) {
    client_t* client = (client_t*)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 32];

    // Notify server about new client
    printf("Client %d connected\n", client->id);

    // Handle client messages
    while (1) {
        int bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        // Just display the message on server console
        printf("Client %d: %s", client->id, buffer);
    }

    // Client disconnected
    pthread_mutex_lock(&clients_mutex);
    printf("Client %d disconnected\n", client->id);
    
    // Remove client from array
    clients[client->id].socket = -1;
    close(client->socket);
    num_clients--;
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

// Thread to handle server input
void* server_input(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            printf("Broadcasting: %s", buffer);
            broadcast_message(buffer);
        }
    }
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    pthread_t tid, server_tid;

    // Initialize all client sockets to -1
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        clients[i].id = i;
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        printf("Error number: %d\n", errno);
        printf("Error details: %s\n", strerror(errno));
        exit(1);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server started successfully on port %d\n", PORT);
    printf("Enter messages to broadcast to all clients:\n");

    // Create thread for server input
    if (pthread_create(&server_tid, NULL, server_input, NULL) != 0) {
        perror("Failed to create server input thread");
        exit(1);
    }
    pthread_detach(server_tid);

    // Accept client connections
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Get client's IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("New connection from %s\n", client_ip);

        // Find an empty slot for the new client
        pthread_mutex_lock(&clients_mutex);
        int client_id = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == -1) {
                client_id = i;
                clients[i].socket = client_socket;
                clients[i].id = i;
                num_clients++;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (client_id == -1) {
            printf("Maximum clients reached. Connection rejected.\n");
            close(client_socket);
            continue;
        }

        // Create thread to handle client
        if (pthread_create(&tid, NULL, handle_client, &clients[client_id]) != 0) {
            perror("Failed to create thread");
            close(client_socket);
            continue;
        }
        pthread_detach(tid);
    }

    return 0;
}
