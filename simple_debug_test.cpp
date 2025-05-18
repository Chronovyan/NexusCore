#include <iostream>
#include <fstream>
#include <string>

int main() {
    // Open a log file
    std::ofstream logFile("debug_log.txt");
    
    if (logFile.is_open()) {
        // Write basic log information
        logFile << "Debug log initialized" << std::endl;
        logFile << "Testing log output" << std::endl;
        
        // Also write to console
        std::cout << "Log file created successfully" << std::endl;
        std::cout << "Check debug_log.txt for output" << std::endl;
        
        // Close the file
        logFile.close();
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
    
    return 0;
} 