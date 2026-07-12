#include "dashboard.h"
#include "task.h"
#include "calendar.h"   // displayCalendar() draws into the same window
#include "suyans.h"

#include <raylib.h>

#include <iostream>
#include <cstring>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <sstream>
#define DrawText(text, x, y, size, color) DrawTextEx(robotoRegular, text, {(float)(x), (float)(y)}, (float)(size), 1.0f, color)
static bool wantsToLoadMusic = false;
using namespace std;
extern Font customFont;
extern Texture2D texLogo, texStreak, texClock, texMusic, texCalendar, texEdit, texDelete;
extern Font robotoRegular;
 extern Font robotoBold;

// ─── Palette ────────────────────────────────────────────────
static const Color C_BG         = { 11,  17,  26, 255 };
static const Color C_PANEL      = { 17,  24,  36, 255 };
static const Color C_SIDEBAR    = { 14,  20,  30, 255 };
static const Color C_CARD       = { 22,  30,  44, 255 };
static const Color C_CARD_HOV   = { 28,  38,  56, 255 };
static const Color C_ORANGE     = {255, 128,   0, 255 };
static const Color C_GREEN      = {  0, 255,  65, 255 };
static const Color C_TEAL       = {  0, 180, 180, 255 };
static const Color C_PURPLE     = {150, 100, 255, 255 };
static const Color C_RED        = {220,  60,  60, 255 };
static const Color C_YELLOW     = {255, 210,  80, 255 };
static const Color C_BLUE       = { 80, 140, 255, 255 };
static const Color C_BORDER     = { 40,  55,  75, 255 };
static const Color C_TEXT_DIM   = {120, 140, 160, 255 };
static const Color C_WHITE      = {240, 245, 255, 255 };
static const Color C_DARKGRAY   = { 35,  45,  60, 255 };

// ─── Layout constants ───────────────────────────────────────
static const int WIN_W  = 1280;
static const int WIN_H  = 800;
static const int SIDE_W = 190;
static const int TOP_H  = 90;

// ─── Navigation views ───────────────────────────────────────
enum View { VIEW_DASH, VIEW_FOCUS, VIEW_STATS, VIEW_CALENDAR, VIEW_PROFILE };

// ─── Stats completion-graph time granularity ─────────────────
enum CompletionGranularity { GRAN_DAILY, GRAN_WEEKLY, GRAN_MONTHLY };

// ─── Sort mode ──────────────────────────────────────────────
enum SortMode { SORT_NONE, SORT_PRIORITY, SORT_DUE };

// ─── Modal / overlay state ──────────────────────────────────
struct ModalState {
    bool open          = false;
    // shared
    int  taskIdx       = -1;     // index into allTasks for edit/focus
    // new task form fields (Task ID is no longer entered — it is auto-assigned)
    char tfName[64]    = "";
    char tfDesc[128]   = "";
    char tfCat[32]     = "";
    char tfPri[4]      = "";
    int  activeField   = 0;
    string errorMsg    = "";
    // Mini due-date calendar picker state (New Task modal)
    int  navMonth      = 0;      // month currently browsed in the picker
    int  navYear       = 0;      // year currently browsed in the picker
    int  pickDay       = -1;     // chosen due-date day, -1 = nothing chosen yet
    int  pickMonth     = 0;      // month the chosen day belongs to
    int  pickYear      = 0;      // year the chosen day belongs to
    // edit fields (only due-date / priority / type)
   // edit fields (only due-date / priority / type)
    char efPri[4]      = "";
    char efDays[8]     = "";
    char efCat[32]     = "";
    // ── Delete confirmation popup ──
    bool   confirmDeleteOpen = false;   // true while the "Are you sure?" dialog is showing
    int    confirmDeleteId   = -1;      // taskId pending deletion
    string confirmDeleteName = "";      // task name shown in the confirmation message
};


// ─── Focus-mode timer state ──────────────────────────────────
struct FocusTimer {
    bool    running     = false;
    bool    inputMode   = true;   // true = user is setting time
    char    setMin[6]   = "25";
    int     totalSecs   = 0;
    int     elapsed     = 0;
    double  startStamp  = 0.0;
};

// ─── Profile edit state ───────────────────────────────────────
struct ProfileEditState {
    bool editing      = false;
    int  activeField  = 0;
    char fullName[48] = "";
    char pronouns[16] = "";
    char bio[140]     = "";
    char website[64]  = "";
    char socials[32]  = "";
    char linkedin[64] = "";
};

// ─── Music state ────────────────────────────────────────────
struct MusicState {
    bool   on       = false;
    Music  track    = {};
    bool   loaded   = false;
    char   path[512] = "";
};

// ─── Session timer ───────────────────────────────────────────
static double g_sessionStart = 0.0;

// ─── Helpers ─────────────────────────────────────────────────

static Color priorityColor(int p) {
    if (p >= 5) return C_RED;
    if (p >= 4) return { 255, 100, 40, 255 };
    if (p >= 3) return C_YELLOW;
    if (p >= 2) return { 100, 200, 100, 255 };
    return { 80, 160, 220, 255 };
}

static const char* priorityLabel(int p) {
    if (p >= 5) return "Critical";
    if (p >= 4) return "High";
    if (p >= 3) return "Medium";
    if (p >= 2) return "Low";
    return "Minimal";
}

static string fmtDate(int d, int m, int y) {
    const char* mn[] = {"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"};
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %02d, %d", mn[m-1], d, y);
    return buf;
}

static string fmtSession(double secs) {
    int s = (int)secs;
    int h = s / 3600; s %= 3600;
    int m = s / 60;   s %= 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    return buf;
}

static string currentTimeStr() {
    time_t now = time(0);
    tm *t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%I:%M %p", t);
    return buf;
}

// Word-wraps `text` inside maxWidth and draws it line by line.
// Returns the total pixel height consumed, so callers can keep laying
// out content below it (same idea as MeasureText, but for paragraphs).
static float DrawWrappedText(const char* text, int x, int y, int maxWidth,
                              int fontSize, Color color, int lineGap = 6) {
    string word, line;
    int cy = y;
    istringstream iss{string(text)};
    while (iss >> word) {
        string test = line.empty() ? word : line + " " + word;
        if (MeasureText(test.c_str(), fontSize) > maxWidth && !line.empty()) {
            DrawText(line.c_str(), x, cy, fontSize, color);
            cy += fontSize + lineGap;
            line = word;
        } else {
            line = test;
        }
    }
    if (!line.empty()) {
        DrawText(line.c_str(), x, cy, fontSize, color);
        cy += fontSize + lineGap;
    }
    return (float)(cy - y);
}

// Draw rounded rectangle helper
static void DrawRoundRect(Rectangle r, float round, Color c) {
    DrawRectangleRounded(r, round, 8, c);
}
static void DrawRoundRectLines(Rectangle r, float round, float thick, Color c) {
    DrawRectangleRoundedLinesEx(r, round, 8, thick, c);
}

// NOTE: raylib provides IsKeyPressedRepeat(int key) natively — it returns true
// on the initial key-down and then repeatedly while held (OS-style key repeat),
// which is exactly what's needed for a held Backspace. We call raylib's
// built-in version directly instead of redefining our own.

// Text input handler — returns true if value changed.
// Supports typing, held-backspace repeat, and Ctrl+C / Ctrl+V / Ctrl+X clipboard shortcuts.
static bool HandleTextInput(char* buf, int maxLen, int key) {
    bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    if (ctrlDown && IsKeyPressed(KEY_V)) {
        const char* clip = GetClipboardText();
        if (clip) {
            int len = TextLength(buf);
            for (int i = 0; clip[i] != '\0' && len < maxLen - 1; i++) {
                // Skip newlines so a multi-line paste doesn't corrupt a single-line field
                if (clip[i] >= 32 && clip[i] <= 125) {
                    buf[len] = clip[i];
                    buf[len+1] = '\0';
                    len++;
                }
            }
            return true;
        }
        return false;
    }
    if (ctrlDown && IsKeyPressed(KEY_C)) {
        SetClipboardText(buf);
        return false;
    }
    if (ctrlDown && IsKeyPressed(KEY_X)) {
        SetClipboardText(buf);
        buf[0] = '\0';
        return true;
    }

    if (key == KEY_BACKSPACE) {
        int len = TextLength(buf);
        if (len > 0) { buf[len-1] = '\0'; return true; }
        return false;
    }
    int ch = GetCharPressed();
    bool changed = false;
    while (ch > 0) {
        int len = TextLength(buf);
        if (ch >= 32 && ch <= 125 && len < maxLen-1) {
            buf[len] = (char)ch;
            buf[len+1] = '\0';
            changed = true;
        }
        ch = GetCharPressed();
    }
    return changed;
}

// Draw a labelled input box; returns true when active
static bool DrawInputBox(const char* label, char* buf, Rectangle r,
                         bool active, Color accentCol = C_ORANGE) {
    DrawText(label, (int)r.x, (int)r.y - 20, 14, C_TEXT_DIM);
    DrawRoundRect(r, 0.15f, C_DARKGRAY);
    DrawRoundRectLines(r, 0.15f, 1.5f, active ? accentCol : C_BORDER);
    DrawText(buf, (int)r.x + 10, (int)r.y + (int)(r.height/2) - 8, 15, C_WHITE);
    if (active && (int)(GetTime()*2)%2 == 0) {
        int tw = MeasureText(buf, 15);
        DrawText("|", (int)r.x + 10 + tw, (int)r.y + (int)(r.height/2) - 8, 15, accentCol);
    }
    return active;
}

// ─────────────────────────────────────────────────────────────
//  Open file dialog (zenity on Linux, fallback to path input)
// ─────────────────────────────────────────────────────────────
static string OpenFileDialog(const char* filter = "*.mp3 *.ogg *.wav *.flac") {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "zenity --file-selection --title='Select Music' "
        "--file-filter='Audio files | %s' 2>/dev/null", filter);
    FILE* fp = popen(cmd, "r");
    if (!fp) return "";
    char result[512] = "";
    if (fgets(result, sizeof(result), fp)) {
        int len = strlen(result);
        if (len > 0 && result[len-1] == '\n') result[len-1] = '\0';
    }
    pclose(fp);
    return string(result);
}

// ─────────────────────────────────────────────────────────────
//  Small calendar-math helpers (shared by the mini due-date
//  picker in the New Task modal AND the full Calendar view)
// ─────────────────────────────────────────────────────────────
static bool calLeap(int y){ return (y%4==0&&(y%100!=0||y%400==0)); }
static int  calDays(int m,int y){ int d[]={31,28,31,30,31,30,31,31,30,31,30,31}; if(m==2&&calLeap(y))return 29; return d[m-1]; }
static int  calStart(int m,int y){ tm t={0}; t.tm_year=y-1900; t.tm_mon=m-1; t.tm_mday=1; mktime(&t); return t.tm_wday; }
static string calMonthName(int m){ string n[]={"January","February","March","April","May","June","July","August","September","October","November","December"}; return n[m-1]; }

// ─────────────────────────────────────────────────────────────
//  Mini due-date calendar picker (used inside the New Task modal)
//  Lets the user click a day instead of typing "days to complete"
// ─────────────────────────────────────────────────────────────
static void DrawMiniCalendarPicker(ModalState& m, Rectangle area, Vector2 mouse);

