#include "calendar.h"
#include <raylib.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <cstdio>
#define CAL_CELL_WIDTH 60
#define CAL_CELL_HEIGHT 50
#define CAL_START_X 45
#define CAL_START_Y 180

using namespace std;

// GitHub Dark-Mode Contribution Color Palette Definitions
#define COLOR_EMPTY        (Color){ 22, 27, 34, 255 }     // Dark Gray Empty Node
#define COLOR_ACTIVE_BASE  (Color){ 57, 211, 83, 255 }    // Base green — alpha scales with completion count

#define CELL_SIZE 30
#define CELL_PADDING 8  // Spacing between the heatmap boxes (enlarged)

// Intensity scaling for the heatmap: how many completions = fully opaque,
// and the alpha range used to represent "a little" vs "a lot" of activity.
#define MAX_INTENSITY_CAP  5
#define ALPHA_MIN          60
#define ALPHA_MAX          255

// LEAP YEAR CHECK
bool isLeapYear(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// DAYS IN MONTH CALCULATOR
int getDaysInMonth(int month, int year) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}

// MONTH STRING UTILITY
string getMonthName(int month) {
    string m[] = {"January", "February", "March", "April", "May", "June", 
                  "July", "August", "September", "October", "November", "December"};
    return m[month - 1];
}

// START DAY DAY-OF-WEEK CALCULATOR
int getStartDay(int month, int year) {
    tm time_in = {0};
    time_in.tm_year = year - 1900;
    time_in.tm_mon = month - 1;
    time_in.tm_mday = 1;
    mktime(&time_in);
    return time_in.tm_wday; // 0 = Sunday, 1 = Monday ...
}

// LEGACY BOOLEAN CHECKER
bool hasTaskOnDate(int day, int month, int year) {
    for (const auto &t : allTasks) {
        if (t.dueDay == day && t.dueMonth == month && t.dueYear == year) {
            return true;
        }
    }
    return false;
}

// UTILITY TO FETCH TOTAL COMPLETED TASKS FOR A GIVEN DATE
// Keys off dateCompleted (the day the task was actually finished), not the
// due date — a task completed today should light up today, not on whatever
// day it happened to be due.
int getCompletedTaskCount(int day, int month, int year) {
    int count = 0;
    for (const auto &t : allTasks) {
        if (!t.isCompleted || t.dateCompleted.empty()) continue;
        int cy = 0, cm = 0, cd = 0;
        sscanf(t.dateCompleted.c_str(), "%d-%d-%d", &cy, &cm, &cd);
        if (cd == day && cm == month && cy == year) count++;
    }
    return count;
}

