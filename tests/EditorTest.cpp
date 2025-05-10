#include "Editor.h" 
#include <iostream> 
#include <string> 
#include <fstream> 
#include <stdexcept> // Include for std::exception
 
int main() { 
    try { 
        Editor editor; 
        std::cout << "Editor initialized successfully\n"; 
        editor.getBuffer().addLine("Test line 1"); 
        editor.typeChar('A'); 
        std::cout << "Typed character successfully\n"; 
        editor.typeText("Testing typing multiple characters"); 
        std::cout << "Typed text successfully\n"; 
        editor.backspace(); 
        std::cout << "Backspace successful\n"; 
        editor.deleteForward(); 
        std::cout << "Delete forward successful\n"; 
        editor.printView(std::cout); 
        std::cout << "Editor test completed successfully!\n"; 
        return 0; 
    } catch (const std::exception& e) { // Catch standard exceptions
        std::cerr << "Error: " << e.what() << std::endl; 
        return 1; 
    } catch (...) { // Catch any other (non-standard) exceptions
        std::cerr << "Unknown error occurred" << std::endl; 
        return 1; 
    } 
} 
