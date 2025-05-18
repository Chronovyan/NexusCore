#include "gtest/gtest.h"
#include "TestEditor.h"
#include "TestUtilities.h"
#include "../src/EditorCommands.h"
#include <memory>
#include <string>

class ForwardDeleteCommandTest : public test_utils::EditorCommandTestBase {
protected:
    // Helper to set up a selection in the editor
    void setupSelection(size_t startLine, size_t startCol, size_t endLine, size_t endCol) {
        editor.setSelectionRange(startLine, startCol, endLine, endCol);
        editor.setCursor(endLine, endCol);
    }
};

// Test forward delete in the middle of a line
TEST_F(ForwardDeleteCommandTest, ForwardDeleteMiddle) {
    // Set up editor with a line and cursor in the middle
    setBufferLines({"Hello World"});
    positionCursor(0, 5); // Cursor after "Hello"

    // Execute forward delete
    auto command = std::make_unique<DeleteCharCommand>(false);
    command->execute(editor);

    // Verify character was deleted and cursor position unchanged
    verifyBufferContent({"HelloWorld"});
    verifyCursorPosition(0, 5);

    // Undo and verify restored state
    command->undo(editor);
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 5);
}

// Test forward delete at the end of a line (joining with next line)
TEST_F(ForwardDeleteCommandTest, ForwardDeleteLineJoin) {
    // Set up editor with two lines
    setBufferLines({"Line 1", "Line 2"});
    positionCursor(0, 6); // Cursor at the end of "Line 1"

    // Execute forward delete
    auto command = std::make_unique<DeleteCharCommand>(false);
    command->execute(editor);

    // Verify lines were joined
    verifyBufferContent({"Line 1Line 2"});
    EXPECT_EQ(1, editor.getBuffer().lineCount());
    verifyCursorPosition(0, 6); // Cursor position shouldn't change

    // Undo and verify restored state
    command->undo(editor);
    verifyBufferContent({"Line 1", "Line 2"});
    EXPECT_EQ(2, editor.getBuffer().lineCount());
    verifyCursorPosition(0, 6);
}

// Test selection behavior for forward delete
TEST_F(ForwardDeleteCommandTest, ForwardDeleteSelection) {
    // Initialize the editor with some text
    setBufferLines({"Hello World"});
    
    // Select characters
    editor.setCursor(0, 1);
    setupSelection(0, 1, 0, 6);  // Select "ello "
    
    // Execute the delete command which creates a ReplaceSelectionCommand("") to delete the selection
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
    // Cursor should be at selection end
    verifyCursorPosition(0, 6);
    
    // Selection should be restored after undo
    EXPECT_TRUE(editor.hasSelection());
    EXPECT_EQ(0, editor.getSelectionStartLine());
    EXPECT_EQ(1, editor.getSelectionStartCol());
    EXPECT_EQ(0, editor.getSelectionEndLine());
    EXPECT_EQ(6, editor.getSelectionEndCol());
}

// Test forward delete at the end of the buffer (should do nothing)
TEST_F(ForwardDeleteCommandTest, ForwardDeleteBufferEnd) {
    // Set up editor with cursor at end of buffer
    setBufferLines({"Hello World"});
    positionCursor(0, 11); // At the end of the buffer

    // Execute forward delete
    auto command = std::make_unique<DeleteCharCommand>(false);
    command->execute(editor);

    // Verify no change (as we're at the end of buffer)
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 11);

    // Undo (should be a no-op)
    command->undo(editor);
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 11);
}

// Test forward delete on an empty line (joining with next line)
TEST_F(ForwardDeleteCommandTest, ForwardDeleteEmptyLine) {
    // Set up editor with an empty line followed by a regular line
    setBufferLines({"", "Hello World"});
    positionCursor(0, 0); // At the empty line

    // Execute forward delete
    auto command = std::make_unique<DeleteCharCommand>(false);
    command->execute(editor);

    // Verify empty line removed and lines joined
    verifyBufferContent({"Hello World"});
    EXPECT_EQ(1, editor.getBuffer().lineCount());
    verifyCursorPosition(0, 0);

    // Undo and verify restored state
    command->undo(editor);
    verifyBufferContent({"", "Hello World"});
    EXPECT_EQ(2, editor.getBuffer().lineCount());
    verifyCursorPosition(0, 0);
}