// ─────────────────────────────────────────────────────────────
//  Sidebar
// ─────────────────────────────────────────────────────────────
// CHANGED: Now returns a bool instead of void!
static bool DrawSidebar(View& view, MusicState& music, Vector2 mouse) {
    DrawRectangle(0, 0, SIDE_W, WIN_H, C_SIDEBAR);
    DrawLine(SIDE_W, 0, SIDE_W, WIN_H, C_BORDER);

    // ── Logo and Title ──
    DrawTextureEx(texLogo, { 15, 18 }, 0.0f, 0.07f, WHITE);
    DrawText("know", 65, 22, 22, WHITE);
    DrawText("Bit", 65 + MeasureText("know", 22), 22, 22, C_ORANGE);
    DrawText("v1.0", 65 + MeasureText("knowBit", 22) + 6, 30, 10, GRAY);

    // ── Profile & Streak Info Box ──
    DrawRoundRect({12, 80, SIDE_W-24, 55}, 0.15f, C_DARKGRAY);
    DrawTextureEx(texStreak, {18, 88}, 0.0f, 0.09f, WHITE); 
    DrawText(activeUsername.c_str(), 58, 86, 15, C_WHITE);

    char strk[32]; snprintf(strk, sizeof(strk), "%d Day Streak", activeUserStreak);
    DrawText(strk, 58, 106, 13, C_ORANGE);

    // ── Navigation Items ──
    struct NavItem { const char* icon; const char* label; View v; };
    NavItem items[] = {
        { "##", "Dashboard",  VIEW_DASH     },
        { " O", "Focus Mode", VIEW_FOCUS    },
        { "||", "Statistics", VIEW_STATS    },
        { " C", "Calendar",   VIEW_CALENDAR },
        { " P", "Profile",    VIEW_PROFILE  },
    };
    
    int ny = 155;
    for (auto& item : items) {
        bool sel = (view == item.v);
        Rectangle nr = {0, (float)ny, (float)SIDE_W, 40};
        bool hov = CheckCollisionPointRec(mouse, nr);
        if (sel) DrawRectangle(0, ny, SIDE_W, 40, { 255,128,0, 25 });
        else if (hov) DrawRectangle(0, ny, SIDE_W, 40, { 255,255,255, 8 });
        if (sel) DrawRectangle(0, ny, 3, 40, C_ORANGE);
        DrawText(item.label, 42, ny + 12, 15, sel ? C_ORANGE : (hov ? C_WHITE : C_TEXT_DIM));
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hov) view = item.v;
        ny += 46;
    }

    // ── Lo-fi Music Interactive Engine (MOVED HIGHER: WIN_H - 105) ──
    Rectangle musicBar = {8, (float)(WIN_H - 105), (float)(SIDE_W-16), 38};
    bool musicHov = CheckCollisionPointRec(mouse, musicBar);
    DrawRoundRect(musicBar, 0.2f, C_DARKGRAY);
    DrawRoundRectLines(musicBar, 0.2f, 1.0f, musicHov ? C_ORANGE : C_BORDER);
    
    DrawTextureEx(texMusic, {16, (float)(WIN_H - 93)}, 0.0f, 0.05f, WHITE);
    DrawText("   Lo-fi Music", 16, WIN_H - 93, 13, C_TEXT_DIM);
    DrawText(music.on ? "ON" : "OFF", SIDE_W - 44, WIN_H - 93, 13, music.on ? C_GREEN : C_TEXT_DIM);
    
    // ── THE FIX: Use RELEASED instead of PRESSED ──
    // ── THE BULLETPROOF FIX: The Cooldown Timer ──
    static double lastMusicClick = 0.0; // Keeps track of when we last clicked

    // ── THE NEW SIDEBAR LOGIC (No freezing allowed here) ──
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && musicHov) {
        if (music.on) {
            // Turn music off instantly
            if (music.loaded) StopMusicStream(music.track);
            music.on = false;
        } else {
            // Tell the main loop to open the file picker!
            wantsToLoadMusic = true;
        }
    }

    // ── Logout Button (MOVED LOWER: WIN_H - 58) ──
    Rectangle logoutBtn = {8, (float)(WIN_H - 58), (float)(SIDE_W-16), 38};
    bool logHov = CheckCollisionPointRec(mouse, logoutBtn);
    DrawRoundRect(logoutBtn, 0.2f, C_DARKGRAY);
    DrawRoundRectLines(logoutBtn, 0.2f, 1.0f, logHov ? RED : C_BORDER);
    
    DrawText("   Logout", 16, WIN_H - 46, 13, logHov ? WHITE : RED);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && logHov) {
        logoutUser();
        return true; // <--- This tells the main dashboard loop to exit!
    }

    return false; // Returns false every frame we don't click logout
}

// ─────────────────────────────────────────────────────────────
//  Top bar
// ─────────────────────────────────────────────────────────────
static void DrawTopBar(double sessionSecs) {
    DrawRectangle(SIDE_W, 0, WIN_W - SIDE_W, TOP_H, C_PANEL);
    DrawLine(SIDE_W, TOP_H, WIN_W, TOP_H, C_BORDER);

    // FIX: Shifted "Hello" slightly to the right to match margins
    string greet = "Hello, " + activeUsername + "!";
    DrawText(greet.c_str(), SIDE_W + 40, 22, 24, C_WHITE);
    DrawText("Let's make today productive.", SIDE_W + 40, 54, 14, C_TEXT_DIM);

    // ── XP / Level panel ──
    Rectangle xc = {(float)(WIN_W - 530), 18, 130, 58};
    DrawRoundRect(xc, 0.2f, C_DARKGRAY);
    DrawRoundRectLines(xc, 0.2f, 1.0f, C_PURPLE);
    int lvl    = levelForXP(totalXP);
    int xpInto = xpIntoCurrentLevel(totalXP);
    int xpNeed = xpNeededForNextLevel();
    char lvlStr[16]; snprintf(lvlStr, sizeof(lvlStr), "Lvl %d", lvl);
    DrawText(lvlStr, (int)xc.x + 12, (int)xc.y + 6, 20, C_PURPLE);
    Rectangle xpBarBg = {xc.x + 12, xc.y + 34, xc.width - 24, 8};
    Rectangle xpBarFg = {xc.x + 12, xc.y + 34, (xc.width - 24) * ((float)xpInto / (float)xpNeed), 8};
    DrawRoundRect(xpBarBg, 0.5f, C_BORDER);
    DrawRoundRect(xpBarFg, 0.5f, C_PURPLE);
    char xpTxt[24]; snprintf(xpTxt, sizeof(xpTxt), "%d / %d XP", xpInto, xpNeed);
    DrawText(xpTxt, (int)xc.x + 12, (int)xc.y + 45, 10, C_TEXT_DIM);

    // FIX: Enlarged Streak Box and centered everything inside it
    Rectangle sc = {(float)(WIN_W - 390), 18, 130, 58}; // Made it wider
    DrawRoundRect(sc, 0.2f, C_DARKGRAY);
    DrawRoundRectLines(sc, 0.2f, 1.0f, C_ORANGE);
    char sstr[16]; snprintf(sstr, sizeof(sstr), "%d", activeUserStreak);
    
    // Icon placed slightly left
    DrawTextureEx(texStreak, {sc.x + 8, sc.y + 10}, 0.0f, 0.08f, WHITE);
    
    // "1" placed directly above the 'S' in Streak
    DrawText(sstr, (int)sc.x + 45, (int)sc.y + 8, 26, C_ORANGE);
    
    // "Day Streak" perfectly centered under the number
    DrawText("Day Streak", (int)sc.x + 45, (int)sc.y + 38, 12, C_TEXT_DIM);

    // Clock Panel (Kept the same)
    Rectangle tc = {(float)(WIN_W - 250), 18, 230, 58};
    DrawRoundRect(tc, 0.2f, C_DARKGRAY);
    DrawRoundRectLines(tc, 0.2f, 1.5f, C_GREEN);
    DrawTextureEx(texClock, {tc.x - 12, tc.y + 0}, 0.0f, 0.12f, WHITE);
    string sess = fmtSession(sessionSecs);
    DrawText(sess.c_str(), (int)tc.x + 44, (int)tc.y + 8, 26, C_GREEN);
    DrawText("Daily App Time", (int)tc.x + 10, (int)tc.y + 38, 12, C_TEXT_DIM);
    
    string ct = currentTimeStr();
    int ctw = MeasureText(ct.c_str(), 13);
    DrawText(ct.c_str(), (int)(tc.x + tc.width - ctw - 10), (int)tc.y + 38, 13, C_TEAL);
}

// ─────────────────────────────────────────────────────────────
//  Mini due-date calendar picker — implementation
// ─────────────────────────────────────────────────────────────
static void DrawMiniCalendarPicker(ModalState& m, Rectangle area, Vector2 mouse) {
    // Month navigation arrows
    Rectangle prevBtn = { area.x, area.y, 24, 24 };
    Rectangle nextBtn = { area.x + area.width - 24, area.y, 24, 24 };
    bool prevHov = CheckCollisionPointRec(mouse, prevBtn);
    bool nextHov = CheckCollisionPointRec(mouse, nextBtn);
    DrawRoundRect(prevBtn, 0.3f, prevHov ? C_ORANGE : C_DARKGRAY);
    DrawText("<", (int)prevBtn.x + 8, (int)prevBtn.y + 4, 14, prevHov ? C_BG : C_WHITE);
    DrawRoundRect(nextBtn, 0.3f, nextHov ? C_ORANGE : C_DARKGRAY);
    DrawText(">", (int)nextBtn.x + 8, (int)nextBtn.y + 4, 14, nextHov ? C_BG : C_WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (prevHov) { m.navMonth--; if (m.navMonth < 1) { m.navMonth = 12; m.navYear--; } }
        if (nextHov) { m.navMonth++; if (m.navMonth > 12) { m.navMonth = 1; m.navYear++; } }
    }

    string hd = calMonthName(m.navMonth) + " " + to_string(m.navYear);
    int hw = MeasureText(hd.c_str(), 14);
    DrawText(hd.c_str(), (int)(area.x + area.width / 2 - hw / 2), (int)area.y + 4, 14, C_WHITE);

    const char* dow[] = { "S","M","T","W","T","F","S" };
    float cellW = area.width / 7.0f;
    float gridTop = area.y + 34;
    for (int i = 0; i < 7; i++) {
        int cw = MeasureText(dow[i], 10);
        DrawText(dow[i], (int)(area.x + i*cellW + cellW/2 - cw/2), (int)gridTop - 16, 10, C_TEXT_DIM);
    }

    int start = calStart(m.navMonth, m.navYear);
    int total = calDays(m.navMonth, m.navYear);
    float cellH = 25;
    int row = 0;

    time_t nowT = time(0);
    tm* lt = localtime(&nowT);
    int todayD = lt->tm_mday, todayM = lt->tm_mon + 1, todayY = lt->tm_year + 1900;

    for (int d = 1; d <= total; d++) {
        int col = (start + d - 1) % 7;
        float rx = area.x + col*cellW;
        float ry = gridTop + row*cellH;
        Rectangle cell = { rx + 2, ry, cellW - 4, cellH - 3 };
        bool hov = CheckCollisionPointRec(mouse, cell);

        bool isSel   = (d == m.pickDay && m.navMonth == m.pickMonth && m.navYear == m.pickYear);
        bool isToday = (d == todayD && m.navMonth == todayM && m.navYear == todayY);

        Color cbg = C_DARKGRAY;
        if (isSel) cbg = C_ORANGE;
        else if (hov) cbg = C_CARD_HOV;
        DrawRoundRect(cell, 0.25f, cbg);
        if (isToday && !isSel) DrawRoundRectLines(cell, 0.25f, 1.5f, C_TEAL);

        char db[4]; snprintf(db, sizeof(db), "%d", d);
        int dw = MeasureText(db, 11);
        DrawText(db, (int)(cell.x + cell.width/2 - dw/2), (int)(cell.y + cell.height/2 - 6), 11,
                 isSel ? C_BG : C_WHITE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hov) {
            m.pickDay = d; m.pickMonth = m.navMonth; m.pickYear = m.navYear;
        }
        if (col == 6) row++;
    }
}

// ─────────────────────────────────────────────────────────────
//  New Task modal
// ─────────────────────────────────────────────────────────────
static void DrawNewTaskModal(ModalState& m, Vector2 mouse) {
    // Dimmed overlay
    DrawRectangle(0, 0, WIN_W, WIN_H, {0,0,0,160});

    Rectangle box = {WIN_W/2 - 340.0f, WIN_H/2 - 260.0f, 680, 520};
    DrawRoundRect(box, 0.04f, C_PANEL);
    DrawRoundRectLines(box, 0.04f, 1.5f, C_ORANGE);

    DrawText("+ New Task", (int)box.x + 24, (int)box.y + 18, 20, C_ORANGE);

    // Left column: text fields (Task ID is no longer asked — it's auto-assigned)
    float fx = box.x + 24, fy = box.y + 60, fw = 340, fh = 36;

   struct Field { const char* label; char* buf; int maxLen; };
Field fields[] = {
    { "Task Name",         m.tfName, 64  },
    { "Description",       m.tfDesc, 128 },
    { "Priority (1-5)",    m.tfPri,  4   },
};

for (int i = 0; i < 3; i++) {
    Rectangle r = {fx, fy + i*76.0f, fw, (float)fh};
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, r))
        m.activeField = i;
    bool act = (m.activeField == i);
    DrawInputBox(fields[i].label, fields[i].buf, r, act);
}

