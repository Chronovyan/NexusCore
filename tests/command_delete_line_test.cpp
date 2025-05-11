#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include <memory>
#include <string>

class DeleteLineCommandTest : public ::testing::Test {
protected:
    TestEditor editor;
    
    void SetUp() override {
        editor.getBuffer().clear(false); // Clear without adding an empty line
    }
    
    void TearDown() override {
        // Clean up any resources if needed
    }
};

// Test deleting a line in the middle of a buffer
TEST_F(DeleteLineCommandTest, DeleteMiddleLine) {
    // Setup the buffer with three lines
    editor.getBuffer().addLine("Line 0");
    editor.getBuffer().addLine("Line 1 to delete");
    editor.getBuffer().addLine("Line 2");
    editor.setCursor(1, 0); // Set cursor to the line to be deleted
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(1); // Delete line 1
    deleteCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after deletion";
    ASSERT_EQ("Line 0", editor.getBuffer().getLine(0)) << "First line should remain unchanged";
    ASSERT_EQ("Line 2", editor.getBuffer().getLine(1)) << "Third line should become the second line";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be at the line that replaced the deleted line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be at the beginning of the line";
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(3, editor.getBuffer().lineCount()) << "Buffer should have 3 lines after undo";
    ASSERT_EQ("Line 0", editor.getBuffer().getLine(0)) << "First line should be restored";
    ASSERT_EQ("Line 1 to delete", editor.getBuffer().getLine(1)) << "Deleted line should be restored";
    ASSERT_EQ("Line 2", editor.getBuffer().getLine(2)) << "Third line should be back in its original position";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be restored to original line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored to original position";
}

// Test deleting the last line in a buffer
TEST_F(DeleteLineCommandTest, DeleteLastLine) {
    // Setup the buffer with two lines
    editor.getBuffer().addLine("Line A");
    editor.getBuffer().addLine("Line B to delete");
    editor.setCursor(1, 0); // Set cursor to the line to be deleted
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(1); // Delete line 1 (the last line)
    deleteCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 line after deletion";
    ASSERT_EQ("Line A", editor.getBuffer().getLine(0)) << "First line should remain unchanged";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should move to the previous line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be at the beginning of the line";
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after undo";
    ASSERT_EQ("Line A", editor.getBuffer().getLine(0)) << "First line should be unchanged";
    ASSERT_EQ("Line B to delete", editor.getBuffer().getLine(1)) << "Deleted line should be restored";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be restored to original line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored to original position";
}

// Test deleting the only line in a buffer
TEST_F(DeleteLineCommandTest, DeleteOnlyLine) {
    // Setup the buffer with a single line
    editor.getBuffer().addLine("Only line to delete");
    editor.setCursor(0, 0); // Set cursor to the beginning of the line
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(0); // Delete line 0 (the only line)
    deleteCmd->execute(editor);
    
    // Verify state after execution - buffer should have one empty line
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 empty line after deletion";
    ASSERT_TRUE(editor.getBuffer().getLine(0).empty()) << "Remaining line should be empty";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should be at line 0";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor should be at position 0";
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 line after undo";
    ASSERT_EQ("Only line to delete", editor.getBuffer().getLine(0)) << "Original line should be restored";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should be restored to original line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored to original position";
}

// Test deleting the first line in a buffer with multiple lines
TEST_F(DeleteLineCommandTest, DeleteFirstLineOfMultiple) {
    // Setup the buffer with two lines
    editor.getBuffer().addLine("First line to delete");
    editor.getBuffer().addLine("Second line");
    editor.setCursor(0, 0); // Set cursor to the beginning of the first line
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(0); // Delete line 0 (the first line)
    deleteCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have 1 line after deletion";
    ASSERT_EQ("Second line", editor.getBuffer().getLine(0)) << "Second line should become the first line";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should be at line 0";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor should be at position 0";
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have 2 lines after undo";
    ASSERT_EQ("First line to delete", editor.getBuffer().getLine(0)) << "First line should be restored";
    ASSERT_EQ("Second line", editor.getBuffer().getLine(1)) << "Second line should be back at position 1";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should be restored to original line";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored to original position";
} 