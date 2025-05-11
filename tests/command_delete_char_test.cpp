#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include <memory>
#include <string>

class DeleteCharCommandTest : public ::testing::Test {
protected:
    TestEditor editor;
    
    void SetUp() override {
        editor.getBuffer().clear(false); // Clear without adding an empty line
        editor.getBuffer().addLine("abc");
    }
};

// Test backspace in the middle of text
TEST_F(DeleteCharCommandTest, BackspaceMiddle) {
    // Position cursor at 'c'
    editor.setCursor(0, 2);
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true /*isBackspace*/);
    backspaceCmd->execute(editor);
    
    // Check the state after backspace at position 2
    ASSERT_EQ("ac", editor.getBuffer().getLine(0)) << "Line content should be 'ac'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(1, editor.getCursorCol()) << "Cursor column should be 1";
    
    // Test undo
    backspaceCmd->undo(editor);
    
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should be restored to 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0 after undo";
    ASSERT_EQ(2, editor.getCursorCol()) << "Cursor column should be restored to 2";
}

// Test delete in the middle of text
TEST_F(DeleteCharCommandTest, DeleteMiddle) {
    // Position cursor at 'b'
    editor.setCursor(0, 1);
    
    auto deleteCmd = std::make_unique<DeleteCharCommand>(false /*isBackspace*/);
    deleteCmd->execute(editor);
    
    // Check the state after delete at position 1
    ASSERT_EQ("ac", editor.getBuffer().getLine(0)) << "Line content should be 'ac'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(1, editor.getCursorCol()) << "Cursor column should remain 1";
    
    // Test undo
    deleteCmd->undo(editor);
    
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should be restored to 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0 after undo";
    ASSERT_EQ(1, editor.getCursorCol()) << "Cursor column should remain 1";
}

// Test backspace at the beginning of a line (joining with previous line)
TEST_F(DeleteCharCommandTest, BackspaceLineStart) {
    // Setup - add another line
    editor.getBuffer().addLine("def");
    editor.setCursor(1, 0); // Position at start of second line
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);
    
    // Check lines joined
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Line count should be 1 after joining";
    ASSERT_EQ("abcdef", editor.getBuffer().getLine(0)) << "Line content should be 'abcdef'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(3, editor.getCursorCol()) << "Cursor column should be at the join point (3)";
    
    // Test undo
    backspaceCmd->undo(editor);
    
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Line count should be restored to 2";
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "First line should be restored to 'abc'";
    ASSERT_EQ("def", editor.getBuffer().getLine(1)) << "Second line should be restored to 'def'";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor line should be restored to 1";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored to 0";
}

// Test delete at the end of a line (joining with next line)
TEST_F(DeleteCharCommandTest, DeleteLineEnd) {
    // Setup - add another line
    editor.getBuffer().addLine("def");
    editor.setCursor(0, 3); // Position at end of first line
    
    auto deleteCmd = std::make_unique<DeleteCharCommand>(false);
    deleteCmd->execute(editor);
    
    // Check lines joined
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Line count should be 1 after joining";
    ASSERT_EQ("abcdef", editor.getBuffer().getLine(0)) << "Line content should be 'abcdef'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(3, editor.getCursorCol()) << "Cursor column should remain at the join point (3)";
    
    // Test undo
    deleteCmd->undo(editor);
    
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Line count should be restored to 2";
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "First line should be restored to 'abc'";
    ASSERT_EQ("def", editor.getBuffer().getLine(1)) << "Second line should be restored to 'def'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be restored to 0";
    ASSERT_EQ(3, editor.getCursorCol()) << "Cursor column should be restored to 3";
}

// Test backspace at buffer start (should do nothing)
TEST_F(DeleteCharCommandTest, BackspaceBufferStart) {
    // Position cursor at start of buffer
    editor.setCursor(0, 0);
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);
    
    // Check no change
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should remain 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should remain 0";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should remain 0";
    
    // Test undo (should also do nothing)
    backspaceCmd->undo(editor);
    
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should still be 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should still be 0";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should still be 0";
}

// Test delete at buffer end (should do nothing)
TEST_F(DeleteCharCommandTest, DeleteBufferEnd) {
    // Position cursor at end of buffer
    editor.setCursor(0, 3); // After 'c'
    
    auto deleteCmd = std::make_unique<DeleteCharCommand>(false);
    deleteCmd->execute(editor);
    
    // Check no change
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should remain 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should remain 0";
    ASSERT_EQ(3, editor.getCursorCol()) << "Cursor column should remain 3";
    
    // Test undo (should also do nothing)
    deleteCmd->undo(editor);
    
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should still be 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should still be 0";
    ASSERT_EQ(3, editor.getCursorCol()) << "Cursor column should still be 3";
}

// Test backspace at an empty line
TEST_F(DeleteCharCommandTest, BackspaceEmptyLine) {
    // Setup - add an empty line
    editor.getBuffer().addLine("");
    editor.setCursor(1, 0); // Position at empty line
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);
    
    // Check empty line removed
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Line count should be 1 after deleting empty line";
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "Line content should be 'abc'";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be 0";
    ASSERT_EQ(3, editor.getCursorCol()) << "Cursor column should be at end of previous line (3)";
    
    // Test undo
    backspaceCmd->undo(editor);
    
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Line count should be restored to 2";
    ASSERT_EQ("abc", editor.getBuffer().getLine(0)) << "First line should be 'abc'";
    ASSERT_EQ("", editor.getBuffer().getLine(1)) << "Second line should be empty";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor line should be restored to 1";
    ASSERT_EQ(0, editor.getCursorCol()) << "Cursor column should be restored to 0";
} 