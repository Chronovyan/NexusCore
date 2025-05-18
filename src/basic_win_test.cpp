#include <iostream>
#include <string>
#include <windows.h>

int main() {
    // Force console output
    AllocConsole();
    FILE* pFile = nullptr;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    
    std::cout << "Testing C++ Windows compilation..." << std::endl;
    std::cout << "This is a simple test to verify compiler setup works." << std::endl;
    
    // Print some system info
    std::cout << "Windows version: " << GetVersion() << std::endl;
    
    std::cout << "Test completed successfully!" << std::endl;
    
    // Wait for user input before closing
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
} 