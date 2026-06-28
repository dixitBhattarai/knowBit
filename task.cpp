#include "task.h"
#include <ctime>
#include <fstream>
#include <sstream>
#include <cmath>
using namespace std;
vector<Task> allTasks;
string activeUsername = "";
int activeUserStreak = 0;
int activeUserMaxStreak = 0;
int lastYear = 0, lastMonth = 0, lastDay = 0;
void loadUserData() {
    allTasks.clear();
    ifstream file(activeUsername + ".txt");
    if (!file.is_open()) {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        lastYear = ltm->tm_year + 1900;
        lastMonth = ltm->tm_mon + 1;
        lastDay = ltm->tm_mday;
        activeUserStreak = 1;
        activeUserMaxStreak = 1;
        saveUserData();
        return;
    }
    string line;
    if (getline(file, line)) {
        stringstream ss(line);
        string temp;
        getline(ss, temp, '|'); activeUserStreak = stoi(temp);
        getline(ss, temp, '|'); activeUserMaxStreak = stoi(temp);
        getline(ss, temp, '|'); lastYear = stoi(temp);
        getline(ss, temp, '|'); lastMonth = stoi(temp);
        getline(ss, temp, '|'); lastDay = stoi(temp);
    }
    time_t now = time(0);
    tm *ltm = localtime(&now);
    int currY = ltm->tm_year + 1900;
    int currM = ltm->tm_mon + 1;
    int currD = ltm->tm_mday;
    tm last_tm = {0};
    last_tm.tm_year = lastYear - 1900;
    last_tm.tm_mon = lastMonth - 1;
    last_tm.tm_mday = lastDay;
    time_t last_time = mktime(&last_tm);
    tm now_tm = {0};
    now_tm.tm_year = currY - 1900;
    now_tm.tm_mon = currM - 1;
    now_tm.tm_mday = currD;
    time_t now_time = mktime(&now_tm);
    double diff = difftime(now_time, last_time) / (60 * 60 * 24);
    int daysDiff = round(diff);
    if (daysDiff == 1) { 
        activeUserStreak++;
        if (activeUserStreak > activeUserMaxStreak) activeUserMaxStreak = activeUserStreak;
    } else if (daysDiff > 1) { 
        activeUserStreak = 1;
    }
    lastYear = currY; lastMonth = currM; lastDay = currD;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string temp;
        Task t;    
        getline(ss, temp, '|'); t.taskId = stoi(temp);
        getline(ss, t.taskName, '|');
        getline(ss, t.taskDescription, '|');
        getline(ss, t.taskCategory, '|');
        getline(ss, temp, '|'); t.priority = stoi(temp);
        getline(ss, temp, '|'); t.isCompleted = (temp == "1");
        getline(ss, temp, '|'); t.daysToComplete = stoi(temp);
        getline(ss, temp, '|'); t.dueDay = stoi(temp);
        getline(ss, temp, '|'); t.dueMonth = stoi(temp);
        getline(ss, temp, '|'); t.dueYear = stoi(temp);
        getline(ss, t.dateCreated, '|');
        allTasks.push_back(t);
    }
    file.close();
    saveUserData(); 
}
void saveUserData() {
    ofstream file(activeUsername + ".txt"); // Saves directly into dixit.txt, guest1.txt, etc.
    file << activeUserStreak << "|" << activeUserMaxStreak << "|" << lastYear << "|" << lastMonth << "|" << lastDay << "\n";   
    for (const auto& t : allTasks) {
        file << t.taskId << "|" << t.taskName << "|" << t.taskDescription << "|" 
             << t.taskCategory << "|" << t.priority << "|" << (t.isCompleted ? "1" : "0") << "|" 
             << t.daysToComplete << "|" << t.dueDay << "|" << t.dueMonth << "|" << t.dueYear << "|" << t.dateCreated << "\n";
    }
    file.close();
}
void addTask() {
    Task t;
    std::cout << "\n========== ADD TASK ==========\n";

    bool isUnique = false;
    while (!isUnique) {
        std::cout << "Enter Task ID: ";
        cin >> t.taskId; 
        cin.ignore(); 

        isUnique = true; 
        for (const auto& existingTask : allTasks) {
            if (existingTask.taskId == t.taskId) {
                std::cout << "[!] ERROR: Task ID " << t.taskId << " already exists! Please enter a unique ID.\n\n";
                isUnique = false; 
                break; 
            }
        }
    }
    std::cout << "Enter Task Name: "; getline(cin, t.taskName);
    std::cout << "Enter Description: "; getline(cin, t.taskDescription);
    std::cout << "Enter Category: "; getline(cin, t.taskCategory);
    std::cout << "Enter Priority (1-5): "; cin >> t.priority;
    std::cout << "Enter Number of Days To Complete: "; cin >> t.daysToComplete;
    time_t now = time(0);
    now += t.daysToComplete * 24 * 60 * 60;
    tm *deadline = localtime(&now);
    t.dueDay = deadline->tm_mday; 
    t.dueMonth = deadline->tm_mon + 1; 
    t.dueYear = deadline->tm_year + 1900;
    t.isCompleted = false;
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
    t.dateCreated = string(buffer);
    allTasks.push_back(t);
    saveUserData(); 
    std::cout << "\nTask Added Successfully!\n";
}
void displayTasks() {
    if (allTasks.empty()) { cout << "\nNo tasks available!\n"; return; }
    cout << "\n========== ALL TASKS ==========\n";
    for (const auto& t : allTasks) {
        cout << "ID: " << t.taskId << " | Name: " << t.taskName 
             << " | Priority: " << t.priority << " | Status: " << (t.isCompleted ? "Completed" : "Pending") 
             << " | Due: " << t.dueDay << "/" << t.dueMonth << "/" << t.dueYear << "\n";
    }
}
void editTask() {
    int id; cout << "\nEnter Task ID: "; cin >> id; cin.ignore();
    for (auto& t : allTasks) {
        if (t.taskId == id) {
            int choice;
            cout << "1. Edit Name\n2. Edit Description\n3. Edit Category\n4. Edit Priority\n5. Edit Days\nEnter choice: ";
            cin >> choice; cin.ignore();
            switch(choice) {
                case 1: getline(cin, t.taskName); break;
                case 2: getline(cin, t.taskDescription); break;
                case 3: getline(cin, t.taskCategory); break;
                case 4: cin >> t.priority; break;
                case 5: {
                    cin >> t.daysToComplete; time_t now = time(0); now += t.daysToComplete * 24 * 60 * 60;
                    tm *deadline = localtime(&now); t.dueDay = deadline->tm_mday; t.dueMonth = deadline->tm_mon + 1; t.dueYear = deadline->tm_year + 1900; break;
                }
                default: cout << "\nInvalid Choice\n"; return;
            }
            saveUserData(); // Save automatically!
            std::cout << "\nTask Edited Successfully!\n";
            return;
        }
    }
    cout << "\nTask Not Found!\n";
}
void removeTask() {
    int id; cout << "\nEnter Task ID: "; cin >> id;
    for (size_t i = 0; i < allTasks.size(); i++) {
        if (allTasks[i].taskId == id) {
            allTasks.erase(allTasks.begin() + i);
            saveUserData(); // Save automatically!
            cout << "\nTask Removed!\n";
            return;
        }
    }
    cout << "\nTask Not Found!\n";
}
void markCompleted() {
    int id; cout << "\nEnter Task ID: "; cin >> id;
    for (auto& t : allTasks) {
        if (t.taskId == id) {
            t.isCompleted = true;
            saveUserData(); // Save automatically!
            cout << "\nTask Marked as Complete!\n";
            return;
        }
    }
    cout << "\nTask Not Found!\n";
}
void checkReminders() {
    bool urgencyFound = false;
    time_t now = time(0);
    for (const auto &t : allTasks) {
        if (!t.isCompleted) {
            tm deadline_tm = {0};
            deadline_tm.tm_year = t.dueYear - 1900;
            deadline_tm.tm_mon = t.dueMonth - 1;
            deadline_tm.tm_mday = t.dueDay;
            time_t deadline_time = mktime(&deadline_tm);
            int secondsLeft = difftime(deadline_time, now);
            int daysLeft =static_cast<int>( secondsLeft / (24 * 3600));
            if( daysLeft <0) daysLeft =0;
            int hoursleft=static_cast<int>((secondsLeft- daysLeft*24*3600)/3600);
            if (daysLeft <= 2.0) {
                if (!urgencyFound) {
                    cout << "\n========================================\n";
                    cout << "          PENDING DUE TASKS              \n";
                    cout << "========================================\n";
                    urgencyFound = true;
                }
                cout << "[!] URGENT: '" << t.taskName << "' is due in " << daysLeft << " days!\n";
                cout << hoursLeft << endl;
            }
        }
    }
    if (urgencyFound) cout << "========================================\n";
}
void displayHistory() {
    cout << "\n=====================================================\n";
    cout << "                    TASK HISTORY                       \n";
    cout << "=====================================================\n";
    for (const auto &t : allTasks) {
        cout << "ID: " << t.taskId << " | Name: " << t.taskName 
             << "\nCreated: " << t.dateCreated 
             << " | Status: " << (t.isCompleted ? "COMPLETED" : "PENDING") << "\n\n";
    }
    cout << "=====================================================\n";
}
//ok 
