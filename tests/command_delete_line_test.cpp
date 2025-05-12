#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>

class DeleteLineCommandTest : public test_utils::EditorCommandTestBase {
    // No additional members needed as all utilities are in the base class
};

// Test deleting a line in the middle of a buffer
TEST_F(DeleteLineCommandTest, DeleteMiddleLine) {
    // Setup the buffer with three lines
    setBufferLines({"Line 0", "Line 1 to delete", "Line 2"});
    positionCursor(1, 0); // Set cursor to the line to be deleted
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(1); // Delete line 1
    deleteCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"Line 0", "Line 2"});
    verifyCursorPosition(1, 0);
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"Line 0", "Line 1 to delete", "Line 2"});
    verifyCursorPosition(1, 0);
}

// Test deleting the last line in a buffer
TEST_F(DeleteLineCommandTest, DeleteLastLine) {
    // Setup the buffer with two lines
    setBufferLines({"Line A", "Line B to delete"});
    positionCursor(1, 0); // Set cursor to the line to be deleted
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(1); // Delete line 1 (the last line)
    deleteCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"Line A"});
    verifyCursorPosition(0, 0);
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"Line A", "Line B to delete"});
    verifyCursorPosition(1, 0);
}

// Test deleting the only line in a buffer
TEST_F(DeleteLineCommandTest, DeleteOnlyLine) {
    // Setup the buffer with a single line
    setBufferContent("Only line to delete");
    positionCursor(0, 0); // Set cursor to the beginning of the line
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(0); // Delete line 0 (the only line)
    deleteCmd->execute(editor);
    
    // Verify state after execution - buffer should have one empty line
    verifyBufferContent({""});
    verifyCursorPosition(0, 0);
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"Only line to delete"});
    verifyCursorPosition(0, 0);
}

// Test deleting the first line in a buffer with multiple lines
TEST_F(DeleteLineCommandTest, DeleteFirstLineOfMultiple) {
    // Setup the buffer with two lines
    setBufferLines({"First line to delete", "Second line"});
    positionCursor(0, 0); // Set cursor to the beginning of the first line
    
    // Execute the command
    auto deleteCmd = std::make_unique<DeleteLineCommand>(0); // Delete line 0 (the first line)
    deleteCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"Second line"});
    verifyCursorPosition(0, 0);
    
    // Undo the command
    deleteCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"First line to delete", "Second line"});
    verifyCursorPosition(0, 0);
} 