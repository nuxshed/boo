#include "sock.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"

#define MAX_PLAYERS 2
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
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
        }
        token = strtok(NULL, ";");
    }

    pthread_mutex_unlock(&game_mutex);
}

void* receive_thread_func(void* arg) {
    int client_socket = *((int*)arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = receive_data(client_socket, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            close(client_socket);
            exit(1);
        }

        buffer[bytes_received] = '\0';
        parse_game_state(buffer);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    int client_socket = init_client_socket(argv[1], 5001);

    InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Pumpkin Panic");
    SetTargetFPS(60);

    initialize_grid();

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread_func, &client_socket);
    pthread_detach(recv_thread);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                DrawRectangleLines(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, LIGHTGRAY);
            }
        }

        pthread_mutex_lock(&game_mutex);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players_info[i].id != 0) {
                Color player_color = (players_info[i].id == local_id) ? BLUE : RED;
                DrawRectangle(players_info[i].x * CELL_SIZE, players_info[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE, player_color);
                DrawText(TextFormat("P%d", players_info[i].id), players_info[i].x * CELL_SIZE + 5, players_info[i].y * CELL_SIZE + 5, 20, WHITE);
            }
        }
        pthread_mutex_unlock(&game_mutex);

        if (IsKeyPressed(KEY_W)) {
            char command[BUFFER_SIZE];
            sprintf(command, "ACTION:MOVE:0:-1");
            send_data(client_socket, command);
        } else if (IsKeyPressed(KEY_S)) {
            char command[BUFFER_SIZE];
            sprintf(command, "ACTION:MOVE:0:1");
            send_data(client_socket, command);
        } else if (IsKeyPressed(KEY_A)) {
            char command[BUFFER_SIZE];
            sprintf(command, "ACTION:MOVE:-1:0");
            send_data(client_socket, command);
        } else if (IsKeyPressed(KEY_D)) {
            char command[BUFFER_SIZE];
            sprintf(command, "ACTION:MOVE:1:0");
            send_data(client_socket, command);
        }

        EndDrawing();
    }

    CloseWindow();
    close(client_socket);
    return 0;
}