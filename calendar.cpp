#include "calendar.h"
#include "include/raylib.h" 
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>

using namespace std;

// GitHub Dark-Mode Contribution Color Palette Definitions
#define COLOR_EMPTY        (Color){ 22, 27, 34, 255 }     // Dark Gray Empty Node
#define COLOR_LEVEL_1      (Color){ 14, 68, 41, 255 }     // Low activity (1 task complete)
#define COLOR_LEVEL_2      (Color){ 0, 109, 50, 255 }     // Medium-low (2 tasks complete)
#define COLOR_LEVEL_3      (Color){ 38, 166, 65, 255 }    // Medium-high (3-4 tasks complete)
#define COLOR_LEVEL_4      (Color){ 57, 211, 83, 255 }    // Maximum activity (5+ tasks complete)

#define CELL_SIZE          18   // Box dimension width and height
#define CELL_PADDING       4    // Spacing between the heatmap boxes

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
int getCompletedTaskCount(int day, int month, int year) {
    int count = 0;
    for (const auto &t : allTasks) {
        if (t.isCompleted && t.dueDay == day && t.dueMonth == month && t.dueYear == year) {
            count++;
        }
    }
    return count;
}

// GITHUB HEATMAP RAYLIB RENDERER BLOCK
void DrawGitHubHeatmap(int startX, int startY, int currentMonth, int currentYear) {
    int totalDays = getDaysInMonth(currentMonth, currentYear);
    int startOffsetWeekday = getStartDay(currentMonth, currentYear);

    // Title text
    DrawText("Task Completion Activity Matrix", startX, startY - 28, 18, WHITE);

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

        // Classify standard GitHub contribution density thresholds
        Color cellColor = COLOR_EMPTY;
        if (completedAmount == 1)               cellColor = COLOR_LEVEL_1;
        else if (completedAmount == 2)          cellColor = COLOR_LEVEL_2;
        else if (completedAmount >= 3 && completedAmount <= 4) cellColor = COLOR_LEVEL_3;
        else if (completedAmount >= 5)          cellColor = COLOR_LEVEL_4;

        // Calculate layout screen space metrics
        float posX = startX + (col * (CELL_SIZE + CELL_PADDING));
        float posY = startY + (row * (CELL_SIZE + CELL_PADDING));

        Rectangle cellRect = { posX, posY, (float)CELL_SIZE, (float)CELL_SIZE };

        // Draw node item using roundness vector parameters to duplicate Github visual engine style
        DrawRectangleRounded(cellRect, 0.25f, 4, cellColor);

        // Draw inner tracking indicator digits inside active boxes
        char dayStr[4];
        snprintf(dayStr, sizeof(dayStr), "%d", d);
        int textWidth = MeasureText(dayStr, 9);
        DrawText(dayStr, posX + (CELL_SIZE - textWidth)/2, posY + 4, 9, (completedAmount >= 3) ? BLACK : LIGHTGRAY);
    }

    // Base UI Palette Indicator Legend Container
    int legendX = startX;
    int legendY = startY + (7 * (CELL_SIZE + CELL_PADDING)) + 20;
    DrawText("Less", legendX, legendY, 13, GRAY);
    
    Color legendPalette[] = { COLOR_EMPTY, COLOR_LEVEL_1, COLOR_LEVEL_2, COLOR_LEVEL_3, COLOR_LEVEL_4 };
    for (int i = 0; i < 5; i++) {
        Rectangle legRect = { (float)(legendX + 45 + (i * 24)), (float)legendY - 2, 16, 16 };
        DrawRectangleRounded(legRect, 0.25f, 4, legendPalette[i]);
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
    const int winWidth = 900;
    const int winHeight = 650;
    
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

        // Workspace headers
        DrawText("CALENDAR CONTROL DASHBOARD", 40, 30, 26, WHITE);
        
        string fullMonthHeading = getMonthName(currentMonth) + " " + to_string(currentYear);
        DrawText(fullMonthHeading.c_str(), 40, 80, 22, (Color){ 255, 128, 0, 255 });
        DrawText("Use Left/Right Arrow Keys to navigate between months", 40, 115, 14, GRAY);

        // Render the main calendar grids layout system using text
        int startDayOfWeek = getStartDay(currentMonth, currentYear);
        int totalDaysInMonth = getDaysInMonth(currentMonth, currentYear);
        
        const char* horizontalHeaders[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        for (int i = 0; i < 7; i++) {
            // Changed from CYAN to pre-defined native SKYBLUE constant
            DrawText(horizontalHeaders[i], 50 + (i * 65), 170, 15, SKYBLUE);
        }

        int currRow = 0;
        for (int d = 1; d <= totalDaysInMonth; d++) {
            int gridColumn = (startDayOfWeek + d - 1) % 7;
            int renderX = 50 + (gridColumn * 65);
            int renderY = 205 + (currRow * 40);

            char dayBuf[8];
            snprintf(dayBuf, sizeof(dayBuf), "%d", d);

            if (d == today && currentMonth == (ltm->tm_mon + 1) && currentYear == (ltm->tm_year + 1900)) {
                DrawCircle(renderX + 12, renderY + 8, 16, (Color){ 230, 80, 80, 255 });
                DrawText(dayBuf, renderX, renderY, 15, WHITE);
            } else if (hasTaskOnDate(d, currentMonth, currentYear)) {
                DrawText(dayBuf, renderX, renderY, 15, (Color){ 0, 255, 150, 255 }); // Pending deadline check
            } else {
                DrawText(dayBuf, renderX, renderY, 15, LIGHTGRAY);
            }

            if (gridColumn == 6) currRow++;
        }

        // Render our Contribution Heatmap Dashboard Matrix Panel
        int heatmapPanelX = 520;
        int heatmapPanelY = 205;
        DrawGitHubHeatmap(heatmapPanelX, heatmapPanelY, currentMonth, currentYear);

        // Frame instructions box overlay
        DrawRectangleLines(40, 520, 820, 80, DARKGRAY);
        DrawText("LEGEND INDICATOR PROFILE:", 60, 535, 13, WHITE);
        DrawText("* Green Matrix Shading indicates task volumes successfully COMPLETED on that specific calendar day.", 60, 555, 12, GRAY);
        DrawText("* Highlighted skyblue columns track weekdays. Red circles specify today.", 60, 575, 12, GRAY);

        EndDrawing();
    }
    
    // Graceful cleanup back to CLI terminal workspace loop
    CloseWindow();
}