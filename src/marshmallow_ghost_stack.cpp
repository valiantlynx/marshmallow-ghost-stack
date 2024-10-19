#include "raylib.h"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <sstream>

// Define game screens and modes
typedef enum GameScreen { LOGO = 0, TITLE, INSTRUCTIONS, MODE_SELECT, NAME_INPUT, GAMEPLAY, ENDING, LEADERBOARD_SELECTION } GameScreen;
typedef enum GameMode { EASY = 0, NORMAL, HARD, TIMED } GameMode;
//#define PLATFORM_WEB

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
const std::string BASE_PATH = "../../"; // this is the path ehere you will run the executable form

const int screenWidth = 800;
const int screenHeight = 600;

Music backgroundMusic;
Texture2D background, platformTexture, bonfireTexture;
Sound clickSound, burnSound;
float deltaTime = 0.0f;
float roastingSpeed = 1.0f;
float timeRemaining = 30.0f;
int winScore = 50;
int score = 0;
int letterCount = 0;
bool displayLeaderboard = false;
char playerName[32] = "";
GameScreen currentScreen = TITLE;
GameMode currentMode = EASY;

// Declare the marshmallow texture array globally
Texture2D marshmallowTextures[4];

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
GameMode currentLeaderboardMode = EASY; // Track the current mode displayed on the leaderboard

//----------------------------------------------------------------------------------
// Module functions declaration
//----------------------------------------------------------------------------------
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