// Test multiline selection behavior for forward delete
TEST_F(ForwardDeleteCommandTest, ForwardDeleteMultilineSelection) {
    // Initialize with multiple lines
    setBufferLines({"First line", "Second line", "Third line"});
    
    // Setup a multi-line selection
    editor.setCursor(0, 6);  // Position at the start of "line" in "First line"
    setupSelection(0, 6, 2, 5);  // Select "line\nSecond line\nThird"
    
    // Execute the delete command to delete the selection
    auto command = std::make_unique<ReplaceSelectionCommand>("");
    command->execute(editor);
    
    // Verify that the lines are properly joined and the selection was deleted
    EXPECT_EQ(1, editor.getBuffer().lineCount());
    EXPECT_EQ("First  line", editor.getBuffer().getLine(0));
    
    // Cursor should be at the selection start
    verifyCursorPosition(0, 6);
    
    // No selection after delete
    EXPECT_FALSE(editor.hasSelection());
    
    // Undo the command
    command->undo(editor);
    
    // Verify original lines are restored
    EXPECT_EQ(3, editor.getBuffer().lineCount());
    verifyBufferContent({"First line", "Second line", "Third line"});
    
    // After undo, the selection SHOULD be restored
    // Cursor should be at selection end
    verifyCursorPosition(2, 5);
    
    // Selection should be restored
    EXPECT_TRUE(editor.hasSelection());
    EXPECT_EQ(0, editor.getSelectionStartLine());
    EXPECT_EQ(6, editor.getSelectionStartCol());
    EXPECT_EQ(2, editor.getSelectionEndLine());
    EXPECT_EQ(5, editor.getSelectionEndCol());
}

// Test deletion of a character with forward delete
TEST_F(ForwardDeleteCommandTest, DeleteCharacter) {
    // Set up editor with a line and cursor at position
    setBufferLines({"Hello World"});
    positionCursor(0, 5); // Cursor after "Hello"

    // Execute forward delete
    auto command = std::make_unique<DeleteCharCommand>(false);
    command->execute(editor);

    // Verify character was deleted
    verifyBufferContent({"HelloWorld"});
    verifyCursorPosition(0, 5); // Cursor should remain at same position

    // Undo and verify restored state
    command->undo(editor);
    verifyBufferContent({"Hello World"});
    verifyCursorPosition(0, 5);
}

// Test to ensure that forward delete works properly with line indentation
TEST_F(ForwardDeleteCommandTest, ForwardDeleteIndentedLines) {
    // Set up indented lines
    setBufferLines({
        "int main() {",
        "    int x = 10;",
        "    return 0;",
        "}"
    });
    
    // Position cursor at the end of the first line
    positionCursor(0, 12); // After "int main() {"
    
    // Execute forward delete to join with indented line
    auto command = std::make_unique<DeleteCharCommand>(false);
    command->execute(editor);
    
    // Check that lines merged correctly with indentation preserved
    verifyBufferContent({"int main() {    int x = 10;", "    return 0;", "}"});
    verifyCursorPosition(0, 12);
    
    // Undo and check original state restored
    command->undo(editor);
    verifyBufferContent({"int main() {", "    int x = 10;", "    return 0;", "}"});
    verifyCursorPosition(0, 12);
}

// Test selection expansion behavior for forward delete
TEST_F(ForwardDeleteCommandTest, SelectionExpansion) {
    // Initialize with multiple lines
    setBufferLines({"Line 1", "Line 2", "Line 3"});
    
    // Setup a multi-line selection with text at the beginning and end
    editor.setCursor(0, 3);  // Position at "e 1" in "Line 1"
    setupSelection(0, 3, 2, 3);  // Select "e 1\nLine 2\nLin" from "Line 1\nLine 2\nLine 3"
    
    // Execute the delete command to delete the selection
    auto command = std::make_unique<ReplaceSelectionCommand>("");
    command->execute(editor);
    
    // Verify the text is correctly joined
    EXPECT_EQ(1, editor.getBuffer().lineCount());
    EXPECT_EQ("Line 3", editor.getBuffer().getLine(0));
    
    // Cursor should be at the selection start
    verifyCursorPosition(0, 3);
    
    // No selection after deletion
    EXPECT_FALSE(editor.hasSelection());
    
    // Undo the command
    command->undo(editor);
    
    // Verify original text is restored
    EXPECT_EQ(3, editor.getBuffer().lineCount());
    verifyBufferContent({"Line 1", "Line 2", "Line 3"});
    
    // After undo, the selection SHOULD be restored
    // Cursor should be at selection end
    verifyCursorPosition(2, 3);
    
    // Selection should be restored
    EXPECT_TRUE(editor.hasSelection());
    EXPECT_EQ(0, editor.getSelectionStartLine());
    EXPECT_EQ(3, editor.getSelectionStartCol());
    EXPECT_EQ(2, editor.getSelectionEndLine());
    EXPECT_EQ(3, editor.getSelectionEndCol());
} 