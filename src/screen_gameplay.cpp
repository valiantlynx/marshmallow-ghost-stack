#include "raylib.h"
#include "screens.h"

// Define the marshmallow structure
struct Marshmallow {
    Vector2 position;
    int state;
    float roastTimer;
    Texture2D texture;
    Rectangle bounds;
};

// Marshmallows data
Marshmallow marshmallows[4];
Texture2D marshmallowTextures[4];

int score = 0;

void InitGameplayScreen() {
    // Load textures and initialize marshmallows here
    for (int i = 0; i < 4; i++) {
        marshmallows[i].state = 0;
        marshmallows[i].roastTimer = 0.0f;
        marshmallows[i].texture = marshmallowTextures[0];
    }
}

void UpdateGameplayScreen() {
    // Update marshmallow states based on roast timers, check for clicks, etc.
}

void DrawGameplayScreen() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Draw marshmallows and UI
    DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);

    EndDrawing();
}

void UnloadGameplayScreen() {
    // Unload textures and resources
}

int FinishGameplayScreen() {
    // Return game finish condition
    return 0;
}
