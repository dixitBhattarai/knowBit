#ifndef SUYANS_H
#define SUYANS_H
float calculateCompletionRate(int totalTasks, int completedTasks);
void showProgressBar(float completionRate);
void showBadge(float completionRate, int currentStreak);
void showLevel(int maxStreak);
void displayGamification();
#endif