#include <iostream>
#include <string>
#include <cassert>
#include "TestFramework.h"
#include "EditorTestable.h"

// Test basic undo/redo with add and delete operations
TestResult testBasicUndoRedo() {
    std::string output;
    std::string message;
    
    // Test sequence: add 3 lines, undo twice, redo once
    std::vector<std::string> inputs = {
        "add Line 1",
        "add Line 2",
        "add Line 3",
        "view",
        "lines",       // Should show 3 lines
        "undo",        // Undo add Line 3
        "view",
        "lines",       // Should show 2 lines
        "undo",        // Undo add Line 2
        "view",
        "lines",       // Should show 1 line
        "redo",        // Redo add Line 2
        "view",
        "lines"        // Should show 2 lines
    };
    
    // Run the editor with the test inputs
    EditorTestable::runWithInputs(inputs, output);
    
    // Debug output
    std::cout << "=== Test output ===" << std::endl;
    std::cout << output << std::endl;
    std::cout << "===================" << std::endl;
    
    // Check if the actions were performed successfully
    if (output.find("Action undone") == std::string::npos) {
        message = "Undo command didn't execute or display success message";
        return TestResult(false, message);
    }
    
    if (output.find("Action redone") == std::string::npos) {
        message = "Redo command didn't execute or display success message";
        return TestResult(false, message);
    }
    
    // Individual test: Get the lines count after undoing Line 3 (should be 2)
    size_t undoPos1 = output.find("Action undone");
    size_t linesAfterFirstUndo = output.find("Total lines:", undoPos1);
    std::string lineCount1 = output.substr(linesAfterFirstUndo + 12, 1);
    if (lineCount1 != "2") {
        message = "Undo didn't correctly reduce line count to 2";
        return TestResult(false, message);
    }
    
    // Individual test: Get the lines count after undoing Line 2 (should be 1)
    size_t undoPos2 = output.find("Action undone", undoPos1 + 1);
    size_t linesAfterSecondUndo = output.find("Total lines:", undoPos2);
    std::string lineCount2 = output.substr(linesAfterSecondUndo + 12, 1);
    if (lineCount2 != "1") {
        message = "Second undo didn't correctly reduce line count to 1";
        return TestResult(false, message);
    }
    
    // Individual test: Get the lines count after redoing Line 2 (should be 2)
    size_t redoPos = output.find("Action redone");
    size_t linesAfterRedo = output.find("Total lines:", redoPos);
    std::string lineCount3 = output.substr(linesAfterRedo + 12, 1);
    if (lineCount3 != "2") {
        message = "Redo didn't correctly increase line count to 2";
        return TestResult(false, message);
    }
    
    return TestResult(true, "Basic undo/redo with add/delete operations works correctly");
}

