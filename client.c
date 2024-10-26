#include "sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void *receive_messages(void *conn) {
    Connection *client = (Connection *)conn;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = receive_from_client(client, buffer, BUFFER_SIZE);
        if (bytes_received > 0) {
            printf("Message from server: %s", buffer);
        } else if (bytes_received == 0) {
            printf("Disconnected from server.\n");
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <SERVER_IP> <PORT>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    Connection client;
    client.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client.sockfd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    client.addr.sin_family = AF_INET;
    client.addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &client.addr.sin_addr);

    printf("Connecting to server %s on port %d...\n", server_ip, server_port);
    if (connect(client.sockfd, (struct sockaddr *)&client.addr, sizeof(client.addr)) == -1) {
        perror("Connection to server failed");
        return 1;
    }
    printf("Connected to the server.\n");

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &client);
    pthread_detach(recv_thread);

    char message[BUFFER_SIZE];
    while (1) {
        printf("Enter message: ");
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        if (!send_to_client(&client, message)) {
            printf("Failed to send message.\n");
        }
    }

    close_client(&client);
    return 0;
}