// ── Category selection chips (replaces free-text category entry) ──
static const char* CATS[] = { "Study", "Work", "Personal", "Health", "Other" };
const int CAT_COUNT = 5;
float caty = fy + 3*76.0f;
DrawText("Category", (int)fx, (int)caty, 13, C_TEXT_DIM);
caty += 20;
float chipX = fx;
for (int i = 0; i < CAT_COUNT; i++) {
    int tw = MeasureText(CATS[i], 13);
    float chipW = tw + 24.0f;
    if (chipX + chipW > fx + fw) { chipX = fx; caty += 36; }
    Rectangle chip = { chipX, caty, chipW, 30 };
    bool chipHov = CheckCollisionPointRec(mouse, chip);
    bool chipSel = (strcmp(m.tfCat, CATS[i]) == 0);
    DrawRoundRect(chip, 0.5f, chipSel ? C_ORANGE : (chipHov ? C_CARD_HOV : C_DARKGRAY));
    DrawRoundRectLines(chip, 0.5f, 1.0f, chipSel ? C_ORANGE : C_BORDER);
    DrawText(CATS[i], (int)chip.x + 12, (int)chip.y + 8, 13, chipSel ? C_BG : C_WHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && chipHov) {
        snprintf(m.tfCat, sizeof(m.tfCat), "%s", CATS[i]);
    }
    chipX += chipW + 8;
}
    // Right column: clickable mini calendar for choosing the due date
    float cx = fx + fw + 32, cy = box.y + 66;
    float cw = box.width - (cx - box.x) - 24;
    DrawText("Due Date", (int)cx, (int)cy - 22, 14, C_TEXT_DIM);
    Rectangle calArea = {cx, cy, cw, 250};
    DrawRoundRect(calArea, 0.06f, C_DARKGRAY);
    DrawRoundRectLines(calArea, 0.06f, 1.0f, C_BORDER);
    DrawMiniCalendarPicker(m, {calArea.x+10, calArea.y+8, calArea.width-20, calArea.height-16}, mouse);

    string chosenTxt = "No date selected yet";
    if (m.pickDay > 0)
        chosenTxt = "Selected: " + calMonthName(m.pickMonth) + " " + to_string(m.pickDay) + ", " + to_string(m.pickYear);
    DrawText(chosenTxt.c_str(), (int)cx, (int)(calArea.y + calArea.height + 12), 13,
             m.pickDay > 0 ? C_GREEN : C_TEXT_DIM);

    // Error message
    if (!m.errorMsg.empty())
        DrawText(m.errorMsg.c_str(), (int)fx, (int)(box.y + box.height - 80), 13, C_RED);

    // Buttons
    Rectangle addBtn = {fx, box.y + box.height - 52, fw/2 - 8, 38};
    Rectangle canBtn = {fx + fw/2 + 8, box.y + box.height - 52, fw/2 - 8, 38};

    bool addHov = CheckCollisionPointRec(mouse, addBtn);
    bool canHov = CheckCollisionPointRec(mouse, canBtn);

    DrawRoundRect(addBtn, 0.2f, addHov ? C_ORANGE : C_DARKGRAY);
    DrawText("Add Task", (int)addBtn.x + 16, (int)addBtn.y + 10, 16,
             addHov ? C_BG : C_TEXT_DIM);

    DrawRoundRect(canBtn, 0.2f, canHov ? C_RED : C_DARKGRAY);
    DrawText("Cancel", (int)canBtn.x + 20, (int)canBtn.y + 10, 16,
             canHov ? C_WHITE : C_TEXT_DIM);

    // Keyboard input for active field: Tab / Shift+Tab cycles fields,
    // Enter submits the form, Escape cancels — same as clicking the buttons.
    if (IsKeyPressed(KEY_TAB)) {
        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        m.activeField = shiftDown ? (m.activeField + 2) % 3 : (m.activeField + 1) % 3;
    }
    HandleTextInput(fields[m.activeField].buf,
                    fields[m.activeField].maxLen,
                    IsKeyPressedRepeat(KEY_BACKSPACE) ? KEY_BACKSPACE : 0);

    bool enterConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool escCancel    = IsKeyPressed(KEY_ESCAPE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || enterConfirm || escCancel) {
        addHov = addHov || enterConfirm;
        canHov = canHov || escCancel;
        if (addHov) {
            // Validate
            if (TextLength(m.tfName) == 0) {
                m.errorMsg = "Task Name is required.";
            } else if (m.pickDay <= 0) {
                m.errorMsg = "Please choose a due date on the mini calendar.";
            } else {
                Task t;
                t.taskId          = nextAvailableTaskId();  // auto-assigned — never asked from the user
                t.taskName        = m.tfName;
                t.taskDescription = m.tfDesc;
                t.taskCategory    = m.tfCat;
                t.priority        = TextLength(m.tfPri) > 0 ? max(1, min(5, atoi(m.tfPri))) : 3;
                t.isCompleted     = false;
                t.dateCompleted   = "";
                t.dueDay = m.pickDay; t.dueMonth = m.pickMonth; t.dueYear = m.pickYear;

                // daysToComplete kept in sync for the Edit modal's day-shift logic
                tm due_tm = {0};
                due_tm.tm_year = t.dueYear - 1900; due_tm.tm_mon = t.dueMonth - 1; due_tm.tm_mday = t.dueDay;
                time_t dueTime = mktime(&due_tm);
                time_t nowTime = time(0);
                t.daysToComplete = max(0, (int)round(difftime(dueTime, nowTime) / 86400.0));

                time_t rawtime; time(&rawtime);
                char buf[32]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M",localtime(&rawtime));
                t.dateCreated = buf;
                allTasks.push_back(t);
                saveUserData();
                // reset modal
                m.open = false;
                memset(m.tfName,0,sizeof(m.tfName));
                memset(m.tfDesc,0,sizeof(m.tfDesc)); memset(m.tfCat,0,sizeof(m.tfCat));
                memset(m.tfPri,0,sizeof(m.tfPri));
                m.errorMsg = ""; m.activeField = 0;
                m.pickDay = -1;
            }
        }
        if (canHov) {
            m.open = false; m.errorMsg = "";
            m.activeField = 0;
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  Edit Task modal (priority, type/category, days only)
// ─────────────────────────────────────────────────────────────
static void DrawEditModal(ModalState& m, Vector2 mouse) {
    DrawRectangle(0, 0, WIN_W, WIN_H, {0,0,0,160});

    Rectangle box = {WIN_W/2 - 240.0f, WIN_H/2 - 180.0f, 480, 360};
    DrawRoundRect(box, 0.04f, C_PANEL);
    DrawRoundRectLines(box, 0.04f, 1.5f, C_TEAL);

    if (m.taskIdx < 0 || m.taskIdx >= (int)allTasks.size()) { m.open=false; return; }
    Task& t = allTasks[m.taskIdx];

    string heading = "Edit Task: " + t.taskName;
    DrawText(heading.c_str(), (int)box.x + 20, (int)box.y + 16, 17, C_TEAL);

    float fx = box.x + 20, fw = box.width - 40;

    Rectangle r0 = {fx, box.y + 60, fw, 36};
    Rectangle r1 = {fx, box.y + 136, fw, 36};
    Rectangle r2 = {fx, box.y + 212, fw, 36};

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, r0)) m.activeField = 0;
        if (CheckCollisionPointRec(mouse, r1)) m.activeField = 1;
        if (CheckCollisionPointRec(mouse, r2)) m.activeField = 2;
    }

    DrawInputBox("Priority (1-5)",   m.efPri,  r0, m.activeField==0, C_TEAL);
    DrawInputBox("Category / Type",  m.efCat,  r1, m.activeField==1, C_TEAL);
    DrawInputBox("Days to Due Date", m.efDays, r2, m.activeField==2, C_TEAL);

    char* bufs[3] = {m.efPri, m.efCat, m.efDays};
    int   maxL[3] = {4, 32, 8};
    HandleTextInput(bufs[m.activeField], maxL[m.activeField],
                    IsKeyPressedRepeat(KEY_BACKSPACE) ? KEY_BACKSPACE : 0);
    if (IsKeyPressed(KEY_TAB)) {
        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        m.activeField = shiftDown ? (m.activeField + 2) % 3 : (m.activeField + 1) % 3;
    }

    Rectangle saveBtn = {fx, box.y + box.height - 52, fw/2-8, 38};
    Rectangle canBtn  = {fx+fw/2+8, box.y+box.height-52, fw/2-8, 38};
    bool sh = CheckCollisionPointRec(mouse,saveBtn);
    bool ch = CheckCollisionPointRec(mouse,canBtn);

    DrawRoundRect(saveBtn, 0.2f, sh ? C_TEAL : C_DARKGRAY);
    DrawText("Save", (int)saveBtn.x+20,(int)saveBtn.y+10,16, sh?C_BG:C_TEXT_DIM);
    DrawRoundRect(canBtn,  0.2f, ch ? C_RED  : C_DARKGRAY);
    DrawText("Cancel",(int)canBtn.x+12,(int)canBtn.y+10,16, ch?C_WHITE:C_TEXT_DIM);

    // Enter saves, Escape cancels — same behavior as clicking the buttons
    bool enterConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool escCancel    = IsKeyPressed(KEY_ESCAPE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || enterConfirm || escCancel) {
        sh = sh || enterConfirm;
        ch = ch || escCancel;
        if (sh) {
            if (TextLength(m.efPri)  > 0) t.priority     = max(1,min(5,atoi(m.efPri)));
            if (TextLength(m.efCat)  > 0) t.taskCategory = m.efCat;
            if (TextLength(m.efDays) > 0) {
                t.daysToComplete = max(1, atoi(m.efDays));
                time_t now = time(0); now += t.daysToComplete*86400;
                tm* dl = localtime(&now);
                t.dueDay=dl->tm_mday; t.dueMonth=dl->tm_mon+1; t.dueYear=dl->tm_year+1900;
            }
            saveUserData();
            m.open=false; m.activeField=0;
        }
        if (ch) { m.open=false; m.activeField=0; }
    }
}

// ─────────────────────────────────────────────────────────────
//  Task card (shared renderer)
// ─────────────────────────────────────────────────────────────
// 1. KEEP THIS: The compiler needs to see this definition first
enum CardMode { CARD_DASH, CARD_FOCUS, CARD_STATS };

// 2. KEEP THIS: Notice the Capital 'C' and Capital 'A'
struct CardAction { 
    bool doDelete   = false; 
    bool doEdit     = false;
    bool doFocus    = false;  
    bool doStats    = false; 
    bool doComplete = false;   // Mark-as-done button pressed directly from the task card
};

// 3. THE UPDATED FUNCTION: Placed directly below the struct
static CardAction DrawTaskCard(const Task& t, int idx, float x, float y, float w,
                               Vector2 mouse, CardMode mode) {
    CardAction act; // Tracks the button click states to pass back safely
    float h = (mode == CARD_STATS) ? 130.0f : 88.0f;
    Rectangle card = {x, y, w, h};
    bool hov = CheckCollisionPointRec(mouse, card);
    DrawRoundRect(card, 0.08f, hov ? C_CARD_HOV : C_CARD);
    DrawRoundRectLines(card, 0.08f, 1.0f, C_BORDER);

    // Priority indicator line on left edge
    Color pc = priorityColor(t.priority);
    DrawRoundRect({x+4, y+8, 4, h-16}, 0.5f, pc);

    // Priority badge container
    Rectangle pb = {x+18, y+10, 72, 22};
    DrawRoundRect(pb, 0.4f, {pc.r, pc.g, pc.b, 60});
    DrawText(priorityLabel(t.priority), (int)pb.x+6, (int)pb.y+4, 12, pc);

    // Task name text assignment
    DrawText(t.taskName.c_str(), (int)x+18, (int)y+38, 16, C_WHITE);

    // Description text (safely truncated to prevent clipping overflows)
    string desc = t.taskDescription;
    if (desc.size() > 55) desc = desc.substr(0,52) + "...";
    DrawText(desc.c_str(), (int)x+18, (int)y+60, 12, C_TEXT_DIM);

    // ── Optimized Meta Placement (Date & Category Row) ──
    // Shifted left to x + w - 155 to prevent any stacking collisions with hover actions
    string due = fmtDate(t.dueDay, t.dueMonth, t.dueYear);
    float anchorX = x + w - 155;
    
    DrawTextureEx(texCalendar, { anchorX - 22, (float)y + 14 }, 0.0f, 0.04f, WHITE);
    DrawText(due.c_str(), (int)anchorX, (int)y + 16, 12, C_TEXT_DIM);
    
    if (!t.taskCategory.empty()) {
        DrawText(t.taskCategory.c_str(), (int)anchorX, (int)y + 36, 11, C_ORANGE); 
    }

    // ── Always-visible Mark Done button (Dashboard task cards) ──
    if (mode == CARD_DASH) {
        Rectangle doneBtn = { x + w - 118, y + h - 32, 100, 24 };
        bool doneHov = CheckCollisionPointRec(mouse, doneBtn);
        if (t.isCompleted) {
            DrawRoundRect(doneBtn, 0.3f, { C_GREEN.r, C_GREEN.g, C_GREEN.b, 40 });
            DrawRoundRectLines(doneBtn, 0.3f, 1.0f, C_GREEN);
            DrawText("Completed", (int)doneBtn.x + 12, (int)doneBtn.y + 5, 12, C_GREEN);
        } else {
            DrawRoundRect(doneBtn, 0.3f, doneHov ? C_GREEN : C_DARKGRAY);
            DrawRoundRectLines(doneBtn, 0.3f, 1.0f, doneHov ? C_GREEN : C_BORDER);
            DrawText("Mark Done", (int)doneBtn.x + 14, (int)doneBtn.y + 5, 12, doneHov ? C_BG : C_WHITE);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && doneHov) act.doComplete = true;
        }
    }

    // ── Action Buttons Control Layer (Renders purely on element hover) ──
    if (hov) {
        if (mode == CARD_DASH) {
            // Edit icon asset (Top Right Corner stack placement)
            Rectangle edBtn = {x + w - 36, y + 14, 24, 24}; 
            bool eh = CheckCollisionPointRec(mouse, edBtn);
            DrawTextureEx(texEdit, {edBtn.x, edBtn.y}, 0.0f, 1.5f, eh ? LIGHTGRAY : WHITE);
            
            if (eh) {
                Rectangle tip = {edBtn.x - 48, edBtn.y + 2, 42, 20};
                DrawRoundRect(tip, 0.3f, C_DARKGRAY);
                DrawText("Edit", (int)tip.x + 8, (int)tip.y + 3, 11, C_WHITE);
            }

            // Delete icon asset (Stacked directly underneath edit button row)
            Rectangle delBtn = {x + w - 36, y + 46, 24, 24}; 
            bool dh = CheckCollisionPointRec(mouse, delBtn);
            DrawTextureEx(texDelete, {delBtn.x, delBtn.y+3}, 0.0f, 0.1f, dh ? LIGHTGRAY : WHITE);
            
            if (dh) {
                Rectangle tip = {delBtn.x - 58, delBtn.y + 2, 52, 20};
                DrawRoundRect(tip, 0.3f, C_DARKGRAY);
                DrawText("Delete", (int)tip.x + 8, (int)tip.y + 3, 11, C_WHITE);
            }
            
            // Mouse event captures tracking return flags safely
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (dh) act.doDelete = true;
                if (eh) act.doEdit   = true;
            }
        }
        else if (mode == CARD_FOCUS) {
            // Focus execution mode action trigger layout (Clock asset link)
            Rectangle focBtn = { x + w - 36, y + 32, 24, 24 }; 
            bool fh = CheckCollisionPointRec(mouse, focBtn);
            DrawTextureEx(texClock, { focBtn.x, focBtn.y }, 0.0f, 0.05f, fh ? LIGHTGRAY : WHITE);
            
            if (fh) {
                Rectangle tip = { focBtn.x - 52, focBtn.y - 24, 62, 20 };
                DrawRoundRect(tip, 0.3f, C_DARKGRAY);
                DrawText("Enter", (int)tip.x + 8, (int)tip.y + 3, 11, C_WHITE);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && fh) {
                act.doFocus = true;
            }
        }
    }

    // Statistics layout row tracking metrics logic
    if (mode == CARD_STATS) {
        DrawLine((int)x+10, (int)(y+92), (int)(x+w-10), (int)(y+92), C_BORDER);
        string info = "ID: " + to_string(t.taskId) +
                      "  |  Created: " + t.dateCreated +
                      "  |  Due: " + fmtDate(t.dueDay,t.dueMonth,t.dueYear) +
                      "  |  " + (t.isCompleted ? "COMPLETED" : "PENDING");
        DrawText(info.c_str(), (int)x+12, (int)y+100, 11, C_TEXT_DIM);
    }

    return act; // Passes clean action state struct data flags back to core application loops
}

