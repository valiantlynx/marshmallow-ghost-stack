#include "raylib.h"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

// Define game screens and modes
typedef enum GameScreen { LOGO = 0, TITLE, INSTRUCTIONS, MODE_SELECT, GAMEPLAY, ENDING } GameScreen;
typedef enum GameMode { EASY = 0, NORMAL, HARD, TIMED } GameMode;

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

// Declare marshmallows array globally so it can be used across all functions
Marshmallow marshmallows[4];

// Leaderboard structure
struct LeaderboardEntry {
    std::string name;
    int score;
    float time;
};

// SQLite database pointer
sqlite3 *db;
std::vector<LeaderboardEntry> leaderboard;

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

// Initialize database
void InitDatabase() {
    if (sqlite3_open("leaderboard.db", &db)) {
        TraceLog(LOG_ERROR, "Can't open database: %s", sqlite3_errmsg(db));
    } else {
        // Create table if not exists
        const char *sql = "CREATE TABLE IF NOT EXISTS leaderboard (id INTEGER PRIMARY KEY, name TEXT, score INT, time FLOAT);";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
}

// Insert score into the database
void InsertScore(const char *name, int score, float time) {
    std::stringstream ss;
    ss << "INSERT INTO leaderboard (name, score, time) VALUES ('" << name << "', " << score << ", " << time << ");";
    sqlite3_exec(db, ss.str().c_str(), nullptr, nullptr, nullptr);
}

// Load leaderboard from the database
void LoadLeaderboard() {
    leaderboard.clear();
    const char *sql = "SELECT name, score, time FROM leaderboard ORDER BY score DESC LIMIT 5;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LeaderboardEntry entry;
            entry.name = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
            entry.score = sqlite3_column_int(stmt, 1);
            entry.time = static_cast<float>(sqlite3_column_double(stmt, 2));
            leaderboard.push_back(entry);
        }
    }
    sqlite3_finalize(stmt);
}

// Display leaderboard
void DisplayLeaderboard() {
    DrawText("Leaderboard", screenWidth / 2 - 100, 100, 30, DARKGRAY);
    for (size_t i = 0; i < leaderboard.size(); i++) {
        DrawText(TextFormat("%d. %s - Score: %d, Time: %.1f sec", i + 1, leaderboard[i].name.c_str(), leaderboard[i].score, leaderboard[i].time),
                 screenWidth / 2 - 200, 150 + (int)i * 30, 20, DARKGRAY);
    }
}

