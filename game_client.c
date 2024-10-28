#include "sock.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define MAX_PLAYERS 4
#define MAX_GHOSTS 10
#define GRID_WIDTH 30
#define GRID_HEIGHT 30
#define CELL_SIZE 30

#define CMD_MOVE "MOVE"
#define CMD_ASSIGN_ID "ASSIGN_ID"
#define CMD_GAME_STATE "GAME_STATE"
#define CMD_SHOOT "SHOOT"
#define CMD_GAME_OVER "GAME_OVER"

#define BUFFER_SIZE 2048

typedef struct {
    int id;
    int x;
    int y;
} PlayerInfo;

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

int grid[GRID_HEIGHT][GRID_WIDTH];
PlayerInfo players_info[MAX_PLAYERS];
Ghost ghosts[MAX_GHOSTS];
Bullet bullets[MAX_PLAYERS];
int local_id = -1;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

Color player_colors[MAX_PLAYERS] = {RED, GREEN, BLUE, YELLOW};

bool game_over = false;  

Texture2D backgroundTile;
Texture2D wallTile;
Texture2D playerSprites[MAX_PLAYERS];
Texture2D ghostSprite;

typedef enum {
    STATE_START_SCREEN,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

Texture2D CreatePixelArtTexture(int width, int height, Color* pixels) {
    Image image = GenImageColor(width, height, BLANK);
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            ImageDrawPixel(&image, x, y, pixels[y * width + x]);
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

Texture2D CreateBackgroundTile() {
    const int size = 16; 
    Color pixels[size * size];

    Color baseColor = (Color){10, 12, 15, 255};  
    Color lineColor = (Color){20, 25, 30, 255};  
    Color highlightColor = (Color){30, 35, 40, 255};  

    for(int i = 0; i < size * size; i++) {
        pixels[i] = baseColor;
    }

    for(int i = 0; i < size; i++) {
        pixels[i * size + size/2] = lineColor;     
        pixels[size/2 * size + i] = lineColor;     
    }

    pixels[0] = highlightColor;
    pixels[size-1] = highlightColor;
    pixels[size * (size-1)] = highlightColor;
    pixels[size * size - 1] = highlightColor;

    return CreatePixelArtTexture(size, size, pixels);
}

Texture2D CreateWallTile() {
    const int size = 16;
    Color pixels[size * size];

    Color baseColor = (Color){25, 0, 51, 255};  
    Color highlightColor = (Color){45, 0, 91, 255};  
    Color glowColor = (Color){60, 20, 120, 255};  

    for(int i = 0; i < size * size; i++) {
        pixels[i] = baseColor;
    }

    for(int y = 0; y < size; y++) {
        for(int x = 0; x < size; x++) {
            if((x + y) % 4 == 0) {
                pixels[y * size + x] = highlightColor;
            }
            if(x == 0 || x == size-1 || y == 0 || y == size-1) {
                pixels[y * size + x] = glowColor;
            }
        }
    }

    return CreatePixelArtTexture(size, size, pixels);
}

Texture2D CreateBatSprite(int playerIndex) {
    const int size = 16;
    Color pixels[size * size];

    Color baseColors[] = {
        (Color){220, 50, 50, 255},   
        (Color){50, 220, 50, 255},   
        (Color){50, 50, 220, 255},   
        (Color){220, 220, 50, 255}   
    };

    Color baseColor = baseColors[playerIndex];
    Color darkColor = (Color){
        baseColor.r/2, 
        baseColor.g/2, 
        baseColor.b/2, 
        255
    };

    for(int i = 0; i < size * size; i++) {
        pixels[i] = BLANK;
    }

    for(int y = 0; y < size; y++) {
        for(int x = 0; x < size; x++) {

            if(y >= size/3 && y < size*2/3 && x >= size/3 && x < size*2/3) {
                pixels[y * size + x] = baseColor;
            }

            if(y >= size/3 && y < size*2/3) {

                if(x >= 2 && x < size/3) {
                    pixels[y * size + x] = baseColor;
                }

                if(x >= size*2/3 && x < size-2) {
                    pixels[y * size + x] = baseColor;
                }
            }

            if(y == size/2 && (x == size/3+1 || x == size/2+1)) {
                pixels[y * size + x] = darkColor;
            }
        }
    }

    return CreatePixelArtTexture(size, size, pixels);
}

Texture2D CreateGhostSprite() {
    const int size = 16;
    Color pixels[size * size];

    Color ghostWhite = (Color){240, 240, 255, 255};  
    Color ghostBlue = (Color){180, 180, 255, 255};   
    Color eyeColor = (Color){40, 40, 80, 255};       

    for(int i = 0; i < size * size; i++) {
        pixels[i] = BLANK;
    }

    for(int y = 0; y < size; y++) {
        for(int x = 0; x < size; x++) {

            if(y >= size/4 && y < size*3/4 && x >= size/4 && x < size*3/4) {
                pixels[y * size + x] = ghostWhite;
            }

            if(y >= size*3/4 && y < size-1) {
                if((x + y) % 3 == 0 && x >= size/4 && x < size*3/4) {
                    pixels[y * size + x] = ghostWhite;
                }
            }

            if(y == size/2 && (x == size/3 || x == size*2/3)) {
                pixels[y * size + x] = eyeColor;
            }

            if(y >= size/4 && y < size*3/4 && x == size*3/4-1) {
                pixels[y * size + x] = ghostBlue;
            }
        }
    }

    return CreatePixelArtTexture(size, size, pixels);
}

Texture2D CreatePlayerSprite(int playerIndex) {
    const int size = 16;
    Color pixels[size * size];

    Color baseColors[] = {
        (Color){220, 50, 50, 255},   
        (Color){50, 220, 50, 255},   
        (Color){50, 50, 220, 255},   
        (Color){220, 220, 50, 255}   
    };

    Color baseColor = baseColors[playerIndex];
    Color darkColor = (Color){
        baseColor.r/2, 
        baseColor.g/2, 
        baseColor.b/2, 
        255
    };
    Color lightColor = (Color){
        MIN(255, baseColor.r*1.2), 
        MIN(255, baseColor.g*1.2), 
        MIN(255, baseColor.b*1.2), 
        255
    };
    Color armorColor = (Color){200, 200, 200, 255}; 

    for(int i = 0; i < size * size; i++) {
        pixels[i] = BLANK;
    }

    for(int y = 0; y < size; y++) {
        for(int x = 0; x < size; x++) {

            if(y >= 2 && y < 6 && x >= 6 && x < 10) {
                pixels[y * size + x] = armorColor;
            }

            if(y == 4 && x >= 7 && x < 9) {
                pixels[y * size + x] = darkColor;
            }

            if(y >= 6 && y < 12 && x >= 5 && x < 11) {
                pixels[y * size + x] = armorColor;
            }

            if(y >= 7 && y < 11 && x >= 7 && x < 9) {
                pixels[y * size + x] = baseColor;
            }

            if(y >= 6 && y < 10) {

                if(x >= 3 && x < 5) {
                    pixels[y * size + x] = lightColor;
                }

                if(x >= 11 && x < 13) {
                    pixels[y * size + x] = lightColor;
                }
            }

            if(y >= 12 && y < 15) {

                if(x >= 5 && x < 7) {
                    pixels[y * size + x] = darkColor;
                }

                if(x >= 9 && x < 11) {
                    pixels[y * size + x] = darkColor;
                }
            }

            if(y >= 8 && y < 10 && x >= 12 && x < 15) {
                pixels[y * size + x] = (Color){200, 200, 220, 255}; 
            }
        }
    }

    return CreatePixelArtTexture(size, size, pixels);
}

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
        bullets[i].active = 0;
    }

    for (int i = 0; i < MAX_GHOSTS; i++) {
        ghosts[i].active = 0;
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
        } else if (strncmp(token, "BULLET:", 7) == 0) {
            int id, x, y;
            char direction;
            sscanf(token, "BULLET:%d:%d:%d:%c", &id, &x, &y, &direction);
            bullets[id].x = x;
            bullets[id].y = y;
            bullets[id].direction = direction;
            bullets[id].active = 1;
        } else if (strncmp(token, "GHOST:", 6) == 0) {
            int x, y;
            sscanf(token, "GHOST:%d:%d", &x, &y);
            for (int i = 0; i < MAX_GHOSTS; i++) {
                if (!ghosts[i].active) {
                    ghosts[i].x = x;
                    ghosts[i].y = y;
                    ghosts[i].active = 1;
                    break;
                }
            }
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
        } else if (strncmp(buffer, CMD_GAME_OVER, strlen(CMD_GAME_OVER)) == 0) {
            printf("Game Over received.\n");
            game_over = true;
        }
    }

    return NULL;
}

