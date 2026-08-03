#include "suyans.h"
#include "task.h" 
#include <iostream>
#include <string>
using namespace std;
using namespace std;
float calculateCompletionRate(int totalTasks, int completedTasks) {
    if (totalTasks == 0) return 0;
    return (completedTasks * 100.0) / totalTasks;
}
void showProgressBar(float completionRate) {
    int barLength = 20; 
    int filled = (completionRate / 100) * barLength;
    cout << "Progress: [";
    for (int i = 0; i < barLength; i++) {
        if (i < filled) cout << "#";
        else cout << "-";
    }
    cout << "] " << completionRate << "%" << endl;
}
void showBadge(float completionRate, int currentStreak) {
    cout << "Badge Earned: ";
    if (completionRate == 100 && completionRate > 0) cout << "🌟 Perfect Day Badge!" << endl;
    else if (completionRate >= 80) cout << "🔥 Productivity Master!" << endl;
    else if (completionRate >= 50) cout << "👍 Keep Grinding!" << endl;
    else cout << "🌱 Beginner Badge" << endl;
}
void showLevel(int maxStreak) {
    cout << "Level: ";
    if (maxStreak >= 30) cout << "Legend" << endl;
    else if (maxStreak >= 15) cout << "Pro" << endl;
    else if (maxStreak >= 7) cout << "Intermediate" << endl;
    else cout << "Novice" << endl;
}
void displayGamification() {
    int totalTasks = allTasks.size(); 
    int completedTasks = 0;

    for (const auto& t : allTasks) {
        if (t.isCompleted) completedTasks++;
    }
    int currentStreak = activeUserStreak;
    int maxStreak = activeUserMaxStreak;
    cout << "\n========== GAMIFICATION STATS ==========\n";
    cout << "Total Tasks:     " << totalTasks << "\n";
    cout << "Completed Tasks: " << completedTasks << "\n\n";
    float rate = calculateCompletionRate(totalTasks, completedTasks);
    showProgressBar(rate);
    showBadge(rate, currentStreak);
    showLevel(maxStreak);
    cout << "========================================\n";
}