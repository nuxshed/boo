// game_server.c
#include "sock.h"
#include <stdio.h>
#include <pthread.h>

#define MAP_SIZE 10

typedef struct {
    int x, y;
} Player;

Player players[5];
int client_sockets[5] = { -1, -1, -1, -1, -1 };
pthread_mutex_t player_mutex = PTHREAD_MUTEX_INITIALIZER;

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    int player_id = -1;

    for (int i = 0; i < 5; i++) {
        if (client_sockets[i] == client_socket) {
            player_id = i;
            break;
        }
    }

    while (1) {
        char buffer[BUFFER_SIZE];
        int bytes_received = receive_data(client_socket, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) break;

        pthread_mutex_lock(&player_mutex);
        if (strcmp(buffer, "w") == 0 && players[player_id].y > 0) players[player_id].y--;
        if (strcmp(buffer, "s") == 0 && players[player_id].y < MAP_SIZE - 1) players[player_id].y++;
        if (strcmp(buffer, "a") == 0 && players[player_id].x > 0) players[player_id].x--;
        if (strcmp(buffer, "d") == 0 && players[player_id].x < MAP_SIZE - 1) players[player_id].x++;
        
        // Send updated map to all clients
        char map[MAP_SIZE * MAP_SIZE + MAP_SIZE];
        int idx = 0;
        for (int y = 0; y < MAP_SIZE; y++) {
            for (int x = 0; x < MAP_SIZE; x++) {
                int found = 0;
                for (int p = 0; p < 5; p++) {
                    if (players[p].x == x && players[p].y == y) {
                        map[idx++] = '0' + p;
                        found = 1;
                        break;
                    }
                }
                if (!found) map[idx++] = '.';
            }
            map[idx++] = '\n';
        }
        map[idx] = '\0';
        
        for (int i = 0; i < 5; i++) {
            if (client_sockets[i] != -1) {
                send_data(client_sockets[i], map);
            }
        }
        pthread_mutex_unlock(&player_mutex);
    }

    close(client_socket);
    client_sockets[player_id] = -1;
    return NULL;
}

int main() {
    int server_socket = init_server_socket();
    printf("Server started on port %d\n", PORT);

    for (int i = 0; i < 5; i++) {
        players[i].x = i;
        players[i].y = i;
    }

    while (1) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) continue;

        int slot = -1;
        for (int i = 0; i < 5; i++) {
            if (client_sockets[i] == -1) {
                slot = i;
                client_sockets[i] = client_socket;
                break;
            }
        }

        if (slot == -1) {
            printf("Server full\n");
            close(client_socket);
        } else {
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, &client_sockets[slot]);
            pthread_detach(tid);
        }
    }

    close(server_socket);
    return 0;
}
