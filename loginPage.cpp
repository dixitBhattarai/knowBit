#include "include/raylib.h"
#include <iostream>
#include <fstream>
#include <string>
#include <ctime> 
#include "task.h" 
using namespace std;
bool registerUser(string username, string password) {
    ofstream file("users.txt", ios::app); 
    if (!file.is_open()) return false;
    file << username << "," << password << "\n";
    file.close();
    return true; 
}
bool login(string username, string password) {
    ifstream file("users.txt");
    string line;
    while (getline(file, line)) {
        size_t comma = line.find(',');
        if (comma != string::npos) {
            string storedUser = line.substr(0, comma);
            string storedPass = line.substr(comma + 1);

            if (storedUser == username && storedPass == password) {
                file.close();
                activeUsername = username; 
                return true;
            }
        }
    }
    file.close();
    return false;
}
bool runLoginGUI() {
    const int screenWidth = 1200; const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "knowBit v1.0 - Login Page");
    SetTargetFPS(60);
    Color bgColor = { 11, 17, 26, 255 }; Color saffronOrange = { 255, 128, 0, 255 }; 
    Color terminalGreen = { 0, 255, 65, 255 }; Color tealText = { 0, 180, 180, 255 };      
    Color purpleAccent = { 150, 100, 255, 255 }; Color inputBg = { 20, 25, 35, 255 };        
    Texture2D logo = LoadTexture("assets/logo.png"); Texture2D mountain = LoadTexture("assets/mountain.png");
    Texture2D textLogo = LoadTexture("assets/knowBit.png"); Texture2D hand = LoadTexture("assets/hi.png");
    Texture2D userIcon = LoadTexture("assets/user.png"); Texture2D lockIcon = LoadTexture("assets/password.png");
    Texture2D eyeIcon = LoadTexture("assets/hidepw.png"); Texture2D clockTower = LoadTexture("assets/clockTower.png");
    const int MAX_INPUT_CHARS = 20;
    char usernameInput[MAX_INPUT_CHARS + 1] = "\0"; char passwordInput[MAX_INPUT_CHARS + 1] = "\0";
    int userLetterCount = 0; int passLetterCount = 0;
    bool mouseOnUser = false; bool mouseOnPass = false; bool isLoggedIn = false;
    string loginMessage = ""; Color messageColor = WHITE;
    Rectangle formBox = { 580, 100, 550, 650 }; Rectangle userBox = { 630, 260, 450, 45 };
    Rectangle passBox = { 630, 360, 450, 45 }; Rectangle loginBtn = { 630, 490, 210, 45 };
    Rectangle guestBtn = { 870, 490, 210, 45 }; Rectangle createBtn = { 630, 590, 450, 45 };
    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();
        if (!isLoggedIn) {
            if (CheckCollisionPointRec(mousePoint, userBox)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mouseOnUser = true; mouseOnPass = false; }
            } else if (CheckCollisionPointRec(mousePoint, passBox)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mouseOnUser = false; mouseOnPass = true; }
            } else {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mouseOnUser = false; mouseOnPass = false; }
            }
            if (mouseOnUser) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (userLetterCount < MAX_INPUT_CHARS)) {
                        usernameInput[userLetterCount] = (char)key; usernameInput[userLetterCount + 1] = '\0'; userLetterCount++;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && userLetterCount > 0) { userLetterCount--; usernameInput[userLetterCount] = '\0'; }
            }
            if (mouseOnPass) {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (passLetterCount < MAX_INPUT_CHARS)) {
                        passwordInput[passLetterCount] = (char)key; passwordInput[passLetterCount + 1] = '\0'; passLetterCount++;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && passLetterCount > 0) { passLetterCount--; passwordInput[passLetterCount] = '\0'; }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePoint, loginBtn)) {
                    if (login(string(usernameInput), string(passwordInput))) {
                        CloseWindow(); return true;
                    } else { loginMessage = "Error: Invalid username or password."; messageColor = RED; }
                }
                if (CheckCollisionPointRec(mousePoint, createBtn)) {
                    if (registerUser(string(usernameInput), string(passwordInput))) {
                        loginMessage = "Account created! Please log in."; messageColor = terminalGreen;
                    } else { loginMessage = "Error: Could not save to database."; messageColor = RED; }
                }
                if (CheckCollisionPointRec(mousePoint, guestBtn)) {
                    int g = 1;
                    while(true) {
                        ifstream f("guest" + to_string(g) + ".txt");
                        if (!f.is_open()) break; 
                        g++;
                    }
                    string guestName = "guest" + to_string(g);
                    registerUser(guestName, "");
                    activeUsername = guestName;  
                    CloseWindow();
                    return true;
                }
            }
        }
        BeginDrawing(); ClearBackground(bgColor);
        if (!isLoggedIn) {
            time_t now = time(0); tm *ltm = localtime(&now); char timeStr[50];
            strftime(timeStr, sizeof(timeStr), "Time: %I:%M:%S %p", ltm);
            DrawText(timeStr, 950, 45, 20, tealText);
            DrawTextureEx(logo, (Vector2){ 25, 20 }, 0.0f, 0.12f, WHITE); 
            int knowWidth = MeasureText("know", 30);
            DrawText("know", 105, 35, 30, WHITE); DrawText("Bit", 105 + knowWidth + 5, 35, 30, saffronOrange); 
            DrawText("v1.0", 105 + knowWidth + 5 + MeasureText("Bit", 30) + 10, 45, 15, LIGHTGRAY);
            Rectangle mntSource = { 0, 0, (float)mountain.width, (float)mountain.height };
            Rectangle mntDest   = { 15, 95, 555, 360 }; DrawTexturePro(mountain, mntSource, mntDest, (Vector2){0, 0}, 0.0f, WHITE);
            DrawTextureEx(textLogo, (Vector2){ 40, 475 }, 0.0f, 0.42f, WHITE); DrawText("Plan. Prioritize. Get It Done.", 50, 590, 30, WHITE); 
            DrawRectangleRoundedLinesEx(formBox, 0.05f, 10, 1.5f, saffronOrange); DrawText("Welcome Back!", 750, 130, 30, saffronOrange);
            DrawTextureEx(hand, (Vector2){ 970, 125 }, 0.0f, 0.15f, WHITE); DrawText("Log in to Continue Your Journey", 650, 180, 18, tealText);
            DrawText("Username", 630, 230, 18, tealText); DrawRectangleRounded(userBox, 0.2f, 10, inputBg);
            DrawRectangleRoundedLinesEx(userBox, 0.2f, 10, 1.0f, mouseOnUser ? saffronOrange : DARKGRAY);
            DrawTextureEx(userIcon, (Vector2){ 645, 271 }, 0.0f, 0.12f, WHITE); DrawText(usernameInput, 690, 272, 20, WHITE);
            DrawTextureEx(eyeIcon, (Vector2){ 1045, 272 }, 0.0f, 0.04f, WHITE);
            if (mouseOnUser && ((int)(GetTime() * 2) % 2 == 0)) DrawText("_", 690 + MeasureText(usernameInput, 20), 272, 20, saffronOrange);
            DrawText("Password", 630, 330, 18, tealText); DrawRectangleRounded(passBox, 0.2f, 10, inputBg);
            DrawRectangleRoundedLinesEx(passBox, 0.2f, 10, 1.0f, mouseOnPass ? saffronOrange : DARKGRAY);
            DrawTextureEx(lockIcon, (Vector2){ 640, 371 }, 0.0f, 0.06f, WHITE);
            string hiddenPass(passLetterCount, '*'); DrawText(hiddenPass.c_str(), 690, 375, 20, WHITE);
            if (mouseOnPass && ((int)(GetTime() * 2) % 2 == 0)) DrawText("_", 690 + MeasureText(hiddenPass.c_str(), 20), 375, 20, saffronOrange);
            DrawText("  Remember Me", 630, 430, 15, WHITE); DrawText("Forgot Password? ", 920, 430, 15, saffronOrange);
            DrawText(loginMessage.c_str(), 630, 465, 15, messageColor);
            DrawRectangleRoundedLinesEx(loginBtn, 0.2f, 10, 1.5f, CheckCollisionPointRec(mousePoint, loginBtn) ? terminalGreen : DARKGRAY);
            DrawText("  Log in", 670, 502, 20, terminalGreen);
            DrawRectangleRoundedLinesEx(guestBtn, 0.2f, 10, 1.5f, tealText); DrawText(" Guest Mode", 900, 502, 20, tealText);
            DrawText("New Here? Create Your Account", 730, 560, 15, tealText);
            DrawRectangleRoundedLinesEx(createBtn, 0.2f, 10, 1.5f, CheckCollisionPointRec(mousePoint, createBtn) ? purpleAccent : DARKGRAY);
            DrawText(" Create Account", 760, 602, 20, purpleAccent);
            DrawTextureEx(clockTower, (Vector2){ 590, 654 }, 0.0f, 0.45f, WHITE);
        }
        EndDrawing();
    }
    UnloadTexture(logo); UnloadTexture(mountain); UnloadTexture(textLogo); UnloadTexture(hand);
    UnloadTexture(userIcon); UnloadTexture(lockIcon); UnloadTexture(eyeIcon); UnloadTexture(clockTower);
    CloseWindow();
    return false;
}