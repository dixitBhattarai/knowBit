#ifndef TASK_H
#define TASK_H
#include <iostream>
#include <vector>
#include <string>

struct Task {
    int taskId;
    std::string taskName;
    std::string taskDescription;
    std::string taskCategory;
    int priority;
    bool isCompleted;
    int daysToComplete;
    int dueDay;
    int dueMonth;
    int dueYear;
    std::string dateCreated;
    std::string dateCompleted; 
};
extern std::vector<Task> allTasks;
extern std::string activeUsername;
extern int activeUserStreak;
extern int activeUserMaxStreak;
extern int lastYear;
extern int lastMonth;
extern int lastDay;
extern int totalXP;               // Cumulative XP earned by completing tasks
void loadUserData();
void saveUserData();
void addTask();
void displayTasks();
void editTask();
void removeTask();
void markCompleted();
void displayHistory();
void checkReminders();
void logoutUser();

// ── XP / Leveling helpers ──
int xpForPriority(int priority);      // XP awarded for completing a task of this priority
int levelForXP(int xp);               // Current level derived from total XP
int xpIntoCurrentLevel(int xp);       // XP progress inside the current level (0-99)
int xpNeededForNextLevel();           // XP required per level (constant, kept as a function for flexibility)

// ── Misc helpers shared across CLI + GUI ──
int nextAvailableTaskId();            // Auto-generated unique task id (user no longer enters one)
std::string todayDateString();        // "YYYY-MM-DD" for the current date, used as dateCompleted
#endif