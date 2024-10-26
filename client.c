#include "sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

void *receive_messages(void *conn) {
    Connection *client = (Connection *)conn;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = receive_from_client(client, buffer, BUFFER_SIZE);
        if (bytes_received > 0) {
            printf("Message from server: %s", buffer);
        }
    }
    return NULL;
}

int main() {
    Connection client;
    client.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    client.addr.sin_family = AF_INET;
    client.addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &client.addr.sin_addr);

    if (connect(client.sockfd, (struct sockaddr *)&client.addr, sizeof(client.addr)) == -1) {
        perror("Connection failed");
        return 1;
    }
    printf("Connected to the server.\n");

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &client);
    pthread_detach(recv_thread);

    char message[BUFFER_SIZE];
    while (1) {
        printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);
        send_to_client(&client, message);
    }

    close_client(&client);
    return 0;
}
