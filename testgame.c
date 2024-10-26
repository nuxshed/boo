#include "raylib.h"
#include <string.h>
#include <ctype.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TILE_SIZE 40
#define MAX_BULLETS 100
#define MAX_INPUT_DIGITS 3

typedef enum {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    SHOOT_UP,
    SHOOT_DOWN,
    SHOOT_LEFT,
    SHOOT_RIGHT,
    WAIT
} Action;

typedef enum {
    PLANNING,    // Players are choosing their moves
    EXECUTING,   // Moves and bullets are being animated
    GAME_OVER    // One player has won
} GameState;

typedef struct {
    Vector2 position;
    Vector2 direction;
    bool active;
    int playerOwner;  // 1 or 2
} Bullet;

typedef struct {
    Vector2 position;
    Color color;
    bool isAlive;
    int score;
    Action plannedAction;
    bool hasPlannedAction;
} Player;

const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 15;
int gameMap[15][20] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,1,0,0,0,0,1,0,0,1,1,0,0,0,0,0,0,1},
    {1,0,0,1,0,1,0,0,0,0,0,1,1,0,0,0,1,0,0,1},
    {1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,1},
    {1,0,0,1,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Time between each bullet movement step (in seconds)
const float EXECUTE_STEP_TIME = 0.1f;

// Function to check collision with walls
bool CheckCollisionWithWalls(Vector2 pos) {
    int mapX = (int)(pos.x / TILE_SIZE);
    int mapY = (int)(pos.y / TILE_SIZE);
    
    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) return true;
    return gameMap[mapY][mapX] == 1;
}

// Function to move a bullet by one tile
void MoveBullet(Bullet* bullet) {
    if (bullet->active) {
        bullet->position.x += bullet->direction.x * TILE_SIZE;
        bullet->position.y += bullet->direction.y * TILE_SIZE;

        // Check collision with walls
        if (CheckCollisionWithWalls(bullet->position)) {
            bullet->active = false;
            return;
        }
    }
}

// Function to respawn a player
void RespawnPlayer(Player* player, bool isPlayer1) {
    if (isPlayer1) {
        player->position = (Vector2){TILE_SIZE * 2, TILE_SIZE * 2};
    } else {
        player->position = (Vector2){TILE_SIZE * (MAP_WIDTH - 3), TILE_SIZE * (MAP_HEIGHT - 3)};
    }
    player->isAlive = true;
}

// Function to get direction vector from action
Vector2 GetDirectionFromAction(Action action) {
    switch (action) {
        case MOVE_UP:    return (Vector2){0, -1};
        case MOVE_DOWN:  return (Vector2){0, 1};
        case MOVE_LEFT:  return (Vector2){-1, 0};
        case MOVE_RIGHT: return (Vector2){1, 0};
        case SHOOT_UP:   return (Vector2){0, -1};
        case SHOOT_DOWN: return (Vector2){0, 1};
        case SHOOT_LEFT: return (Vector2){-1, 0};
        case SHOOT_RIGHT:return (Vector2){1, 0};
        default:         return (Vector2){0, 0};
    }
}

// Function to determine if an action is a shooting action
bool IsShootAction(Action action) {
    return action >= SHOOT_UP && action <= SHOOT_RIGHT;
}

