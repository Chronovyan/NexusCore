#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include <memory>
#include <string>

class JoinLinesCommandTest : public ::testing::Test {
protected:
    TestEditor editor;
    
    void SetUp() override {
        // Setup: Add two lines
        editor.getBuffer().clear(false);  // Clear without adding an empty line
        editor.getBuffer().addLine("First line");
        editor.getBuffer().addLine("Second line");
        editor.setCursor(0, 0); // Cursor at start of first line
    }
    
    void TearDown() override {
        // Nothing special to tear down
    }
};

// Test basic JoinLinesCommand execution
TEST_F(JoinLinesCommandTest, Execute) {
    auto joinCmd = std::make_unique<JoinLinesCommand>(0); // Join line 0 with line 1
    
    joinCmd->execute(editor);

    // Check line count
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Line count should be 1 after joining";
    
    // Check joined line content
    std::string expectedJoinedLine = "First lineSecond line";
    ASSERT_EQ(expectedJoinedLine, editor.getBuffer().getLine(0)) 
        << "Line content should be '" << expectedJoinedLine << "'";
    
    // Check cursor position - should be at the join point
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(std::string("First line").length(), editor.getCursorCol()) 
        << "Cursor should be at the end of the first part of joined line";
}

// Test JoinLinesCommand undo
TEST_F(JoinLinesCommandTest, Undo) {
    auto joinCmd = std::make_unique<JoinLinesCommand>(0); // Join line 0 with line 1
    
    // Execute first
    joinCmd->execute(editor);
    
    // Then undo
    joinCmd->undo(editor);

    // Check line count
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Line count should be 2 after undo";
    
    // Check line content is restored
    ASSERT_EQ("First line", editor.getBuffer().getLine(0)) << "Line 0 should be restored";
    ASSERT_EQ("Second line", editor.getBuffer().getLine(1)) << "Line 1 should be restored";
    
    // Check cursor position
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor line should be 1 after undo";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be 0 after undo";
}

// Test joining with empty lines
TEST_F(JoinLinesCommandTest, JoinWithEmptyLine) {
    // Setup: Clear and add an empty line followed by a non-empty line
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("");
    editor.getBuffer().addLine("Non-empty line");
    editor.setCursor(0, 0);
    
    auto joinCmd = std::make_unique<JoinLinesCommand>(0);
    joinCmd->execute(editor);
    
    // Check results
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Line count should be 1 after joining";
    ASSERT_EQ("Non-empty line", editor.getBuffer().getLine(0)) 
        << "Joining empty line should result in second line's content";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be 0";
    
    // Test undo as well
    joinCmd->undo(editor);
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Line count should be 2 after undo";
    ASSERT_EQ("", editor.getBuffer().getLine(0)) << "First line should be empty after undo";
    ASSERT_EQ("Non-empty line", editor.getBuffer().getLine(1)) << "Second line should be restored";
}

// Test joining the last line (should do nothing or handle gracefully)
TEST_F(JoinLinesCommandTest, JoinLastLine) {
    // Position on the last line
    editor.setCursor(1, 0);
    
    auto joinCmd = std::make_unique<JoinLinesCommand>(1);
    joinCmd->execute(editor);
    
    // Should not change anything since there's no next line
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Line count should remain 2";
    ASSERT_EQ("First line", editor.getBuffer().getLine(0)) << "First line should be unchanged";
    ASSERT_EQ("Second line", editor.getBuffer().getLine(1)) << "Last line should be unchanged";
    
    // Cursor should not move
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor line should remain on last line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should not change";
} 