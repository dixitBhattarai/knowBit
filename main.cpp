#include <iostream>
#include "task.h"
#include "dashboard.h"
#include "include/raylib.h"

bool runLoginGUI();   
Font customFont;
Texture2D texLogo, texStreak, texClock, texMusic, texCalendar, texEdit, texDelete;

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "knowBit v1.0");
    SetTargetFPS(60);
    texLogo = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/logo.png");
    texStreak = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/streak.png");
    texClock = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/clock.png");
    texMusic = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/music.png");
    texCalendar = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/calendar.png");
    texEdit = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/edit.png");
    texDelete = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/delete.png");
    bool loginSuccess = runLoginGUI();
    if (loginSuccess) {
        loadUserData();
        Dashboard();   
    } else {
        std::cout << "\n[knowBit]: Login cancelled. Goodbye.\n";
    }
    UnloadFont(customFont);
    UnloadTexture(texLogo);
    UnloadTexture(texStreak);
    UnloadTexture(texClock);
    UnloadTexture(texMusic);
    UnloadTexture(texCalendar);
    UnloadTexture(texEdit);
    UnloadTexture(texDelete);
    CloseWindow();
    
    return 0;
}