// ============================================================
//  knowBit v1.0  –  GUI Dashboard  (dashboard.cpp)
//  Replaces arohaMain.cpp's CLI Dashboard() loop.
//  Single Raylib window; no terminal switching.
// ============================================================

#include "dashboard.h"
#include "task.h"
#include "calendar.h"   // displayCalendar() draws into the same window
#include "suyans.h"

#include "include/raylib.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <sstream>

using namespace std;
extern Font customFont;
extern Texture2D texLogo, texStreak, texClock, texMusic, texCalendar, texEdit, texDelete;

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
enum View { VIEW_DASH, VIEW_FOCUS, VIEW_STATS, VIEW_CALENDAR };

// ─── Sort mode ──────────────────────────────────────────────
enum SortMode { SORT_NONE, SORT_PRIORITY, SORT_DUE };

// ─── Modal / overlay state ──────────────────────────────────
struct ModalState {
    bool open          = false;
    // shared
    int  taskIdx       = -1;     // index into allTasks for edit/focus
    // new task form fields
    char tfId[8]       = "";
    char tfName[64]    = "";
    char tfDesc[128]   = "";
    char tfCat[32]     = "";
    char tfPri[4]      = "";
    char tfDays[8]     = "";
    int  activeField   = 0;
    string errorMsg    = "";
    // edit fields (only due-date / priority / type)
    char efPri[4]      = "";
    char efDays[8]     = "";
    char efCat[32]     = "";
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

// Draw rounded rectangle helper
static void DrawRoundRect(Rectangle r, float round, Color c) {
    DrawRectangleRounded(r, round, 8, c);
}
static void DrawRoundRectLines(Rectangle r, float round, float thick, Color c) {
    DrawRectangleRoundedLinesEx(r, round, 8, thick, c);
}

// Text input handler — returns true if value changed
static bool HandleTextInput(char* buf, int maxLen, int key) {
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
//  Sidebar
// ─────────────────────────────────────────────────────────────
static void DrawSidebar(View& view, MusicState& music, Vector2 mouse) {
    DrawRectangle(0, 0, SIDE_W, WIN_H, C_SIDEBAR);
    DrawLine(SIDE_W, 0, SIDE_W, WIN_H, C_BORDER);

    // ── Logo and Title (Perfectly level with Top Bar greeting) ──
    DrawTextureEx(texLogo, { 15, 18 }, 0.0f, 0.07f, WHITE);
    DrawText("know", 65, 22, 22, WHITE);
    DrawText("Bit", 65 + MeasureText("know", 22), 22, 22, C_ORANGE);
    DrawText("v1.0", 65 + MeasureText("knowBit", 22) + 6, 30, 10, GRAY);

    // ── Profile & Streak Info Box ──
    // ── Profile & Streak Info Box ──
    DrawRoundRect({12, 80, SIDE_W-24, 55}, 0.15f, C_DARKGRAY);

    // FIX: Moved icon up (Y: 88) and enlarged scale to 0.09f to span both rows
    DrawTextureEx(texStreak, {18, 88}, 0.0f, 0.09f, WHITE); 

    // Both text rows start at X: 58 so they sit beautifully next to the larger flame
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

    // ── Calendar Dynamic Hint Text ──
    if (view == VIEW_CALENDAR) {
        DrawText("Please use Arrows keys", 15, ny + 20, 12, C_TEXT_DIM);
    }

    // ── Lo-fi Music Interactive Engine ──
// ── Lo-fi Music Interactive Engine ──
    Rectangle musicBar = {8, (float)(WIN_H - 58), (float)(SIDE_W-16), 38};
    bool musicHov = CheckCollisionPointRec(mouse, musicBar);
    DrawRoundRect(musicBar, 0.2f, C_DARKGRAY);
    DrawRoundRectLines(musicBar, 0.2f, 1.0f, musicHov ? C_ORANGE : C_BORDER);
    
    DrawTextureEx(texMusic, {16, (float)(WIN_H - 46)}, 0.0f, 0.05f, WHITE);
    DrawText("   Lo-fi Music", 16, WIN_H - 46, 13, C_TEXT_DIM);
    DrawText(music.on ? "ON" : "OFF", SIDE_W - 44, WIN_H - 46, 13, music.on ? C_GREEN : C_TEXT_DIM);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && musicHov) {
        music.on = !music.on;
    }

    // ── Pop-up Audio Track Selection Menu Window ──
    if (music.on) {
        // FIX: Using a local static variable to track the active song index safely
        static int selectedTrack = 0; 

        Rectangle popupMenu = {8, (float)(WIN_H - 210), (float)(SIDE_W - 16), 145};
        DrawRoundRect(popupMenu, 0.15f, C_PANEL);
        DrawRoundRectLines(popupMenu, 0.15f, 1.0f, C_ORANGE);
        
        DrawText("Select Track:", (int)popupMenu.x + 12, (int)popupMenu.y + 10, 12, C_ORANGE);
        
        const char* tracks[] = { "1. Lofi Chill Beats", "2. Study Focus Session", "3. Midnight Cafe Vibes" };
        int trackY = popupMenu.y + 32;
        
        for (int i = 0; i < 3; i++) {
            Rectangle trackRow = { popupMenu.x + 6, (float)trackY, popupMenu.width - 12, 26 };
            bool rowHov = CheckCollisionPointRec(mouse, trackRow);
            bool isCurrent = (selectedTrack == i); 
            
            if (isCurrent)   DrawRectangleRec(trackRow, {255, 128, 0, 40});
            else if (rowHov) DrawRectangleRec(trackRow, {255, 255, 255, 15});
            
            DrawText(tracks[i], (int)trackRow.x + 8, (int)trackRow.y + 6, 11, 
                     isCurrent ? C_ORANGE : (rowHov ? C_WHITE : C_TEXT_DIM));
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && rowHov) {
                selectedTrack = i;
                // Optional: If you have a play track function, call it here using 'i'
            }
            trackY += 30;
        }
    }
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
    DrawTextureEx(texClock, {tc.x + 10, tc.y + 10}, 0.0f, 0.08f, WHITE);
    string sess = fmtSession(sessionSecs);
    DrawText(sess.c_str(), (int)tc.x + 44, (int)tc.y + 8, 26, C_GREEN);
    DrawText("Daily App Time", (int)tc.x + 10, (int)tc.y + 38, 12, C_TEXT_DIM);
    
