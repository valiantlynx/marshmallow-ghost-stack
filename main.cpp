#include "raylib.h"

// Window dimensions
const int screenWidth = 800;
const int screenHeight = 600;

// Define the marshmallow structure
struct Marshmallow {
    Vector2 position;
    int state; // 0: white, 1: yellow, 2: brown, 3: black
    float roastTimer;
    Texture2D texture;
    Rectangle bounds;
};

// Function to reset marshmallow state
void ResetMarshmallow(Marshmallow &m, Texture2D textures[]) {
    m.state = 0;
    m.roastTimer = 0;
    m.texture = textures[0]; // Start with white marshmallow
}

// Function to update the marshmallow roasting state
void UpdateMarshmallow(Marshmallow &m, float deltaTime, Texture2D textures[]) {
    m.roastTimer += deltaTime;
    if (m.roastTimer > 6.0f) {
        m.state = 3; // Burnt
        m.texture = textures[3];
    } else if (m.roastTimer > 4.0f) {
        m.state = 2; // Brown
        m.texture = textures[2];
    } else if (m.roastTimer > 2.0f) {
        m.state = 1; // Yellow
        m.texture = textures[1];
    }
}

// Check if the marshmallow was clicked
bool IsMarshmallowClicked(Marshmallow m, Vector2 mousePos) {
    return CheckCollisionPointRec(mousePos, m.bounds);
}

int main() {
    // Initialize the game window
    InitWindow(screenWidth, screenHeight, "Marshmallow Roasting Game");
    SetTargetFPS(60);

    // Load assets
    Texture2D background = LoadTexture("background.png");
    Texture2D marshmallowTextures[4] = {
        LoadTexture("marshmallow_white.png"),
        LoadTexture("marshmallow_yellow.png"),
        LoadTexture("marshmallow_brown.png"),
        LoadTexture("marshmallow_black.png")
    };
    Texture2D platformTexture = LoadTexture("wooden_platform.png");
    Texture2D bonfireTexture = LoadTexture("bonfire.png");

    // Load sounds
    Sound clickSound = LoadSound("click-sound.mp3");
    Sound burnSound = LoadSound("burn-sound.mp3");

    // Initialize marshmallows
    Marshmallow marshmallows[4] = {
        {{150, 200}, 0, 0.0f, marshmallowTextures[0], {150, 200, 64, 64}},
        {{600, 200}, 0, 0.0f, marshmallowTextures[0], {600, 200, 64, 64}},
        {{150, 350}, 0, 0.0f, marshmallowTextures[0], {150, 350, 64, 64}},
        {{600, 350}, 0, 0.0f, marshmallowTextures[0], {600, 350, 64, 64}},
    };

    // Variables for game logic
    int score = 0;
    float deltaTime = 0.0f;

    while (!WindowShouldClose()) {
        // Calculate delta time
        deltaTime = GetFrameTime();

        // Get mouse position
        Vector2 mousePos = GetMousePosition();

        // Update marshmallow roasting
        for (int i = 0; i < 4; i++) {
            UpdateMarshmallow(marshmallows[i], deltaTime, marshmallowTextures);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsMarshmallowClicked(marshmallows[i], mousePos)) {
                if (marshmallows[i].state == 2) { // Perfectly roasted
                    score += 5;
                    PlaySound(clickSound);
                } else if (marshmallows[i].state == 1) { // Partially roasted
                    score += 1;
                    PlaySound(clickSound);
                } else if (marshmallows[i].state == 3) { // Burnt
                    score -= 2;
                    PlaySound(burnSound);
                }
                ResetMarshmallow(marshmallows[i], marshmallowTextures);
            }
        }

        // Draw everything
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw background
        DrawTexture(background, 0, 0, WHITE);

        // Draw the platforms
        for (int i = 0; i < 2; i++) {
            DrawTexture(platformTexture, 300, 250 + (i * 150), WHITE); // Two platforms between marshmallows
        }

        // Draw the bonfire
        DrawTexture(bonfireTexture, (screenWidth / 2) - 64, screenHeight - 128, WHITE);

        // Draw marshmallows
        for (int i = 0; i < 4; i++) {
            DrawTexture(marshmallows[i].texture, marshmallows[i].position.x, marshmallows[i].position.y, WHITE);
        }

        // Draw the score
        DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);

        EndDrawing();
    }

    // Unload assets and close window
    for (int i = 0; i < 4; i++) {
        UnloadTexture(marshmallowTextures[i]);
    }
    UnloadTexture(platformTexture);
    UnloadTexture(bonfireTexture);
    UnloadTexture(background);
    UnloadSound(clickSound);
    UnloadSound(burnSound);

    CloseWindow();

    return 0;
}

