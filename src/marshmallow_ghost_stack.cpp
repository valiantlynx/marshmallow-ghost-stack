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
    std::string mode;
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

// Convert GameMode enum to string
const char* GetGameModeString(GameMode mode) {
    switch (mode) {
        case EASY: return "EASY";
        case NORMAL: return "NORMAL";
        case HARD: return "HARD";
        case TIMED: return "TIMED";
        default: return "UNKNOWN";
    }
}

// Initialize database
void InitDatabase() {
    if (sqlite3_open("leaderboard.db", &db)) {
        TraceLog(LOG_ERROR, "Can't open database: %s", sqlite3_errmsg(db));
    } else {
        // Create table if not exists
        const char *sql = "CREATE TABLE IF NOT EXISTS leaderboard (id INTEGER PRIMARY KEY, name TEXT, score INT, time FLOAT, mode TEXT);";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
}

// Insert score into the database
void InsertScore(const char *name, int score, float time, const char *mode) {
    std::stringstream ss;
    ss << "INSERT INTO leaderboard (name, score, time, mode) VALUES ('" << name << "', " << score << ", " << time << ", '" << mode << "');";
    sqlite3_exec(db, ss.str().c_str(), nullptr, nullptr, nullptr);
}

// Load leaderboard from the database filtered by game mode
void LoadLeaderboard(const char *mode) {
    leaderboard.clear();
    std::stringstream ss;
    ss << "SELECT name, score, time FROM leaderboard WHERE mode = '" << mode << "' ORDER BY score DESC LIMIT 5;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, ss.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
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
    DrawText("Leaderboard", screenWidth / 2 - 100, 100, 30, ORANGE);  // Brighter text color
    for (size_t i = 0; i < leaderboard.size(); i++) {
        DrawText(TextFormat("%d. %s - Score: %d, Time: %.1f sec", i + 1, leaderboard[i].name.c_str(), leaderboard[i].score, leaderboard[i].time),
                 screenWidth / 2 - 200, 150 + (int)i * 30, 20, ORANGE);  // Brighter text color
    }
}

// Function to load and resize a texture to specified dimensions
Texture2D LoadTextureAndResize(const char* fileName, int width, int height) {
    Image img = LoadImage(fileName);  // Load the image file
    ImageResize(&img, width, height); // Resize the image to the desired dimensions
    Texture2D texture = LoadTextureFromImage(img); // Convert Image to Texture
    UnloadImage(img);  // Unload image from memory after conversion
    return texture;
}

// Main game entry
int main() {
    // Initialization
    InitWindow(screenWidth, screenHeight, "Marshmallow Roasting Game");
    InitAudioDevice();
    SetTargetFPS(60);
    InitDatabase();
    LoadLeaderboard("EASY");  // Load leaderboard for default game mode (EASY)

    // Load and resize assets
    Texture2D background = LoadTextureAndResize("resources/textures/background.png", screenWidth, screenHeight);
    Texture2D marshmallowTextures[4] = {
        LoadTextureAndResize("resources/images/marshmallow_white.png", 64, 64),
        LoadTextureAndResize("resources/images/marshmallow_yellow.png", 64, 64),
        LoadTextureAndResize("resources/images/marshmallow_brown.png", 64, 64),
        LoadTextureAndResize("resources/images/marshmallow_black.png", 64, 64)
    };
    Texture2D platformTexture = LoadTextureAndResize("resources/images/wooden_platform.png", 128, 32);
    Texture2D bonfireTexture = LoadTextureAndResize("resources/images/bonfire.png", 128, 128);
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
                if (IsKeyPressed(KEY_ENTER)) currentScreen = TITLE;
                break;

            case TITLE:
                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = INSTRUCTIONS;
                }
                if (IsKeyPressed(KEY_L)) {
                    displayLeaderboard = true;
                }
                break;

            case INSTRUCTIONS:
                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = MODE_SELECT;
                }
                break;

            case MODE_SELECT:
                if (IsKeyPressed(KEY_ONE)) {
                    currentMode = EASY;
                    roastingSpeed = 0.8f;
                    winScore = 50;
                    currentScreen = GAMEPLAY;
                } else if (IsKeyPressed(KEY_TWO)) {
                    currentMode = NORMAL;
                    roastingSpeed = 1.0f;
                    winScore = 100;
                    currentScreen = GAMEPLAY;
                } else if (IsKeyPressed(KEY_THREE)) {
                    currentMode = HARD;
                    roastingSpeed = 1.5f;
                    winScore = 150;
                    currentScreen = GAMEPLAY;
                } else if (IsKeyPressed(KEY_FOUR)) {
                    currentMode = TIMED;
                    roastingSpeed = 1.2f;
                    winScore = 75;
                    currentScreen = GAMEPLAY;
                }
                ResetGame();
                break;

            case GAMEPLAY: {
                Vector2 mousePos = GetMousePosition();

                for (int i = 0; i < 4; i++) {
                    UpdateMarshmallow(marshmallows[i], deltaTime, marshmallowTextures, roastingSpeed);

                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsMarshmallowClicked(marshmallows[i], mousePos)) {
                        if (marshmallows[i].state == 2) {
                            score += 5;
                            PlaySound(clickSound);
                        } else if (marshmallows[i].state == 1) {
                            score += 1;
                            PlaySound(clickSound);
                        } else if (marshmallows[i].state == 3) {
                            score -= 2;
                            PlaySound(burnSound);
                        }
                        ResetMarshmallow(marshmallows[i], marshmallowTextures);
                    }
                }

                if (currentMode == TIMED) {
                    timeRemaining -= deltaTime;
                    if (timeRemaining <= 0) {
                        currentScreen = ENDING;
                    }
                }

                if (score >= winScore || (currentMode == TIMED && timeRemaining <= 0)) {
                    currentScreen = ENDING;
                }
                break;
            }

            case ENDING:
                InsertScore(playerName, score, timeRemaining, GetGameModeString(currentMode));
                LoadLeaderboard(GetGameModeString(currentMode)); // Reload leaderboard based on game mode

                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = TITLE;
                }
                break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentScreen) {
            case LOGO:
                DrawText("LOGO SCREEN", screenWidth / 2 - 100, screenHeight / 2 - 20, 40, ORANGE);
                break;

            case TITLE:
                DrawTexture(background, 0, 0, WHITE);
                DrawText("Marshmallow Roasting Game", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, ORANGE);
                DrawText("Press Enter to Continue", screenWidth / 2 - 150, screenHeight / 2 + 40, 20, ORANGE);
                DrawText("Press 'L' to View Leaderboard", screenWidth / 2 - 150, screenHeight / 2 + 80, 20, ORANGE);
                if (displayLeaderboard) {
                    DisplayLeaderboard();
                }
                break;

            case INSTRUCTIONS:
                DrawText("How to Play:", screenWidth / 2 - 100, screenHeight / 2 - 80, 30, ORANGE);
                DrawText("Click the marshmallows to roast them!", screenWidth / 2 - 200, screenHeight / 2 - 40, 20, ORANGE);
                DrawText("Score points by perfectly roasting (brown) marshmallows.", screenWidth / 2 - 200, screenHeight / 2, 20, ORANGE);
                DrawText("Avoid burning them!", screenWidth / 2 - 100, screenHeight / 2 + 40, 20, ORANGE);
                DrawText("Press Enter to select mode", screenWidth / 2 - 150, screenHeight / 2 + 80, 20, ORANGE);
                break;

            case MODE_SELECT:
                DrawText("Select Mode", screenWidth / 2 - 100, screenHeight / 2 - 100, 40, ORANGE);
                DrawText("1. Easy", screenWidth / 2 - 100, screenHeight / 2 - 60, 20, ORANGE);
                DrawText("2. Normal", screenWidth / 2 - 100, screenHeight / 2 - 20, 20, ORANGE);
                DrawText("3. Hard", screenWidth / 2 - 100, screenHeight / 2 + 20, 20, ORANGE);
                DrawText("4. Timed", screenWidth / 2 - 100, screenHeight / 2 + 60, 20, ORANGE);
                break;

            case GAMEPLAY:
                DrawTexture(background, 0, 0, WHITE);
                DrawTexture(bonfireTexture, (screenWidth / 2) - 64, screenHeight - 128, WHITE);

                for (int i = 0; i < 4; i++) {
                    DrawTexture(platformTexture, 300, 250 + (i % 2) * 150, WHITE);
                    DrawTexture(marshmallows[i].texture, marshmallows[i].position.x, marshmallows[i].position.y, WHITE);
                }

                DrawText(TextFormat("Score: %d", score), 10, 10, 20, ORANGE);
                if (currentMode == TIMED) {
                    DrawText(TextFormat("Time: %.1f", timeRemaining), screenWidth - 150, 10, 20, ORANGE);
                }
                break;

            case ENDING:
                DrawText("Congratulations!", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, ORANGE);
                DrawText(TextFormat("Your Score: %d", score), screenWidth / 2 - 100, screenHeight / 2 + 20, 30, ORANGE);
                DrawText("Press Enter to return to Title Screen", screenWidth / 2 - 250, screenHeight / 2 + 60, 20, ORANGE);

                DisplayLeaderboard();
                break;
        }

        EndDrawing();
    }

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
