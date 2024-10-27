#include "sock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_PLAYERS 4
#define MAX_GHOSTS 10
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
} Ghost;

typedef struct {
    int x;
    int y;
    int active;
    char direction;
} Bullet;

Player players[MAX_PLAYERS];
Ghost ghosts[MAX_GHOSTS];
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

    for (int i = 0; i < MAX_GHOSTS; i++) {
        if (ghosts[i].active) {
            char ghost_info[30];
            snprintf(ghost_info, sizeof(ghost_info), "GHOST:%d:%d;", ghosts[i].x, ghosts[i].y);
            strcat(state_msg, ghost_info);
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

            if (new_x >= 0 && new_x < GRID_WIDTH && new_y >= 0 && new_y < GRID_HEIGHT && grid[new_y][new_x] == 0) {
                players[player_slot].x = new_x;
                players[player_slot].y = new_y;
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
    (void)arg;  // Suppress unused parameter warning
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
                    for (int j = 0; j < MAX_GHOSTS; j++) {
                        if (ghosts[j].active && ghosts[j].x == new_x && ghosts[j].y == new_y) {
                            ghosts[j].active = 0;
                            bullets[i].active = 0;
                            printf("Ghost at (%d, %d) was killed by a bullet!\n", new_x, new_y);
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

void* ghost_thread(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&game_mutex);
        // Increase spawn chance from 5% to 20%
        if (rand() % 100 < 20) {  // 20% chance to spawn a ghost each cycle
            for (int i = 0; i < MAX_GHOSTS; i++) {
                if (!ghosts[i].active) {
                    ghosts[i].active = 1;
                    // Spawn from a random edge
                    if (rand() % 2 == 0) {
                        ghosts[i].x = (rand() % 2) * (GRID_WIDTH - 1);
                        ghosts[i].y = rand() % GRID_HEIGHT;
                    } else {
                        ghosts[i].x = rand() % GRID_WIDTH;
                        ghosts[i].y = (rand() % 2) * (GRID_HEIGHT - 1);
                    }
                    break;
                }
            }
        }

        // Move ghosts towards the nearest player
        for (int i = 0; i < MAX_GHOSTS; i++) {
            if (ghosts[i].active) {
                int closest_player = -1;
                int min_distance = GRID_WIDTH * GRID_HEIGHT;
                for (int j = 0; j < MAX_PLAYERS; j++) {
                    if (players[j].active) {
                        int distance = abs(players[j].x - ghosts[i].x) + abs(players[j].y - ghosts[i].y);
                        if (distance < min_distance) {
                            min_distance = distance;
                            closest_player = j;
                        }
                    }
                }

                if (closest_player != -1) {
                    int dx = players[closest_player].x - ghosts[i].x;
                    int dy = players[closest_player].y - ghosts[i].y;
                    if (abs(dx) > abs(dy)) {
                        ghosts[i].x += (dx > 0) ? 1 : -1;
                    } else {
                        ghosts[i].y += (dy > 0) ? 1 : -1;
                    }

                    // Check if a ghost catches a player
                    if (ghosts[i].x == players[closest_player].x && ghosts[i].y == players[closest_player].y) {
                        players[closest_player].active = 0;
                        printf("Player %d was caught by a ghost!\n", players[closest_player].id);
                        // Send a game over message to the client
                        char game_over_msg[BUFFER_SIZE];
                        snprintf(game_over_msg, sizeof(game_over_msg), "GAME_OVER");
                        send_data(players[closest_player].socket, game_over_msg);
                    }
                }
            }
        }
        pthread_mutex_unlock(&game_mutex);

        broadcast_game_state();
        usleep(500000); // Move ghosts every 500ms
    }
    return NULL;
}

int main() {
    srand(time(NULL));  // Seed the random number generator

    int server_socket = init_server_socket(DEFAULT_PORT);
    printf("Server started on port %d\n", DEFAULT_PORT);

    initialize_grid();
    generate_walls();

    pthread_t bullet_tid, ghost_tid;
    pthread_create(&bullet_tid, NULL, bullet_thread, NULL);
    pthread_create(&ghost_tid, NULL, ghost_thread, NULL);

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