// ─────────────────────────────────────────────────────────────
//  Scrollable task list (Dashboard / Focus / Stats)
// ─────────────────────────────────────────────────────────────
static void DrawTaskList(vector<Task>& tasks, float x, float y,
                         float w, float h, float& scroll,
                         Vector2 mouse, CardMode mode,
                         ModalState& modal,
                         int& focusTaskIdx) {
    // Scissor clip to list area
    BeginScissorMode((int)x, (int)y, (int)w, (int)h);

    float cardH = (mode==CARD_STATS) ? 134.0f : 92.0f;
    float gap   = 10.0f;
    float totalH = tasks.size() * (cardH + gap);

    // Scroll via mouse wheel when hovering over list area
    Rectangle listRect = {x,y,w,h};
    if (CheckCollisionPointRec(mouse, listRect)) {
        scroll -= GetMouseWheelMove() * 40.0f;
    }
    scroll = max(0.0f, min(scroll, max(0.0f, totalH - h)));

    float cy = y - scroll;
    for (int i = 0; i < (int)tasks.size(); i++) {
        if (cy + cardH < y || cy > y + h) { cy += cardH + gap; continue; }
        CardAction act = DrawTaskCard(tasks[i], i, x+4, cy, w-8, mouse, mode);
        if (act.doComplete) {
            int id = tasks[i].taskId;
            for (auto& t : allTasks) {
                if (t.taskId == id && !t.isCompleted) {
                    t.isCompleted = true;
                    t.dateCompleted = todayDateString();
                    totalXP += xpForPriority(t.priority);
                    saveUserData();
                    break;
                }
            }
        }
      if (act.doDelete) {
            // Don't delete immediately — open a confirmation popup instead.
            modal.confirmDeleteOpen = true;
            modal.confirmDeleteId   = tasks[i].taskId;
            modal.confirmDeleteName = tasks[i].taskName;
        }
        if (act.doEdit) {
            // find real index in allTasks by ID
            int id = tasks[i].taskId;
            modal.taskIdx = -1;
            for (int j=0;j<(int)allTasks.size();j++) {
                if (allTasks[j].taskId==id) { modal.taskIdx=j; break; }
            }
            memset(modal.efPri,0,sizeof(modal.efPri));
            memset(modal.efCat,0,sizeof(modal.efCat));
            memset(modal.efDays,0,sizeof(modal.efDays));
            modal.open=true; modal.activeField=0;
        }
        if (act.doFocus) {
            int id = tasks[i].taskId;
            for (int j=0;j<(int)allTasks.size();j++) {
                if (allTasks[j].taskId==id) { focusTaskIdx=j; break; }
            }
        }
        cy += cardH + gap;
    }

    EndScissorMode();

   // Scroll bar
    if (totalH > h) {
        float sbH = (h/totalH)*h;
        float sbY = y + (scroll/totalH)*h;
        DrawRectangle((int)(x+w-6),(int)sbY, 4,(int)sbH, C_BORDER);
    }
}