    string ct = currentTimeStr();
    int ctw = MeasureText(ct.c_str(), 13);
    DrawText(ct.c_str(), (int)(tc.x + tc.width - ctw - 10), (int)tc.y + 38, 13, C_TEAL);
}

// ─────────────────────────────────────────────────────────────
//  New Task modal
// ─────────────────────────────────────────────────────────────
static void DrawNewTaskModal(ModalState& m, Vector2 mouse) {
    // Dimmed overlay
    DrawRectangle(0, 0, WIN_W, WIN_H, {0,0,0,160});

    Rectangle box = {WIN_W/2 - 300.0f, WIN_H/2 - 280.0f, 600, 560};
    DrawRoundRect(box, 0.04f, C_PANEL);
    DrawRoundRectLines(box, 0.04f, 1.5f, C_ORANGE);

    DrawText("+ New Task", (int)box.x + 24, (int)box.y + 18, 20, C_ORANGE);

    float fx = box.x + 24, fy = box.y + 60, fw = box.width - 48, fh = 36;

    // 6 fields: ID, Name, Desc, Category, Priority, Days
    struct Field { const char* label; char* buf; int maxLen; };
    Field fields[] = {
        { "Task ID",           m.tfId,   8   },
        { "Task Name",         m.tfName, 64  },
        { "Description",       m.tfDesc, 128 },
        { "Category / Type",   m.tfCat,  32  },
        { "Priority (1-5)",    m.tfPri,  4   },
        { "Days to Complete",  m.tfDays, 8   },
    };

    for (int i = 0; i < 6; i++) {
        Rectangle r = {fx, fy + i*76.0f, fw, (float)fh};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, r))
            m.activeField = i;
        bool act = (m.activeField == i);
        DrawInputBox(fields[i].label, fields[i].buf, r, act);
    }

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

    // Keyboard input for active field
    if (IsKeyPressed(KEY_TAB)) m.activeField = (m.activeField + 1) % 6;
    HandleTextInput(fields[m.activeField].buf,
                    fields[m.activeField].maxLen,
                    IsKeyPressed(KEY_BACKSPACE) ? KEY_BACKSPACE : 0);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (addHov) {
            // Validate
            if (TextLength(m.tfId) == 0 || TextLength(m.tfName) == 0) {
                m.errorMsg = "Task ID and Name are required.";
            } else {
                int newId = atoi(m.tfId);
                bool dup = false;
                for (auto& t : allTasks) if (t.taskId == newId) { dup=true; break; }
                if (dup) {
                    m.errorMsg = "Task ID already exists!";
                } else {
                    Task t;
                    t.taskId          = newId;
                    t.taskName        = m.tfName;
                    t.taskDescription = m.tfDesc;
                    t.taskCategory    = m.tfCat;
                    t.priority        = max(1, min(5, atoi(m.tfPri)));
                    t.daysToComplete  = max(1, atoi(m.tfDays));
                    t.isCompleted     = false;
                    time_t now = time(0);
                    now += t.daysToComplete * 86400;
                    tm* dl = localtime(&now);
                    t.dueDay = dl->tm_mday; t.dueMonth = dl->tm_mon+1; t.dueYear = dl->tm_year+1900;
                    time_t rawtime; time(&rawtime);
                    char buf[32]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M",localtime(&rawtime));
                    t.dateCreated = buf;
                    allTasks.push_back(t);
                    saveUserData();
                    // reset modal
                    m.open = false;
                    memset(m.tfId,0,sizeof(m.tfId)); memset(m.tfName,0,sizeof(m.tfName));
                    memset(m.tfDesc,0,sizeof(m.tfDesc)); memset(m.tfCat,0,sizeof(m.tfCat));
                    memset(m.tfPri,0,sizeof(m.tfPri)); memset(m.tfDays,0,sizeof(m.tfDays));
                    m.errorMsg = ""; m.activeField = 0;
                }
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
                    IsKeyPressed(KEY_BACKSPACE) ? KEY_BACKSPACE : 0);
    if (IsKeyPressed(KEY_TAB)) m.activeField = (m.activeField+1)%3;

    Rectangle saveBtn = {fx, box.y + box.height - 52, fw/2-8, 38};
    Rectangle canBtn  = {fx+fw/2+8, box.y+box.height-52, fw/2-8, 38};
    bool sh = CheckCollisionPointRec(mouse,saveBtn);
    bool ch = CheckCollisionPointRec(mouse,canBtn);

    DrawRoundRect(saveBtn, 0.2f, sh ? C_TEAL : C_DARKGRAY);
    DrawText("Save", (int)saveBtn.x+20,(int)saveBtn.y+10,16, sh?C_BG:C_TEXT_DIM);
    DrawRoundRect(canBtn,  0.2f, ch ? C_RED  : C_DARKGRAY);
    DrawText("Cancel",(int)canBtn.x+12,(int)canBtn.y+10,16, ch?C_WHITE:C_TEXT_DIM);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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
    bool doDelete = false; 
    bool doEdit = false;
    bool doFocus = false;  
    bool doStats = false; 
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

    // ── Action Buttons Control Layer (Renders purely on element hover) ──
    if (hov) {
        if (mode == CARD_DASH) {
            // Edit icon asset (Top Right Corner stack placement)
            Rectangle edBtn = {x + w - 36, y + 14, 24, 24}; 
            bool eh = CheckCollisionPointRec(mouse, edBtn);
            DrawTextureEx(texEdit, {edBtn.x, edBtn.y}, 0.0f, 0.05f, eh ? LIGHTGRAY : WHITE);
            
            if (eh) {
                Rectangle tip = {edBtn.x - 48, edBtn.y + 2, 42, 20};
                DrawRoundRect(tip, 0.3f, C_DARKGRAY);
                DrawText("Edit", (int)tip.x + 8, (int)tip.y + 3, 11, C_WHITE);
            }

            // Delete icon asset (Stacked directly underneath edit button row)
            Rectangle delBtn = {x + w - 36, y + 46, 24, 24}; 
            bool dh = CheckCollisionPointRec(mouse, delBtn);
            DrawTextureEx(texDelete, {delBtn.x, delBtn.y}, 0.0f, 0.05f, dh ? LIGHTGRAY : WHITE);
            
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
        if (act.doDelete) {
            int id = tasks[i].taskId;
            allTasks.erase(remove_if(allTasks.begin(), allTasks.end(),
                [id](const Task& t){ return t.taskId==id; }), allTasks.end());
            saveUserData();
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
        HandleTextInput(ft.setMin, 6, IsKeyPressed(KEY_BACKSPACE)?KEY_BACKSPACE:0);

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
        const Task& t = allTasks[taskIdx];
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
    } else {
        DrawText("Select a task from the list →", (int)(cx-100),(int)(cy+170),13,C_TEXT_DIM);
    }
}

// ─────────────────────────────────────────────────────────────
//  Dashboard view
// ─────────────────────────────────────────────────────────────
static void DrawDashView(float& scroll, SortMode& sortMode, ModalState& modal, Vector2 mouse) {
    float lx = SIDE_W + 12, ly = TOP_H + 16;
    float lw = WIN_W - SIDE_W - 24, lh = WIN_H - TOP_H - 24;

    // Header row
    DrawText("Active Tasks", (int)lx, (int)ly, 20, C_WHITE);

    // ── Unified Single Sort Button ──
    // Positioned neatly to the left of the "+ Task" button
    Rectangle sortBtn = {(float)(WIN_W - 260), (float)(ly + 2), 150, 28};
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
    Rectangle nb = {(float)(WIN_W-96),(float)(ly-2), 84, 34};
    bool nh = CheckCollisionPointRec(mouse,nb);
    DrawRoundRect(nb, 0.3f, nh?C_GREEN:C_ORANGE);
    DrawText("+ Task", (int)nb.x+10,(int)nb.y+9, 14, C_BG);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && nh) {
        modal.open=true; modal.taskIdx=-1; modal.activeField=0; modal.errorMsg="";
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
    DrawTaskList(sorted, lx, ly+44, lw, lh-44, scroll, mouse,
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
//  Statistics view
// ─────────────────────────────────────────────────────────────
static void DrawStatsView(float& scroll, ModalState& modal, Vector2 mouse) {
    float lx = SIDE_W + 12, ly = TOP_H + 16;
    float lw = WIN_W - SIDE_W - 24, lh = WIN_H - TOP_H - 24;

    // Summary row
    int total = allTasks.size();
    int done  = 0;
    for (auto& t: allTasks) if (t.isCompleted) done++;

    char sum[128];
    snprintf(sum,sizeof(sum),
             "Total: %d   Completed: %d   Pending: %d   Streak: %d days   Max Streak: %d",
             total, done, total-done, activeUserStreak, activeUserMaxStreak);
    DrawText("Statistics", (int)lx, (int)ly, 20, C_WHITE);
    DrawText(sum, (int)lx, (int)ly+30, 13, C_TEXT_DIM);

    // Progress bar
    float rate = total>0 ? (done*100.0f/total) : 0.0f;
    Rectangle barBg = {lx, (float)(ly+54), lw*0.6f, 14};
    Rectangle barFg = {lx, (float)(ly+54), lw*0.6f*(rate/100.f), 14};
    DrawRoundRect(barBg,0.5f,C_DARKGRAY);
    DrawRoundRect(barFg,0.5f,C_GREEN);
    char pct[16]; snprintf(pct,sizeof(pct),"%.0f%%",rate);
    DrawText(pct,(int)(lx+lw*0.6f+8),(int)(ly+51),13,C_GREEN);

    DrawText("Hover a task and click ▤ for full stats",
             (int)lx,(int)(ly+78),12,C_TEXT_DIM);

    int dummy=-1;
    DrawTaskList(allTasks, lx, ly+96, lw, lh-96, scroll, mouse,
                 CARD_STATS, modal, dummy);
}

// ─────────────────────────────────────────────────────────────
//  Calendar view  — reuses calendar.cpp logic inline
// ─────────────────────────────────────────────────────────────
#include <iomanip>

static bool calLeap(int y){ return (y%4==0&&(y%100!=0||y%400==0)); }
static int  calDays(int m,int y){ int d[]={31,28,31,30,31,30,31,31,30,31,30,31}; if(m==2&&calLeap(y))return 29; return d[m-1]; }
static int  calStart(int m,int y){ tm t={0}; t.tm_year=y-1900; t.tm_mon=m-1; t.tm_mday=1; mktime(&t); return t.tm_wday; }
static string calMonthName(int m){ string n[]={"January","February","March","April","May","June","July","August","September","October","November","December"}; return n[m-1]; }

static int calCompletedCount(int d,int m,int y){
    int c=0;
    for(auto& t:allTasks) if(t.isCompleted&&t.dueDay==d&&t.dueMonth==m&&t.dueYear==y) c++;
    return c;
}
static bool calHasTask(int d,int m,int y){
    for(auto& t:allTasks) if(t.dueDay==d&&t.dueMonth==m&&t.dueYear==y) return true;
    return false;
}

static void DrawCalView(int& calMonth, int& calYear, Vector2 mouse) {
    float lx = SIDE_W + 24, ly = TOP_H + 20;

    // Nav
    if (IsKeyPressed(KEY_RIGHT)){calMonth++; if(calMonth>12){calMonth=1;calYear++;}}
    if (IsKeyPressed(KEY_LEFT)) {calMonth--; if(calMonth<1){calMonth=12;calYear--;}}

    time_t now2=time(0); tm* lt=localtime(&now2);
    int today2=lt->tm_mday, todayM=lt->tm_mon+1, todayY=lt->tm_year+1900;

    string heading = calMonthName(calMonth) + "  " + to_string(calYear);
    DrawText(heading.c_str(),(int)lx,(int)ly,22,C_ORANGE);
    // DrawText(" Please Use Arrow keys to navigate months",(int)lx,(int)ly+32,13,C_TEXT_DIM);

    // Day-of-week headers
    const char* dow[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    for(int i=0;i<7;i++)
        DrawText(dow[i],(int)(lx+i*78),(int)(ly+62),14,C_TEAL);

    int start=calStart(calMonth,calYear);
    int total=calDays(calMonth,calYear);
    int row=0;
    for(int d=1;d<=total;d++){
        int col=(start+d-1)%7;
        int rx=(int)(lx+col*78);
        int ry=(int)(ly+92+row*48);
        char db[8]; snprintf(db,sizeof(db),"%d",d);
        bool isToday=(d==today2&&calMonth==todayM&&calYear==todayY);
        bool hasTask=calHasTask(d,calMonth,calYear);
        if(isToday){
            DrawCircle(rx+12,ry+10,14,{200,60,60,200});
            DrawText(db,rx,ry,15,C_WHITE);
        } else if(hasTask){
            DrawText(db,rx,ry,15,{0,220,130,255});
        } else {
            DrawText(db,rx,ry,15,C_TEXT_DIM);
        }
        if(col==6) row++;
    }

    // Heatmap on right side
    float hx = lx + 7*78 + 40, hy = (float)(ly+62);
    DrawText("Completion Heatmap",(int)hx,(int)hy-22,14,C_WHITE);

    #define CELL 18
    #define CPAD 4
    const char* dow2[]={"S","M","T","W","T","F","S"};
    for(int i=0;i<7;i++) DrawText(dow2[i],(int)hx-20,(int)(hy+i*(CELL+CPAD))+2,11,C_TEXT_DIM);

    for(int d=1;d<=total;d++){
        int ci=start+(d-1);
        int r=ci%7, c=ci/7;
        int cnt=calCompletedCount(d,calMonth,calYear);
        Color cc = {22,27,34,255};
        if(cnt==1) cc={14,68,41,255};
        else if(cnt==2) cc={0,109,50,255};
        else if(cnt>=3&&cnt<=4) cc={38,166,65,255};
        else if(cnt>=5) cc={57,211,83,255};
        Rectangle cr={(float)(hx+c*(CELL+CPAD)),(float)(hy+r*(CELL+CPAD)),(float)CELL,(float)CELL};
        DrawRectangleRounded(cr,0.25f,4,cc);
        char ds[4]; snprintf(ds,sizeof(ds),"%d",d);
        int tw=MeasureText(ds,8);
        DrawText(ds,(int)(cr.x+(CELL-tw)/2),(int)(cr.y+4),8,cnt>=3?BLACK:LIGHTGRAY);
    }
    // Legend
    int legy=(int)(hy+7*(CELL+CPAD)+12);
    DrawText("Less",(int)hx,(int)legy,12,C_TEXT_DIM);
    Color lpal[]={{22,27,34,255},{14,68,41,255},{0,109,50,255},{38,166,65,255},{57,211,83,255}};
    for(int i=0;i<5;i++){
        Rectangle lr={(float)((int)hx+44+i*22),(float)(legy-2),16,16};
        DrawRectangleRounded(lr,0.25f,4,lpal[i]);
    }
    DrawText("More",(int)hx+160,(int)legy,12,C_TEXT_DIM);
    #undef CELL
    #undef CPAD
}

// ─────────────────────────────────────────────────────────────
//  Main Dashboard() — one Raylib window, all views
// ─────────────────────────────────────────────────────────────
void Dashboard() {
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
    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        double sessionSecs = GetTime() - g_sessionStart;

        // Music update
        if (music.loaded && music.on) UpdateMusicStream(music.track);


        // Detect if we're in new-task modal vs edit modal
        bool showNewTask = (modal.open && modal.taskIdx == -1);
        bool showEdit    = (modal.open && modal.taskIdx >= 0);

        BeginDrawing();
        ClearBackground(C_BG);

        // ── Sidebar + Topbar (always) ──
        DrawSidebar(view, music, mouse);
        DrawTopBar(sessionSecs);

        // ── Content area ──
        if (!modal.open) {
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
            }
        } else {
            // Draw the underlying view dimmed, then overlay
            switch (view) {
                case VIEW_DASH:
                    DrawDashView(dashScroll, sortMode, modal, mouse);
                    break;
                case VIEW_FOCUS:
                    DrawFocusView(focusScroll, modal, focTimer, focusTaskIdx, mouse);
                    break;
                default: break;
            }
            if (showNewTask) DrawNewTaskModal(modal, mouse);
            if (showEdit)    DrawEditModal(modal, mouse);
        }

        // ── Reminder banner ──
        {
            time_t nowT=time(0);
            int urgent=0;
            for(auto& t:allTasks) if(!t.isCompleted){
                tm dl={0}; dl.tm_year=t.dueYear-1900; dl.tm_mon=t.dueMonth-1; dl.tm_mday=t.dueDay;
                double secs=difftime(mktime(&dl),nowT);
                if(secs/(86400.0)<2.0) urgent++;
            }
            if(urgent>0){
                char bang[64]; snprintf(bang,sizeof(bang),"!! %d task(s) due within 2 days!",urgent);
                int bw=MeasureText(bang,14)+24;
                DrawRoundRect({(float)(WIN_W/2-bw/2),2,(float)bw,26},0.4f,{180,40,40,220});
                DrawText(bang,WIN_W/2-bw/2+12,6,14,C_WHITE);
            }
        }

        EndDrawing();
    }

    if (music.loaded) { StopMusicStream(music.track); UnloadMusicStream(music.track); }
    CloseAudioDevice();
}
