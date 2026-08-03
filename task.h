#ifndef TASK_H
#define TASK_H
#include <iostream>
#include <vector>
#include <string>

class Task {
public:
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

    // OOP Concept: Constructor
    // Ensures every Task starts in a known, safe default state instead
    // of having uninitialized int/bool fields.
    Task();

    // Marks this task as completed (sets isCompleted, stamps today's date,
    // and awards XP into the global totalXP tally). Returns the XP gained
    // so the caller can show it in a message/popup if it wants to.
    // Caller is still responsible for calling saveUserData() afterward,
    // same as every other task mutation in this codebase.
    int markDone();
};
extern std::vector<Task> allTasks;
extern std::string activeUsername;
extern int activeUserStreak;
extern int activeUserMaxStreak;
extern int lastYear;
extern int lastMonth;
extern int lastDay;
extern int totalXP;               
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

// ── Profile data (GitHub-style profile view) ──
// Persisted separately from task data so older save files stay untouched.
struct ProfileData {
    std::string joinDate;   // "YYYY-MM-DD", stamped once the first time a user opens Profile
    std::string fullName;
    std::string pronouns;
    std::string bio;
    std::string website;
    std::string socials;   // renamed from `twitter` — any social handle/link (X, Instagram, etc.)
    std::string linkedin;
};
extern ProfileData profile;
void loadProfileData();   // reads "<activeUsername>_profile.txt", creates one if missing
void saveProfileData();   // writes `profile` back to "<activeUsername>_profile.txt"
#endif