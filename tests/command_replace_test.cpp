#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>

class ReplaceCommandTest : public test_utils::EditorCommandTestBase {
    // No additional members needed as all utilities are in the base class
};

// Test case-sensitive replacement with a single occurrence
TEST_F(ReplaceCommandTest, CaseSensitiveSimple) {
    // Setup buffer with initial content
    setBufferContent("Hello world, hello World.");
    positionCursor(0, 0); // For ReplaceCommand, this sets the context for where search starts
    
    auto replaceCmd = std::make_unique<ReplaceCommand>("world", "planet", true);
    
    // Execute the command
    replaceCmd->execute(editor);
    
    // Verify text was replaced correctly
    verifyBufferContent({"Hello planet, hello World."});
    
    // Verify cursor position after replacement
    verifyCursorPosition(0, 12);
    
    // Undo the command
    replaceCmd->undo(editor);
    
    // Verify text restoration
    verifyBufferContent({"Hello world, hello World."});
    
    // Verify cursor position restoration
    verifyCursorPosition(0, 0);
}

// Test case-insensitive replacement with multiple occurrences
TEST_F(ReplaceCommandTest, CaseInsensitiveMultiple) {
    // Setup buffer with initial content
    setBufferContent("Hello world, hello World.");
    positionCursor(0, 0);
    
    // First replacement (replace "world" with "galaxy")
    auto replaceCmd1 = std::make_unique<ReplaceCommand>("wOrLd", "galaxy", false);
    replaceCmd1->execute(editor);
    
    // Verify first replacement
    verifyBufferContent({"Hello galaxy, hello World."});
    
    // Second replacement (replace "World" with "galaxy")
    auto replaceCmd2 = std::make_unique<ReplaceCommand>("wOrLd", "galaxy", false);
    replaceCmd2->execute(editor);
    
    // Verify both replacements
    verifyBufferContent({"Hello galaxy, hello galaxy."});
    
    // Undo the second replacement
    replaceCmd2->undo(editor);
    
    // Verify second replacement was undone
    verifyBufferContent({"Hello galaxy, hello World."});
    
    // Undo the first replacement
    replaceCmd1->undo(editor);
    
    // Verify fully restored state
    verifyBufferContent({"Hello world, hello World."});
}

// Test when search term is not found
TEST_F(ReplaceCommandTest, NotFound) {
    // Setup buffer with initial content
    setBufferContent("Hello world, hello World.");
    positionCursor(0, 0);
    
    auto replaceCmd = std::make_unique<ReplaceCommand>("nonexistent", "stuff", true);
    
    // Execute the command (should do nothing)
    replaceCmd->execute(editor);
    
    // Verify nothing changed
    verifyBufferContent({"Hello world, hello World."});
    verifyCursorPosition(0, 0);
    
    // Undo should also do nothing
    replaceCmd->undo(editor);
    
    // Verify still nothing changed
    verifyBufferContent({"Hello world, hello World."});
    verifyCursorPosition(0, 0);
}

// Test with empty replacement text
TEST_F(ReplaceCommandTest, EmptyReplacement) {
    // Setup buffer with initial content
    setBufferContent("Hello world, hello World.");
    positionCursor(0, 13); // After "Hello world, "
    
    auto replaceCmd = std::make_unique<ReplaceCommand>("hello", "", true);
    
    // Execute the command (should remove "hello")
    replaceCmd->execute(editor);
    
    // Verify "hello" was replaced with nothing
    verifyBufferContent({"Hello world,  World."});
    
    // Undo
    replaceCmd->undo(editor);
    
    // Verify restoration
    verifyBufferContent({"Hello world, hello World."});
    verifyCursorPosition(0, 13);
}

// Test with multi-line buffer
TEST_F(ReplaceCommandTest, MultiLineBuffer) {
    // Setup buffer with multiple lines
    setBufferLines({
        "Hello world, hello World.",
        "Another world reference.", 
        "No matches on this line."
    });
    positionCursor(0, 0);
    
    auto replaceCmd = std::make_unique<ReplaceCommand>("world", "planet", true);
    
    // Execute the command to replace first occurrence
    replaceCmd->execute(editor);
    
    // Verify first replacement on line 0
    verifyBufferContent({
        "Hello planet, hello World.",
        "Another world reference.", 
        "No matches on this line."
    });
    
    // Set cursor to line 1 for next replacement
    positionCursor(1, 0);
    replaceCmd->execute(editor);
    
    // Verify replacement on line 1
    verifyBufferContent({
        "Hello planet, hello World.",
        "Another planet reference.", 
        "No matches on this line."
    });
    
    // Undo second replacement
    replaceCmd->undo(editor);
    
    // Verify second line restored
    verifyBufferContent({
        "Hello planet, hello World.",
        "Another world reference.", 
        "No matches on this line."
    });
}

// Test the wasSuccessful() method
TEST_F(ReplaceCommandTest, WasSuccessful) {
    // Setup buffer with initial content
    setBufferContent("Hello world, hello World.");
    positionCursor(0, 0);
    
    // Test with a match
    auto successCmd = std::make_unique<ReplaceCommand>("world", "planet", true);
    successCmd->execute(editor);
    ASSERT_TRUE(successCmd->wasSuccessful())
        << "wasSuccessful() should return true when text was replaced";
    
    // Reset the buffer for the next test
    setBufferContent("Hello world, hello World.");
    positionCursor(0, 0);
    
    // Test with no match
    auto failCmd = std::make_unique<ReplaceCommand>("nonexistent", "stuff", true);
    failCmd->execute(editor);
    ASSERT_FALSE(failCmd->wasSuccessful())
        << "wasSuccessful() should return false when no text was replaced";
} 