// GITHUB HEATMAP RAYLIB RENDERER BLOCK
void DrawGitHubHeatmap(int startX, int startY, int currentMonth, int currentYear) {
    int totalDays = getDaysInMonth(currentMonth, currentYear);
    int startOffsetWeekday = getStartDay(currentMonth, currentYear);

    // Title text
    DrawText(
"Task Completion Activity",
startX,
startY-35,
22,
WHITE
);

    // Days of the week row-labels helper (S M T W T F S)
    const char* daysOfWeek[] = {"S", "M", "T", "W", "T", "F", "S"};
    for (int i = 0; i < 7; i++) {
        int labelY = startY + (i * (CELL_SIZE + CELL_PADDING)) + 2;
        DrawText(daysOfWeek[i], startX - 25, labelY, 12, GRAY);
    }

    // Process every single valid day of the calendar month inside the contribution array grid
    for (int d = 1; d <= totalDays; d++) {
        int targetCellIndex = startOffsetWeekday + (d - 1);
        int row = targetCellIndex % 7;       // Map into vertical Day of Week row position
        int col = targetCellIndex / 7;       // Map into horizontal Week Number column position

        // Determine activity metrics count
        int completedAmount = getCompletedTaskCount(d, currentMonth, currentYear);

        // No completions -> empty dark cell. Otherwise, keep the same green
        // hue for every day but scale its transparency (alpha) with how many
        // tasks were completed — a light wash for 1 task, fully solid once
        // completions reach MAX_INTENSITY_CAP. No yellow/amber is ever used
        // for pending tasks; this heatmap only reflects completions.
        Color cellColor = COLOR_EMPTY;
        if (completedAmount > 0) {
            int capped = completedAmount > MAX_INTENSITY_CAP ? MAX_INTENSITY_CAP : completedAmount;
            unsigned char alpha = (unsigned char)(ALPHA_MIN +
                ((ALPHA_MAX - ALPHA_MIN) * (capped - 1)) / (MAX_INTENSITY_CAP - 1));
            cellColor = COLOR_ACTIVE_BASE;
            cellColor.a = alpha;
        }

        // Calculate layout screen space metrics
        float posX = startX + (col * (CELL_SIZE + CELL_PADDING));
        float posY = startY + (row * (CELL_SIZE + CELL_PADDING));

        Rectangle cellRect = { posX, posY, (float)CELL_SIZE, (float)CELL_SIZE };

        // Draw node item using roundness vector parameters to duplicate Github visual engine style
        DrawRectangleRounded(cellRect, 0.25f, 4, cellColor);

        // Draw inner tracking indicator digits inside active boxes
        char dayStr[4];
        snprintf(dayStr, sizeof(dayStr), "%d", d);
        int textWidth = MeasureText(dayStr, 10);
        DrawText(dayStr, posX + (CELL_SIZE - textWidth)/2, posY + 7, 10, (completedAmount >= 3) ? BLACK : LIGHTGRAY);
    }

    // Base UI Palette Indicator Legend Container — same green hue throughout,
    // increasing opacity from left (empty / low activity) to right (high activity).
    int legendX = startX;
    int legendY = startY + (7 * (CELL_SIZE + CELL_PADDING)) + 20;
    DrawText("Less", legendX, legendY, 13, GRAY);

    for (int i = 0; i < 5; i++) {
        Color legColor;
        if (i == 0) {
            legColor = COLOR_EMPTY;
        } else {
            unsigned char a = (unsigned char)(ALPHA_MIN + ((ALPHA_MAX - ALPHA_MIN) * i) / 4);
            legColor = COLOR_ACTIVE_BASE;
            legColor.a = a;
        }
        Rectangle legRect = { (float)(legendX + 45 + (i * 24)), (float)legendY - 2, 16, 16 };
        DrawRectangleRounded(legRect, 0.25f, 4, legColor);
    }
    DrawText("More", legendX + 175, legendY, 13, GRAY);
}