// ─────────────────────────────────────────────────────────────
//  Delete-confirmation popup ("Are you sure you want to remove
//  this task?"). Shown on top of everything else whenever
//  modal.confirmDeleteOpen is true. Actual deletion + save only
//  happens if the user presses "Delete" (or Enter); "Cancel"
//  (or Escape) just closes the popup and keeps the task.
// ─────────────────────────────────────────────────────────────
static void DrawConfirmDeleteModal(ModalState& m, Vector2 mouse) {
    DrawRectangle(0, 0, WIN_W, WIN_H, {0, 0, 0, 160});

    Rectangle box = { WIN_W/2 - 200.0f, WIN_H/2 - 100.0f, 400, 200 };
    DrawRoundRect(box, 0.06f, C_PANEL);
    DrawRoundRectLines(box, 0.06f, 1.5f, C_RED);

    DrawText("Remove Task?", (int)box.x + 24, (int)box.y + 20, 18, C_WHITE);

    string msg = "Are you sure you want to remove \"" + m.confirmDeleteName + "\"?";
    DrawWrappedText(msg.c_str(), (int)box.x + 24, (int)box.y + 54, (int)box.width - 48, 14, C_TEXT_DIM);
    DrawText("This cannot be undone.", (int)box.x + 24, (int)box.y + 100, 12, C_RED);

    Rectangle delBtn = { box.x + 24, box.y + box.height - 52, box.width/2 - 32, 38 };
    Rectangle canBtn  = { box.x + box.width/2 + 8, box.y + box.height - 52, box.width/2 - 32, 38 };
    bool delHov = CheckCollisionPointRec(mouse, delBtn);
    bool canHov = CheckCollisionPointRec(mouse, canBtn);

    DrawRoundRect(delBtn, 0.2f, delHov ? C_RED : C_DARKGRAY);
    DrawText("Delete", (int)delBtn.x + 20, (int)delBtn.y + 10, 16, delHov ? C_WHITE : C_TEXT_DIM);

    DrawRoundRect(canBtn, 0.2f, canHov ? C_GREEN : C_DARKGRAY);
    DrawText("Cancel", (int)canBtn.x + 20, (int)canBtn.y + 10, 16, canHov ? C_BG : C_TEXT_DIM);

    bool enterConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool escCancel    = IsKeyPressed(KEY_ESCAPE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || enterConfirm || escCancel) {
        delHov = delHov || enterConfirm;
        canHov = canHov || escCancel;
        if (delHov) {
            int id = m.confirmDeleteId;
            allTasks.erase(remove_if(allTasks.begin(), allTasks.end(),
                [id](const Task& t){ return t.taskId == id; }), allTasks.end());
            saveUserData();
        }
        if (delHov || canHov) {
            m.confirmDeleteOpen = false;
            m.confirmDeleteId   = -1;
            m.confirmDeleteName = "";
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  Focus Mode right panel
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────
//  Focus Mode right panel
// ─────────────────────────────────────────────────────────────
static void DrawFocusPanel(int taskIdx, FocusTimer& ft, Vector2 mouse) {
    float px = WIN_W / 2.0f;
    float pw = WIN_W - px;

    DrawRectangle((int)px, TOP_H, (int)pw, WIN_H-TOP_H, {8,12,20,255});
    DrawLine((int)px, TOP_H, (int)px, WIN_H, C_BORDER);

    float cx = px + pw/2.0f;
    float cy = TOP_H + 120.0f;

    // Circular timer ring
    float radius = 90.0f;
    DrawCircleLines((int)cx,(int)cy,(int)radius+6, C_BORDER);
    DrawCircleLines((int)cx,(int)cy,(int)radius,   C_BORDER);

    float progress = 0.0f;
    if (!ft.inputMode && ft.totalSecs > 0)
        progress = (float)ft.elapsed / (float)ft.totalSecs;

    // Arc — approximate with line segments
    int segs = 120;
    for (int i = 0; i < segs; i++) {
        float a0 = -PI/2 + (2*PI * i/segs);
        float a1 = -PI/2 + (2*PI * (i+1)/segs);
        if ((float)i/segs > progress) break;
        Color arc = { (unsigned char)(57*(1-progress)+0*progress),
                      (unsigned char)(211*(1-progress)+255*progress),
                      (unsigned char)(83*(1-progress)+65*progress), 255 };
        DrawLineEx({cx + cosf(a0)*radius, cy + sinf(a0)*radius},
                   {cx + cosf(a1)*radius, cy + sinf(a1)*radius},
                   3.5f, arc);
    }

    // Timer display
    if (ft.inputMode) {
        DrawText("Set Minutes:", (int)cx-55, (int)cy-36, 14, C_TEXT_DIM);
        Rectangle minBox = {cx-50, cy-16, 100, 36};
        DrawInputBox("", ft.setMin, minBox, true, C_GREEN);
        HandleTextInput(ft.setMin, 6, IsKeyPressedRepeat(KEY_BACKSPACE)?KEY_BACKSPACE:0);

        Rectangle startBtn = {cx-55, cy+36, 110, 36};
        bool sh = CheckCollisionPointRec(mouse, startBtn);
        DrawRoundRect(startBtn, 0.3f, sh?C_GREEN:C_DARKGRAY);
        DrawText("Start", (int)startBtn.x+28,(int)startBtn.y+9, 15,
                 sh?C_BG:C_TEXT_DIM);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sh) {
            ft.totalSecs  = max(1, atoi(ft.setMin)) * 60;
            ft.elapsed    = 0;
            ft.startStamp = GetTime();
            ft.running    = true;
            ft.inputMode  = false;
        }
    } else {
        // Running timer
        if (ft.running) ft.elapsed = (int)(GetTime() - ft.startStamp);
        if (ft.elapsed >= ft.totalSecs) { ft.running=false; ft.elapsed=ft.totalSecs; }

        int rem = ft.totalSecs - ft.elapsed;
        char tstr[12];
        snprintf(tstr, sizeof(tstr), "%02d:%02d", rem/60, rem%60);
        int tw = MeasureText(tstr, 42);
        DrawText(tstr, (int)cx - tw/2, (int)cy-24, 42, ft.running?C_GREEN:C_YELLOW);

        DrawText("FOCUS", (int)cx-22, (int)cy+30, 13, C_TEXT_DIM);

        // Pause/Resume & Reset
        Rectangle pb = {cx-62, cy+56, 56, 30};
        Rectangle rb = {cx+8,  cy+56, 56, 30};
        bool ph = CheckCollisionPointRec(mouse,pb);
        bool rh = CheckCollisionPointRec(mouse,rb);
        DrawRoundRect(pb,0.3f, ph?C_GREEN:C_DARKGRAY);
        DrawText(ft.running?"Pause":"Resume",(int)pb.x+4,(int)pb.y+7,12,ph?C_BG:C_TEXT_DIM);
        DrawRoundRect(rb,0.3f, rh?C_ORANGE:C_DARKGRAY);
        DrawText("Reset",(int)rb.x+10,(int)rb.y+7,12,rh?C_BG:C_TEXT_DIM);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (ph) ft.running = !ft.running;
            if (rh) { ft.inputMode=true; ft.running=false; ft.elapsed=0; }
        }
    }

    // Task info below timer
    if (taskIdx >= 0 && taskIdx < (int)allTasks.size()) {
        Task& t = allTasks[taskIdx]; // FIX: Removed const so we can change completion status
        float ty = cy + 170.0f;
        int nw = MeasureText(t.taskName.c_str(), 18);
        DrawText(t.taskName.c_str(), (int)(cx-nw/2), (int)ty, 18, C_WHITE);
        string desc = t.taskDescription;
        if (desc.size()>60) desc=desc.substr(0,57)+"...";
        int dw = MeasureText(desc.c_str(), 13);
        DrawText(desc.c_str(), (int)(cx-dw/2), (int)ty+28, 13, C_TEXT_DIM);
        
        // Type tag
        if (!t.taskCategory.empty()) {
            int tw2 = MeasureText(t.taskCategory.c_str(),13)+16;
            Rectangle tr = {cx-tw2/2.0f, ty+54, (float)tw2, 24};
            DrawRoundRect(tr,0.4f,C_DARKGRAY);
            DrawText(t.taskCategory.c_str(),(int)tr.x+8,(int)tr.y+5,13,C_TEAL);
        }

        // ── ADDED: TASK COMPLETION ACTION UI ──
        if (!t.isCompleted) {
            Rectangle doneBtn = { cx - 70, ty + 90, 140, 34 };
            bool dbHov = CheckCollisionPointRec(mouse, doneBtn);
            DrawRoundRect(doneBtn, 0.3f, dbHov ? C_GREEN : C_DARKGRAY);
            DrawRoundRectLines(doneBtn, 0.3f, 1.0f, dbHov ? C_GREEN : C_BORDER);
            DrawText("Mark as Done", (int)doneBtn.x + 22, (int)doneBtn.y + 9, 14, dbHov ? C_BG : C_WHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && dbHov) {
                t.isCompleted = true;
                t.dateCompleted = todayDateString();
                totalXP += xpForPriority(t.priority);
                saveUserData(); // Instantly write the new data state to disk
            }
        } else {
            Rectangle doneBadge = { cx - 60, ty + 90, 120, 30 };
            DrawRoundRect(doneBadge, 0.3f, { 0, 255, 65, 40 }); // Semi-transparent green background fill
            DrawRoundRectLines(doneBadge, 0.3f, 1.0f, C_GREEN);
            DrawText("Completed", (int)doneBadge.x + 16, (int)doneBadge.y + 8, 14, C_GREEN);
        }
    } else {
        DrawText("Select a task from the list ", (int)(cx-100),(int)(cy+170),13,C_TEXT_DIM);
    }
}

// ─────────────────────────────────────────────────────────────
//  Today's Due Tasks panel — ring-style completion indicator
//  plus a quick-complete list, shown at the top of the Dashboard
// ─────────────────────────────────────────────────────────────
static void DrawTodayPanel(float x, float y, float w, float h, Vector2 mouse) {
    time_t now = time(0);
    tm* lt = localtime(&now);
    int td = lt->tm_mday, tmo = lt->tm_mon + 1, ty = lt->tm_year + 1900;

    vector<int> todayIdx;
    for (int i = 0; i < (int)allTasks.size(); i++) {
        if (allTasks[i].dueDay == td && allTasks[i].dueMonth == tmo && allTasks[i].dueYear == ty)
            todayIdx.push_back(i);
    }
    int total = (int)todayIdx.size();
    int done = 0;
    for (int i : todayIdx) if (allTasks[i].isCompleted) done++;

    Rectangle box = {x, y, w, h};
    DrawRoundRect(box, 0.05f, C_PANEL);
    DrawRoundRectLines(box, 0.05f, 1.0f, C_BORDER);
    DrawText("Today's Due Tasks", (int)x + 20, (int)y + 12, 16, C_WHITE);

    // Completion ring
    float rcx = x + 76, rcy = y + h/2 + 8, rad = 44;
    DrawCircleLines((int)rcx, (int)rcy, (int)rad + 5, C_BORDER);
    float progress = total > 0 ? (float)done / (float)total : 0.0f;
    int segs = 100;
    for (int i = 0; i < segs; i++) {
        if ((float)i/segs > progress) break;
        float a0 = -PI/2 + (2*PI * i/segs);
        float a1 = -PI/2 + (2*PI * (i+1)/segs);
        DrawLineEx({rcx + cosf(a0)*rad, rcy + sinf(a0)*rad},
                   {rcx + cosf(a1)*rad, rcy + sinf(a1)*rad}, 6.0f, C_GREEN);
    }
    char ringTxt[16]; snprintf(ringTxt, sizeof(ringTxt), "%d/%d", done, total);
    int rw = MeasureText(ringTxt, 20);
    DrawText(ringTxt, (int)(rcx - rw/2), (int)(rcy - 11), 20, C_WHITE);
    DrawText("done today", (int)(rcx - 32), (int)(rcy + 14), 11, C_TEXT_DIM);

    // Quick list with tap-to-complete checkboxes
    float lx2 = x + 160, ly2 = y + 40;
    if (total == 0) {
        DrawText("Nothing due today — enjoy the calm!", (int)lx2, (int)(y + h/2 - 8), 14, C_TEXT_DIM);
    } else {
        int shown = 0;
        for (int idx : todayIdx) {
            if (shown >= 3) {
                char more[32]; snprintf(more, sizeof(more), "+ %d more due today", total - shown);
                DrawText(more, (int)lx2, (int)(ly2 + shown*30), 12, C_TEXT_DIM);
                break;
            }
            Task& t = allTasks[idx];
            Rectangle chk = {lx2, ly2 + shown*30, 18, 18};
            bool chkHov = CheckCollisionPointRec(mouse, chk);
            if (t.isCompleted) {
                DrawRoundRect(chk, 0.3f, C_GREEN);
                DrawLineEx({chk.x+4, chk.y+9}, {chk.x+7, chk.y+13}, 2.0f, C_BG);
                DrawLineEx({chk.x+7, chk.y+13}, {chk.x+14, chk.y+4}, 2.0f, C_BG);
            } else {
                DrawRoundRectLines(chk, 0.3f, 1.5f, chkHov ? C_GREEN : C_BORDER);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && chkHov) {
                    t.isCompleted = true;
                    t.dateCompleted = todayDateString();
                    totalXP += xpForPriority(t.priority);
                    saveUserData();
                }
            }
            Color nameCol = t.isCompleted ? C_TEXT_DIM : C_WHITE;
            string nm = t.taskName;
            if (nm.size() > 34) nm = nm.substr(0,31) + "...";
            DrawText(nm.c_str(), (int)chk.x + 26, (int)chk.y, 14, nameCol);
            const char* pr = priorityLabel(t.priority);
            int pw = MeasureText(pr, 11);
            DrawText(pr, (int)(x + w - pw - 24), (int)chk.y + 2, 11, priorityColor(t.priority));
            shown++;
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  Dashboard view
// ─────────────────────────────────────────────────────────────
static void DrawDashView(float& scroll, SortMode& sortMode, ModalState& modal, Vector2 mouse) {
    float lx = SIDE_W + 12, ly = TOP_H + 16;
    float lw = WIN_W - SIDE_W - 24, lh = WIN_H - TOP_H - 24;

    // ── Today's Due Tasks summary (ring + quick list) ──
    float todayH = 138.0f;
    DrawTodayPanel(lx, ly, lw, todayH, mouse);
    float listTop = ly + todayH + 18;

    // Header row
    DrawText("Active Tasks", (int)lx, (int)listTop, 20, C_WHITE);

    // ── Unified Single Sort Button ──
    // Positioned neatly to the left of the "+ Task" button
    Rectangle sortBtn = {(float)(WIN_W - 260), (float)(listTop + 2), 150, 28};
    bool sortHov = CheckCollisionPointRec(mouse, sortBtn);
    
    // Dynamic text options cycling using "v" as the arrow indicator
    const char* sortText = "Sort: None v";
    if (sortMode == SORT_PRIORITY) sortText = "Sort: Priority v";
    else if (sortMode == SORT_DUE)       sortText = "Sort: Due Date v";

    // Renders matching background highlights when sorting is actively enabled
    DrawRoundRect(sortBtn, 0.3f, (sortMode != SORT_NONE) ? C_ORANGE : C_DARKGRAY);
    DrawRoundRectLines(sortBtn, 0.3f, 1.0f, sortHov ? C_ORANGE : C_BORDER);
    DrawText(sortText, (int)sortBtn.x + 12, (int)sortBtn.y + 6, 13,
             (sortMode != SORT_NONE) ? C_BG : C_TEXT_DIM);

    // Click interactive cycle logic handler
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sortHov) {
        if (sortMode == SORT_NONE)          sortMode = SORT_PRIORITY;
        else if (sortMode == SORT_PRIORITY) sortMode = SORT_DUE;
        else                                sortMode = SORT_NONE;
    }

    // ── New Task Button (Kept completely intact) ──
    Rectangle nb = {(float)(WIN_W-96),(float)(listTop-2), 84, 34};
    bool nh = CheckCollisionPointRec(mouse,nb);
    DrawRoundRect(nb, 0.3f, nh?C_GREEN:C_ORANGE);
    DrawText("+ Task", (int)nb.x+10,(int)nb.y+9, 14, C_BG);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && nh) {
        modal.open=true; modal.taskIdx=-1; modal.activeField=0; modal.errorMsg="";
        modal.pickDay=-1;
        time_t nt = time(0); tm* nl = localtime(&nt);
        modal.navMonth = nl->tm_mon+1; modal.navYear = nl->tm_year+1900;
    }

    // ── Build Sorted List Vector ──
    vector<Task> sorted = allTasks;
    if (sortMode==SORT_PRIORITY)
        sort(sorted.begin(),sorted.end(),[](const Task& a,const Task& b){ return a.priority>b.priority; });
    else if (sortMode==SORT_DUE)
        sort(sorted.begin(),sorted.end(),[](const Task& a,const Task& b){
            if (a.dueYear!=b.dueYear) return a.dueYear<b.dueYear;
            if (a.dueMonth!=b.dueMonth) return a.dueMonth<b.dueMonth;
            return a.dueDay<b.dueDay;
        });

    int dummy = -1;
    DrawTaskList(sorted, lx, listTop+44, lw, (ly+lh)-(listTop+44), scroll, mouse,
                 CARD_DASH, modal, dummy);
}

// ─────────────────────────────────────────────────────────────
//  Focus view
// ─────────────────────────────────────────────────────────────
static void DrawFocusView(float& scroll, ModalState& modal,
                          FocusTimer& ft, int& focusTaskIdx, Vector2 mouse) {
    float lx = SIDE_W + 12, ly = TOP_H + 16;
    float lw = (WIN_W/2.0f) - SIDE_W - 20;
    float lh = WIN_H - TOP_H - 24;

    DrawText("Focus Mode", (int)lx, (int)ly, 18, C_WHITE);
    // FIX: Removed the emoji and just changed the text to say "the clock icon"
    DrawText("Hover a task and click the clock icon to enter focus", (int)lx, (int)ly+26, 12, C_TEXT_DIM);

    DrawTaskList(allTasks, lx, ly+50, lw, lh-50, scroll, mouse,
                 CARD_FOCUS, modal, focusTaskIdx);

    DrawFocusPanel(focusTaskIdx, ft, mouse);
}

// ─────────────────────────────────────────────────────────────
//  Line-graph helper — generic, reused for every graph in the
//  Statistics view (small side panels AND the large/maximized
//  primary completion graph — same function, just bigger w/h).
// ─────────────────────────────────────────────────────────────
static string calFmtISO(tm t) {
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
    return string(buf);
}
static string calFmtShort(tm t) {
    char buf[8];
    strftime(buf, sizeof(buf), "%d/%m", &t);
    return string(buf);
}
static void BuildDateRange(int fromOffsetDays, int toOffsetDays, vector<tm>& out) {
    time_t now = time(0);
    for (int i = fromOffsetDays; i <= toOffsetDays; i++) {
        time_t tt = now + i * 86400;
        tm* lt = localtime(&tt);
        out.push_back(*lt);
    }
}

// Parse a "YYYY-MM-DD" dateCompleted string to a time_t (noon, to dodge DST edges)
static time_t isoToTime(const string& iso) {
    int y = 0, m = 0, d = 0;
    sscanf(iso.c_str(), "%d-%d-%d", &y, &m, &d);
    tm t = {0};
    t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d; t.tm_hour = 12;
    return mktime(&t);
}

// Daily: one point per day for the last `days` days (including today)
static void BuildDailyCompletion(int days, vector<float>& vals, vector<string>& labels) {
    vector<tm> dates;
    BuildDateRange(-(days - 1), 0, dates);
    for (auto& d : dates) {
        string iso = calFmtISO(d);
        int c = 0;
        for (auto& t : allTasks) if (t.isCompleted && t.dateCompleted == iso) c++;
        vals.push_back((float)c);
        labels.push_back(calFmtShort(d));
    }
}

// Weekly: one point per 7-day bucket for the last `weeks` weeks (most recent bucket ends today)
static void BuildWeeklyCompletion(int weeks, vector<float>& vals, vector<string>& labels) {
    time_t now = time(0);
    tm* nowTm = localtime(&now);
    tm todayNoon = *nowTm; todayNoon.tm_hour = 12; todayNoon.tm_min = 0; todayNoon.tm_sec = 0;
    time_t todayT = mktime(&todayNoon);

    for (int w = weeks - 1; w >= 0; w--) {
        time_t bucketEnd   = todayT - (time_t)w * 7 * 86400;
        time_t bucketStart = bucketEnd - 6 * 86400;
        int c = 0;
        for (auto& t : allTasks) {
            if (!t.isCompleted || t.dateCompleted.empty()) continue;
            time_t ct = isoToTime(t.dateCompleted);
            if (ct >= bucketStart && ct <= bucketEnd) c++;
        }
        vals.push_back((float)c);
        tm* endTm = localtime(&bucketEnd);
        char buf[8]; strftime(buf, sizeof(buf), "%d/%m", endTm);
        labels.push_back(string(buf));
    }
}

// Monthly: one point per calendar month for the last `months` months (most recent = current month)
static void BuildMonthlyCompletion(int months, vector<float>& vals, vector<string>& labels) {
    time_t now = time(0);
    tm* nowTm = localtime(&now);
    int curY = nowTm->tm_year + 1900, curM = nowTm->tm_mon + 1;
    static const char* mn[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

    for (int i = months - 1; i >= 0; i--) {
        int m = curM - i, y = curY;
        while (m <= 0) { m += 12; y -= 1; }
        int c = 0;
        for (auto& t : allTasks) {
            if (!t.isCompleted || t.dateCompleted.empty()) continue;
            int ty = 0, tm_ = 0, td = 0;
            sscanf(t.dateCompleted.c_str(), "%d-%d-%d", &ty, &tm_, &td);
            if (ty == y && tm_ == m) c++;
        }
        vals.push_back((float)c);
        labels.push_back(string(mn[m - 1]));
    }
}

static void DrawLineGraph(const char* title, vector<float>& vals, vector<string>& labels,
                          float x, float y, float w, float h, Color lineColor) {
    Rectangle box = {x, y, w, h};
    DrawRoundRect(box, 0.04f, C_PANEL);
    DrawRoundRectLines(box, 0.04f, 1.0f, C_BORDER);
    DrawText(title, (int)x + 14, (int)y + 12, 14, C_WHITE);

    float gx = x + 16, gy = y + 40, gw = w - 32, gh = h - 62;
    DrawLine((int)gx, (int)(gy + gh), (int)(gx + gw), (int)(gy + gh), C_BORDER);

    if (vals.empty()) return;
    float maxV = 1.0f;
    for (float v : vals) if (v > maxV) maxV = v;

    int n = (int)vals.size();
    float stepX = n > 1 ? gw / (float)(n - 1) : 0.0f;

    // Soft filled area under the line for a nicer "interesting" look
    for (int i = 0; i < n - 1; i++) {
        float x0 = gx + i*stepX,     y0 = gy + gh - (vals[i]/maxV)*gh;
        float x1 = gx + (i+1)*stepX, y1 = gy + gh - (vals[i+1]/maxV)*gh;
        Color fillCol = { lineColor.r, lineColor.g, lineColor.b, 30 };
        DrawTriangle({x0, y0}, {x0, gy+gh}, {x1, gy+gh}, fillCol);
        DrawTriangle({x0, y0}, {x1, gy+gh}, {x1, y1}, fillCol);
        DrawLineEx({x0, y0}, {x1, y1}, 2.5f, lineColor);
    }
    for (int i = 0; i < n; i++) {
        float xi = gx + i*stepX, yi = gy + gh - (vals[i]/maxV)*gh;
        DrawCircle((int)xi, (int)yi, 3.2f, lineColor);
        // On the large/maximized graph there's room to show the exact value above each point
        if (h > 300) {
            char vb[8]; snprintf(vb, sizeof(vb), "%.0f", vals[i]);
            int vbw = MeasureText(vb, 10);
            DrawText(vb, (int)(xi - vbw/2), (int)(yi - 16), 10, C_TEXT_DIM);
        }
    }
    // Sparse x-axis labels so they don't collide
    for (int i = 0; i < n; i++) {
        if (n > 7 && (i % 2 != 0) && i != n-1) continue;
        float xi = gx + i*stepX;
        int lw2 = MeasureText(labels[i].c_str(), 9);
        DrawText(labels[i].c_str(), (int)(xi - lw2/2), (int)(gy + gh + 6), 9, C_TEXT_DIM);
    }
    char mv[16]; snprintf(mv, sizeof(mv), "%.0f", maxV);
    DrawText(mv, (int)x + 14, (int)gy - 4, 10, C_TEXT_DIM);
}

// Small pill-style toggle button used for Daily / Weekly / Monthly and Maximize
static bool DrawPillButton(const char* label, Rectangle r, bool active, Vector2 mouse) {
    bool hov = CheckCollisionPointRec(mouse, r);
    Color bg = active ? C_ORANGE : (hov ? C_CARD_HOV : C_CARD);
    Color fg = active ? C_BG : C_TEXT_DIM;
    DrawRoundRect(r, 0.5f, bg);
    if (!active) DrawRoundRectLines(r, 0.5f, 1.0f, C_BORDER);
    int tw = MeasureText(label, 12);
    DrawText(label, (int)(r.x + r.width/2 - tw/2), (int)(r.y + r.height/2 - 6), 12, fg);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// ─────────────────────────────────────────────────────────────
//  Statistics view — a primary "Tasks Completed" line graph
//  with Daily / Weekly / Monthly granularity toggle and a
//  maximize button, plus supporting graphs and the task list.
// ─────────────────────────────────────────────────────────────
static void DrawStatsView(float& scroll, ModalState& modal, Vector2 mouse) {
    static CompletionGranularity gran = GRAN_DAILY;
    static bool maximized = false;

    float lx = SIDE_W + 12, ly = TOP_H + 16;
    float lw = WIN_W - SIDE_W - 24, lh = WIN_H - TOP_H - 24;

    int total = allTasks.size();
    int done  = 0;
    for (auto& t: allTasks) if (t.isCompleted) done++;

    char sum[176];
    snprintf(sum, sizeof(sum),
             "Total: %d   Completed: %d   Pending: %d   Streak: %d days   Level %d (%d/%d XP)",
             total, done, total-done, activeUserStreak,
             levelForXP(totalXP), xpIntoCurrentLevel(totalXP), xpNeededForNextLevel());
    DrawText("Statistics", (int)lx, (int)ly, 20, C_WHITE);
    DrawText(sum, (int)lx, (int)ly+28, 13, C_TEXT_DIM);

    // ── Build the primary "Tasks Completed" dataset for the selected granularity ──
    vector<float> completedVals;
    vector<string> completedLabels;
    const char* graphTitle = "Tasks Completed — Daily";
    switch (gran) {
        case GRAN_DAILY:   BuildDailyCompletion(14, completedVals, completedLabels);
                           graphTitle = "Tasks Completed — Daily (14d)"; break;
        case GRAN_WEEKLY:  BuildWeeklyCompletion(10, completedVals, completedLabels);
                           graphTitle = "Tasks Completed — Weekly (10w)"; break;
        case GRAN_MONTHLY: BuildMonthlyCompletion(9, completedVals, completedLabels);
                           graphTitle = "Tasks Completed — Monthly (9mo)"; break;
    }

    // ── Toggle row: Daily / Weekly / Monthly + Maximize ──
    float togY = ly + 50;
    Rectangle dBtn = {lx,            togY, 64, 26};
    Rectangle wBtn = {lx + 70,       togY, 64, 26};
    Rectangle mBtn = {lx + 140,      togY, 74, 26};
    if (DrawPillButton("Daily",   dBtn, gran==GRAN_DAILY,   mouse)) gran = GRAN_DAILY;
    if (DrawPillButton("Weekly",  wBtn, gran==GRAN_WEEKLY,  mouse)) gran = GRAN_WEEKLY;
    if (DrawPillButton("Monthly", mBtn, gran==GRAN_MONTHLY, mouse)) gran = GRAN_MONTHLY;

    Rectangle maxBtn = {lx + lw - 110, togY, 110, 26};
    if (DrawPillButton(maximized ? "Minimize" : "Maximize", maxBtn, maximized, mouse)) maximized = !maximized;

    if (maximized) {
        // ── Maximized mode: the completion graph fills almost the entire view ──
        float gY = togY + 36;
        float gH = (ly + lh) - gY - 8;
        DrawLineGraph(graphTitle, completedVals, completedLabels, lx, gY, lw, gH, C_GREEN);
        return; // skip the secondary graphs and task list while maximized
    }

    // ── Normal mode: primary graph (full width), then two smaller
    //    supporting graphs, then the full task list underneath ──
    float gY = togY + 36, gH = 250;
    DrawLineGraph(graphTitle, completedVals, completedLabels, lx, gY, lw, gH, C_GREEN);

    // Supporting graphs: Upcoming Due (next 14 days) + XP Growth (last 14 days)
    vector<tm> pastDates, futureDates;
    BuildDateRange(-13, 0, pastDates);
    BuildDateRange(0, 13, futureDates);

    vector<float> dueVals, xpVals;
    vector<string> pastLabels, futureLabels;
    float xpRunning = 0.0f;
    for (auto& d : pastDates) {
        string iso = calFmtISO(d);
        float xpDay = 0;
        for (auto& t : allTasks) if (t.isCompleted && t.dateCompleted == iso) xpDay += xpForPriority(t.priority);
        xpRunning += xpDay;
        xpVals.push_back(xpRunning);
        pastLabels.push_back(calFmtShort(d));
    }
    for (auto& d : futureDates) {
        int dd = d.tm_mday, mm = d.tm_mon+1, yy = d.tm_year+1900;
        int c = 0;
        for (auto& t : allTasks) if (t.dueDay==dd && t.dueMonth==mm && t.dueYear==yy) c++;
        dueVals.push_back((float)c);
        futureLabels.push_back(calFmtShort(d));
    }

    float sgY = gY + gH + 16, sgH = 170, gap = 14;
    float sgW = (lw - gap) / 2.0f;
    DrawLineGraph("Upcoming Due (14d)", dueVals, futureLabels, lx,        sgY, sgW, sgH, C_BLUE);
    DrawLineGraph("XP Growth (14d)",    xpVals,  pastLabels,   lx+sgW+gap, sgY, sgW, sgH, C_PURPLE);

    // Task list underneath the graphs
    float listY = sgY + sgH + 22;
    float listH = (ly + lh) - listY;
    DrawText("All Tasks", (int)lx, (int)listY - 22, 15, C_WHITE);
    int dummy = -1;
    DrawTaskList(allTasks, lx, listY, lw, listH, scroll, mouse, CARD_STATS, modal, dummy);
}

// ─────────────────────────────────────────────────────────────
//  Calendar view — interactive month grid + click-to-inspect
//  day panel + a monthly activity heatmap
// ─────────────────────────────────────────────────────────────
static void DrawCalView(int& calMonth, int& calYear, Vector2 mouse) {
    // Persists which day is currently selected while browsing the calendar
    static int selDay = -1, selMonth = 0, selYear = 0;

    // ── Overall canvas: everything right of the sidebar, below the top bar ──
    float lx = SIDE_W + 24, ly = TOP_H + 20;
    float canvasW = WIN_W - lx - 24;      // full usable width
    float canvasH = WIN_H - ly - 24;      // full usable height

    // Right column (detail panel + heatmap) gets a fixed width; the grid
    // takes the rest, so both scale to fill the whole window instead of
    // leaving space unused on the right / bottom.
    float rightW   = 380;
    float colGap   = 28;
    float gridW    = canvasW - rightW - colGap;

    // On-screen nav arrows (keyboard left/right arrows still work too)
    Rectangle prevBtn = {lx, ly, 32, 32};
    Rectangle nextBtn = {lx + 230, ly, 32, 32};
    bool prevHov = CheckCollisionPointRec(mouse, prevBtn);
    bool nextHov = CheckCollisionPointRec(mouse, nextBtn);
    DrawRoundRect(prevBtn, 0.3f, prevHov ? C_ORANGE : C_DARKGRAY);
    DrawText("<", (int)prevBtn.x+12, (int)prevBtn.y+7, 17, prevHov ? C_BG : C_WHITE);
    DrawRoundRect(nextBtn, 0.3f, nextHov ? C_ORANGE : C_DARKGRAY);
    DrawText(">", (int)nextBtn.x+12, (int)nextBtn.y+7, 17, nextHov ? C_BG : C_WHITE);

    bool clickPrev = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && prevHov;
    bool clickNext = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && nextHov;
    if (IsKeyPressed(KEY_RIGHT) || clickNext) { calMonth++; if(calMonth>12){calMonth=1;calYear++;} }
    if (IsKeyPressed(KEY_LEFT)  || clickPrev) { calMonth--; if(calMonth<1){calMonth=12;calYear--;} }

    time_t now2=time(0); tm* lt=localtime(&now2);
    int today2=lt->tm_mday, todayM=lt->tm_mon+1, todayY=lt->tm_year+1900;

    string heading = calMonthName(calMonth) + "  " + to_string(calYear);
    int hw = MeasureText(heading.c_str(), 24);
    DrawText(heading.c_str(), (int)(lx + 130 - hw/2), (int)ly + 4, 24, C_ORANGE);
    DrawText("Click a day to see its tasks", (int)lx, (int)ly + 44, 13, C_TEXT_DIM);

    // ── Calendar grid: cells sized to fill gridW, and rows sized to fill
    //    the remaining canvas height, so the grid never looks cramped. ──
    const char* dow[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    const float cellGap = 8;
    float gridTop = ly + 84;
    float cellW = (gridW - 6*cellGap) / 7.0f;

    int start = calStart(calMonth, calYear);
    int total = calDays(calMonth, calYear);
    int numRows = (start + total + 6) / 7;   // rows actually needed this month
    float gridBottomLimit = ly + canvasH;
    float cellH = (gridBottomLimit - gridTop - (numRows-1)*cellGap) / numRows;
    if (cellH > 110) cellH = 110;   // don't let cells get absurdly tall on short months
    if (cellH < 58)  cellH = 58;

    for (int i = 0; i < 7; i++) {
        int cw = MeasureText(dow[i], 14);
        DrawText(dow[i], (int)(lx + i*(cellW+cellGap) + cellW/2 - cw/2), (int)gridTop - 24, 14, C_TEAL);
    }

    int row = 0;
    for (int d = 1; d <= total; d++) {
        int col = (start + d - 1) % 7;
        float rx = lx + col*(cellW+cellGap);
        float ry = gridTop + row*(cellH+cellGap);
        Rectangle cell = {rx, ry, cellW, cellH};
        bool hov = CheckCollisionPointRec(mouse, cell);

        bool isToday = (d==today2 && calMonth==todayM && calYear==todayY);
        bool isSel   = (d==selDay && calMonth==selMonth && calYear==selYear);

        int pendingCount = 0, completedCount = 0;
        for (auto& t : allTasks) {
            if (t.dueDay==d && t.dueMonth==calMonth && t.dueYear==calYear) {
                if (t.isCompleted) completedCount++; else pendingCount++;
            }
        }

        Color cellBg = C_CARD;
        if (completedCount > 0 && pendingCount == 0) cellBg = {14, 68, 41, 255};   // all due tasks done -> green
        else if (pendingCount > 0)                    cellBg = {58, 44, 20, 255};  // still pending -> amber
        DrawRoundRect(cell, 0.12f, hov ? C_CARD_HOV : cellBg);

        Color borderCol = C_BORDER;
        float borderThick = 1.0f;
        if (isSel)      { borderCol = C_ORANGE; borderThick = 2.0f; }
        else if (isToday) { borderCol = C_RED;    borderThick = 2.0f; }
        DrawRoundRectLines(cell, 0.12f, borderThick, borderCol);

        char db[8]; snprintf(db, sizeof(db), "%d", d);
        DrawText(db, (int)rx+10, (int)ry+8, 17, isToday ? C_RED : (hov ? C_WHITE : C_TEXT_DIM));

        if (completedCount > 0) {
            char cb[8]; snprintf(cb, sizeof(cb), "%d", completedCount);
            DrawCircle((int)(rx+cellW-18), (int)(ry+18), 9, C_GREEN);
            DrawText(cb, (int)(rx+cellW-21), (int)(ry+11), 12, C_BG);
        }
        if (pendingCount > 0) {
            char pb[8]; snprintf(pb, sizeof(pb), "%d", pendingCount);
            float ox = completedCount > 0 ? cellW - 40 : cellW - 18;
            DrawCircle((int)(rx+ox), (int)(ry+18), 9, C_YELLOW);
            DrawText(pb, (int)(rx+ox-3), (int)(ry+11), 12, C_BG);
        }

        // For tall cells (short months), show up to 2 task names inline
        if (cellH > 72) {
            float ny = ry + 32;
            int shown = 0;
            for (auto& t : allTasks) {
                if (shown >= 2) break;
                if (t.dueDay==d && t.dueMonth==calMonth && t.dueYear==calYear) {
                    string nm = t.taskName;
                    if (nm.size() > 14) nm = nm.substr(0,12) + "..";
                    DrawText(nm.c_str(), (int)rx+8, (int)ny, 10, t.isCompleted ? C_TEXT_DIM : C_WHITE);
                    ny += 14; shown++;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hov) {
            selDay = d; selMonth = calMonth; selYear = calYear;
        }
        if (col == 6) row++;
    }

    // ── Right column: selected-day detail panel (top) + monthly heatmap (bottom) ──
    float rx0 = lx + gridW + colGap;
    float ry0 = ly;
    float rw0 = rightW;

    float detailH = canvasH * 0.42f;
    Rectangle detailBox = {rx0, ry0, rw0, detailH};
    DrawRoundRect(detailBox, 0.04f, C_PANEL);
    DrawRoundRectLines(detailBox, 0.04f, 1.0f, C_BORDER);

    if (selDay < 0) {
        DrawText("Select a day", (int)rx0+18, (int)ry0+18, 17, C_WHITE);
        DrawText("Click any date on the calendar", (int)rx0+18, (int)ry0+50, 13, C_TEXT_DIM);
        DrawText("to view and complete its tasks.", (int)rx0+18, (int)ry0+70, 13, C_TEXT_DIM);
    } else {
        string dlabel = calMonthName(selMonth) + " " + to_string(selDay) + ", " + to_string(selYear);
        DrawText(("Due: " + dlabel).c_str(), (int)rx0+18, (int)ry0+16, 17, C_WHITE);

        vector<int> idxs;
        for (int i = 0; i < (int)allTasks.size(); i++) {
            Task& t = allTasks[i];
            if (t.dueDay==selDay && t.dueMonth==selMonth && t.dueYear==selYear) idxs.push_back(i);
        }
        if (idxs.empty()) {
            DrawText("No tasks due this day.", (int)rx0+18, (int)ry0+54, 13, C_TEXT_DIM);
        } else {
            float yy = ry0 + 52;
            float rowH = 28;
            int maxRows = (int)((detailH - 60) / rowH);
            for (int i = 0; i < (int)idxs.size() && i < maxRows; i++) {
                Task& t = allTasks[idxs[i]];
                Rectangle chk = {rx0+18, yy, 19, 19};
                bool chkHov = CheckCollisionPointRec(mouse, chk);
                if (t.isCompleted) {
                    DrawRoundRect(chk, 0.3f, C_GREEN);
                } else {
                    DrawRoundRectLines(chk, 0.3f, 1.5f, chkHov ? C_GREEN : C_BORDER);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && chkHov) {
                        t.isCompleted = true;
                        t.dateCompleted = todayDateString();
                        totalXP += xpForPriority(t.priority);
                        saveUserData();
                    }
                }
                string nm = t.taskName;
                int maxChars = (int)((rw0 - 60) / 7.5f);
                if ((int)nm.size() > maxChars) nm = nm.substr(0, maxChars-3) + "...";
                DrawText(nm.c_str(), (int)chk.x+28, (int)chk.y, 14, t.isCompleted ? C_TEXT_DIM : C_WHITE);
                yy += rowH;
            }
            if ((int)idxs.size() > maxRows) DrawText("...", (int)rx0+18, (int)yy, 12, C_TEXT_DIM);
        }
    }

    // ── Monthly activity heatmap — fills the rest of the right column ──
    // Cells light up on the day a task was actually COMPLETED (dateCompleted),
    // not on the day it happened to be due. Intensity (alpha) scales with how
    // many tasks were completed that day — more completions = more opaque green.
    // Pending tasks are never shown here (no yellow/amber for pending).
    float hy0 = ry0 + detailH + 24;
    float heatmapH = canvasH - detailH - 24;

    Rectangle heatBox = {rx0, hy0, rw0, heatmapH};
    DrawRoundRect(heatBox, 0.04f, C_PANEL);
    DrawRoundRectLines(heatBox, 0.04f, 1.0f, C_BORDER);

    float hx = rx0 + 34, hy = hy0 + 44;
    DrawText("Monthly Activity", (int)rx0+18, (int)hy0+16, 15, C_WHITE);

    // Cell size scales with the available heatmap height so it never
    // looks squeezed into a leftover sliver.
    float availForGrid = heatmapH - 44 - 44; // minus header and legend space
    float cell = min((rw0 - 34 - 16) / 5.0f, availForGrid / 7.0f);
    if (cell > 30) cell = 30;
    if (cell < 18) cell = 18;
    float cpad = cell * 0.28f;

    const char* dow2[] = {"S","M","T","W","T","F","S"};
    for (int i = 0; i < 7; i++) DrawText(dow2[i], (int)hx-22, (int)(hy+i*(cell+cpad))+ (int)(cell/2-6), 12, C_TEXT_DIM);

    const int MAX_INTENSITY_CAP = 5;   // completions at/above this count = full opacity
    const unsigned char ALPHA_MIN = 60;
    const unsigned char ALPHA_MAX = 255;

    for (int d = 1; d <= total; d++) {
        int ci = start + (d-1);
        int r = ci % 7, c = ci / 7;

        int comp = 0;
        for (auto& t : allTasks) {
            if (!t.isCompleted || t.dateCompleted.empty()) continue;
            int cy = 0, cm = 0, cd = 0;
            sscanf(t.dateCompleted.c_str(), "%d-%d-%d", &cy, &cm, &cd);
            if (cd == d && cm == calMonth && cy == calYear) comp++;
        }

        Color cc = {22,27,34,255}; // no completions -> empty dark cell
        if (comp > 0) {
            int capped = comp > MAX_INTENSITY_CAP ? MAX_INTENSITY_CAP : comp;
            unsigned char alpha = (unsigned char)(ALPHA_MIN +
                ((ALPHA_MAX - ALPHA_MIN) * (capped - 1)) / (MAX_INTENSITY_CAP - 1));
            cc = {57, 211, 83, alpha};
        }

        Rectangle cr = {(float)(hx+c*(cell+cpad)), (float)(hy+r*(cell+cpad)), cell, cell};
        bool isSelCell = (d==selDay && calMonth==selMonth && calYear==selYear);
        DrawRectangleRounded(cr, 0.25f, 4, cc);
        if (isSelCell) DrawRoundRectLines(cr, 0.25f, 2.0f, WHITE);
        char ds[4]; snprintf(ds, sizeof(ds), "%d", d);
        int tw = MeasureText(ds, 10);
        DrawText(ds, (int)(cr.x+(cell-tw)/2), (int)(cr.y+cell/2-5), 10, comp>=3 ? BLACK : LIGHTGRAY);
    }
    int legy = (int)(hy0 + heatmapH - 26);
    DrawText("Less", (int)hx-22, (int)legy, 11, C_TEXT_DIM);
    for (int i = 0; i < 5; i++) {
        unsigned char a = (i==0) ? 255 : (unsigned char)(ALPHA_MIN + ((ALPHA_MAX - ALPHA_MIN) * i) / 4);
        Color sw = (i==0) ? Color{22,27,34,255} : Color{57,211,83,a};
        Rectangle legSw = {(float)(hx+14+i*20), (float)legy-2, 16, 16};
        DrawRectangleRounded(legSw, 0.3f, 4, sw);
    }
    DrawText("More", (int)hx+14+5*20+6, (int)legy, 11, C_TEXT_DIM);
}

// ─────────────────────────────────────────────────────────────
//  Achievements — computed live from real task/XP/streak data.
//  Nothing here is hardcoded to a specific user; unlock state is
//  re-derived every frame from allTasks / totalXP / streak globals.
// ─────────────────────────────────────────────────────────────
struct Achievement { const char* icon; const char* name; bool earned; };

static vector<Achievement> ComputeAchievements() {
    int total = (int)allTasks.size();
    int completed = 0;
    bool hadCriticalDone = false;
    for (auto& t : allTasks) {
        if (t.isCompleted) {
            completed++;
            if (t.priority >= 5) hadCriticalDone = true;
        }
    }
    int lvl = levelForXP(totalXP);

    vector<Achievement> a;
    a.push_back({ "1",   "First Task",     total >= 1 });
    a.push_back({ ">10", "Task Slayer",    completed >= 10 });
    a.push_back({ "100", "Century",        totalXP >= 100 });
    a.push_back({ "3d",  "Streak Starter", activeUserStreak >= 3 });
    a.push_back({ "7d",  "Streak Master",  activeUserMaxStreak >= 7 });
    a.push_back({ "P5",  "High Roller",    hadCriticalDone });
    a.push_back({ "ALL", "Perfectionist",  total > 0 && completed == total });
    a.push_back({ "Lv2", "Rising Star",    lvl >= 2 });
    return a;
}

// ─────────────────────────────────────────────────────────────
//  Profile view — GitHub-style profile page wired to real user data.
//  Left column: avatar / identity / bio / links.
//  Right column: about card with live stats, then achievement grid.
//  Clicking "Edit Profile" swaps the right column for an inline form
//  (same DrawInputBox pattern used by the New Task modal) and persists
//  via saveProfileData() into "<username>_profile.txt".
// ─────────────────────────────────────────────────────────────
static void DrawProfileView(Vector2 mouse) {
    static ProfileEditState edit;
    static Texture2D texAvatar = {0};
    if (texAvatar.id == 0) texAvatar = LoadTexture("assets/user.png");

    float lx = SIDE_W + 12, ly = TOP_H + 16;
    float lw = WIN_W - SIDE_W - 24;
    float lh = WIN_H - TOP_H - 24;
    float leftW = 280.f, gap = 24.f;
    float rx = lx + leftW + gap, rw = lw - leftW - gap;

    // ═══════════════ LEFT COLUMN — Avatar / Identity / Links ═══════════════
    float cx = lx + leftW / 2.0f;

    DrawCircle((int)cx, (int)(ly + 64), 62, C_DARKGRAY);
    DrawCircleLines((int)cx, (int)(ly + 64), 62, C_ORANGE);
    if (texAvatar.id != 0) {
        float scale = 90.0f / (float)texAvatar.width;
        DrawTextureEx(texAvatar,
            { cx - (texAvatar.width * scale) / 2.0f, ly + 64 - (texAvatar.height * scale) / 2.0f },
            0.0f, scale, WHITE);
    }

    float ny = ly + 145;
    string dispName = profile.fullName.empty() ? activeUsername : profile.fullName;
    int nw = MeasureText(dispName.c_str(), 22);
    DrawText(dispName.c_str(), (int)(cx - nw / 2.0f), (int)ny, 22, C_WHITE);
    ny += 28;

    string handle = "@" + activeUsername;
    int hw = MeasureText(handle.c_str(), 15);
    DrawText(handle.c_str(), (int)(cx - hw / 2.0f), (int)ny, 15, C_TEXT_DIM);
    ny += 24;

    if (!profile.pronouns.empty()) {
        int pw = MeasureText(profile.pronouns.c_str(), 12) + 16;
        Rectangle chip = { cx - pw / 2.0f, ny, (float)pw, 22 };
        DrawRoundRect(chip, 0.5f, C_DARKGRAY);
        DrawRoundRectLines(chip, 0.5f, 1.0f, C_BORDER);
        DrawText(profile.pronouns.c_str(), (int)chip.x + 8, (int)chip.y + 5, 12, C_TEXT_DIM);
        ny += 34;
    } else {
        ny += 10;
    }

    if (!profile.bio.empty()) {
        ny += DrawWrappedText(profile.bio.c_str(), (int)lx, (int)ny, (int)leftW, 13, C_TEXT_DIM);
        ny += 6;
    } else if (!edit.editing) {
        DrawText("No bio yet.", (int)lx, (int)ny, 13, C_TEXT_DIM);
        ny += 26;
    }

    // Edit Profile button — reuses the app's existing edit icon asset
    Rectangle editBtn = { lx, ny + 4, leftW, 34 };
    bool editHov = CheckCollisionPointRec(mouse, editBtn);
    DrawRoundRect(editBtn, 0.2f, editHov ? C_ORANGE : C_DARKGRAY);
    if (texEdit.id != 0)
        DrawTextureEx(texEdit, { editBtn.x + 10, editBtn.y + 9 }, 0.0f, 0.035f, editHov ? C_BG : C_WHITE);
    DrawText(edit.editing ? "Editing..." : "Edit Profile",
             (int)editBtn.x + 34, (int)editBtn.y + 9, 15, editHov ? C_BG : C_WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && editHov && !edit.editing) {
        edit.editing = true;
        edit.activeField = 0;
        snprintf(edit.fullName, sizeof(edit.fullName), "%s", profile.fullName.c_str());
        snprintf(edit.pronouns, sizeof(edit.pronouns), "%s", profile.pronouns.c_str());
        snprintf(edit.bio,      sizeof(edit.bio),      "%s", profile.bio.c_str());
        snprintf(edit.website,  sizeof(edit.website),  "%s", profile.website.c_str());
        snprintf(edit.socials,  sizeof(edit.socials),  "%s", profile.socials.c_str());
        snprintf(edit.linkedin, sizeof(edit.linkedin), "%s", profile.linkedin.c_str());
    }
    ny = editBtn.y + editBtn.height + 20;

    // Links — clickable, opens the system's default browser via raylib's OpenURL
    DrawLine((int)lx, (int)ny, (int)(lx + leftW), (int)ny, C_BORDER);
    ny += 16;
    DrawText("LINKS", (int)lx, (int)ny, 11, C_TEXT_DIM);
    ny += 20;

    struct LinkRow { const char* label; string* value; Color col; };
    LinkRow links[] = {
        { "Website",  &profile.website,  C_BLUE   },
        { "Socials",  &profile.socials,  C_TEAL   },
        { "LinkedIn", &profile.linkedin, C_PURPLE },
    };
    bool anyLink = false;
    for (auto& lr : links) {
        if (lr.value->empty()) continue;
        anyLink = true;
        Rectangle row = { lx, ny, leftW, 24 };
        bool hov = CheckCollisionPointRec(mouse, row);
        string txt = string(lr.label) + ":  " + *lr.value;
        DrawText(txt.c_str(), (int)lx, (int)ny + 4, 13, hov ? lr.col : C_TEXT_DIM);
        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            string url = *lr.value;
            if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0)
                url = "https://" + url;
            OpenURL(url.c_str());
        }
        ny += 26;
    }
    if (!anyLink) { DrawText("No links added yet.", (int)lx, (int)ny, 12, C_TEXT_DIM); ny += 26; }

    // Member-since footer, pinned near the bottom of the left column
    string joined = "Joined " + (profile.joinDate.empty() ? string("recently") : profile.joinDate);
    DrawText(joined.c_str(), (int)lx, (int)(ly + lh - 18), 12, C_TEXT_DIM);

    // ═══════════════ RIGHT COLUMN ═══════════════
    if (edit.editing) {
        // ── Inline edit form (same interaction pattern as DrawNewTaskModal) ──
        Rectangle box = { rx, ly, rw, 420 };
        DrawRoundRect(box, 0.03f, C_PANEL);
        DrawRoundRectLines(box, 0.03f, 1.5f, C_ORANGE);
        DrawText("Edit Profile", (int)box.x + 20, (int)box.y + 16, 18, C_ORANGE);

        struct Field { const char* label; char* buf; int maxLen; };
        Field fields[] = {
            { "Full Name",      edit.fullName, 48  },
            { "Pronouns",       edit.pronouns, 16  },
            { "Bio",            edit.bio,      140 },
            { "Website",        edit.website,  64  },
            { "Socials",        edit.socials,  32  },
            { "LinkedIn",       edit.linkedin, 64  },
        };
        const int FCOUNT = 6;
        float fx = box.x + 20, fy = box.y + 60, fw = box.width - 40, fh = 36;

        for (int i = 0; i < FCOUNT; i++) {
            Rectangle r = { fx, fy + i * 56.0f, fw, fh };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, r))
                edit.activeField = i;
            DrawInputBox(fields[i].label, fields[i].buf, r, edit.activeField == i);
        }

        if (IsKeyPressed(KEY_TAB)) {
            bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            edit.activeField = shiftDown ? (edit.activeField + FCOUNT - 1) % FCOUNT
                                          : (edit.activeField + 1) % FCOUNT;
        }
        HandleTextInput(fields[edit.activeField].buf, fields[edit.activeField].maxLen,
                        IsKeyPressedRepeat(KEY_BACKSPACE) ? KEY_BACKSPACE : 0);

        Rectangle saveBtn = { fx, fy + FCOUNT * 56.0f + 8, fw / 2 - 8, 38 };
        Rectangle canBtn  = { fx + fw / 2 + 8, fy + FCOUNT * 56.0f + 8, fw / 2 - 8, 38 };
        bool saveHov = CheckCollisionPointRec(mouse, saveBtn);
        bool canHov  = CheckCollisionPointRec(mouse, canBtn);

        DrawRoundRect(saveBtn, 0.2f, saveHov ? C_GREEN : C_DARKGRAY);
        DrawText("Save", (int)saveBtn.x + 20, (int)saveBtn.y + 10, 16, saveHov ? C_BG : C_WHITE);
        DrawRoundRect(canBtn, 0.2f, canHov ? C_RED : C_DARKGRAY);
        DrawText("Cancel", (int)canBtn.x + 20, (int)canBtn.y + 10, 16, canHov ? C_WHITE : C_TEXT_DIM);

        // Enter saves, Escape cancels — same behavior as clicking the buttons
        bool enterConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
        bool escCancel    = IsKeyPressed(KEY_ESCAPE);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || enterConfirm || escCancel) {
            saveHov = saveHov || enterConfirm;
            canHov  = canHov  || escCancel;
            if (saveHov) {
                profile.fullName = edit.fullName;
                profile.pronouns = edit.pronouns;
                profile.bio      = edit.bio;
                profile.website  = edit.website;
                profile.socials  = edit.socials;
                profile.linkedin = edit.linkedin;
                saveProfileData();
                edit.editing = false;
            } else if (canHov) {
                edit.editing = false;
            }
        }
        return; // Skip the read-only About/Achievements while the form is open
    }

    // ── About card ──
    int total = (int)allTasks.size();
    int completed = 0;
    for (auto& t : allTasks) if (t.isCompleted) completed++;

    Rectangle about = { rx, ly, rw, 190 };
    DrawRoundRect(about, 0.03f, C_PANEL);
    DrawRoundRectLines(about, 0.03f, 1.0f, C_BORDER);
    DrawText("About", (int)about.x + 20, (int)about.y + 16, 16, C_WHITE);

    string aboutBio = profile.bio.empty() ? "This user hasn't written a bio yet." : profile.bio;
    DrawWrappedText(aboutBio.c_str(), (int)about.x + 20, (int)about.y + 46, (int)about.width - 40, 14, C_TEXT_DIM);

    float vy = about.y + about.height - 46;
    DrawLine((int)about.x + 20, (int)vy - 14, (int)(about.x + about.width - 20), (int)vy - 14, C_BORDER);

    struct Vital { const char* label; string value; Color col; };
    char xpBuf[24], streakBuf[32], taskBuf[24], lvlBuf[24];
    snprintf(xpBuf, sizeof(xpBuf), "%d XP", totalXP);
    snprintf(streakBuf, sizeof(streakBuf), "%d days (best %d)", activeUserStreak, activeUserMaxStreak);
    snprintf(taskBuf, sizeof(taskBuf), "%d / %d done", completed, total);
    snprintf(lvlBuf, sizeof(lvlBuf), "Level %d", levelForXP(totalXP));
    Vital vitals[] = {
        { "LEVEL",  lvlBuf,    C_PURPLE },
        { "XP",     xpBuf,     C_PURPLE },
        { "STREAK", streakBuf, C_ORANGE },
        { "TASKS",  taskBuf,   C_GREEN  },
    };
    float vw = (about.width - 40) / 4.0f;
    for (int i = 0; i < 4; i++) {
        float vx = about.x + 20 + i * vw;
        DrawText(vitals[i].label, (int)vx, (int)vy, 10, C_TEXT_DIM);
        DrawText(vitals[i].value.c_str(), (int)vx, (int)vy + 14, 14, vitals[i].col);
    }

    // ── Achievements grid ──
    float ay = about.y + about.height + 20;
    DrawText("Achievements", (int)rx, (int)ay, 16, C_WHITE);
    ay += 30;

    vector<Achievement> ach = ComputeAchievements();
    int cols = 4;
    float cellW = rw / cols, cellH = 118;
    for (size_t i = 0; i < ach.size(); i++) {
        int col = (int)i % cols, row = (int)i / cols;
        float bx = rx + col * cellW + cellW / 2.0f;
        float by = ay + row * cellH + 50;
        Color hexCol = ach[i].earned ? C_ORANGE : C_BORDER;
        Color txtCol = ach[i].earned ? C_WHITE  : C_TEXT_DIM;
        DrawPoly({ bx, by }, 6, 34, 90, ach[i].earned ? Color{255,128,0,40} : Color{40,55,75,40});
        DrawPolyLines({ bx, by }, 6, 34, 90, hexCol);
        int iw = MeasureText(ach[i].icon, 14);
        DrawText(ach[i].icon, (int)(bx - iw / 2.0f), (int)(by - 7), 14, hexCol);
        int nwid = MeasureText(ach[i].name, 12);
        DrawText(ach[i].name, (int)(bx - nwid / 2.0f), (int)(by + 42), 12, txtCol);
    }
}

