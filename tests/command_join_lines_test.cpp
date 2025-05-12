#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>

class JoinLinesCommandTest : public test_utils::EditorCommandTestBase {
    // No additional members needed as all utilities are in the base class
};

// Test basic JoinLinesCommand execution
TEST_F(JoinLinesCommandTest, Execute) {
    // Setup: Add two lines
    setBufferLines({"First line", "Second line"});
    positionCursor(0, 0); // Cursor at start of first line
    
    auto joinCmd = std::make_unique<JoinLinesCommand>(0); // Join line 0 with line 1
    
    joinCmd->execute(editor);

    // Verify buffer content after join
    verifyBufferContent({"First lineSecond line"});
    
    // Verify cursor position - should be at the join point
    verifyCursorPosition(0, std::string("First line").length());
}

// Test JoinLinesCommand undo
TEST_F(JoinLinesCommandTest, Undo) {
    // Setup: Add two lines
    setBufferLines({"First line", "Second line"});
    positionCursor(0, 0); // Cursor at start of first line
    
    auto joinCmd = std::make_unique<JoinLinesCommand>(0); // Join line 0 with line 1
    
    // Execute first
    joinCmd->execute(editor);
    
    // Then undo
    joinCmd->undo(editor);

    // Verify buffer content is restored
    verifyBufferContent({"First line", "Second line"});
    
    // Verify cursor position after undo
    verifyCursorPosition(1, 0);
}

// Test joining with empty lines
TEST_F(JoinLinesCommandTest, JoinWithEmptyLine) {
    // Setup: Add an empty line followed by a non-empty line
    setBufferLines({"", "Non-empty line"});
    positionCursor(0, 0);
    
    auto joinCmd = std::make_unique<JoinLinesCommand>(0);
    joinCmd->execute(editor);
    
    // Verify results after join
    verifyBufferContent({"Non-empty line"});
    verifyCursorPosition(0, 0);
    
    // Test undo as well
    joinCmd->undo(editor);
    
    // Verify state after undo
    verifyBufferContent({"", "Non-empty line"});
    verifyCursorPosition(1, 0);
}

// Test joining the last line (should do nothing or handle gracefully)
TEST_F(JoinLinesCommandTest, JoinLastLine) {
    // Setup: Add two lines
    setBufferLines({"First line", "Second line"});
    positionCursor(1, 0); // Position on the last line
    
    auto joinCmd = std::make_unique<JoinLinesCommand>(1);
    joinCmd->execute(editor);
    
    // Verify buffer content remains unchanged
    verifyBufferContent({"First line", "Second line"});
    
    // Verify cursor position remains unchanged
    verifyCursorPosition(1, 0);
} 