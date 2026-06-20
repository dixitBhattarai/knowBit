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
};
extern std::vector<Task> allTasks;
extern std::string activeUsername;
extern int activeUserStreak;
extern int activeUserMaxStreak;
extern int lastYear;
extern int lastMonth;
extern int lastDay;
void loadUserData();
void saveUserData();
void addTask();
void displayTasks();
void editTask();
void removeTask();
void markCompleted();
void displayHistory();
void checkReminders();
#endif