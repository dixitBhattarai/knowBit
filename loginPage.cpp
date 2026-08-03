#include <raylib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdio>
#include <cmath>
#include "task.h" 
#define DrawText(text, x, y, size, color) DrawTextEx(robotoRegular, text, {(float)(x), (float)(y)}, (float)(size)+8, 1.0f, color)

using namespace std;
extern Font robotoRegular;
extern Font robotoBold;

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
    // OOP Concept: Exception Handling
    // Throwing a simple char exception is a lightweight way to signal
    // exactly *which* field is invalid back up to the caller, without
    // changing registerUser()'s return type or the rest of the login flow.
    if (username.empty()) throw 'U';   // 'U' = invalid/empty Username
    if (password.empty()) throw 'P';   // 'P' = invalid/empty Password

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

void logoutUser() {
    remove("remember.txt");
    activeUsername = "";
}

bool runLoginGUI() {
    ifstream remFile("remember.txt");
    if (remFile.is_open()) {
        string remUser, remPass;
        if (getline(remFile, remUser) && getline(remFile, remPass)) {
            if (login(remUser, remPass)) {
                remFile.close();
                return true;
            }
        }
        remFile.close();
    }

    Color bgColor = { 11, 17, 26, 255 }; 
    Color saffronOrange = { 255, 128, 0, 255 }; 
    Color terminalGreen = { 0, 255, 65, 255 }; 
    Color tealText = { 0, 180, 180, 255 };      
    Color purpleAccent = { 150, 100, 255, 255 }; 
    Color inputBg = { 20, 25, 35, 255 };        
    
    Texture2D logo = LoadTexture("assets/logo.png"); 
    Texture2D mountain = LoadTexture("assets/mountain.png");
    Texture2D textLogo = LoadTexture("assets/knowBit.png"); 
    Texture2D hand = LoadTexture("assets/hi.png");
    Texture2D userIcon = LoadTexture("assets/user.png"); 
    Texture2D lockIcon = LoadTexture("assets/password.png");
    Texture2D eyeIcon = LoadTexture("assets/hidepw.png"); 
    Texture2D clockTower = LoadTexture("assets/clockTower.png");
    
    const int MAX_INPUT_CHARS = 20;
    char usernameInput[MAX_INPUT_CHARS + 1] = "\0"; 
    char passwordInput[MAX_INPUT_CHARS + 1] = "\0";
    int userLetterCount = 0; 
    int passLetterCount = 0;
    
    bool mouseOnUser = false; 
    bool mouseOnPass = false; 
    bool isLoggedIn = false;
    bool showPassword = false; 
    bool rememberMe = false;
    
    string loginMessage = ""; 
    Color messageColor = WHITE;

    while (!WindowShouldClose()) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        
        // Use uniform scale based on smallest dimension to maintain aspect ratio
        float baseWidth = 1280.0f;
        float baseHeight = 800.0f;
        float scale = fminf(screenWidth / baseWidth, screenHeight / baseHeight);
        
        // Calculate offsets to center content if there's extra space
        float offsetX = (screenWidth - (baseWidth * scale)) / 2.0f;
        float offsetY = (screenHeight - (baseHeight * scale)) / 2.0f;
        
        // Helper function to scale coordinates
        auto ScalePos = [&](float x, float y) -> Vector2 {
            return {offsetX + x * scale, offsetY + y * scale};
        };
        
        auto ScaleRect = [&](float x, float y, float w, float h) -> Rectangle {
            return {offsetX + x * scale, offsetY + y * scale, w * scale, h * scale};
        };
        
        auto ScaleVal = [&](float val) -> float {
            return val * scale;
        };
        
        // Original design rectangles (1280x800 base)
        Rectangle userBox = ScaleRect(630, 260, 450, 45);
        Rectangle passBox = ScaleRect(630, 360, 450, 45);
        Rectangle loginBtn = ScaleRect(630, 490, 210, 45);
        Rectangle guestBtn = ScaleRect(870, 490, 210, 45);
        Rectangle createBtn = ScaleRect(630, 590, 450, 45);
        Rectangle formBox = ScaleRect(580, 100, 550, 650);
        Rectangle eyeBtn = ScaleRect(1040, 368, 30, 30);
        Rectangle rememberBox = ScaleRect(630, 430, 15, 15);
        Rectangle forgotBtn = ScaleRect(920, 430, 130, 15);
        
        Vector2 mousePoint = GetMousePosition();
        
        if (!isLoggedIn) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePoint, userBox)) { mouseOnUser = true; mouseOnPass = false; }
                else if (CheckCollisionPointRec(mousePoint, passBox)) { mouseOnUser = false; mouseOnPass = true; }
                else { mouseOnUser = false; mouseOnPass = false; }
                
                if (CheckCollisionPointRec(mousePoint, eyeBtn)) showPassword = !showPassword;
                
                Rectangle rememberText = ScaleRect(630, 430, 120, 20);
                if (CheckCollisionPointRec(mousePoint, rememberBox) || CheckCollisionPointRec(mousePoint, rememberText)) {
                    rememberMe = !rememberMe;
                }
                
                if (CheckCollisionPointRec(mousePoint, forgotBtn)) {
                    loginMessage = "Please contact admin to reset password."; 
                    messageColor = saffronOrange;
                }
            }

            if (IsKeyPressed(KEY_TAB)) {
                if (mouseOnUser)      { mouseOnUser = false; mouseOnPass = true; }
                else if (mouseOnPass) { mouseOnUser = true;  mouseOnPass = false; }
                else                  { mouseOnUser = true;  mouseOnPass = false; }
            }

            bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            
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
                            usernameInput[userLetterCount] = (char)key;
                            usernameInput[userLetterCount + 1] = '\0';
                            userLetterCount++;
                        }
                        key = GetCharPressed();
                    }
                    if (IsFieldKeyPressedRepeat(KEY_BACKSPACE, 0) && userLetterCount > 0) { 
                        userLetterCount--; 
                        usernameInput[userLetterCount] = '\0'; 
                    }
                }
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) { mouseOnUser = false; mouseOnPass = true; }
            }
            
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
                            passwordInput[passLetterCount] = (char)key;
                            passwordInput[passLetterCount + 1] = '\0';
                            passLetterCount++;
                        }
                        key = GetCharPressed();
                    }
                    if (IsFieldKeyPressedRepeat(KEY_BACKSPACE, 1) && passLetterCount > 0) { 
                        passLetterCount--; 
                        passwordInput[passLetterCount] = '\0'; 
                    }
                }
            }
            
            bool enterSubmit = (mouseOnPass && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)));
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || enterSubmit) {
                if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePoint, loginBtn)) || enterSubmit) {
                    if (login(string(usernameInput), string(passwordInput))) {
                        if (rememberMe) {
                            ofstream outRem("remember.txt");
                            outRem << usernameInput << "\n" << passwordInput << "\n";
                            outRem.close();
                        }
                        return true;
                    } else { 
                        loginMessage = "Error: Invalid username or password."; 
                        messageColor = RED; 
                    }
                }
                if (CheckCollisionPointRec(mousePoint, createBtn)) {
                    // OOP Concept: Exception Handling
                    // Catch the char exceptions thrown by registerUser()
                    // and show a friendly message instead of creating an
                    // account with an empty username/password.
                    try {
                        if (registerUser(string(usernameInput), string(passwordInput))) {
                            loginMessage = "Account created! Please log in."; 
                            messageColor = terminalGreen;
                        } else { 
                            loginMessage = "Error: Could not save to database."; 
                            messageColor = RED; 
                        }
                    } catch (char invalidField) {
                        if (invalidField == 'U')
                            loginMessage = "Error: Username cannot be empty.";
                        else if (invalidField == 'P')
                            loginMessage = "Error: Password cannot be empty.";
                        else
                            loginMessage = "Error: Invalid input.";
                        messageColor = RED;
                    }
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
        
        BeginDrawing();
        ClearBackground(bgColor);
        
        if (!isLoggedIn) {
            // Time
            time_t now = time(0); 
            tm *ltm = localtime(&now); 
            char timeStr[50];
            strftime(timeStr, sizeof(timeStr), "Time: %I:%M:%S %p", ltm);
            DrawText(timeStr, (int)ScalePos(950, 45).x, (int)ScalePos(950, 45).y, (int)ScaleVal(20), tealText);
            
            // Header logo
            Vector2 logoPos = ScalePos(25, 20);
            DrawTextureEx(logo, logoPos, 0.0f, ScaleVal(0.12f), WHITE); 
            
            // Header text
            int knowWidth = MeasureText("know", (int)ScaleVal(30));
            DrawText("know", (int)ScalePos(105, 35).x, (int)ScalePos(105, 35).y, (int)ScaleVal(30), WHITE);
            DrawText("Bit", (int)(ScalePos(105, 35).x + knowWidth + ScaleVal(5)), (int)ScalePos(105, 35).y, (int)ScaleVal(30), saffronOrange); 
            DrawText("v1.0", (int)(ScalePos(105, 35).x + knowWidth + ScaleVal(5) + MeasureText("Bit", (int)ScaleVal(30)) + ScaleVal(10)), (int)ScalePos(105, 45).y, (int)ScaleVal(15), LIGHTGRAY);
            
            // Mountain background
            Rectangle mntSource = { 0, 0, (float)mountain.width, (float)mountain.height };
            Rectangle mntDest = ScaleRect(15, 95, 555, 360);
            DrawTexturePro(mountain, mntSource, mntDest, (Vector2){0, 0}, 0.0f, WHITE);
            
            // Bottom left logo
            Vector2 textLogoPos = ScalePos(40, 475);
            DrawTextureEx(textLogo, textLogoPos, 0.0f, ScaleVal(0.42f), WHITE);
            DrawText("Plan. Prioritize. Get It Done.", (int)ScalePos(50, 590).x, (int)ScalePos(50, 590).y, (int)ScaleVal(30), WHITE);
            
            // Clock tower
            Vector2 clockPos = ScalePos(590, 654);
            DrawTextureEx(clockTower, clockPos, 0.0f, ScaleVal(0.45f), WHITE);
            
            // Form box
            DrawRectangleRoundedLinesEx(formBox, 0.05f, 10, ScaleVal(1.5f), saffronOrange);
            
            // Form title
            DrawText("Welcome Back!", (int)ScalePos(750, 130).x, (int)ScalePos(750, 130).y, (int)ScaleVal(30), saffronOrange);
            
            // Hand icon
            Vector2 handPos = ScalePos(970, 125);
            DrawTextureEx(hand, handPos, 0.0f, ScaleVal(0.15f), WHITE);
            
            // Subtitle
            DrawText("Log in to Continue Your Journey", (int)ScalePos(650, 180).x, (int)ScalePos(650, 180).y, (int)ScaleVal(18), tealText);
            
            // Username label
            DrawText("Username", (int)ScalePos(630, 230).x, (int)ScalePos(630, 230).y, (int)ScaleVal(18), tealText);
            DrawRectangleRounded(userBox, 0.2f, 10, inputBg);
            DrawRectangleRoundedLinesEx(userBox, 0.2f, 10, ScaleVal(1.0f), mouseOnUser ? saffronOrange : DARKGRAY);
            DrawTextureEx(userIcon, ScalePos(645, 271), 0.0f, ScaleVal(0.12f), WHITE);
            DrawText(usernameInput, (int)ScalePos(690, 272).x, (int)ScalePos(690, 272).y, (int)ScaleVal(20), WHITE);
            if (mouseOnUser && ((int)(GetTime() * 2) % 2 == 0)) {
                DrawText("_", (int)(ScalePos(690, 272).x + MeasureText(usernameInput, (int)ScaleVal(20))), (int)ScalePos(690, 272).y, (int)ScaleVal(20), saffronOrange);
            }
            
            // Password label
            DrawText("Password", (int)ScalePos(630, 330).x, (int)ScalePos(630, 330).y, (int)ScaleVal(18), tealText);
            DrawRectangleRounded(passBox, 0.2f, 10, inputBg);
            DrawRectangleRoundedLinesEx(passBox, 0.2f, 10, ScaleVal(1.0f), mouseOnPass ? saffronOrange : DARKGRAY);
            DrawTextureEx(lockIcon, ScalePos(640, 371), 0.0f, ScaleVal(0.06f), WHITE);
            DrawTextureEx(eyeIcon, ScalePos(1045, 372), 0.0f, ScaleVal(0.04f), showPassword ? saffronOrange : WHITE);
            
            if (showPassword) {
                DrawText(passwordInput, (int)ScalePos(690, 375).x, (int)ScalePos(690, 375).y, (int)ScaleVal(20), WHITE);
                if (mouseOnPass && ((int)(GetTime() * 2) % 2 == 0)) {
                    DrawText("_", (int)(ScalePos(690, 375).x + MeasureText(passwordInput, (int)ScaleVal(20))), (int)ScalePos(690, 375).y, (int)ScaleVal(20), saffronOrange);
                }
            } else {
                string hiddenPass(passLetterCount, '*');
                DrawText(hiddenPass.c_str(), (int)ScalePos(690, 375).x, (int)ScalePos(690, 375).y, (int)ScaleVal(20), WHITE);
                if (mouseOnPass && ((int)(GetTime() * 2) % 2 == 0)) {
                    DrawText("_", (int)(ScalePos(690, 375).x + MeasureText(hiddenPass.c_str(), (int)ScaleVal(20))), (int)ScalePos(690, 375).y, (int)ScaleVal(20), saffronOrange);
                }
            }
            
            // Remember Me
            DrawRectangleLinesEx(rememberBox, ScaleVal(1.5f), rememberMe ? saffronOrange : tealText);
            if (rememberMe) DrawRectangle((int)(rememberBox.x + ScaleVal(3)), (int)(rememberBox.y + ScaleVal(3)), (int)(rememberBox.width - ScaleVal(6)), (int)(rememberBox.height - ScaleVal(6)), saffronOrange);
            DrawText("  Remember Me", (int)ScalePos(645, 430).x, (int)ScalePos(645, 430).y, (int)ScaleVal(15), WHITE);
            
            // Forgot Password
            DrawText("Forgot Password? ", (int)ScalePos(920, 430).x, (int)ScalePos(920, 430).y, (int)ScaleVal(15), CheckCollisionPointRec(mousePoint, forgotBtn) ? WHITE : saffronOrange);
            
            // Login message
            DrawText(loginMessage.c_str(), (int)ScalePos(630, 465).x, (int)ScalePos(630, 465).y, (int)ScaleVal(15), messageColor);
            
            // Login button
            DrawRectangleRoundedLinesEx(loginBtn, 0.2f, 10, ScaleVal(1.5f), CheckCollisionPointRec(mousePoint, loginBtn) ? terminalGreen : DARKGRAY);
            DrawText("  Log in", (int)ScalePos(670, 502).x, (int)ScalePos(670, 502).y, (int)ScaleVal(20), terminalGreen);
            
            // Guest button
            DrawRectangleRoundedLinesEx(guestBtn, 0.2f, 10, ScaleVal(1.5f), tealText);
            DrawText(" Guest Mode", (int)ScalePos(900, 502).x, (int)ScalePos(900, 502).y, (int)ScaleVal(20), tealText);
            
            // Sign up info
            DrawText("New Here? Please Fill Up Username", (int)ScalePos(700, 545).x, (int)ScalePos(700, 545).y, (int)ScaleVal(15), tealText);
            DrawText("and Password And Click Create Account", (int)ScalePos(685, 565).x, (int)ScalePos(685, 565).y, (int)ScaleVal(15), tealText);
            
            // Create Account button
            DrawRectangleRoundedLinesEx(createBtn, 0.2f, 15, ScaleVal(1.5f), CheckCollisionPointRec(mousePoint, createBtn) ? purpleAccent : DARKGRAY);
            DrawText(" Create Account", (int)ScalePos(760, 602).x, (int)ScalePos(760, 602).y, (int)ScaleVal(20), purpleAccent);
        }
        
        EndDrawing();
    }
    
    UnloadTexture(logo); 
    UnloadTexture(mountain); 
    UnloadTexture(textLogo); 
    UnloadTexture(hand);
    UnloadTexture(userIcon); 
    UnloadTexture(lockIcon); 
    UnloadTexture(eyeIcon); 
    UnloadTexture(clockTower);
    
    return false;
}