// Initialize the database
void InitDatabase() {
    if (sqlite3_open((BASE_PATH + "leaderboard.db").c_str(), &db)) {
        TraceLog(LOG_ERROR, "Can't open database: %s", sqlite3_errmsg(db));
    } else {
        const char *sql = "CREATE TABLE IF NOT EXISTS leaderboard (id INTEGER PRIMARY KEY, name TEXT, score INT, time FLOAT, mode TEXT);";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
}

// Insert or update scores in the database
void InsertScore(const char *name, int score, float time, const char *mode) {
    std::stringstream checkQuery;
    checkQuery << "SELECT score FROM leaderboard WHERE name = '" << name << "' AND mode = '" << mode << "';";
    
    sqlite3_stmt *stmt;
    int existingScore = -1;

    if (sqlite3_prepare_v2(db, checkQuery.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            existingScore = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    if (existingScore == -1) {
        std::stringstream insertQuery;
        insertQuery << "INSERT INTO leaderboard (name, score, time, mode) VALUES ('" 
                    << name << "', " << score << ", " << time << ", '" << mode << "');";
        sqlite3_exec(db, insertQuery.str().c_str(), nullptr, nullptr, nullptr);
    } else if (score > existingScore) {
        std::stringstream updateQuery;
        updateQuery << "UPDATE leaderboard SET score = " << score << ", time = " << time
                    << " WHERE name = '" << name << "' AND mode = '" << mode << "';";
        sqlite3_exec(db, updateQuery.str().c_str(), nullptr, nullptr, nullptr);
    }
}

// Load leaderboard from the database based on mode
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
    if (leaderboard.empty()) {
        DrawText("No leaderboard data yet.", screenWidth / 2 - 150, 200, 30, WHITE);
    } else {
        DrawText("Leaderboard", screenWidth / 2 - 100, 100, 30, WHITE);
        DrawText(TextFormat("Current Mode: %s", GetGameModeString(currentLeaderboardMode)), screenWidth / 2 - 150, 140, 20, WHITE);
        for (size_t i = 0; i < leaderboard.size(); i++) {
            DrawText(TextFormat("%d. %s - Score: %d, Time: %.1f sec", i + 1, leaderboard[i].name.c_str(), leaderboard[i].score, leaderboard[i].time),
                     screenWidth / 2 - 200, 180 + (int)i * 30, 20, WHITE);
        }
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

// Background scrolling variables for parallax effect
struct ParallaxLayer {
    Texture2D texture;
    float scrollingOffset;
    float speed;
};

ParallaxLayer parallaxLayers[5];

// Function to update a specific parallax layer if it has a non-zero speed
void UpdateParallaxLayer(ParallaxLayer &layer, float deltaTime) {
    if (layer.speed != 0.0f) {
        layer.scrollingOffset -= layer.speed * deltaTime;
        if (layer.scrollingOffset <= -layer.texture.width) {
            layer.scrollingOffset = 0;
        }
    }
}

// Function to initialize parallax layers
void InitializeParallaxLayers() {
    parallaxLayers[0] = { LoadTextureAndResize((BASE_PATH + "resources/textures/parallax/background 2/Plan-5.png").c_str(), screenWidth, screenHeight), 0.0f, 50.0f }; // Static background
    parallaxLayers[1] = { LoadTextureAndResize((BASE_PATH + "resources/textures/parallax/background 2/Plan-4.png").c_str(), screenWidth, screenHeight), 0.0f, 1.0f }; // Slow moving clouds
    parallaxLayers[2] = { LoadTextureAndResize((BASE_PATH + "resources/textures/parallax/background 2/Plan-3.png").c_str(), screenWidth, screenHeight), 0.0f, 0.0f }; // Static mountains
    parallaxLayers[3] = { LoadTextureAndResize((BASE_PATH + "resources/textures/parallax/background 2/Plan-2.png").c_str(), screenWidth, screenHeight), 0.0f, 0.0f }; // Moving trees
    parallaxLayers[4] = { LoadTextureAndResize((BASE_PATH + "resources/textures/parallax/background 2/Plan-1.png").c_str(), screenWidth, screenHeight), 0.0f, 25.0f }; // Fast moving foreground
}

// Function to draw the parallax layers
void DrawParallaxLayers() {
    for (int i = 0; i < 5; i++) {
        DrawTextureEx(parallaxLayers[i].texture, (Vector2){ parallaxLayers[i].scrollingOffset, 0 }, 0.0f, 1.0f, WHITE);
        DrawTextureEx(parallaxLayers[i].texture, (Vector2){ parallaxLayers[i].scrollingOffset + parallaxLayers[i].texture.width, 0 }, 0.0f, 1.0f, WHITE);
    }
}
// Function to reset the game
void ResetGame() {
    score = 0;
    timeRemaining = 30.0f;
    for (int i = 0; i < 4; i++) {
        ResetMarshmallow(marshmallows[i], marshmallowTextures);
    }
}

void UpdateDrawFrame(void);     // Update and Draw one frame


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    InitWindow(screenWidth, screenHeight, "Marshmallow Roasting Game with Parallax Background");
    InitAudioDevice();
    SetTargetFPS(60);
    InitDatabase();
    LoadLeaderboard("EASY");  // Load leaderboard for default game mode (EASY)

    // Load and resize assets
    background = LoadTextureAndResize((BASE_PATH + "resources/textures/background.png").c_str(), screenWidth, screenHeight);
    marshmallowTextures[0] = LoadTextureAndResize((BASE_PATH + "resources/images/marshmallow_white.png").c_str(), 64, 64);
    marshmallowTextures[1] = LoadTextureAndResize((BASE_PATH + "resources/images/marshmallow_yellow.png").c_str(), 64, 64);
    marshmallowTextures[2] = LoadTextureAndResize((BASE_PATH + "resources/images/marshmallow_brown.png").c_str(), 64, 64);
    marshmallowTextures[3] = LoadTextureAndResize((BASE_PATH + "resources/images/marshmallow_black.png").c_str(), 64, 64);

    platformTexture = LoadTextureAndResize((BASE_PATH + "resources/images/wooden_platform.png").c_str(), 600, 32); 
    bonfireTexture = LoadTextureAndResize((BASE_PATH + "resources/images/bonfire.png").c_str(), 128, 128);
    
    // Load sound/music once and unload at the end
    clickSound = LoadSound((BASE_PATH + "resources/audio/click-sound.mp3").c_str());
    burnSound = LoadSound((BASE_PATH + "resources/audio/burn-sound.mp3").c_str());
    backgroundMusic = LoadMusicStream((BASE_PATH + "resources/audio/ritual.ogg").c_str());
    PlayMusicStream(backgroundMusic);

    // Initialize parallax layers
    InitializeParallaxLayers();


    marshmallows[0] = {{150, 200}, 0, 0.0f, marshmallowTextures[0], {150, 200, 64, 64}};
    marshmallows[1] = {{600, 200}, 0, 0.0f, marshmallowTextures[0], {600, 200, 64, 64}};
    marshmallows[2] = {{150, 350}, 0, 0.0f, marshmallowTextures[0], {150, 350, 64, 64}};
    marshmallows[3] = {{600, 350}, 0, 0.0f, marshmallowTextures[0], {600, 350, 64, 64}};


#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif
     // Free resources
    for (int i = 0; i < 4; i++) {
        UnloadTexture(marshmallowTextures[i]);
    }
    UnloadTexture(platformTexture);
    UnloadTexture(bonfireTexture);

    // Unload parallax layers
    for (int i = 0; i < 5; i++) {
        UnloadTexture(parallaxLayers[i].texture);
    }

    // Unload sounds and music
    UnloadSound(clickSound);
    UnloadSound(burnSound);
    UnloadMusicStream(backgroundMusic);

    sqlite3_close(db);  // Close database
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    deltaTime = GetFrameTime();
    UpdateMusicStream(backgroundMusic); // Update the music stream properly

    // Update parallax backgrounds
    for (int i = 0; i < 5; i++) {
        UpdateParallaxLayer(parallaxLayers[i], deltaTime);
    }

    switch (currentScreen) {
            case LOGO:
                if (IsKeyPressed(KEY_ENTER)) currentScreen = TITLE;
                break;

            case TITLE:
                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = NAME_INPUT;
                }
                if (IsKeyPressed(KEY_L)) {
                    currentScreen = LEADERBOARD_SELECTION;  // Go to leaderboard mode selection screen
                }
                break;

            case LEADERBOARD_SELECTION:
                DrawText("Select Leaderboard Mode", screenWidth / 2 - 150, screenHeight / 2 - 100, 30, WHITE);
                DrawText("1. Easy", screenWidth / 2 - 100, screenHeight / 2 - 60, 20, WHITE);
                DrawText("2. Normal", screenWidth / 2 - 100, screenHeight / 2 - 20, 20, WHITE);
                DrawText("3. Hard", screenWidth / 2 - 100, screenHeight / 2 + 20, 20, WHITE);
                DrawText("4. Timed", screenWidth / 2 - 100, screenHeight / 2 + 60, 20, WHITE);

                if (IsKeyPressed(KEY_ONE)) {
                    currentLeaderboardMode = EASY;
                    LoadLeaderboard(GetGameModeString(currentLeaderboardMode));
                    displayLeaderboard = true;  // Set the leaderboard to display
                    currentScreen = TITLE;  // Return to title after selecting
                } else if (IsKeyPressed(KEY_TWO)) {
                    currentLeaderboardMode = NORMAL;
                    LoadLeaderboard(GetGameModeString(currentLeaderboardMode));
                    displayLeaderboard = true;  // Set the leaderboard to display
                    currentScreen = TITLE;
                } else if (IsKeyPressed(KEY_THREE)) {
                    currentLeaderboardMode = HARD;
                    LoadLeaderboard(GetGameModeString(currentLeaderboardMode));
                    displayLeaderboard = true;  // Set the leaderboard to display
                    currentScreen = TITLE;
                } else if (IsKeyPressed(KEY_FOUR)) {
                    currentLeaderboardMode = TIMED;
                    LoadLeaderboard(GetGameModeString(currentLeaderboardMode));
                    displayLeaderboard = true;  // Set the leaderboard to display
                    currentScreen = TITLE;
                }
                break;

            case MODE_SELECT:
                DrawText("Select Game Mode", screenWidth / 2 - 100, screenHeight / 2 - 100, 30, WHITE);
                DrawText("1. Easy", screenWidth / 2 - 100, screenHeight / 2 - 60, 20, WHITE);
                DrawText("2. Normal", screenWidth / 2 - 100, screenHeight / 2 - 20, 20, WHITE);
                DrawText("3. Hard", screenWidth / 2 - 100, screenHeight / 2 + 20, 20, WHITE);
                DrawText("4. Timed", screenWidth / 2 - 100, screenHeight / 2 + 60, 20, WHITE);
                
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
                ResetGame();  // Reset game state
                break;

            case INSTRUCTIONS:
                DrawText("Instructions: Roast marshmallows to score points.", screenWidth / 2 - 200, screenHeight / 2 - 50, 20, WHITE);
                DrawText("Press Enter to return to the Title screen.", screenWidth / 2 - 200, screenHeight / 2, 20, WHITE);
                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = TITLE;
                }
                break;

            case NAME_INPUT: {
                if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
                    playerName[--letterCount] = '\0';
                }

                int key = GetKeyPressed();
                if (key >= 32 && key <= 125 && letterCount < 31) {
                    playerName[letterCount++] = (char)key;
                    playerName[letterCount] = '\0';
                }

                if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                    currentScreen = MODE_SELECT;
                }

                DrawText("Enter your name:", screenWidth / 2 - 150, screenHeight / 2 - 50, 20, DARKGRAY);
                DrawText(playerName, screenWidth / 2 - 150, screenHeight / 2, 30, WHITE);
                break;
            }

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
                LoadLeaderboard(GetGameModeString(currentMode));  // Reload leaderboard based on game mode

                if (IsKeyPressed(KEY_ENTER)) {
                    currentScreen = TITLE;
                }
                break;
        }

    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
        ClearBackground(RAYWHITE);

         // Draw the parallax background layers
        DrawParallaxLayers();

        switch (currentScreen) {
            case LOGO:
                DrawText("LOGO SCREEN", screenWidth / 2 - 100, screenHeight / 2 - 20, 40, WHITE);
                break;

            case TITLE:
                DrawTexture(background, 0, 0, WHITE);
                DrawText("Marshmallow Roasting Game", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, WHITE);
                DrawText("Press Enter to Continue", screenWidth / 2 - 150, screenHeight / 2 + 40, 20, WHITE);
                DrawText("Press 'L' to View Leaderboard", screenWidth / 2 - 150, screenHeight / 2 + 80, 20, WHITE);
                if (displayLeaderboard) {
                    DisplayLeaderboard();
                }
                break;

            case INSTRUCTIONS:
                DrawText("Instructions: Roast marshmallows to score points.", screenWidth / 2 - 200, screenHeight / 2 - 50, 20, WHITE);
                DrawText("Press Enter to return to the Title screen.", screenWidth / 2 - 200, screenHeight / 2, 20, WHITE);
                break;
            

            case NAME_INPUT: {
                DrawText("Enter your name:", screenWidth / 2 - 150, screenHeight / 2 - 50, 20, DARKGRAY);
                DrawText(playerName, screenWidth / 2 - 150, screenHeight / 2, 30, WHITE);
                break;
            }

            case LEADERBOARD_SELECTION:
                DrawText("Select Leaderboard Mode", screenWidth / 2 - 150, screenHeight / 2 - 100, 30, WHITE);
                DrawText("1. Easy", screenWidth / 2 - 100, screenHeight / 2 - 60, 20, WHITE);
                DrawText("2. Normal", screenWidth / 2 - 100, screenHeight / 2 - 20, 20, WHITE);
                DrawText("3. Hard", screenWidth / 2 - 100, screenHeight / 2 + 20, 20, WHITE);
                DrawText("4. Timed", screenWidth / 2 - 100, screenHeight / 2 + 60, 20, WHITE);
                break;

            case MODE_SELECT:
                DrawText("Select Game Mode", screenWidth / 2 - 100, screenHeight / 2 - 100, 30, WHITE);
                DrawText("1. Easy", screenWidth / 2 - 100, screenHeight / 2 - 60, 20, WHITE);
                DrawText("2. Normal", screenWidth / 2 - 100, screenHeight / 2 - 20, 20, WHITE);
                DrawText("3. Hard", screenWidth / 2 - 100, screenHeight / 2 + 20, 20, WHITE);
                DrawText("4. Timed", screenWidth / 2 - 100, screenHeight / 2 + 60, 20, WHITE);
                break;

            case GAMEPLAY:
                DrawTexture(background, 0, 0, WHITE);
                DrawTexture(bonfireTexture, (screenWidth / 2) - 64, screenHeight - 128, WHITE);

                for (int i = 0; i < 4; i++) {
                    DrawTexture(platformTexture, 120, 250 + (i % 2) * 150, WHITE);
                    DrawTexture(marshmallows[i].texture, marshmallows[i].position.x, marshmallows[i].position.y, WHITE);
                }

                DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
                if (currentMode == TIMED) {
                    DrawText(TextFormat("Time: %.1f", timeRemaining), screenWidth - 150, 10, 20, WHITE);
                }
                break;

            case ENDING:
                DrawText("Congratulations!", screenWidth / 2 - 200, screenHeight / 2 - 20, 40, WHITE);
                DrawText(TextFormat("Your Score: %d", score), screenWidth / 2 - 100, screenHeight / 2 + 20, 30, WHITE);
                DrawText("Press Enter to return to Title Screen", screenWidth / 2 - 250, screenHeight / 2 + 60, 20, WHITE);

                DisplayLeaderboard();  // Show leaderboard on ending screen
                break;
        }

    EndDrawing();
}