// Function to draw action prompts for players
void DrawActionPrompt(Player player, int playerNum, int x, int y) {
    const char* actionText = "Waiting...";
    if (!player.hasPlannedAction) {
        if (playerNum == 1) {
            actionText = "Choose action (WASD=move, IJKL=shoot, SPACE=wait)";
        } else {
            actionText = "Choose action (Arrow Keys=move, TFGH=shoot, ENTER=wait)";
        }
    }
    DrawText(TextFormat("Player %d: %s", playerNum, actionText), x, y, 20, player.color);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Turn-Based Two-Player Game");
    SetTargetFPS(60);

    Player player1 = {
        .position = {TILE_SIZE * 2, TILE_SIZE * 2},
        .color = BLUE,
        .isAlive = true,
        .score = 0,
        .hasPlannedAction = false
    };

    Player player2 = {
        .position = {TILE_SIZE * (MAP_WIDTH - 3), TILE_SIZE * (MAP_HEIGHT - 3)},
        .color = GREEN,
        .isAlive = true,
        .score = 0,
        .hasPlannedAction = false
    };

    Bullet bullets[MAX_BULLETS] = {0};
    GameState gameState = PLANNING;
    float executionTimer = EXECUTE_STEP_TIME;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        switch (gameState) {
            case PLANNING:
                // Player 1 input
                if (!player1.hasPlannedAction && player1.isAlive) {
                    if (IsKeyPressed(KEY_W)) player1.plannedAction = MOVE_UP;
                    if (IsKeyPressed(KEY_S)) player1.plannedAction = MOVE_DOWN;
                    if (IsKeyPressed(KEY_A)) player1.plannedAction = MOVE_LEFT;
                    if (IsKeyPressed(KEY_D)) player1.plannedAction = MOVE_RIGHT;
                    if (IsKeyPressed(KEY_I)) player1.plannedAction = SHOOT_UP;
                    if (IsKeyPressed(KEY_J)) player1.plannedAction = SHOOT_DOWN;
                    if (IsKeyPressed(KEY_K)) player1.plannedAction = SHOOT_LEFT;
                    if (IsKeyPressed(KEY_L)) player1.plannedAction = SHOOT_RIGHT;
                    if (IsKeyPressed(KEY_SPACE)) player1.plannedAction = WAIT;
                    
                    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
                        IsKeyPressed(KEY_I) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) || IsKeyPressed(KEY_L) ||
                        IsKeyPressed(KEY_SPACE)) {
                        player1.hasPlannedAction = true;
                    }
                }

                // Player 2 input
                if (!player2.hasPlannedAction && player2.isAlive) {
                    if (IsKeyPressed(KEY_UP)) player2.plannedAction = MOVE_UP;
                    if (IsKeyPressed(KEY_DOWN)) player2.plannedAction = MOVE_DOWN;
                    if (IsKeyPressed(KEY_LEFT)) player2.plannedAction = MOVE_LEFT;
                    if (IsKeyPressed(KEY_RIGHT)) player2.plannedAction = MOVE_RIGHT;
                    if (IsKeyPressed(KEY_T)) player2.plannedAction = SHOOT_UP;
                    if (IsKeyPressed(KEY_F)) player2.plannedAction = SHOOT_DOWN;
                    if (IsKeyPressed(KEY_G)) player2.plannedAction = SHOOT_LEFT;
                    if (IsKeyPressed(KEY_H)) player2.plannedAction = SHOOT_RIGHT;
                    if (IsKeyPressed(KEY_ENTER)) player2.plannedAction = WAIT;
                    
                    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
                        IsKeyPressed(KEY_T) || IsKeyPressed(KEY_F) || IsKeyPressed(KEY_G) || IsKeyPressed(KEY_H) ||
                        IsKeyPressed(KEY_ENTER)) {
                        player2.hasPlannedAction = true;
                    }
                }

                // If both players have planned their actions, start execution
                if ((player1.hasPlannedAction || !player1.isAlive) && 
                    (player2.hasPlannedAction || !player2.isAlive)) {
                    gameState = EXECUTING;
                    executionTimer = EXECUTE_STEP_TIME;
                }
                break;

            case EXECUTING:
                executionTimer -= deltaTime;
                
                if (executionTimer <= 0) {
                    // Execute player movements
                    if (player1.isAlive && !IsShootAction(player1.plannedAction)) {
                        Vector2 dir = GetDirectionFromAction(player1.plannedAction);
                        Vector2 newPos = {
                            player1.position.x + dir.x * TILE_SIZE,
                            player1.position.y + dir.y * TILE_SIZE
                        };
                        if (!CheckCollisionWithWalls(newPos)) {
                            player1.position = newPos;
                        }
                    }

                    if (player2.isAlive && !IsShootAction(player2.plannedAction)) {
                        Vector2 dir = GetDirectionFromAction(player2.plannedAction);
                        Vector2 newPos = {
                            player2.position.x + dir.x * TILE_SIZE,
                            player2.position.y + dir.y * TILE_SIZE
                        };
                        if (!CheckCollisionWithWalls(newPos)) {
                            player2.position = newPos;
                        }
                    }

                    // Create bullets for shooting actions
                    if (player1.isAlive && IsShootAction(player1.plannedAction)) {
                        for (int i = 0; i < MAX_BULLETS; i++) {
                            if (!bullets[i].active) {
                                bullets[i].position = player1.position;
                                bullets[i].direction = GetDirectionFromAction(player1.plannedAction);
                                bullets[i].active = true;
                                bullets[i].playerOwner = 1;
                                break;
                            }
                        }
                    }

                    if (player2.isAlive && IsShootAction(player2.plannedAction)) {
                        for (int i = 0; i < MAX_BULLETS; i++) {
                            if (!bullets[i].active) {
                                bullets[i].position = player2.position;
                                bullets[i].direction = GetDirectionFromAction(player2.plannedAction);
                                bullets[i].active = true;
                                bullets[i].playerOwner = 2;
                                break;
                            }
                        }
                    }

                    // Move bullets one step
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active) {
                            MoveBullet(&bullets[i]);
                        }
                    }

                    // Check bullet collisions with players
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active) {
                            Rectangle bulletRect = {
                                bullets[i].position.x,
                                bullets[i].position.y,
                                TILE_SIZE, TILE_SIZE  // Adjusted to cover the tile area
                            };

                            Rectangle player1Rect = {
                                player1.position.x,
                                player1.position.y,
                                TILE_SIZE, TILE_SIZE
                            };

                            Rectangle player2Rect = {
                                player2.position.x,
                                player2.position.y,
                                TILE_SIZE, TILE_SIZE
                            };

                            if (bullets[i].playerOwner == 2 && player1.isAlive && 
                                CheckCollisionRecs(bulletRect, player1Rect)) {
                                player1.isAlive = false;
                                player2.score++;
                                bullets[i].active = false;
                            }

                            if (bullets[i].playerOwner == 1 && player2.isAlive && 
                                CheckCollisionRecs(bulletRect, player2Rect)) {
                                player2.isAlive = false;
                                player1.score++;
                                bullets[i].active = false;
                            }
                        }
                    }

                    // Reset for next movement step
                    player1.hasPlannedAction = false;
                    player2.hasPlannedAction = false;
                    
                    // Reset execution timer
                    executionTimer = EXECUTE_STEP_TIME;

                    // Check if there are any active bullets left
                    bool anyActiveBullets = false;
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active) {
                            anyActiveBullets = true;
                            break;
                        }
                    }

                    // If no active bullets, check for game over or respawn
                    if (!anyActiveBullets) {
                        if (!player1.isAlive || !player2.isAlive) {
                            RespawnPlayer(&player1, true);
                            RespawnPlayer(&player2, false);
                        }
                        gameState = PLANNING;
                    }
                }
                break;

            case GAME_OVER:
                // Future implementation for game over state
                break;
        }

        // Drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (gameMap[y][x] == 1) {
                    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, GRAY);
                }
            }
        }

        // Draw players
        if (player1.isAlive) {
            DrawRectangle(player1.position.x, player1.position.y, TILE_SIZE, TILE_SIZE, player1.color);
        }
        
        if (player2.isAlive) {
            DrawRectangle(player2.position.x, player2.position.y, TILE_SIZE, TILE_SIZE, player2.color);
        }

        // Draw bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                Color bulletColor = bullets[i].playerOwner == 1 ? BLUE : GREEN;
                DrawRectangle(bullets[i].position.x + TILE_SIZE/4, 
                              bullets[i].position.y + TILE_SIZE/4, 
                              TILE_SIZE/2, TILE_SIZE/2, bulletColor);
            }
        }

        // Draw game state info
        if (gameState == PLANNING) {
            DrawActionPrompt(player1, 1, 10, SCREEN_HEIGHT - 60);
            DrawActionPrompt(player2, 2, 10, SCREEN_HEIGHT - 30);
        } else if (gameState == EXECUTING) {
            DrawText("Executing moves...", 10, SCREEN_HEIGHT - 30, 20, RED);
        }

        // Draw scores
        DrawText(TextFormat("Player 1: %d", player1.score), 10, 10, 20, BLUE);
        DrawText(TextFormat("Player 2: %d", player2.score), SCREEN_WIDTH - 120, 10, 20, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}