#ifndef CALENDAR_H
#define CALENDAR_H

#include "task.h"


bool isLeapYear(int year);
int getDaysInMonth(int month, int year);
std::string getMonthName(int month);
int getStartDay(int month, int year);
bool hasTaskOnDate(int day, int month, int year);
void DrawGitHubHeatmap(int startX, int startY, int currentMonth, int currentYear);
void displayCalendar();
#endif 