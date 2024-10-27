#include "sock.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"

#define MAX_PLAYERS 4
#define GRID_WIDTH 30
#define GRID_HEIGHT 30
#define CELL_SIZE 30

#define CMD_MOVE "MOVE"
#define CMD_ASSIGN_ID "ASSIGN_ID"
#define CMD_GAME_STATE "GAME_STATE"

#define BUFFER_SIZE 1024

typedef struct {
    int id;
    int x;
    int y;
} PlayerInfo;

int grid[GRID_HEIGHT][GRID_WIDTH];
PlayerInfo players_info[MAX_PLAYERS];
int local_id = -1;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_grid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x] = 0;
        }
    }
}

void parse_game_state(char* data) {
    pthread_mutex_lock(&game_mutex);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        players_info[i].id = 0;
        players_info[i].x = 0;
        players_info[i].y = 0;
    }

    char* token = strtok(data, ";");
    while (token != NULL) {
        if (strncmp(token, "PLAYER:", 7) == 0) {
            int id, x, y;
            sscanf(token, "PLAYER:%d:%d:%d", &id, &x, &y);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (players_info[i].id == 0) {
                    players_info[i].id = id;
                    players_info[i].x = x;
                    players_info[i].y = y;
                    break;
                }
            }
        } else if (strncmp(token, "WALL:", 5) == 0) {
            int x, y;
            sscanf(token, "WALL:%d:%d", &x, &y);
            grid[y][x] = 1;
        }
        token = strtok(NULL, ";");
    }

    pthread_mutex_unlock(&game_mutex);
}

void* receive_thread(void* arg) {
    int client_socket = *((int*)arg);

    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = receive_data(client_socket, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        if (strncmp(buffer, CMD_ASSIGN_ID, strlen(CMD_ASSIGN_ID)) == 0) {
            sscanf(buffer, "ASSIGN_ID:%d", &local_id);
            printf("Assigned ID: %d\n", local_id);
        } else if (strncmp(buffer, CMD_GAME_STATE, strlen(CMD_GAME_STATE)) == 0) {
            parse_game_state(buffer + strlen(CMD_GAME_STATE) + 1);
        }
    }

    return NULL;
}

int main() {
    int client_socket = init_client_socket("127.0.0.1", DEFAULT_PORT);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, receive_thread, &client_socket);

    InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Game Client");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_W)) {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "ACTION:MOVE:1:W");
            send_data(client_socket, command);
        } else if (IsKeyPressed(KEY_S)) {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "ACTION:MOVE:1:S");
            send_data(client_socket, command);
        } else if (IsKeyPressed(KEY_A)) {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "ACTION:MOVE:1:A");
            send_data(client_socket, command);
        } else if (IsKeyPressed(KEY_D)) {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "ACTION:MOVE:1:D");
            send_data(client_socket, command);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        pthread_mutex_lock(&game_mutex);
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                Color cell_color = (grid[y][x] == 1) ? DARKGRAY : LIGHTGRAY;
                DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, cell_color);
            }
        }

        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players_info[i].id != 0) {
                Color player_color = (players_info[i].id == local_id) ? BLUE : (Color){255, 0, 0, 255};
                DrawRectangle(players_info[i].x * CELL_SIZE, players_info[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE, player_color);
            }
        }
        pthread_mutex_unlock(&game_mutex);

        EndDrawing();
    }

    CloseWindow();
    close(client_socket);
    return 0;
}
