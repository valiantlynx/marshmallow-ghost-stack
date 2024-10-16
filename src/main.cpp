#include "raylib.h"
#include "screens.h"

int main() {
    InitWindow(800, 600, "Marshmallow Roasting Game");

    // Initialize screens
    InitLogoScreen();
    
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        switch (currentScreen) {
            case LOGO:
                UpdateLogoScreen();
                DrawLogoScreen();
                if (FinishLogoScreen()) InitTitleScreen();  // Move to next screen
                break;
            case TITLE:
                UpdateTitleScreen();
                DrawTitleScreen();
                if (FinishTitleScreen()) InitGameplayScreen();  // Start the game
                break;
            case GAMEPLAY:
                UpdateGameplayScreen();
                DrawGameplayScreen();
                break;
        }
    }

    // Unload resources and close window
    CloseWindow();
    return 0;
}
