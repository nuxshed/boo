#include "sock.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_PLAYERS 2
#define GRID_WIDTH 20
#define GRID_HEIGHT 20

#define CMD_MOVE "MOVE"
#define CMD_ASSIGN_ID "ASSIGN_ID"
#define CMD_GAME_STATE "GAME_STATE"

typedef struct {
    int id;
    int socket;
    int x, y;
    int active;
} Player;

int grid[GRID_HEIGHT][GRID_WIDTH];
Player players[MAX_PLAYERS];
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = 0;
        }
    }
}

void assign_player_id(Player* player, int id) {
    player->id = id;
    player->active = 1;
    player->x = (id == 1) ? 1 : GRID_WIDTH - 2;
    player->y = (id == 1) ? 1 : GRID_HEIGHT - 2;

    char assign_msg[BUFFER_SIZE];
    sprintf(assign_msg, "%s:%d", CMD_ASSIGN_ID, id);
    send_data(player->socket, assign_msg);
}

void broadcast_game_state() {
    char state_msg[BUFFER_SIZE] = "";
    char temp[64];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active) {
            sprintf(temp, "PLAYER:%d:%d:%d;", players[i].id, players[i].x, players[i].y);
            strcat(state_msg, temp);
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active) {
            send_data(players[i].socket, state_msg);
        }
    }
}

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);

    pthread_mutex_lock(&game_mutex);
    int player_slot = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active) {
            player_slot = i;
            players[i].socket = client_socket;
            assign_player_id(&players[i], i + 1);
            break;
        }
    }
    pthread_mutex_unlock(&game_mutex);

    if (player_slot == -1) {
        close(client_socket);
        printf("Connection refused: Max players reached.\n");
        return NULL;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = receive_data(client_socket, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) {
            printf("Player %d disconnected.\n", players[player_slot].id);
            pthread_mutex_lock(&game_mutex);
            players[player_slot].active = 0;
            pthread_mutex_unlock(&game_mutex);
            close(client_socket);
            break;
        }

        if (strncmp(buffer, "ACTION:MOVE:", 12) == 0) {
            int dx, dy;
            sscanf(buffer, "ACTION:MOVE:%d:%d", &dx, &dy);

            pthread_mutex_lock(&game_mutex);
            int new_x = players[player_slot].x + dx;
            int new_y = players[player_slot].y + dy;

            if (new_x >= 0 && new_x < GRID_WIDTH && new_y >= 0 && new_y < GRID_HEIGHT) {
                players[player_slot].x = new_x;
                players[player_slot].y = new_y;
                printf("Player %d moved to (%d, %d)\n", players[player_slot].id, new_x, new_y);
            }
            pthread_mutex_unlock(&game_mutex);

            broadcast_game_state();
        }
    }

    return NULL;
}

void run_server(int port) {
    initialize_grid();
    int server_socket = init_server_socket(port);
    printf("Server started on port %d. Waiting for players...\n", port);

    while (1) {
        struct sockaddr_in address;
        socklen_t addrlen = sizeof(address);
        int new_socket = accept(server_socket, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        int* pclient = malloc(sizeof(int));
        *pclient = new_socket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, pclient) != 0) {
            perror("Failed to create thread");
            free(pclient);
            close(new_socket);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_socket);
}

int main() {
    run_server(5001);
    return 0;
}