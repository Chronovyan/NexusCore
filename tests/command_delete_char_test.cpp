#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>

class DeleteCharCommandTest : public test_utils::EditorCommandTestBase {
    // No additional members needed as all utilities are in the base class
};

// Test backspace in the middle of text
TEST_F(DeleteCharCommandTest, BackspaceMiddle) {
    // Setup buffer with a single line
    setBufferContent("abc");
    
    // Position cursor at 'c'
    positionCursor(0, 2);
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true /*isBackspace*/);
    backspaceCmd->execute(editor);
    
    // Verify state after backspace at position 2
    verifyBufferContent({"ac"});
    verifyCursorPosition(0, 1);
    
    // Test undo
    backspaceCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 2);
}

// Test delete in the middle of text
TEST_F(DeleteCharCommandTest, DeleteMiddle) {
    // Setup buffer with a single line
    setBufferContent("abc");
    
    // Position cursor at 'b'
    positionCursor(0, 1);
    
    auto deleteCmd = std::make_unique<DeleteCharCommand>(false /*isBackspace*/);
    deleteCmd->execute(editor);
    
    // Verify state after delete at position 1
    verifyBufferContent({"ac"});
    verifyCursorPosition(0, 1);
    
    // Test undo
    deleteCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 1);
}

// Test backspace at the beginning of a line (joining with previous line)
TEST_F(DeleteCharCommandTest, BackspaceLineStart) {
    // Setup buffer with two lines
    setBufferLines({"abc", "def"});
    positionCursor(1, 0); // Position at start of second line
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);
    
    // Verify state after joining lines
    verifyBufferContent({"abcdef"});
    verifyCursorPosition(0, 3);
    
    // Test undo
    backspaceCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"abc", "def"});
    verifyCursorPosition(1, 0);
}

// Test delete at the end of a line (joining with next line)
TEST_F(DeleteCharCommandTest, DeleteLineEnd) {
    // Setup buffer with two lines
    setBufferLines({"abc", "def"});
    positionCursor(0, 3); // Position at end of first line
    
    auto deleteCmd = std::make_unique<DeleteCharCommand>(false);
    deleteCmd->execute(editor);
    
    // Verify state after joining lines
    verifyBufferContent({"abcdef"});
    verifyCursorPosition(0, 3);
    
    // Test undo
    deleteCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"abc", "def"});
    verifyCursorPosition(0, 3);
}

// Test backspace at buffer start (should do nothing)
TEST_F(DeleteCharCommandTest, BackspaceBufferStart) {
    // Setup buffer with a single line
    setBufferContent("abc");
    
    // Position cursor at start of buffer
    positionCursor(0, 0);
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);
    
    // Verify no change
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 0);
    
    // Test undo (should also do nothing)
    backspaceCmd->undo(editor);
    
    // Verify still no change
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 0);
}

// Test delete at buffer end (should do nothing)
TEST_F(DeleteCharCommandTest, DeleteBufferEnd) {
    // Setup buffer with a single line
    setBufferContent("abc");
    
    // Position cursor at end of buffer
    positionCursor(0, 3); // After 'c'
    
    auto deleteCmd = std::make_unique<DeleteCharCommand>(false);
    deleteCmd->execute(editor);
    
    // Verify no change
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 3);
    
    // Test undo (should also do nothing)
    deleteCmd->undo(editor);
    
    // Verify still no change
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 3);
}

// Test backspace at an empty line
TEST_F(DeleteCharCommandTest, BackspaceEmptyLine) {
    // Setup buffer with a line followed by an empty line
    setBufferLines({"abc", ""});
    positionCursor(1, 0); // Position at empty line
    
    auto backspaceCmd = std::make_unique<DeleteCharCommand>(true);
    backspaceCmd->execute(editor);
    
    // Verify state after deleting empty line
    verifyBufferContent({"abc"});
    verifyCursorPosition(0, 3);
    
    // Test undo
    backspaceCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"abc", ""});
    verifyCursorPosition(1, 0);
} 