// ─────────────────────────────────────────────────────────────
//  Main Dashboard() — one Raylib window, all views
// ─────────────────────────────────────────────────────────────
bool Dashboard() {
    SetTraceLogLevel(LOG_NONE);
    InitAudioDevice();

    View      view       = VIEW_DASH;
    SortMode  sortMode   = SORT_NONE;
    ModalState modal;
    modal.open = false;
    bool editMode = false;

    FocusTimer focTimer;
    int focusTaskIdx = -1;

    MusicState music;

    float dashScroll  = 0.f;
    float focusScroll = 0.f;
    float statsScroll = 0.f;

    int calMonth, calYear;
    { time_t n=time(0); tm*l=localtime(&n); calMonth=l->tm_mon+1; calYear=l->tm_year+1900; }

    g_sessionStart = GetTime();

    // ─── Main loop ──────────────────────────────────────────
    // ─── Main loop ──────────────────────────────────────────
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        double sessionSecs = GetTime() - g_sessionStart;

        // ── Music Engine Tracking Layer ──
        if (wantsToLoadMusic) {
            wantsToLoadMusic = false; // Reset flag layer
            string filepath = OpenFileDialog(); 
            if (!filepath.empty()) {
                filepath.erase(filepath.find_last_not_of(" \n\r\t") + 1); // Clean Linux string termination
                if (music.loaded) {
                    StopMusicStream(music.track);
                    UnloadMusicStream(music.track);
                }
                music.track = LoadMusicStream(filepath.c_str());
                if (music.track.frameCount > 0) {
                    PlayMusicStream(music.track);
                    music.loaded = true;
                    music.on = true;
                    cout << "\n>>> [SUCCESS] TRACK LOADED SAFELY!\n";
                }
            }
        }

        if (music.loaded && music.on) {
            UpdateMusicStream(music.track);
        }

        // ── Drawing Operations Start Phase ──
        BeginDrawing();
        ClearBackground(C_BG);

        // ── Global Interface Containers (Always Visible) ──
        if (DrawSidebar(view, music, mouse)) {
            EndDrawing(); // Safely wrap render pipelines before function termination
            // Clean up audio the same way the normal exit path below does,
            // so the next login's Dashboard() call starts from a clean state.
            if (music.loaded) { StopMusicStream(music.track); UnloadMusicStream(music.track); }
            CloseAudioDevice();
            return true;  // Tell main() this was a logout — go back to the login screen
        }
        DrawTopBar(sessionSecs);

        // ── View Router Content Engine Layout (Unlocks Calendar & Statistics) ──
        switch (view) {
            case VIEW_DASH:
                DrawDashView(dashScroll, sortMode, modal, mouse);
                break;
            case VIEW_FOCUS:
                DrawFocusView(focusScroll, modal, focTimer, focusTaskIdx, mouse);
                break;
            case VIEW_STATS:
                DrawStatsView(statsScroll, modal, mouse);
                break;
            case VIEW_CALENDAR:
                DrawCalView(calMonth, calYear, mouse);
                break;
            case VIEW_PROFILE:
                DrawProfileView(mouse);
                break;
        }

        // ── Popup Modal Overlay System Layer (Stays completely crash-proof) ──
     // ── Popup Modal Overlay System Layer (Stays completely crash-proof) ──
        if (modal.open) {
            if (modal.taskIdx == -1) {
                DrawNewTaskModal(modal, mouse); // Opens New Form Input Box Layer
            } else {
                DrawEditModal(modal, mouse);    // Passes direct reference array layout safely
            }
        }

        // ── Delete confirmation popup (drawn above everything else) ──
        if (modal.confirmDeleteOpen) {
            DrawConfirmDeleteModal(modal, mouse);
        }

        // ── Reminder Banner Overlay Notification ──
        {
            time_t nowT = time(0);
            int urgent = 0;
            for (auto& t : allTasks) if (!t.isCompleted) {
                tm dl = {0}; dl.tm_year = t.dueYear - 1900; dl.tm_mon = t.dueMonth - 1; dl.tm_mday = t.dueDay;
                double secs = difftime(mktime(&dl), nowT);
                if (secs / (86400.0) < 2.0) urgent++;
            }
            if (urgent > 0) {
                char bang[64]; snprintf(bang, sizeof(bang), "!! %d task(s) due within 2 days!", urgent);
                int bw = MeasureText(bang, 14) + 24;
                DrawRoundRect({(float)(WIN_W / 2 - bw / 2), 2, (float)bw, 26}, 0.4f, {180, 40, 40, 220});
                DrawText(bang, WIN_W / 2 - bw / 2 + 12, 6, 14, C_WHITE);
            }
        }

        EndDrawing();
    }

    if (music.loaded) { StopMusicStream(music.track); UnloadMusicStream(music.track); }
    CloseAudioDevice();
    return false; // Window was closed (not a logout) — main() should quit entirely
}
