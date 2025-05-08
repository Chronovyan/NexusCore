#include <iostream>
#include <string>
#include <cassert>
#include "TestFramework.h"
#include "EditorTestable.h"

// Test basic line operations
TestResult testBasicLineOperations() {
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add Second line",
        "lines",
        "view",
        "insert 1 Middle line",
        "lines",
        "view",
        "delete 0",
        "lines",
        "view",
        "replace 0 New middle line",
        "view"
    };
    
    std::string output;
    bool success = EditorTestable::runWithInputs(inputs, output);
    
    if (!success) {
        return TestResult(false, "Failed to run editor with inputs");
    }
    
    std::string message;
    
    // Check if output contains expected lines count after initial adds
    if (!TestAssert::stringContains(output, "Total lines: 2", message)) {
        return TestResult(false, "Failed to verify line count after initial add: " + message);
    }
    
    // Check if output contains the line we inserted
    if (!TestAssert::stringContains(output, "Middle line", message)) {
        return TestResult(false, "Failed to verify inserted line: " + message);
    }
    
    // Check if output contains updated line count after insertion
    if (!TestAssert::stringContains(output, "Total lines: 3", message)) {
        return TestResult(false, "Failed to verify line count after insert: " + message);
    }
    
    // Check if output contains updated line count after deletion
    if (!TestAssert::stringContains(output, "Total lines: 2", message)) {
        return TestResult(false, "Failed to verify line count after delete: " + message);
    }
    
    // Check if output contains replaced line
    if (!TestAssert::stringContains(output, "New middle line", message)) {
        return TestResult(false, "Failed to verify replaced line: " + message);
    }
    
    return TestResult(true, "");
}

// Test cursor movement operations
TestResult testCursorMovement() {
    std::vector<std::string> inputs = {
        "add First line",
        "add Second line",
        "add Third line",
        "cursor",  // Should be at [2, 0]
        "setcursor 0 5",
        "cursor",  // Should be at [0, 5]
        "cr",      // Move right
        "cursor",  // Should be at [0, 6]
        "cd",      // Move down
        "cursor",  // Should be at [1, 6]
        "cl",      // Move left
        "cursor",  // Should be at [1, 5]
        "cu",      // Move up
        "cursor",  // Should be at [0, 5]
        "end",     // Move to end of line
        "cursor",  // Should be at [0, 10]
        "home",    // Move to start of line
        "cursor",  // Should be at [0, 0]
        "bottom",  // Move to end of buffer
        "cursor",  // Should be at [2, 0]
        "top",     // Move to top of buffer
        "cursor"   // Should be at [0, 0]
    };
    
    std::string output;
    bool success = EditorTestable::runWithInputs(inputs, output);
    
    if (!success) {
        return TestResult(false, "Failed to run editor with inputs");
    }
    
    std::string message;
    
    // Initial cursor position after adding lines
    if (!TestAssert::stringContains(output, "Cursor at: [2, 0]", message)) {
        return TestResult(false, "Failed initial cursor position check: " + message);
    }
    
    // After setcursor
    if (!TestAssert::stringContains(output, "Cursor set to: [0, 5]", message)) {
        return TestResult(false, "Failed setcursor check: " + message);
    }
    
    // After move right
    if (!TestAssert::stringContains(output, "Cursor at: [0, 6]", message)) {
        return TestResult(false, "Failed move right check: " + message);
    }
    
    // After move down
    if (!TestAssert::stringContains(output, "Cursor at: [1, 6]", message)) {
        return TestResult(false, "Failed move down check: " + message);
    }
    
    // After move left
    if (!TestAssert::stringContains(output, "Cursor at: [1, 5]", message)) {
        return TestResult(false, "Failed move left check: " + message);
    }
    
    // After move up
    if (!TestAssert::stringContains(output, "Cursor at: [0, 5]", message)) {
        return TestResult(false, "Failed move up check: " + message);
    }
    
    // After move to end of line
    if (!TestAssert::stringContains(output, "Cursor at: [0, 10]", message)) {
        return TestResult(false, "Failed move to end check: " + message);
    }
    
    // After move to start of line
    if (!TestAssert::stringContains(output, "Cursor at: [0, 0]", message)) {
        return TestResult(false, "Failed move to start check: " + message);
    }
    
    return TestResult(true, "");
}

