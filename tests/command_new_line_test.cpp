#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include <memory>
#include <string>

class NewLineCommandTest : public ::testing::Test {
protected:
    TestEditor editor;
    
    void SetUp() override {
        editor.getBuffer().clear(false); // Clear without adding an empty line
    }
    
    void TearDown() override {
        // Clean up any resources if needed
    }
};

// Test splitting a line in the middle
TEST_F(NewLineCommandTest, SplitLineMiddle) {
    // Setup: Add a test line and position cursor in the middle
    editor.getBuffer().addLine("Line1Part1Line1Part2");
    editor.setCursor(0, 10); // Cursor after "Line1Part1"
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after split";
    ASSERT_EQ("Line1Part1", editor.getBuffer().getLine(0)) << "First line should contain text before cursor";
    ASSERT_EQ("Line1Part2", editor.getBuffer().getLine(1)) << "Second line should contain text after cursor";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be on the second line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor should be at beginning of second line";
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 line after undo";
    ASSERT_EQ("Line1Part1Line1Part2", editor.getBuffer().getLine(0)) << "Line should be restored to original state";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be restored";
    ASSERT_EQ(10, editor.getCursorCol()) << "Cursor column should be restored";
}

// Test adding a newline at the end of a line
TEST_F(NewLineCommandTest, AddNewLineAtEnd) {
    // Setup: Add a test line and position cursor at the end
    editor.getBuffer().addLine("EndOfLine");
    editor.setCursor(0, 9); // Cursor at end of "EndOfLine"
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after newline";
    ASSERT_EQ("EndOfLine", editor.getBuffer().getLine(0)) << "First line should be unchanged";
    ASSERT_EQ("", editor.getBuffer().getLine(1)) << "Second line should be empty";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be on the second line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor should be at beginning of second line";
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 line after undo";
    ASSERT_EQ("EndOfLine", editor.getBuffer().getLine(0)) << "Line should be restored to original state";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be restored";
    ASSERT_EQ(9, editor.getCursorCol()) << "Cursor column should be restored";
}

// Test adding a newline at the beginning of a line
TEST_F(NewLineCommandTest, AddNewLineAtBeginning) {
    // Setup: Add a test line and position cursor at the beginning
    editor.getBuffer().addLine("BeginningOfLine");
    editor.setCursor(0, 0); // Cursor at beginning of "BeginningOfLine"
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after newline";
    ASSERT_EQ("", editor.getBuffer().getLine(0)) << "First line should be empty";
    ASSERT_EQ("BeginningOfLine", editor.getBuffer().getLine(1)) << "Second line should contain the original text";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be on the second line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor should be at beginning of second line";
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 line after undo";
    ASSERT_EQ("BeginningOfLine", editor.getBuffer().getLine(0)) << "Line should be restored to original state";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be restored";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored";
}

// Test adding a newline in an empty buffer
TEST_F(NewLineCommandTest, EmptyBuffer) {
    // Setup: Ensure buffer is empty
    ASSERT_EQ(0, editor.getBuffer().lineCount()) << "Buffer should be empty before test";
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution - we expect two empty lines based on NewLineCommand implementation
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after newline";
    ASSERT_EQ("", editor.getBuffer().getLine(0)) << "First line should be empty";
    ASSERT_EQ("", editor.getBuffer().getLine(1)) << "Second line should be empty";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be on the second line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor should be at beginning of second line";
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo - should revert to empty buffer
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 empty line after undo";
    ASSERT_EQ("", editor.getBuffer().getLine(0)) << "Line should be empty";
} 