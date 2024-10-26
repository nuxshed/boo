// game_client.c
#include "sock.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int client_socket;

void* receive_map(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = receive_data(client_socket, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) break;
        printf("%s", buffer);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    client_socket = init_client_socket(argv[1]);
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_map, NULL);

    printf("Controls: w = up, s = down, a = left, d = right\n");

    char input;
    while (1) {
        input = getchar();
        if (input == 'w' || input == 's' || input == 'a' || input == 'd') {
            char message[2] = { input, '\0' };
            send_data(client_socket, message);
        }
    }

    close(client_socket);
    return 0;
}