void LoadGameTextures() {
    backgroundTile = CreateBackgroundTile();
    wallTile = CreateWallTile();
    ghostSprite = CreateGhostSprite();

    for(int i = 0; i < MAX_PLAYERS; i++) {
        playerSprites[i] = CreatePlayerSprite(i);
    }
}

void UnloadGameTextures() {
    UnloadTexture(backgroundTile);
    UnloadTexture(wallTile);
    UnloadTexture(ghostSprite);
    for(int i = 0; i < MAX_PLAYERS; i++) {
        UnloadTexture(playerSprites[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    int client_socket = init_client_socket(argv[1], DEFAULT_PORT);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, receive_thread, &client_socket);

    InitWindow(GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, "Game Client");
    LoadGameTextures();

    GameState gameState = STATE_START_SCREEN;
    int steps = 0;

    while (!WindowShouldClose()) {
        switch (gameState) {
            case STATE_START_SCREEN:
                BeginDrawing();
                ClearBackground(BLACK);

                const char* booText = "boo";
                int booFontSize = 80;
                int booTextWidth = MeasureText(booText, booFontSize);
                DrawText(booText, (GRID_WIDTH * CELL_SIZE - booTextWidth) / 2, GRID_HEIGHT * CELL_SIZE / 3, booFontSize, WHITE);

                const char* startText = "Press ENTER to Start";
                int startFontSize = 20;
                int startTextWidth = MeasureText(startText, startFontSize);
                DrawText(startText, (GRID_WIDTH * CELL_SIZE - startTextWidth) / 2, GRID_HEIGHT * CELL_SIZE * 2 / 3, startFontSize, WHITE);

                EndDrawing();

                if (IsKeyPressed(KEY_ENTER)) {
                    gameState = STATE_PLAYING;
                }
                break;

            case STATE_PLAYING:
                if (!game_over) {
                    for (int i = KEY_ZERO; i <= KEY_NINE; i++) {
                        if (IsKeyPressed(i)) {
                            steps = steps * 10 + (i - KEY_ZERO);
                        }
                    }

                    char command[BUFFER_SIZE];
                    char direction = '\0';

                    if (IsKeyPressed(KEY_W)) {
                        direction = 'W';
                    } else if (IsKeyPressed(KEY_S)) {
                        direction = 'S';
                    } else if (IsKeyPressed(KEY_A)) {
                        direction = 'A';
                    } else if (IsKeyPressed(KEY_D)) {
                        direction = 'D';
                    }

                    if (direction != '\0') {
                        if (steps == 0) steps = 1;  
                        snprintf(command, sizeof(command), "ACTION:MOVE:%d:%c", steps, direction);
                        send_data(client_socket, command);
                        steps = 0;  
                    }

                    if (IsKeyPressed(KEY_UP)) {
                        snprintf(command, sizeof(command), "ACTION:SHOOT:U");
                        send_data(client_socket, command);
                    } else if (IsKeyPressed(KEY_DOWN)) {
                        snprintf(command, sizeof(command), "ACTION:SHOOT:D");
                        send_data(client_socket, command);
                    } else if (IsKeyPressed(KEY_LEFT)) {
                        snprintf(command, sizeof(command), "ACTION:SHOOT:L");
                        send_data(client_socket, command);
                    } else if (IsKeyPressed(KEY_RIGHT)) {
                        snprintf(command, sizeof(command), "ACTION:SHOOT:R");
                        send_data(client_socket, command);
                    }
                }

                BeginDrawing();
                ClearBackground(BLACK);

                if (game_over) {
                    DrawText("GAME OVER", GRID_WIDTH * CELL_SIZE / 2 - MeasureText("GAME OVER", 40) / 2, GRID_HEIGHT * CELL_SIZE / 2 - 20, 40, RED);
                } else {
                    pthread_mutex_lock(&game_mutex);

                    for (int y = 0; y < GRID_HEIGHT; y++) {
                        for (int x = 0; x < GRID_WIDTH; x++) {

                            DrawTextureEx(backgroundTile, 
                                (Vector2){x * CELL_SIZE, y * CELL_SIZE}, 
                                0.0f, 
                                CELL_SIZE/16.0f,  
                                WHITE);

                            if (grid[y][x] == 1) {
                                DrawTextureEx(wallTile, 
                                    (Vector2){x * CELL_SIZE, y * CELL_SIZE}, 
                                    0.0f, 
                                    CELL_SIZE/16.0f, 
                                    WHITE);
                            }
                        }
                    }

                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if (players_info[i].id != 0) {
                            DrawTextureEx(playerSprites[players_info[i].id - 1],
                                (Vector2){players_info[i].x * CELL_SIZE, players_info[i].y * CELL_SIZE},
                                0.0f,
                                CELL_SIZE/16.0f,
                                WHITE);
                        }
                    }

                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if (bullets[i].active) {
                            DrawCircle(bullets[i].x * CELL_SIZE + CELL_SIZE / 2, 
                                      bullets[i].y * CELL_SIZE + CELL_SIZE / 2, 
                                      CELL_SIZE / 4, WHITE);
                        }
                    }

                    for (int i = 0; i < MAX_GHOSTS; i++) {
                        if (ghosts[i].active) {
                            DrawTextureEx(ghostSprite,
                                (Vector2){ghosts[i].x * CELL_SIZE, ghosts[i].y * CELL_SIZE},
                                0.0f,
                                CELL_SIZE/16.0f,
                                WHITE);
                        }
                    }

                    pthread_mutex_unlock(&game_mutex);
                }

                EndDrawing();
                break;

            case STATE_GAME_OVER:

                break;
        }
    }

    UnloadGameTextures();
    CloseWindow();
    close(client_socket);
    return 0;
}