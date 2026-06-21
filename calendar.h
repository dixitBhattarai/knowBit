#ifndef CALENDAR_H
#define CALENDAR_H

#include "task.h"

// Core Date Logic Functions
bool isLeapYear(int year);
int getDaysInMonth(int month, int year);
std::string getMonthName(int month);
int getStartDay(int month, int year);
bool hasTaskOnDate(int day, int month, int year);

// GitHub Heatmap Graphic Engine Function
void DrawGitHubHeatmap(int startX, int startY, int currentMonth, int currentYear);

// Main Calendar Interface Wrapper
void displayCalendar();

#endif // CALENDAR_H