#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>

class NewLineCommandTest : public test_utils::EditorCommandTestBase {
    // No additional members needed as all utilities are in the base class
};

// Test splitting a line in the middle
TEST_F(NewLineCommandTest, SplitLineMiddle) {
    // Setup the buffer with a test line and position cursor in the middle
    setBufferContent("Line1Part1Line1Part2");
    positionCursor(0, 10); // Cursor after "Line1Part1"
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"Line1Part1", "Line1Part2"});
    verifyCursorPosition(1, 0);
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"Line1Part1Line1Part2"});
    verifyCursorPosition(0, 10);
}

// Test adding a newline at the end of a line
TEST_F(NewLineCommandTest, AddNewLineAtEnd) {
    // Setup the buffer with a test line and position cursor at the end
    setBufferContent("EndOfLine");
    positionCursor(0, 9); // Cursor at end of "EndOfLine"
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"EndOfLine", ""});
    verifyCursorPosition(1, 0);
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"EndOfLine"});
    verifyCursorPosition(0, 9);
}

// Test adding a newline at the beginning of a line
TEST_F(NewLineCommandTest, AddNewLineAtBeginning) {
    // Setup the buffer with a test line and position cursor at the beginning
    setBufferContent("BeginningOfLine");
    positionCursor(0, 0); // Cursor at beginning of "BeginningOfLine"
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"", "BeginningOfLine"});
    verifyCursorPosition(1, 0);
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"BeginningOfLine"});
    verifyCursorPosition(0, 0);
}

// Test adding a newline in an empty buffer
TEST_F(NewLineCommandTest, EmptyBuffer) {
    // Buffer is already empty from SetUp
    
    // Execute the command
    auto newLineCmd = std::make_unique<NewLineCommand>();
    newLineCmd->execute(editor);
    
    // Verify state after execution - we expect two empty lines based on NewLineCommand implementation
    verifyBufferContent({"", ""});
    verifyCursorPosition(1, 0);
    
    // Undo the command
    newLineCmd->undo(editor);
    
    // Verify state after undo - should revert to empty buffer with one line
    verifyBufferContent({""});
    verifyCursorPosition(0, 0);
} 