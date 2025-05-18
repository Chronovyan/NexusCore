#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>
#include <iostream> // Add for debugging

class BackspaceCommandTest : public test_utils::EditorCommandTestBase {
    // No additional members needed as all utilities are in the base class
};

// Test for backspace at middle of line
TEST_F(BackspaceCommandTest, BackspaceMiddle) {
    // Set up editor with a line and cursor in the middle
    setBufferLines({"Hello World"});
    positionCursor(0, 5); // Cursor after "Hello"

    // Execute backspace
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);

    // Verify character was deleted and cursor moved back
    verifyBufferContent({"Hell World"});
    verifyCursorPosition(0, 4);

    // Undo and verify restored state
    backspaceCmd->undo(editor);
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 5);
}

// Test backspace at the beginning of a line (joining with previous line)
TEST_F(BackspaceCommandTest, BackspaceLineJoin) {
    // Set up editor with two lines
    setBufferLines({"Line 1", "Line 2"});
    positionCursor(1, 0); // Beginning of "Line 2"

    // Execute backspace
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);

    // Verify lines were joined
    verifyBufferContent({"Line 1Line 2"});
    // Check line count through buffer directly
    EXPECT_EQ(1, editor.getBuffer().lineCount());
    verifyCursorPosition(0, 6); // Cursor should be at position after "Line 1"

    // Undo and verify restored state
    backspaceCmd->undo(editor);
    verifyBufferContent({"Line 1", "Line 2"});
    EXPECT_EQ(2, editor.getBuffer().lineCount());
    verifyCursorPosition(1, 0);
}

// Test selection behavior for backspace
TEST_F(BackspaceCommandTest, BackspaceSelection) {
    // Initialize the editor with some text
    setBufferLines({"Hello World"});
    
    // Select characters
    editor.setCursor(0, 1);
    editor.setSelectionRange(0, 1, 0, 6);  // Select "ello "
    
    // Execute the backspace command which creates a ReplaceSelectionCommand("") to delete the selection
    auto command = std::make_unique<ReplaceSelectionCommand>("");
    command->execute(editor);
    
    // Verify that the selection was deleted
    verifyBufferContent({"HWorld"});
    
    // Cursor should be at the selection start
    verifyCursorPosition(0, 1);
    
    // No selection remaining after delete
    EXPECT_FALSE(editor.hasSelection());
    
    // Undo the command
    command->undo(editor);
    
    // Verify that text is restored to original state
    verifyBufferContent({"Hello World"});
    
    // After undo, the selection SHOULD be restored according to our diagnostic test
    // Cursor will be at selection end
    verifyCursorPosition(0, 6);
    
    // Selection should be restored after undo
    EXPECT_TRUE(editor.hasSelection());
    EXPECT_EQ(0, editor.getSelectionStartLine());
    EXPECT_EQ(1, editor.getSelectionStartCol());
    EXPECT_EQ(0, editor.getSelectionEndLine());
    EXPECT_EQ(6, editor.getSelectionEndCol());
}

// Test backspace at buffer start (should do nothing)
TEST_F(BackspaceCommandTest, BackspaceBufferStart) {
    // Set up editor with cursor at start of buffer
    setBufferLines({"Hello World"});
    positionCursor(0, 0); // At the beginning of the buffer

    // Execute backspace
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);

    // Verify no change (as we're at the start of buffer)
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 0);

    // Undo (should be a no-op)
    backspaceCmd->undo(editor);
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 0);
}

// Test backspace at an empty line
TEST_F(BackspaceCommandTest, BackspaceEmptyLine) {
    // Set up editor with an empty line followed by a regular line
    setBufferLines({"", "Hello World"});
    positionCursor(1, 0); // At the beginning of line 1

    // Execute backspace
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);

    // Verify empty line removed
    verifyBufferContent({"Hello World"});
    EXPECT_EQ(1, editor.getBuffer().lineCount());
    verifyCursorPosition(0, 0);

    // Undo and verify restored state
    backspaceCmd->undo(editor);
    verifyBufferContent({"", "Hello World"});
    EXPECT_EQ(2, editor.getBuffer().lineCount());
    verifyCursorPosition(1, 0);
} 