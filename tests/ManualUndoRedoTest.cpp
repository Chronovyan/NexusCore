#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"

int main() {
    std::cout << "=== Undo/Redo Functionality Manual Test ===" << std::endl;
    
    // Create vectors for each test case
    std::vector<std::string> basicTest = {
        "add Line 1",
        "add Line 2", 
        "add Line 3",
        "view",
        "lines",
        "undo",
        "view", 
        "lines",
        "undo", 
        "view",
        "lines", 
        "redo",
        "view",
        "lines"
    };
    
    std::vector<std::string> textEditTest = {
        "add Hello",
        "view",
        "setcursor 0 5",
        "type , world",
        "view",
        "backspace",
        "view",
        "undo",
        "view",
        "undo",
        "view",
        "redo",
        "view"
    };
    
    std::vector<std::string> lineOperationsTest = {
        "add First line",
        "add Second line", 
        "add Third line",
        "view",
        "delete 1",
        "view",
        "undo",
        "view",
        "replace 0 New first",
        "view",
        "undo",
        "view"
    };
    
    // Run each test case
    std::string output;
    
    std::cout << "\n=== Basic Undo/Redo Test ===" << std::endl;
    EditorTestable::runWithInputs(basicTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Text Editing Undo/Redo Test ===" << std::endl;
    EditorTestable::runWithInputs(textEditTest, output);
    std::cout << output << std::endl;
    
    output.clear();
    std::cout << "\n=== Line Operations Undo/Redo Test ===" << std::endl;
    EditorTestable::runWithInputs(lineOperationsTest, output);
    std::cout << output << std::endl;
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "Manual verification is required to confirm that undo/redo is working correctly." << std::endl;
    
    return 0;
} 