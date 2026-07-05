#include <iostream>
#include "task.h"
#include "dashboard.h"
#include <raylib.h>

bool runLoginGUI();   
Font customFont;
Texture2D texLogo, texStreak, texClock, texMusic, texCalendar, texEdit, texDelete;

int main() {
    // Put this line right BEFORE InitWindow() in your main file
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);

    InitWindow(1280, 800, "knowBit v1.0");

// After loading your font, add this to smooth the text:
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);
    texLogo = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/logo.png");
    texStreak = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/streak.png");
    texClock = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/clock.png");
    texMusic = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/music.png");
    texCalendar = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/calendar.png");
    texEdit = LoadTexture("/home/dixit/Documents/knowBit_Project/assets/edit.png");
        if (texEdit.id == 0) {
    printf("\n\n!!! ERROR: RAYLIB CANNOT FIND THE EDIT IMAGE !!!\n\n");
} else {
    printf("\n\n>>> SUCCESS: EDIT IMAGE LOADED PERFECTLY <<<\n\n");
}
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