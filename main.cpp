#include <iostream>
#include "task.h"
#include "dashboard.h"
#include <raylib.h>

bool runLoginGUI();   
Font customFont;
Texture2D texLogo, texStreak, texClock, texMusic, texCalendar, texEdit, texDelete;
Font robotoRegular;
Font robotoBold;

int main() {
    // Put this line right BEFORE InitWindow() in your main file
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);

    InitWindow(1280, 800, "knowBit v1.0");
    robotoRegular = LoadFontEx("assets/Roboto-Regular.ttf", 20, NULL, 0);
    robotoBold = LoadFontEx("assets/Roboto-Bold.ttf", 48, NULL, 0); 
    SetTextureFilter(robotoRegular.texture, TEXTURE_FILTER_BILINEAR);
     SetTextureFilter(robotoBold.texture, TEXTURE_FILTER_BILINEAR);

// After loading your font, add this to smooth the text:
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);
 texLogo = LoadTexture("assets/logo.png");
    texStreak = LoadTexture("assets/streak.png");
    texClock = LoadTexture("assets/clock.png");
    texMusic = LoadTexture("assets/music.png");
    texCalendar = LoadTexture("assets/calendar.png");
    texEdit = LoadTexture("assets/edit.png");
        if (texEdit.id == 0) {
    printf("\n\n!!! ERROR: RAYLIB CANNOT FIND THE EDIT IMAGE !!!\n\n");
} else {
    printf("\n\n>>> SUCCESS: EDIT IMAGE LOADED PERFECTLY <<<\n\n");
}
    texDelete = LoadTexture("assets/delete.png");  
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