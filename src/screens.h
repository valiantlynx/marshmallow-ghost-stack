#ifndef SCREENS_H
#define SCREENS_H

enum GameScreen { LOGO, TITLE, GAMEPLAY };
extern GameScreen currentScreen;

// Logo Screen
void InitLogoScreen();
void UpdateLogoScreen();
void DrawLogoScreen();
void UnloadLogoScreen();
int FinishLogoScreen();

// Title Screen
void InitTitleScreen();
void UpdateTitleScreen();
void DrawTitleScreen();
void UnloadTitleScreen();
int FinishTitleScreen();

// Gameplay Screen
void InitGameplayScreen();
void UpdateGameplayScreen();
void DrawGameplayScreen();
void UnloadGameplayScreen();
int FinishGameplayScreen();

#endif
