#include <iostream>
#include "suyans.h"
#include "task.h"
#include "calendar.h"

using namespace std;

void Dashboard(){
    int choice;
    do{      
        checkReminders(); 
        cout << "\n";
        cout << "========================================\n";
        cout << "         TASK MANAGER SYSTEM\n";
        cout << "========================================\n";
        cout << "1. Add Task\n";
        cout << "2. Display Tasks\n";
        cout << "3. Edit Task\n";
        cout << "4. Remove Task\n";
        cout << "5. Mark Complete\n";
        cout << "6. Show Calendar\n";
        cout << "7. Show History\n";
        cout << "8. Gamification & Quiz\n"; 
        cout << "9. Exit\n";
        cout << "========================================\n";
        cout << "Enter Choice: ";
        cin >> choice;
        switch(choice){
            case 1:
             addTask(); 
             break;
            case 2:
             displayTasks(); 
             break;
            case 3:
             editTask();
              break;
            case 4:
             removeTask();
              break;
            case 5: 
            markCompleted();
             break;
            case 6:
             displayCalendar();
              break;
            case 7:
             displayHistory();
             break;
            case 8:
             displayGamification();
              break;
            case 9: 
                cout << "\nThank You For Using Task Manager!\n"; 
                break;
            default: cout << "\nInvalid Choice!\n";
        }

    } while(choice != 9);
}