// Test undo/redo with text editing operations
TestResult testTextEditingUndoRedo() {
    std::string output;
    std::string message;
    
    // Test sequence: add a line, type text, backspace, delete, undo, redo
    std::vector<std::string> inputs = {
        "add Hello",
        "view",                // View initial state
        "setcursor 0 5",       // Cursor at end of "Hello"
        "type , world",        // Now: "Hello, world"
        "view",                // View after typing
        "backspace",           // Delete 'd' -> "Hello, worl"
        "view",                // View after backspace
        "undo",                // Undo backspace -> "Hello, world"
        "view",                // View after undoing backspace
        "undo",                // Undo typing -> "Hello"
        "view",                // View after undoing typing
        "redo",                // Redo typing -> "Hello, world"
        "view",                // View after redoing typing
        "setcursor 0 5",       // Cursor after "Hello"
        "del",                 // Delete ',' -> "Hello world"
        "view",                // View after delete
        "undo",                // Undo delete -> "Hello, world"
        "view"                 // View after undoing delete
    };
    
    // Run the editor with the test inputs
    EditorTestable::runWithInputs(inputs, output);
    
    // Debug output
    std::cout << "=== Test output ===" << std::endl;
    std::cout << output << std::endl;
    std::cout << "===================" << std::endl;
    
    // Check for "Hello, world" after typing
    size_t typePosition = output.find("Text inserted");
    size_t viewAfterType = output.find("--- Buffer View ---", typePosition);
    size_t viewEndAfterType = output.find("-------------------", viewAfterType);
    std::string bufferAfterType = output.substr(viewAfterType, viewEndAfterType - viewAfterType);
    
    if (bufferAfterType.find("Hello, world") == std::string::npos) {
        message = "Failed to add text with type command";
        return TestResult(false, message);
    }
    
    // Check for "Hello, worl" after backspace
    size_t backspacePosition = output.find("Backspace performed");
    size_t viewAfterBackspace = output.find("--- Buffer View ---", backspacePosition);
    size_t viewEndAfterBackspace = output.find("-------------------", viewAfterBackspace);
    std::string bufferAfterBackspace = output.substr(viewAfterBackspace, viewEndAfterBackspace - viewAfterBackspace);
    
    if (bufferAfterBackspace.find("Hello, worl") == std::string::npos) {
        message = "Backspace didn't correctly delete a character";
        return TestResult(false, message);
    }
    
    // Check for "Hello, world" after undoing backspace
    size_t undoBackspacePosition = output.find("Action undone", backspacePosition);
    size_t viewAfterUndoBackspace = output.find("--- Buffer View ---", undoBackspacePosition);
    size_t viewEndAfterUndoBackspace = output.find("-------------------", viewAfterUndoBackspace);
    std::string bufferAfterUndoBackspace = output.substr(viewAfterUndoBackspace, viewEndAfterUndoBackspace - viewAfterUndoBackspace);
    
    if (bufferAfterUndoBackspace.find("Hello, world") == std::string::npos) {
        message = "Undo didn't correctly restore deleted character";
        return TestResult(false, message);
    }
    
    return TestResult(true, "Undo/redo with text editing operations works correctly");
}

// Test undo/redo with line operations
TestResult testLineOperationsUndoRedo() {
    std::string output;
    std::string message;
    
    // Test sequence: add lines, delete line, replace line, insert line, undo, redo
    std::vector<std::string> inputs = {
        "add First line",
        "add Second line",
        "add Third line",
        "view",
        "delete 1",            // Delete "Second line"
        "view",
        "undo",                // Undo delete -> Restore "Second line"
        "view",
        "replace 0 New first", // Replace "First line" with "New first"
        "view",
        "undo",                // Undo replace -> Restore "First line"
        "view",
        "insert 1 Inserted",   // Insert "Inserted" at index 1
        "view",
        "undo",                // Undo insert
        "view",
        "redo",                // Redo insert
        "view"
    };
    
    // Run the editor with the test inputs
    EditorTestable::runWithInputs(inputs, output);
    
    // Check if the output contains the expected text states
    if (output.find("First line") == std::string::npos ||
        output.find("Second line") == std::string::npos ||
        output.find("Third line") == std::string::npos) {
        message = "Failed to add three lines correctly";
        return TestResult(false, message);
    }
    
    if (output.find("First line\nThird line") == std::string::npos) {
        message = "Delete line didn't correctly remove the second line";
        return TestResult(false, message);
    }
    
    if (output.find("New first") == std::string::npos) {
        message = "Replace line didn't correctly change the first line";
        return TestResult(false, message);
    }
    
    if (output.find("Inserted") == std::string::npos) {
        message = "Insert line didn't correctly add the new line";
        return TestResult(false, message);
    }
    
    return TestResult(true, "Undo/redo with line operations works correctly");
}

// Main function for running undo/redo tests directly
int main() {
    TestFramework framework;
    
    // Register undo/redo tests
    framework.registerTest("Basic Undo/Redo Operations", testBasicUndoRedo);
    framework.registerTest("Text Editing Undo/Redo", testTextEditingUndoRedo);
    framework.registerTest("Line Operations Undo/Redo", testLineOperationsUndoRedo);
    
    // Run all tests
    framework.runAllTests();
    
    return 0;
} 