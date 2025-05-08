#include <iostream>
#include <string>
#include <cassert>
#include "TestFramework.h"
#include "EditorTestable.h"

// Test complex editing scenario with checkpoints
TestResult testComplexEditingWithCheckpoints() {
    std::vector<std::string> inputs = {
        "add First line",                  // 0
        "add Third line",                  // 1
        "insert 1 Second line",            // 2
        "add Fourth line",                 // 3
        "setcursor 1 0",                   // 4
        "type Modified ",                  // 5
        "view",                            // 6
        "setcursor 0 0",                   // 7
        "selstart",                        // 8
        "setcursor 0 5",                   // 9
        "selend",                          // 10
        "cut",                             // 11
        "setcursor 3 0",                   // 12
        "paste",                          // 13
        "view",                            // 14
        "join",                            // 15
        "view"                             // 16
    };
    
    // Define checkpoints
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        // After adding initial lines and before typing "Modified"
        {4, [](const Editor& editor) {
            // Should have 4 lines
            assert(editor.getBuffer().lineCount() == 4);
            // Cursor should be at the beginning of line 1
            assert(editor.getCursorLine() == 1);
            assert(editor.getCursorCol() == 0);
            // Line content verification
            assert(editor.getBuffer().getLine(0) == "First line");
            assert(editor.getBuffer().getLine(1) == "Second line");
            assert(editor.getBuffer().getLine(3) == "Fourth line");
        }},
        
        // After typing "Modified" to line 1
        {6, [](const Editor& editor) {
            // Should have 4 lines
            assert(editor.getBuffer().lineCount() == 4);
            // Line content verification
            assert(editor.getBuffer().getLine(1) == "Modified Second line");
        }},
        
        // After cutting "First" from line 0
        {11, [](const Editor& editor) {
            // Line 0 should no longer have "First"
            assert(editor.getBuffer().getLine(0) == " line");
            // Cursor should be at beginning of line 0
            assert(editor.getCursorLine() == 0);
            assert(editor.getCursorCol() == 0);
        }},
        
        // After pasting "First" at beginning of line 3
        {14, [](const Editor& editor) {
            // Line 3 should now start with "First"
            assert(editor.getBuffer().getLine(3) == "FirstFourth line");
        }},
        
        // After joining lines 3 and (implied) 4
        {16, [](const Editor& editor) {
            // Should now have 3 lines (after joining)
            assert(editor.getBuffer().lineCount() == 3);
        }}
    };
    
    std::string output;
    bool success = EditorTestable::runWithCheckpoints(inputs, checkpoints, output);
    
    if (!success) {
        return TestResult(false, "Failed to run editor with inputs and checkpoints");
    }
    
    // All assertions in checkpoints passed if we got here
    return TestResult(true, "Complex editing with checkpoints passed");
}

// Test undo/redo operations (would need to add this functionality first)
/*
TestResult testUndoRedo() {
    // To be implemented when undo/redo functionality is added
    return TestResult(true, "Placeholder for undo/redo test");
}
*/

// Test search and replace (would need to add this functionality first)
/*
TestResult testSearchAndReplace() {
    // To be implemented when search/replace functionality is added
    return TestResult(true, "Placeholder for search and replace test");
}
*/

// Stress test with many operations
TestResult testLargeFileEditing() {
    // Generate a large number of lines
    std::vector<std::string> inputs;
    
    // Add 100 lines
    for (int i = 0; i < 100; ++i) {
        inputs.push_back("add Line number " + std::to_string(i));
    }
    
    // Random edits throughout
    inputs.push_back("setcursor 25 0");
    inputs.push_back("type MODIFIED: ");
    inputs.push_back("setcursor 50 0");
    inputs.push_back("type MODIFIED: ");
    inputs.push_back("setcursor 75 0");
    inputs.push_back("type MODIFIED: ");
    
    // Delete some lines
    inputs.push_back("delete 10");
    inputs.push_back("delete 30");
    inputs.push_back("delete 50");
    
    // Move cursor around extensively
    inputs.push_back("top");
    inputs.push_back("setcursor 20 5");
    inputs.push_back("bottom");
    
    // Final cursor position and line count check
    inputs.push_back("cursor");
    inputs.push_back("lines");
    
    std::string output;
    bool success = EditorTestable::runWithInputs(inputs, output);
    
    if (!success) {
        return TestResult(false, "Failed to run large file editing test");
    }
    
    std::string message;
    
    // Check final line count (should be 97 after deleting 3 lines)
    if (!TestAssert::stringContains(output, "Total lines: 97", message)) {
        return TestResult(false, "Failed to verify correct line count after large edits: " + message);
    }
    
    // Verify cursor is at the bottom of the buffer
    if (!TestAssert::stringContains(output, "Cursor at: [96,", message)) {
        return TestResult(false, "Failed to verify cursor at buffer end: " + message);
    }
    
    return TestResult(true, "Large file editing test passed");
}

int main() {
    TestFramework framework;
    
    // Register advanced tests
    framework.registerTest("Complex Editing with Checkpoints", testComplexEditingWithCheckpoints);
    framework.registerTest("Large File Editing", testLargeFileEditing);
    
    // Placeholder tests for future features
    // framework.registerTest("Undo/Redo Operations", testUndoRedo);
    // framework.registerTest("Search and Replace", testSearchAndReplace);
    
    // Run all tests
    framework.runAllTests();
    
    return 0;
} 