// Main game entry
int main() {
    // Initialization
    InitWindow(screenWidth, screenHeight, "Marshmallow Roasting Game");
    InitAudioDevice();
    SetTargetFPS(60);
    InitDatabase();
    LoadLeaderboard();

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
    GameMode currentMode = EASY;
    int score = 0;
    float deltaTime = 0.0f;
    float roastingSpeed = 1.0f;
    int winScore = 50;
    float timeRemaining = 30.0f;  // For timed mode
    char playerName[32] = "Player";
    bool displayLeaderboard = false;

    // Marshmallows initialization
    marshmallows[0] = {{150, 200}, 0, 0.0f, marshmallowTextures[0], {150, 200, 64, 64}};
    marshmallows[1] = {{600, 200}, 0, 0.0f, marshmallowTextures[0], {600, 200, 64, 64}};
    marshmallows[2] = {{150, 350}, 0, 0.0f, marshmallowTextures[0], {150, 350, 64, 64}};
    marshmallows[3] = {{600, 350}, 0, 0.0f, marshmallowTextures[0], {600, 350, 64, 64}};

    // Function to reset the game state
    auto ResetGame = [&]() {
        score = 0;
        timeRemaining = 30.0f;  // Reset time for timed mode
        // Reset all marshmallows
        for (int i = 0; i < 4; i++) {
            ResetMarshmallow(marshmallows[i], marshmallowTextures);
        }
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
                // Title screen
                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = INSTRUCTIONS;
                }
                if (IsKeyPressed(KEY_L)) {
                    displayLeaderboard = true;
                }
                break;

            case INSTRUCTIONS:
                // Show instructions and go to mode select screen
                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = MODE_SELECT;
                }
                break;

            case MODE_SELECT:
                // Mode selection logic
                if (IsKeyPressed(KEY_ONE)) {
                    currentMode = EASY;
                    roastingSpeed = 0.8f;
                    winScore = 50;
                    currentScreen = GAMEPLAY;
                }
                else if (IsKeyPressed(KEY_TWO)) {
                    currentMode = NORMAL;
                    roastingSpeed = 1.0f;
                    winScore = 100;
                    currentScreen = GAMEPLAY;
                }
                else if (IsKeyPressed(KEY_THREE)) {
                    currentMode = HARD;
                    roastingSpeed = 1.5f;
                    winScore = 150;
                    currentScreen = GAMEPLAY;
                }
                else if (IsKeyPressed(KEY_FOUR)) {
                    currentMode = TIMED;
                    roastingSpeed = 1.2f;
                    winScore = 75;
                    currentScreen = GAMEPLAY;
                }
                ResetGame();  // Reset the game each time a mode is selected
                break;

            case GAMEPLAY: {
                // Get mouse position
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

                // For timed mode, decrease the time
                if (currentMode == TIMED) {
                    timeRemaining -= deltaTime;
                    if (timeRemaining <= 0) {
                        currentScreen = ENDING;
                    }
                }

                if (score >= winScore || (currentMode == TIMED && timeRemaining <= 0)) {  // Move to ENDING screen
                    currentScreen = ENDING;
                }
                break;
            }

            case ENDING:
                // Handle ENDING screen
                InsertScore(playerName, score, timeRemaining);
                LoadLeaderboard(); // Reload leaderboard with the latest scores

                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = TITLE;  // Return to TITLE screen
                }
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
                DrawText("Press Enter to Continue", screenWidth / 2 - 150, screenHeight / 2 + 40, 20, DARKGRAY);
                DrawText("Press 'L' to View Leaderboard", screenWidth / 2 - 150, screenHeight / 2 + 80, 20, DARKGRAY);
                if (displayLeaderboard) {
                    DisplayLeaderboard();  // Display leaderboard on title screen if 'L' is pressed
                }
                break;

            case INSTRUCTIONS:
                DrawText("How to Play:", screenWidth / 2 - 100, screenHeight / 2 - 80, 30, LIGHTGRAY);
                DrawText("Click the marshmallows to roast them!", screenWidth / 2 - 200, screenHeight / 2 - 40, 20, DARKGRAY);
                DrawText("Score points by perfectly roasting (brown) marshmallows.", screenWidth / 2 - 200, screenHeight / 2, 20, DARKGRAY);
                DrawText("Avoid burning them!", screenWidth / 2 - 100, screenHeight / 2 + 40, 20, DARKGRAY);
                DrawText("Press Enter to select mode", screenWidth / 2 - 150, screenHeight / 2 + 80, 20, DARKGRAY);
                break;

            case MODE_SELECT:
                DrawText("Select Mode", screenWidth / 2 - 100, screenHeight / 2 - 100, 40, LIGHTGRAY);
                DrawText("1. Easy", screenWidth / 2 - 100, screenHeight / 2 - 60, 20, DARKGRAY);
                DrawText("2. Normal", screenWidth / 2 - 100, screenHeight / 2 - 20, 20, DARKGRAY);
                DrawText("3. Hard", screenWidth / 2 - 100, screenHeight / 2 + 20, 20, DARKGRAY);
                DrawText("4. Timed", screenWidth / 2 - 100, screenHeight / 2 + 60, 20, DARKGRAY);
                break;

            case GAMEPLAY:
                DrawTexture(background, 0, 0, WHITE);
                DrawTexture(bonfireTexture, (screenWidth / 2) - 64, screenHeight - 128, WHITE);

                // Draw marshmallows and platforms
                for (int i = 0; i < 4; i++) {
                    DrawTexture(platformTexture, 300, 250 + (i % 2) * 150, WHITE);
                    DrawTexture(marshmallows[i].texture, marshmallows[i].position.x, marshmallows[i].position.y, WHITE);
                }

                // Draw score and timer (if applicable)
                DrawText(TextFormat("Score: %d", score), 10, 10, 20, DARKGRAY);
                if (currentMode == TIMED) {
                    DrawText(TextFormat("Time: %.1f", timeRemaining), screenWidth - 150, 10, 20, DARKGRAY);
                }
                break;

            case ENDING:
                DrawText("Congratulations!", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, LIGHTGRAY);
                DrawText(TextFormat("Your Score: %d", score), screenWidth / 2 - 100, screenHeight / 2 + 20, 30, DARKGRAY);
                DrawText("Press Enter to return to Title Screen", screenWidth / 2 - 250, screenHeight / 2 + 60, 20, DARKGRAY);

                DisplayLeaderboard();  // Show leaderboard on ending screen
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

    sqlite3_close(db);  // Close database

    CloseWindow();

    return 0;
}
