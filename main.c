#include <stdio.h>
#include "raylib.h"
#include "sock.h"

#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define PLAYER_SIZE 20
#define PUMPKIN_SIZE 15
#define PLAYER_SPEED 5

typedef struct {
    int x, y;
    int pumpkins;
} Player;

typedef struct {
    int x, y;
    bool collected;
} Pumpkin;

// Render game objects
void render_player(Player player) {
    DrawRectangle(player.x, player.y, PLAYER_SIZE, PLAYER_SIZE, BLUE);
}

void render_pumpkin(Pumpkin pumpkin) {
    if (!pumpkin.collected) {
        DrawCircle(pumpkin.x, pumpkin.y, PUMPKIN_SIZE, ORANGE);
    }
}

int main() {
    InitWindow(800, 600, "Pumpkin Pursuit");
    SetTargetFPS(60);

    // Setup client connection
    Connection client = setup_client(SERVER_IP, PORT);

    Player player = { 400, 300, 0 };
    Pumpkin pumpkins[10];
    for (int i = 0; i < 10; i++) {
        pumpkins[i].x = GetRandomValue(50, 750);
        pumpkins[i].y = GetRandomValue(50, 550);
        pumpkins[i].collected = false;
    }

    while (!WindowShouldClose()) {
        // Send player movements to the server
        if (IsKeyDown(KEY_RIGHT)) { player.x += PLAYER_SPEED; send_data(&client, "MOVE_RIGHT", 10); }
        if (IsKeyDown(KEY_LEFT)) { player.x -= PLAYER_SPEED; send_data(&client, "MOVE_LEFT", 9); }
        if (IsKeyDown(KEY_UP)) { player.y -= PLAYER_SPEED; send_data(&client, "MOVE_UP", 7); }
        if (IsKeyDown(KEY_DOWN)) { player.y += PLAYER_SPEED; send_data(&client, "MOVE_DOWN", 9); }

        // Clear background and draw
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Render player and pumpkins
        render_player(player);
        for (int i = 0; i < 10; i++) {
            render_pumpkin(pumpkins[i]);
        }

        // Display pumpkin count
        DrawText(TextFormat("Pumpkins Collected: %d", player.pumpkins), 10, 10, 20, RAYWHITE);

        EndDrawing();
    }

    close(client.sockfd);
    CloseWindow();

    return 0;
}
