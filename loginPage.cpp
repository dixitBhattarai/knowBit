#include <raylib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdio> // Needed for remove() to delete remember.txt
#include "task.h" 
#define DrawText(text, x, y, size, color) DrawTextEx(robotoRegular, text, {(float)(x), (float)(y)}, (float)(size), 1.0f, color)

using namespace std;
extern Font robotoRegular;
 extern Font robotoBold;

// raylib provides a built-in IsKeyPressedRepeat(int key) for OS-style key
// repeat, but it only tracks one key globally — not enough here since
// Username and Password each need their own independent hold-timer for
// Backspace. This wraps the same repeat logic per fieldId (0 = username,
// 1 = password) so holding backspace in one field doesn't affect the other.
static bool IsFieldKeyPressedRepeat(int key, int fieldId) {
    const double INITIAL_DELAY = 0.35;
    const double REPEAT_RATE   = 0.035;

    static int    heldKey[2]   = { -1, -1 };
    static double heldSince[2] = { 0.0, 0.0 };
    static double lastFire[2]  = { 0.0, 0.0 };

    if (IsKeyPressed(key)) {
        heldKey[fieldId] = key;
        heldSince[fieldId] = GetTime();
        lastFire[fieldId] = heldSince[fieldId];
        return true;
    }
    if (IsKeyDown(key) && heldKey[fieldId] == key) {
        double now = GetTime();
        if (now - heldSince[fieldId] >= INITIAL_DELAY && now - lastFire[fieldId] >= REPEAT_RATE) {
            lastFire[fieldId] = now;
            return true;
        }
        return false;
    }
    if (heldKey[fieldId] == key && !IsKeyDown(key)) heldKey[fieldId] = -1;
    return false;
}

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

// Global helper to call when the user clicks Logout in the dashboard
void logoutUser() {
    remove("remember.txt"); // Deletes the auto-login save file
    activeUsername = "";
}

