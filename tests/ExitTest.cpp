#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"
#include "TestEditor.h"

// Test that exit/quit commands terminate the editor properly
TEST(ExitTest, ExitCommands) {
    std::string output;
    
    // Test 'exit' command
    std::vector<std::string> exitTest = {
        "add This is a test line",
        "add Another line",
        "view",
        "exit" // Should terminate the editor loop
    };
    
    ASSERT_TRUE(EditorTestable::runWithInputs(exitTest, output)) << "Failed to run editor with 'exit' command";
    
    // Verify the output contains expected elements
    ASSERT_NE(output.find("Exiting editor."), std::string::npos) << "Editor did not properly handle 'exit' command. Output: " << output;
    
    output.clear();
    
    // Test 'quit' command
    std::vector<std::string> quitTest = {
        "add This is a test line",
        "add Another line",
        "view",
        "quit" // Should terminate the editor loop
    };
    
    ASSERT_TRUE(EditorTestable::runWithInputs(quitTest, output)) << "Failed to run editor with 'quit' command";
    
    // Verify the output contains expected elements
    ASSERT_NE(output.find("Exiting editor."), std::string::npos) << "Editor did not properly handle 'quit' command. Output: " << output;
}

// Test that exit commands don't execute if there are unsaved changes (if implemented)
TEST(ExitTest, ExitWithUnsavedChanges) {
    // This test is a placeholder - implement if the editor has unsaved changes protection
    // For now, we'll just return a passing result
    
    SUCCEED() << "Exit with unsaved changes test placeholder (not implemented)";
}

