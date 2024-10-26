// chat_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define PORT 5000

int client_socket;

// Function to receive messages from server
void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("\nDisconnected from server\n");
            exit(1);
        }
        buffer[bytes_received] = '\0';
        printf("Server broadcast: %s", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    pthread_t receive_thread;
    
    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "10.2.138.213", &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }

    printf("Attempting to connect to server on port %d...\n", PORT);

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        printf("Error number: %d\n", errno);
        printf("Error details: %s\n", strerror(errno));
        exit(1);
    }

    printf("Successfully connected to server\n");
    printf("Waiting for server broadcasts...\n");

    // Create thread to receive server broadcasts
    if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
        perror("Failed to create thread");
        exit(1);
    }

    // Main loop to send messages to server
    char message[BUFFER_SIZE];
    printf("Enter messages (they will be sent to server):\n");
    while (1) {
        if (fgets(message, BUFFER_SIZE, stdin) != NULL) {
            if (send(client_socket, message, strlen(message), 0) < 0) {
                perror("Send failed");
                break;
            }
        }
    }

    // Cleanup
    close(client_socket);
    return 0;
}
