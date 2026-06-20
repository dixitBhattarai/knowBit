#include <iostream>
#include <cstdlib>
#include "task.h"
bool runLoginGUI();
void Dashboard();
int main() {
    std::cout << "Starting knowBit System...\n";
    bool loginSuccess = runLoginGUI();
    if (loginSuccess) {
        system("clear");
        loadUserData(); 
        Dashboard();
    } else {
        std::cout << "\n[SYSTEM]: Login cancelled or failed. Exiting program.\n";
    }
    return 0;
}