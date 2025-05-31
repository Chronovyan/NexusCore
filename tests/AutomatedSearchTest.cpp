#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"

class AutomatedSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }
    
    void TearDown() override {
        // Common cleanup for all tests
    }
    
    // Helper function to create a test editor with sample content
    void setupTestEditor(TestEditor& editor) {
        editor.addLine("Hello, world!");
        editor.addLine("This is a test with multiple words.");
        editor.addLine("Another line with the test in it.");
    }
};

/**
 * Test basic search functionality
 */
TEST_F(AutomatedSearchTest, BasicSearch) {
    std::string output;
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "search test",
        "searchnext",
        "search nonexistent"
    };
    
    ASSERT_TRUE(EditorTestable::runWithInputs(inputs, output));
    
    // Verify search results for "test"
    ASSERT_NE(output.find("Found match"), std::string::npos) 
        << "Should find the first occurrence of 'test'";
    ASSERT_NE(output.find("Found next match"), std::string::npos) 
        << "Should find the second occurrence of 'test'";
    
    // Verify search results for "nonexistent"
    ASSERT_NE(output.find("No matches found for \"nonexistent\""), std::string::npos) 
        << "Should not find any matches for 'nonexistent'";
}

/**
 * Test case-insensitive search
 */
TEST_F(AutomatedSearchTest, CaseInsensitiveSearch) {
    std::string output;
    
    // Create test with mixed case
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a TEST with multiple words.",
        "add Another line with the test in it.",
        "search test",
        "searchnext"
    };
    
    // Define checkpoints to verify search matches
    auto checkFirstMatch = [](TestEditor& editor) {
        ASSERT_TRUE(editor.hasSelection()) << "Editor should have selection after search";
        std::string selectedText = editor.getSelectedText();
        ASSERT_EQ(selectedText, "TEST") << "First search should find 'TEST' (case insensitive)";
        ASSERT_EQ(editor.getCursorLine(), 1) << "Cursor should be on second line (index 1)";
    };
    
    auto checkSecondMatch = [](TestEditor& editor) {
        ASSERT_TRUE(editor.hasSelection()) << "Editor should have selection after searchnext";
        std::string selectedText = editor.getSelectedText();
        ASSERT_EQ(selectedText, "test") << "Second search should find 'test'";
        ASSERT_EQ(editor.getCursorLine(), 2) << "Cursor should be on third line (index 2)";
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {3, checkFirstMatch},
        {4, checkSecondMatch}
    };
    
    ASSERT_TRUE(EditorTestable::runWithCheckpoints(inputs, checkpoints, output));
}

/**
 * Test search and replace
 */
TEST_F(AutomatedSearchTest, SearchAndReplace) {
    std::string output;
    
    // We need to use checkpoints for replacement since EditorTestable doesn't have a direct replace command
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "search test",
        "view",
        "undo",         // Undo the replacement (will be performed in checkpoint)
        "view",
        "search nonexistent",
        "view"
    };
    
    // Define checkpoints to perform and verify replacements
    auto performReplace = [](TestEditor& editor) {
        ASSERT_TRUE(editor.hasSelection()) << "Editor should have selection after search";
        std::string selectedText = editor.getSelectedText();
        ASSERT_EQ(selectedText, "test") << "Search should find 'test'";
        
        // Replace the selected text
        editor.deleteSelectedText();
        editor.typeText("EXAMPLE");
        
        // Verify replacement
        editor.setCursor(1, 10); // Position before replacement
        editor.search("EXAMPLE");
        ASSERT_TRUE(editor.hasSelection()) << "Editor should find the replacement text";
    };
    
    auto checkAfterUndo = [](TestEditor& editor) {
        // Verify text was restored after undo
        editor.setCursor(1, 10); // Position before original text
        editor.search("test");
        ASSERT_TRUE(editor.hasSelection()) << "Editor should find the original text after undo";
        ASSERT_EQ(editor.getSelectedText(), "test") << "Original text should be restored";
    };
    
    auto checkNonexistentSearch = [](TestEditor& editor) {
        // Verify that searching for non-existent text doesn't create a selection
        ASSERT_FALSE(editor.hasSelection()) << "Editor should not have selection after failed search";
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {4, performReplace},
        {6, checkAfterUndo},
        {8, checkNonexistentSearch}
    };
    
    ASSERT_TRUE(EditorTestable::runWithCheckpoints(inputs, checkpoints, output));
    
    // Verify output messages
    ASSERT_NE(output.find("Found match"), std::string::npos) 
        << "Should find the first occurrence of 'test'";
    ASSERT_NE(output.find("No matches found for \"nonexistent\""), std::string::npos) 
        << "Should not find any matches for 'nonexistent'";
}

