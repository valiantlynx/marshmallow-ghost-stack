#include "raylib.h"

// Define game screens
typedef enum GameScreen { LOGO = 0, TITLE, GAMEPLAY, ENDING } GameScreen;

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
void UpdateMarshmallow(Marshmallow &m, float deltaTime, Texture2D textures[], float roastSpeed) {
    m.roastTimer += deltaTime * roastSpeed;
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

// Main game entry
int main() {
    // Initialization
    InitWindow(screenWidth, screenHeight, "Marshmallow Roasting Game");
    InitAudioDevice();
    SetTargetFPS(60);

    // Load assets
    Texture2D background = LoadTexture("resources/textures/background.png");
    Texture2D marshmallowTextures[4] = {
        LoadTexture("resources/images/marshmallow_white.png"),
        LoadTexture("resources/images/marshmallow_yellow.png"),
        LoadTexture("resources/images/marshmallow_brown.png"),
        LoadTexture("resources/images/marshmallow_black.png")
    };
    Texture2D platformTexture = LoadTexture("resources/images/wooden_platform.png");
    Texture2D bonfireTexture = LoadTexture("resources/images/bonfire.png");
    Sound clickSound = LoadSound("resources/audio/click-sound.mp3");
    Sound burnSound = LoadSound("resources/audio/burn-sound.mp3");

    Music backgroundMusic = LoadMusicStream("resources/audio/ritual.ogg");
    PlayMusicStream(backgroundMusic);

    // Initialize game state variables
    GameScreen currentScreen = TITLE;
    int score = 0;
    float deltaTime = 0.0f;
    float roastingSpeed = 1.0f;

    // Marshmallows initialization
    Marshmallow marshmallows[4] = {
        {{150, 200}, 0, 0.0f, marshmallowTextures[0], {150, 200, 64, 64}},
        {{600, 200}, 0, 0.0f, marshmallowTextures[0], {600, 200, 64, 64}},
        {{150, 350}, 0, 0.0f, marshmallowTextures[0], {150, 350, 64, 64}},
        {{600, 350}, 0, 0.0f, marshmallowTextures[0], {600, 350, 64, 64}},
    };

    while (!WindowShouldClose()) {
        // Update game state
        deltaTime = GetFrameTime();
        UpdateMusicStream(backgroundMusic);

        switch (currentScreen) {
            case LOGO:
                // Placeholder for logo screen logic
                if (IsKeyPressed(KEY_ENTER)) currentScreen = TITLE;
                break;

            case TITLE:
                // Placeholder for title screen logic
                if (IsKeyPressed(KEY_ENTER)) currentScreen = GAMEPLAY;
                break;

            case GAMEPLAY: {
                // Get mouse position (move this declaration inside the GAMEPLAY case)
                Vector2 mousePos = GetMousePosition();

                // Update marshmallow roasting
                for (int i = 0; i < 4; i++) {
                    UpdateMarshmallow(marshmallows[i], deltaTime, marshmallowTextures, roastingSpeed);

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

                if (score >= 50) {  // Move to ENDING screen
                    currentScreen = ENDING;
                }
                break;
            }

            case ENDING:
                // Handle ENDING screen
                if (IsKeyPressed(KEY_ENTER)) currentScreen = TITLE;
                break;
        }

        // Draw game
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentScreen) {
            case LOGO:
                DrawText("LOGO SCREEN", screenWidth / 2 - 100, screenHeight / 2 - 20, 40, LIGHTGRAY);
                break;

            case TITLE:
                DrawTexture(background, 0, 0, WHITE);
                DrawText("Marshmallow Roasting Game", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, LIGHTGRAY);
                DrawText("Press Enter to Start", screenWidth / 2 - 150, screenHeight / 2 + 40, 20, DARKGRAY);
                break;

            case GAMEPLAY:
                DrawTexture(background, 0, 0, WHITE);
                DrawTexture(bonfireTexture, (screenWidth / 2) - 64, screenHeight - 128, WHITE);

                // Draw marshmallows and platforms
                for (int i = 0; i < 4; i++) {
                    DrawTexture(platformTexture, 300, 250 + (i % 2) * 150, WHITE);
                    DrawTexture(marshmallows[i].texture, marshmallows[i].position.x, marshmallows[i].position.y, WHITE);
                }

                // Draw score
                DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
                break;

            case ENDING:
                DrawText("Congratulations!", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, LIGHTGRAY);
                DrawText("Press Enter to return to Title Screen", screenWidth / 2 - 250, screenHeight / 2 + 40, 20, DARKGRAY);
                break;
        }

        EndDrawing();
    }

    // Unload assets
    for (int i = 0; i < 4; i++) {
        UnloadTexture(marshmallowTextures[i]);
    }
    UnloadTexture(platformTexture);
    UnloadTexture(bonfireTexture);
    UnloadTexture(background);
    UnloadSound(clickSound);
    UnloadSound(burnSound);
    UnloadMusicStream(backgroundMusic);
    CloseAudioDevice();

    CloseWindow();

    return 0;
}
