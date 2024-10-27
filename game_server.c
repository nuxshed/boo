#include "sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_PLAYERS 4
#define GRID_WIDTH 30
#define GRID_HEIGHT 30
#define BUFFER_SIZE 2048  // Increased buffer size

typedef struct {
    int id;
    int socket;
    int x;
    int y;
    int active;
} Player;

typedef struct {
    int x;
    int y;
    int active;
    char direction;
} Bullet;

Player players[MAX_PLAYERS];
Bullet bullets[MAX_PLAYERS];
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
    char state_msg[BUFFER_SIZE] = "GAME_STATE:";
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
        if (bullets[i].active) {
            char bullet_info[30];
            snprintf(bullet_info, sizeof(bullet_info), "BULLET:%d:%d:%d:%c;", i, bullets[i].x, bullets[i].y, bullets[i].direction);
            strcat(state_msg, bullet_info);
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
        } else if (strncmp(buffer, "ACTION:SHOOT:", 13) == 0) {
            char direction;
            sscanf(buffer + 13, "%c", &direction);

            pthread_mutex_lock(&game_mutex);
            bullets[player_slot].x = players[player_slot].x;
            bullets[player_slot].y = players[player_slot].y;
            bullets[player_slot].direction = direction;
            bullets[player_slot].active = 1;
            pthread_mutex_unlock(&game_mutex);

            broadcast_game_state();
        }
    }

    return NULL;
}

void* bullet_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&game_mutex);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (bullets[i].active) {
                int dx = 0, dy = 0;
                switch (bullets[i].direction) {
                    case 'U': dy = -1; break;
                    case 'D': dy = 1; break;
                    case 'L': dx = -1; break;
                    case 'R': dx = 1; break;
                }

                int new_x = bullets[i].x + dx;
                int new_y = bullets[i].y + dy;

                if (new_x < 0 || new_x >= GRID_WIDTH || new_y < 0 || new_y >= GRID_HEIGHT || grid[new_y][new_x] == 1) {
                    bullets[i].active = 0;
                } else {
                    for (int j = 0; j < MAX_PLAYERS; j++) {
                        if (players[j].active && players[j].x == new_x && players[j].y == new_y) {
                            players[j].active = 0;
                            bullets[i].active = 0;
                            printf("Player %d was hit by a bullet!\n", players[j].id);
                            break;
                        }
                    }
                    if (bullets[i].active) {
                        bullets[i].x = new_x;
                        bullets[i].y = new_y;
                    }
                }
            }
        }
        pthread_mutex_unlock(&game_mutex);

        broadcast_game_state();
        usleep(100000); // Move bullets every 100ms
    }
    return NULL;
}

int main() {
    int server_socket = init_server_socket(DEFAULT_PORT);
    printf("Server started on port %d\n", DEFAULT_PORT);

    initialize_grid();
    generate_walls();

    pthread_t bullet_tid;
    pthread_create(&bullet_tid, NULL, bullet_thread, NULL);

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