// MAIN GRAPHICAL CALENDAR RUNTIME ENVIRONMENT
void displayCalendar() {
    // Acquire current date from local hardware system clock configurations
    time_t now = time(0);
    tm *ltm = localtime(&now);
    int currentYear = ltm->tm_year + 1900;
    int currentMonth = ltm->tm_mon + 1;
    int today = ltm->tm_mday;

    // Window Setup configuration routines
    const int winWidth = 1100;
const int winHeight = 700;
    
    // Mute annoying Raylib debug initialization terminal traces
    SetTraceLogLevel(LOG_NONE); 

    // Avoid double-initializing if window structure is already alive in main login modules
    if (!IsWindowReady()) {
        InitWindow(winWidth, winHeight, "Task Workspace & Contribution Heatmap Dashboard");
        SetTargetFPS(60);
    }

    while (!WindowShouldClose()) {
        // Handle Month navigation buttons via arrow keys
        if (IsKeyPressed(KEY_RIGHT)) {
            currentMonth++;
            if (currentMonth > 12) { currentMonth = 1; currentYear++; }
        }
        if (IsKeyPressed(KEY_LEFT)) {
            currentMonth--;
            if (currentMonth < 1) { currentMonth = 12; currentYear--; }
        }

        BeginDrawing();
        ClearBackground((Color){ 11, 17, 26, 255 }); // GitHub dark background palette match
        DrawRectangleRounded(
    {25, 145, 470, 520},
    0.05f,
    5,
    (Color){18,24,32,255}
);

DrawRectangleRounded(
    {515,145,560,520},
    0.05f,
    5,
    (Color){18,24,32,255}
);

        // Workspace headers
        const char* title = "TASK WORKSPACE DASHBOARD";

int titleWidth = MeasureText(title,30);

DrawText(
    title,
    (winWidth-titleWidth)/2,
    20,
    30,
    WHITE
);
        
        string fullMonthHeading = getMonthName(currentMonth) + " " + to_string(currentYear);
        DrawText(
    fullMonthHeading.c_str(),
    45,
    95,
    26,
    ORANGE
);
    DrawText(
"← Previous Month        → Next Month",
45,
125,
18,
LIGHTGRAY
);

        // Render the main calendar grids layout system using text
        int startDayOfWeek = getStartDay(currentMonth, currentYear);
        int totalDaysInMonth = getDaysInMonth(currentMonth, currentYear);
        DrawText(
    "Monthly Calendar",
    45,
    155,
    22,
    WHITE
);
        const char* horizontalHeaders[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        for (int i = 0; i < 7; i++) {
            // Changed from CYAN to pre-defined native SKYBLUE constant
            DrawText(horizontalHeaders[i], CAL_START_X + i * CAL_CELL_WIDTH, 170, 18, SKYBLUE);
        }

        int currRow = 0;
        for (int d = 1; d <= totalDaysInMonth; d++) {
            int gridColumn = (startDayOfWeek + d - 1) % 7;
            int renderX = CAL_START_X + gridColumn * CAL_CELL_WIDTH;
int renderY = CAL_START_Y + currRow * CAL_CELL_HEIGHT;



Rectangle dayRect =
{
    (float)renderX,
    (float)renderY,
    52,
    42
};

DrawRectangleRounded(
    dayRect,
    0.15f,
    4,
    (Color){24,30,40,255}
);

DrawRectangleRoundedLines(
    dayRect,
    0.15f,
    4,
    DARKGRAY
);

            char dayBuf[8];
            snprintf(dayBuf, sizeof(dayBuf), "%d", d);

            if (d == today && currentMonth == (ltm->tm_mon + 1) && currentYear == (ltm->tm_year + 1900)) {
               DrawCircle(
renderX + 26,
renderY + 21,
18,
RED
);
                DrawText(dayBuf, renderX, renderY, 15, WHITE);
            } else if (hasTaskOnDate(d, currentMonth, currentYear)) {
                DrawText(dayBuf, renderX, renderY, 15, (Color){ 0, 255, 150, 255 }); // Pending deadline check
            } else {
                int tw = MeasureText(dayBuf,20);

DrawText(
dayBuf,
renderX + (52-tw)/2,
renderY + 10,
20,
WHITE
);
            }

            if (gridColumn == 6) currRow++;
        }

        // Render our Contribution Heatmap Dashboard Matrix Panel
        int heatmapPanelX = 560;
        int heatmapPanelY = 185;

        DrawText(
"Contribution Heatmap",
560,
155,
22,
WHITE
);
        DrawGitHubHeatmap(heatmapPanelX, heatmapPanelY, currentMonth, currentYear);

        // Frame instructions box overlay
        
        DrawRectangle(
0,
660,
1100,
40,
(Color){18,24,32,255}
);

DrawText(
"Green = Completed Tasks      Red = Today      ← → Change Month",
30,
672,
18,
LIGHTGRAY
);

DrawLine(
510,
150,
510,
645,
(Color){50,60,70,255}
);
        int completed = 0;

for (const auto& t : allTasks)
{
    if(t.isCompleted)
        completed++;
}

int pending = allTasks.size() - completed;

DrawRectangleRounded(
    {550,500,500,140},
    0.1f,
    5,
    (Color){20,25,34,255}
);

DrawText(
"Statistics",
570,
515,
24,
WHITE
);

DrawText(
TextFormat("Completed : %d",completed),
570,
555,
20,
GREEN
);

DrawText(
TextFormat("Pending : %d",pending),
570,
585,
20,
ORANGE
);

DrawText(
TextFormat("Total Tasks : %d",(int)allTasks.size()),
800,
555,
20,
WHITE
);

float percent = 0;

if(!allTasks.empty())
    percent = completed*100.0f/allTasks.size();

DrawText(
TextFormat("Completion : %.1f%%",percent),
800,
585,
20,
SKYBLUE
);

        EndDrawing();
    }
    
    // Graceful cleanup back to CLI terminal workspace loop
    CloseWindow();
}