// Test text editing operations
TestResult testTextEditing() {
    std::vector<std::string> inputs = {
        "add Example text",
        "setcursor 0 8",   // Position cursor after "Example "
        "type more ",      // Insert "more " after "Example "
        "view",            // Should show "Example more text"
        "backspace",       // Delete the space
        "view",            // Should show "Example moretext"
        "setcursor 0 12",  // Position cursor after "Example more"
        "del",             // Delete 't' in "text"
        "view",            // Should show "Example moreext"
        "newline",         // Split line
        "view",            // Should have "Example more" on line 0 and "ext" on line 1
        "setcursor 0 0",
        "type New ",       // Insert at beginning of first line
        "view"             // Should have "New Example more" on line 0
    };
    
    std::string output;
    bool success = EditorTestable::runWithInputs(inputs, output);
    
    if (!success) {
        return TestResult(false, "Failed to run editor with inputs");
    }
    
    std::string message;
    
    // Check text after insertion
    if (!TestAssert::stringContains(output, "Example more text", message)) {
        return TestResult(false, "Failed text insertion check: " + message);
    }
    
    // Check text after backspace
    if (!TestAssert::stringContains(output, "Example moretext", message)) {
        return TestResult(false, "Failed backspace check: " + message);
    }
    
    // Check text after delete
    if (!TestAssert::stringContains(output, "Example moreext", message)) {
        return TestResult(false, "Failed delete check: " + message);
    }
    
    // Check text after newline
    if (!TestAssert::stringContains(output, "Example more", message) && 
        TestAssert::stringContains(output, "ext", message)) {
        return TestResult(false, "Failed newline check: " + message);
    }
    
    // Check text after insert at beginning
    if (!TestAssert::stringContains(output, "New Example more", message)) {
        return TestResult(false, "Failed insert at beginning check: " + message);
    }
    
    return TestResult(true, "");
}

// Test selection and clipboard operations
TestResult testSelectionOperations() {
    std::vector<std::string> inputs = {
        "clear",
        "add The quick brown fox jumps over the lazy dog",
        "setcursor 0 4",         // Position cursor at "q" in "quick"
        "selstart",              // Start selection
        "setcursor 0 9",         // Move to end of "quick"
        "selend",                // End selection
        "selshow",               // Should show "quick"
        "copy",                  // Copy "quick"
        "setcursor 0 20",        // Move to after "fox "
        "paste",                 // Paste "quick"
        "view",                  // Should show "The quick brown fox quick jumps..."
        "setcursor 0 20",        // Position at beginning of pasted "quick"
        "selstart",
        "setcursor 0 25",        // Position at end of pasted "quick"
        "selend",
        "cut",                   // Cut "quick"
        "view",                  // Should show "The quick brown fox  jumps..."
        "setcursor 0 10",        // Position after "brown"
        "selword",               // Select word "brown"
        "selshow",               // Should show "brown"
        "type green",            // Replace "brown" with "green"
        "view"                   // Should show "The quick green fox  jumps..."
    };
    
    std::string output;
    bool success = EditorTestable::runWithInputs(inputs, output);
    
    if (!success) {
        return TestResult(false, "Failed to run editor with inputs");
    }
    
    std::string message;
    
    // Check selected text
    if (!TestAssert::stringContains(output, "Selected text: \"quick\"", message)) {
        return TestResult(false, "Failed selection check: " + message);
    }
    
    // Check text after paste
    if (!TestAssert::stringContains(output, "fox quick jumps", message)) {
        return TestResult(false, "Failed paste check: " + message);
    }
    
    // Check text after cut
    if (!TestAssert::stringContains(output, "fox  jumps", message)) {
        return TestResult(false, "Failed cut check: " + message);
    }
    
    // Check selected word
    if (!TestAssert::stringContains(output, "Selected text: \"brown\"", message)) {
        return TestResult(false, "Failed word selection check: " + message);
    }
    
    // Check text after word replacement
    if (!TestAssert::stringContains(output, "quick green fox", message)) {
        return TestResult(false, "Failed word replacement check: " + message);
    }
    
    return TestResult(true, "");
}

int main() {
    TestFramework framework;
    
    // Register tests
    framework.registerTest("Basic Line Operations", testBasicLineOperations);
    framework.registerTest("Cursor Movement", testCursorMovement);
    framework.registerTest("Text Editing", testTextEditing);
    framework.registerTest("Selection Operations", testSelectionOperations);
    
    // Run all tests
    framework.runAllTests();
    
    return 0;
} 