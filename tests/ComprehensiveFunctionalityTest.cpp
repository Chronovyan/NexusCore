#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"

int main() {
    std::cout << "=== Comprehensive Editor Functionality Tests ===" << std::endl;
    std::string output;

    // Test 1: Basic Line Operations (add, insert, delete, replace, clear)
    std::vector<std::string> lineOperationsTest = {
        "add First line",
        "view",
        "insert 0 Inserted at beginning",
        "view",
        "add Last line",
        "view",
        "lines",
        "delete 1",
        "view",
        "replace 0 Replaced first line",
        "view",
        "clear",
        "view"
    };

    // Test 2: Cursor Movement (setcursor, cu, cd, cl, cr, home, end, top, bottom, nextword, prevword)
    std::vector<std::string> cursorMovementTest = {
        "clear",
        "add First line with multiple words",
        "add Second line with text",
        "add Third line here",
        "view",
        "setcursor 0 5",
        "cursor",
        "cl",
        "cursor",
        "cr",
        "cursor",
        "cu",
        "cursor",
        "cd",
        "cursor",
        "home",
        "cursor",
        "end",
        "cursor", 
        "top",
        "cursor",
        "bottom",
        "cursor",
        "setcursor 0 0",
        "nextword",
        "cursor",
        "nextword",
        "cursor",
        "prevword",
        "cursor"
    };

    // Test 3: Text Editing (type, backspace, del, newline, join)
    std::vector<std::string> textEditingTest = {
        "clear",
        "add Hello world",
        "view",
        "setcursor 0 5",
        "type , beautiful",
        "view",
        "backspace",
        "view",
        "del",
        "view",
        "setcursor 0 6",
        "newline",
        "view",
        "join",
        "view"
    };

    // Test 4: Selection & Clipboard Operations (selstart, selend, selclear, selshow, cut, copy, paste, delword, selword)
    std::vector<std::string> selectionTest = {
        "clear",
        "add The quick brown fox jumps over the lazy dog",
        "view",
        "setcursor 0 4",
        "selstart",
        "setcursor 0 9",
        "selend",
        "selshow",
        "copy",
        "setcursor 0 40",
        "paste",
        "view",
        "setcursor 0 4",
        "selstart",
        "setcursor 0 9",
        "selend",
        "cut",
        "view",
        "setcursor 0 0",
        "paste",
        "view",
        "selclear",
        "setcursor 0 10",
        "selword",
        "selshow",
        "setcursor 0 20",
        "delword",
        "view"
    };

    // Test 5: File Operations (simulated save and load)
    std::vector<std::string> fileOperationsTest = {
        "clear",
        "add This is a test line for file operations",
        "add Another line to be saved",
        "view",
        "save test_file.txt",
        "clear",
        "view",
        "load test_file.txt"
    };

    // Test 6: Undo/Redo Operations
    std::vector<std::string> undoRedoTest = {
        "clear",
        "add First line",
        "add Second line",
        "view",
        "undo",
        "view",
        "undo",
        "view",
        "redo",
        "view",
        "redo",
        "view"
    };

    // Test 7: Help Command
    std::vector<std::string> helpTest = {
        "help"
    };

    // Run each test category
    std::cout << "\n=== Test 1: Basic Line Operations ===" << std::endl;
    EditorTestable::runWithInputs(lineOperationsTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 2: Cursor Movement ===" << std::endl;
    EditorTestable::runWithInputs(cursorMovementTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 3: Text Editing ===" << std::endl;
    EditorTestable::runWithInputs(textEditingTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 4: Selection & Clipboard Operations ===" << std::endl;
    EditorTestable::runWithInputs(selectionTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 5: File Operations ===" << std::endl;
    EditorTestable::runWithInputs(fileOperationsTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 6: Undo/Redo Operations ===" << std::endl;
    EditorTestable::runWithInputs(undoRedoTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 7: Help Command ===" << std::endl;
    EditorTestable::runWithInputs(helpTest, output);
    std::cout << output << std::endl;

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "Manual verification is required to confirm that all functionality is working correctly." << std::endl;

    return 0;
} 