bool runLoginGUI() {
    // ── Auto-Login Check (Remember Me) ──
    ifstream remFile("remember.txt");
    if (remFile.is_open()) {
        string remUser, remPass;
        if (getline(remFile, remUser) && getline(remFile, remPass)) {
            if (login(remUser, remPass)) {
                remFile.close();
                return true; // Skips GUI and logs in instantly
            }
        }
        remFile.close();
    }

    const int screenWidth = 1200; const int screenHeight = 800;
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
    bool showPassword = false; bool rememberMe = false; // New Backend States
    
    string loginMessage = ""; Color messageColor = WHITE;
    
    Rectangle formBox = { 580, 100, 550, 650 }; 
    Rectangle userBox = { 630, 260, 450, 45 };
    Rectangle passBox = { 630, 360, 450, 45 }; 
    Rectangle loginBtn = { 630, 490, 210, 45 };
    Rectangle guestBtn = { 870, 490, 210, 45 }; 
    Rectangle createBtn = { 630, 590, 450, 45 };
    
    // New Hitboxes for the interactive elements
    Rectangle eyeBtn = { 1040, 368, 30, 30 }; 
    Rectangle rememberBox = { 630, 430, 15, 15 };
    Rectangle forgotBtn = { 920, 430, 130, 15 };

    while (!WindowShouldClose()) {
        Vector2 mousePoint = GetMousePosition();
        if (!isLoggedIn) {
            
            // ── Input Box Selection ──
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePoint, userBox)) { mouseOnUser = true; mouseOnPass = false; }
                else if (CheckCollisionPointRec(mousePoint, passBox)) { mouseOnUser = false; mouseOnPass = true; }
                else { mouseOnUser = false; mouseOnPass = false; }
                
                // Toggle Show Password
                if (CheckCollisionPointRec(mousePoint, eyeBtn)) showPassword = !showPassword;
                
                // Toggle Remember Me
                if (CheckCollisionPointRec(mousePoint, rememberBox) || CheckCollisionPointRec(mousePoint, {630, 430, 120, 20})) {
                    rememberMe = !rememberMe;
                }
                
                // Forgot Password Click
                if (CheckCollisionPointRec(mousePoint, forgotBtn)) {
                    loginMessage = "Please contact admin to reset password."; messageColor = saffronOrange;
                }
            }

            // ── Tab cycling: with two fields, Tab just flips focus between them ──
            if (IsKeyPressed(KEY_TAB)) {
                if (mouseOnUser)      { mouseOnUser = false; mouseOnPass = true; }
                else if (mouseOnPass) { mouseOnUser = true;  mouseOnPass = false; }
                else                  { mouseOnUser = true;  mouseOnPass = false; } // nothing focused -> start at Username
            }

            bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            
            // ── Typing Logic (Username) ──
            if (mouseOnUser) {
                if (ctrlDown && IsKeyPressed(KEY_V)) {
                    const char* clip = GetClipboardText();
                    if (clip) {
                        for (int i = 0; clip[i] != '\0' && userLetterCount < MAX_INPUT_CHARS; i++) {
                            if (clip[i] >= 32 && clip[i] <= 125) {
                                usernameInput[userLetterCount] = clip[i];
                                usernameInput[userLetterCount + 1] = '\0';
                                userLetterCount++;
                            }
                        }
                    }
                } else if (ctrlDown && (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_X))) {
                    SetClipboardText(usernameInput);
                    if (IsKeyPressed(KEY_X)) { userLetterCount = 0; usernameInput[0] = '\0'; }
                } else {
                    int key = GetCharPressed();
                    while (key > 0) {
                        if ((key >= 32) && (key <= 125) && (userLetterCount < MAX_INPUT_CHARS)) {
                            usernameInput[userLetterCount] = (char)key; usernameInput[userLetterCount + 1] = '\0'; userLetterCount++;
                        }
                        key = GetCharPressed();
                    }
                    if (IsFieldKeyPressedRepeat(KEY_BACKSPACE, 0) && userLetterCount > 0) { userLetterCount--; usernameInput[userLetterCount] = '\0'; }
                }
                // Enter moves focus down to the Password field
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) { mouseOnUser = false; mouseOnPass = true; }
            }
            
            // ── Typing Logic (Password) ──
            if (mouseOnPass) {
                if (ctrlDown && IsKeyPressed(KEY_V)) {
                    const char* clip = GetClipboardText();
                    if (clip) {
                        for (int i = 0; clip[i] != '\0' && passLetterCount < MAX_INPUT_CHARS; i++) {
                            if (clip[i] >= 32 && clip[i] <= 125) {
                                passwordInput[passLetterCount] = clip[i];
                                passwordInput[passLetterCount + 1] = '\0';
                                passLetterCount++;
                            }
                        }
                    }
                } else if (ctrlDown && (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_X))) {
                    SetClipboardText(passwordInput);
                    if (IsKeyPressed(KEY_X)) { passLetterCount = 0; passwordInput[0] = '\0'; }
                } else {
                    int key = GetCharPressed();
                    while (key > 0) {
                        if ((key >= 32) && (key <= 125) && (passLetterCount < MAX_INPUT_CHARS)) {
                            passwordInput[passLetterCount] = (char)key; passwordInput[passLetterCount + 1] = '\0'; passLetterCount++;
                        }
                        key = GetCharPressed();
                    }
                    if (IsFieldKeyPressedRepeat(KEY_BACKSPACE, 1) && passLetterCount > 0) { passLetterCount--; passwordInput[passLetterCount] = '\0'; }
                }
            }
            
            // ── Main Action Buttons ──
            // Enter, while focused on Password, submits the login just like clicking "Log in"
            bool enterSubmit = (mouseOnPass && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)));
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || enterSubmit) {
                if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePoint, loginBtn)) || enterSubmit) {
                    if (login(string(usernameInput), string(passwordInput))) {
                        // Backend for Remember Me!
                        if (rememberMe) {
                            ofstream outRem("remember.txt");
                            outRem << usernameInput << "\n" << passwordInput << "\n";
                            outRem.close();
                        }
                        return true;
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
            
            // Username Box
            DrawText("Username", 630, 230, 18, tealText); DrawRectangleRounded(userBox, 0.2f, 10, inputBg);
            DrawRectangleRoundedLinesEx(userBox, 0.2f, 10, 1.0f, mouseOnUser ? saffronOrange : DARKGRAY);
            DrawTextureEx(userIcon, (Vector2){ 645, 271 }, 0.0f, 0.12f, WHITE); DrawText(usernameInput, 690, 272, 20, WHITE);
            if (mouseOnUser && ((int)(GetTime() * 2) % 2 == 0)) DrawText("_", 690 + MeasureText(usernameInput, 20), 272, 20, saffronOrange);
            
            // Password Box
            DrawText("Password", 630, 330, 18, tealText); DrawRectangleRounded(passBox, 0.2f, 10, inputBg);
            DrawRectangleRoundedLinesEx(passBox, 0.2f, 10, 1.0f, mouseOnPass ? saffronOrange : DARKGRAY);
            DrawTextureEx(lockIcon, (Vector2){ 640, 371 }, 0.0f, 0.06f, WHITE);
            
            // Eye Icon Rendering (Moved to Password box line!)
            DrawTextureEx(eyeIcon, (Vector2){ 1045, 372 }, 0.0f, 0.04f, showPassword ? saffronOrange : WHITE);
            
            // Password Show/Hide Rendering logic
            if (showPassword) {
                DrawText(passwordInput, 690, 375, 20, WHITE);
                if (mouseOnPass && ((int)(GetTime() * 2) % 2 == 0)) DrawText("_", 690 + MeasureText(passwordInput, 20), 375, 20, saffronOrange);
            } else {
                string hiddenPass(passLetterCount, '*'); DrawText(hiddenPass.c_str(), 690, 375, 20, WHITE);
                if (mouseOnPass && ((int)(GetTime() * 2) % 2 == 0)) DrawText("_", 690 + MeasureText(hiddenPass.c_str(), 20), 375, 20, saffronOrange);
            }
            
            // Remember Me UI Checkbox
            DrawRectangleLinesEx(rememberBox, 1.5f, rememberMe ? saffronOrange : tealText);
            if (rememberMe) DrawRectangle(rememberBox.x + 3, rememberBox.y + 3, 9, 9, saffronOrange);
            DrawText("  Remember Me", 645, 430, 15, WHITE); 
            
            DrawText("Forgot Password? ", 920, 430, 15, CheckCollisionPointRec(mousePoint, forgotBtn) ? WHITE : saffronOrange);
            DrawText(loginMessage.c_str(), 630, 465, 15, messageColor);
            
            DrawRectangleRoundedLinesEx(loginBtn, 0.2f, 10, 1.5f, CheckCollisionPointRec(mousePoint, loginBtn) ? terminalGreen : DARKGRAY);
            DrawText("  Log in", 670, 502, 20, terminalGreen);
            DrawRectangleRoundedLinesEx(guestBtn, 0.2f, 10, 1.5f, tealText); DrawText(" Guest Mode", 900, 502, 20, tealText);
            DrawText("New Here? Please Fill Up Username ", 700, 545, 15, tealText);
            DrawText("and Password And Click Create Account", 685, 565, 15, tealText);
            DrawRectangleRoundedLinesEx(createBtn, 0.2f, 10, 1.5f, CheckCollisionPointRec(mousePoint, createBtn) ? purpleAccent : DARKGRAY);
            DrawText(" Create Account", 760, 602, 20, purpleAccent);
            DrawTextureEx(clockTower, (Vector2){ 590, 654 }, 0.0f, 0.45f, WHITE);
        }
        EndDrawing();
    }
    UnloadTexture(logo); UnloadTexture(mountain); UnloadTexture(textLogo); UnloadTexture(hand);
    UnloadTexture(userIcon); UnloadTexture(lockIcon); UnloadTexture(eyeIcon); UnloadTexture(clockTower);
    return false;
}