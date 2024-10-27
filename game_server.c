#include "sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_PLAYERS 4
#define GRID_WIDTH 30
#define GRID_HEIGHT 30
#define BUFFER_SIZE 1024

typedef struct {
    int id;
    int socket;
    int x;
    int y;
    int active;
} Player;

Player players[MAX_PLAYERS];
int grid[GRID_HEIGHT][GRID_WIDTH];
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = 0;
        }
    }
}

void generate_walls() {
    for (int y = 1; y < GRID_HEIGHT - 1; y++) {
        for (int x = 1; x < GRID_WIDTH - 1; x++) {
            if (rand() % 5 == 0) { // 20% chance to place a wall
                grid[y][x] = 1;
            }
        }
    }
}

void assign_player_id(Player* player, int id) {
    player->id = id;
    player->active = 1;

    // Find a random empty spot for the player
    do {
        player->x = rand() % GRID_WIDTH;
        player->y = rand() % GRID_HEIGHT;
    } while (grid[player->y][player->x] != 0);

    char assign_msg[BUFFER_SIZE];
    snprintf(assign_msg, sizeof(assign_msg), "ASSIGN_ID:%d", id);
    send_data(player->socket, assign_msg);
}

void broadcast_game_state() {
    char state_msg[BUFFER_SIZE * 2] = "GAME_STATE:"; // Increase buffer size
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active) {
            char player_info[50];
            snprintf(player_info, sizeof(player_info), "PLAYER:%d:%d:%d;", players[i].id, players[i].x, players[i].y);
            strcat(state_msg, player_info);
        }
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x] == 1) {
                char wall_info[20];
                snprintf(wall_info, sizeof(wall_info), "WALL:%d:%d;", x, y);
                strcat(state_msg, wall_info);
            }
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
            int steps = 1;
            char direction;
            sscanf(buffer + 12, "%d:%c", &steps, &direction);

            int dx = 0, dy = 0;
            switch (direction) {
                case 'W': dy = -steps; break;
                case 'S': dy = steps; break;
                case 'A': dx = -steps; break;
                case 'D': dx = steps; break;
            }

            pthread_mutex_lock(&game_mutex);
            int new_x = players[player_slot].x + dx;
            int new_y = players[player_slot].y + dy;

            // Check for wall collision
            if (new_x >= 0 && new_x < GRID_WIDTH && new_y >= 0 && new_y < GRID_HEIGHT && grid[new_y][new_x] == 0) {
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

int main() {
    int server_socket = init_server_socket(DEFAULT_PORT);
    printf("Server started on port %d\n", DEFAULT_PORT);

    initialize_grid();
    generate_walls();

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int* client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);

        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_socket);
        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}