/**
 * Test replace all functionality
 */
TEST_F(AutomatedSearchTest, ReplaceAll) {
    std::string output;
    
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "add One more test line for testing.",
        "view"
    };
    
    // Define checkpoints to perform and verify replaceAll
    auto performReplaceAll = [](TestEditor& editor) {
        // Count initial occurrences of "test"
        int initialCount = 0;
        editor.setCursor(0, 0);
        while (editor.search("test")) {
            initialCount++;
            editor.setCursor(editor.getCursorLine(), editor.getCursorCol() + 1);
        }
        
        // Verify we have at least 3 occurrences
        ASSERT_GE(initialCount, 3) << "Should have at least 3 occurrences of 'test'";
        
        // Replace all occurrences
        bool replaced = editor.replaceAll("test", "EXAMPLE");
        ASSERT_TRUE(replaced) << "ReplaceAll should return true when replacements are made";
        
        // Verify no more occurrences of "test"
        editor.setCursor(0, 0);
        ASSERT_FALSE(editor.search("test")) << "Should not find 'test' after replacing all";
        
        // Verify the number of "EXAMPLE" occurrences matches initial "test" occurrences
        int replacementCount = 0;
        editor.setCursor(0, 0);
        while (editor.search("EXAMPLE")) {
            replacementCount++;
            editor.setCursor(editor.getCursorLine(), editor.getCursorCol() + 1);
        }
        ASSERT_EQ(replacementCount, initialCount) << "Number of replacements should match initial count";
    };
    
    auto afterReplaceAll = [](TestEditor& editor) {
        // Verify buffer state after replaceAll
        const TextBuffer& buffer = editor.getBuffer();
        for (size_t i = 0; i < buffer.lineCount(); i++) {
            std::string line = buffer.getLine(i);
            ASSERT_EQ(line.find("test"), std::string::npos) 
                << "Line " << i << " should not contain 'test' after replaceAll";
        }
    };
    
    auto performUndo = [](TestEditor& editor) {
        bool undone = editor.undo();
        ASSERT_TRUE(undone) << "Should be able to undo replaceAll";
    };
    
    auto afterUndo = [](TestEditor& editor) {
        // Verify buffer state after undo
        const TextBuffer& buffer = editor.getBuffer();
        bool foundTest = false;
        for (size_t i = 0; i < buffer.lineCount(); i++) {
            std::string line = buffer.getLine(i);
            if (line.find("test") != std::string::npos) {
                foundTest = true;
                break;
            }
        }
        ASSERT_TRUE(foundTest) << "Should find 'test' after undoing replaceAll";
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {4, performReplaceAll},
        {4, afterReplaceAll},
        {4, performUndo},
        {4, afterUndo}
    };
    
    ASSERT_TRUE(EditorTestable::runWithCheckpoints(inputs, checkpoints, output));
}

/**
 * Test search selection and cursor positioning
 */
TEST_F(AutomatedSearchTest, SearchSelectionAndCursor) {
    std::string output;
    
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "search test"
    };
    
    auto checkSelection = [](TestEditor& editor) {
        // Verify selection after search
        ASSERT_TRUE(editor.hasSelection()) << "Editor should have selection after search";
        
        // Check selection boundaries
        ASSERT_EQ(editor.selectionStartLine_, 1) << "Selection should start on line 1";
        ASSERT_EQ(editor.selectionEndLine_, 1) << "Selection should end on line 1";
        
        // Assuming "test" starts at position 10 in "This is a test with multiple words."
        ASSERT_EQ(editor.selectionStartCol_, 10) << "Selection should start at position 10";
        ASSERT_EQ(editor.selectionEndCol_, 14) << "Selection should end at position 14";
        
        // Check selected text
        std::string selectedText = editor.getSelectedText();
        ASSERT_EQ(selectedText, "test") << "Selected text should be 'test'";
        
        // Check cursor position (should be at end of selection)
        ASSERT_EQ(editor.getCursorLine(), 1) << "Cursor line should be 1";
        ASSERT_EQ(editor.getCursorCol(), 14) << "Cursor column should be 14";
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {3, checkSelection}
    };
    
    ASSERT_TRUE(EditorTestable::runWithCheckpoints(inputs, checkpoints, output));
} 