#include "Editor.h" 
#include <iostream> 
#include <string> 
#include <fstream> 
#include <stdexcept> // Include for std::exception

// Removed main() function. 
// The logic in the original main() should be converted to Google Test TEST or TEST_F macros.
// For example:
// TEST(EditorBasicOps, TypeAndPrint) {
//     Editor editor;
//     editor.getBuffer().addLine("Test line 1"); 
//     editor.typeChar('A'); 
//     editor.typeText("Testing typing multiple characters"); 
//     editor.backspace(); 
//     editor.deleteForward(); 
//     // Add ASSERT/EXPECT calls here to verify editor state
// }
 
// int main() { 
//     try { 
//         Editor editor; 
//         std::cout << "Editor initialized successfully\n"; 
//         editor.getBuffer().addLine("Test line 1"); 
//         editor.typeChar('A'); 
//         std::cout << "Typed character successfully\n"; 
//         editor.typeText("Testing typing multiple characters"); 
//         std::cout << "Typed text successfully\n"; 
//         editor.backspace(); 
//         std::cout << "Backspace successful\n"; 
//         editor.deleteForward(); 
//         std::cout << "Delete forward successful\n"; 
//         editor.printView(std::cout); 
//         std::cout << "Editor test completed successfully!\n"; 
//         return 0; 
//     } catch (const std::exception& e) { // Catch standard exceptions
//         std::cerr << "Error: " << e.what() << std::endl; 
//         return 1; 
//     } catch (...) { // Catch any other (non-standard) exceptions
//         std::cerr << "Unknown error occurred" << std::endl; 
//         return 1; 
//     } 
// } 
