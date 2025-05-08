#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"

int main() {
    std::cout << "=== Comprehensive Undo/Redo Tests ===" << std::endl;
    std::string output;

    // Test 1: Basic operations (add, undo, redo)
    std::vector<std::string> basicTest = {
        "add Line 1",
        "add Line 2", 
        "add Line 3",
        "view",
        "lines",
        "undo",         // Undo add Line 3
        "view", 
        "lines",
        "undo",         // Undo add Line 2
        "view",
        "lines", 
        "redo",         // Redo add Line 2
        "view",
        "lines"
    };
    
    // Test 2: Text editing operations
    std::vector<std::string> textEditTest = {
        "clear",        // Start fresh
        "add Hello",
        "view",
        "setcursor 0 5",
        "type , world", // Type ", world"
        "view",
        "backspace",    // Delete the "d"
        "view",
        "undo",         // Undo backspace
        "view",
        "undo",         // Undo typing
        "view",
        "redo",         // Redo typing
        "view"
    };
    
    // Test 3: Line operations
    std::vector<std::string> lineOperationsTest = {
        "clear",          // Start fresh
        "add First line",
        "add Second line", 
        "add Third line",
        "view",
        "delete 1",       // Delete "Second line"
        "view",
        "undo",           // Undo delete
        "view",
        "replace 0 New first", // Replace "First line"
        "view",
        "undo",           // Undo replace
        "view"
    };

    // Test 4: Complex operations (cut/copy/paste)
    std::vector<std::string> complexOperationsTest = {
        "clear",          // Start fresh
        "add First line",
        "add Second line",
        "add Third line",
        "view",
        "setcursor 0 0",
        "selstart",       // Start selection
        "cr",             // Move cursor right
        "cr",
        "cr",
        "cr",
        "cr",             // Select "First"
        "selend",
        "selshow",        // Show selection
        "cut",            // Cut selected text
        "view",
        "undo",           // Undo cut
        "view",
        "selshow",        // Selection should be restored
        "copy",           // Copy selection
        "setcursor 1 0",  // Move to next line
        "paste",          // Paste copied text
        "view",
        "undo",           // Undo paste
        "view"
    };

    // Test 5: Boundary cases - undo/redo limits
    std::vector<std::string> boundaryTest = {
        "clear",         // Start fresh
        "add Test line", // Add one line
        "undo",          // Undo add
        "undo",          // Try to undo beyond history (nothing should happen)
        "redo",          // Redo add
        "redo",          // Try to redo beyond history (nothing should happen)
        "view",
        "lines"
    };

    // Test 6: Newline and delete operations
    std::vector<std::string> newlineTest = {
        "clear",            // Start fresh
        "add Long line with text",
        "view",
        "setcursor 0 9",    // Set cursor after "Long"
        "newline",          // Split line
        "view",
        "undo",             // Undo newline
        "view",
        "setcursor 0 4",    // Set cursor after "Long"
        "del",              // Delete forward (space)
        "view",
        "undo",             // Undo delete
        "view"
    };

    // Test 7: Multiple undo/redo cycles
    std::vector<std::string> multiCycleTest = {
        "clear",         // Start fresh
        "add Line 1",
        "add Line 2",
        "undo",          // Undo Line 2
        "undo",          // Undo Line 1
        "redo",          // Redo Line 1
        "redo",          // Redo Line 2
        "add Line 3",    // Add Line 3 (clears redo stack)
        "undo",          // Undo Line 3
        "undo",          // Undo Line 2
        "redo",          // Redo Line 2
        "view",
        "lines"
    };

    // Run each test
    std::cout << "\n=== Test 1: Basic Undo/Redo Operations ===" << std::endl;
    EditorTestable::runWithInputs(basicTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Test 2: Text Editing Undo/Redo ===" << std::endl;
    EditorTestable::runWithInputs(textEditTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Test 3: Line Operations Undo/Redo ===" << std::endl;
    EditorTestable::runWithInputs(lineOperationsTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Test 4: Complex Operations (Cut/Copy/Paste) ===" << std::endl;
    EditorTestable::runWithInputs(complexOperationsTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Test 5: Boundary Cases - Undo/Redo Limits ===" << std::endl;
    EditorTestable::runWithInputs(boundaryTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Test 6: Newline and Delete Operations ===" << std::endl;
    EditorTestable::runWithInputs(newlineTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Test 7: Multiple Undo/Redo Cycles ===" << std::endl;
    EditorTestable::runWithInputs(multiCycleTest, output);
    std::cout << output << std::endl;
    
    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "Manual verification is required to confirm that all undo/redo functionality is working correctly." << std::endl;
    
    return 0;
} 