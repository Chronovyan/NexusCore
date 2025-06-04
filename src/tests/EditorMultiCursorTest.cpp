#include <gtest/gtest.h>
#include "../Editor.h"
#include "../TextBuffer.h"
#include "../FileManager.h"
#include "../MultiCursor.h"
#include <memory>

// Test fixture for Editor with MultiCursor support
class EditorMultiCursorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a new editor with necessary dependencies
        textBuffer = std::make_unique<TextBuffer>();
        fileManager = std::make_shared<FileManager>();
        editor = std::make_unique<Editor>(textBuffer.get(), fileManager);
        
        // Initialize the text buffer with some test data
        std::vector<std::string> lines = {
            "Line 1 with some text",
            "Line 2 with repeated text repeated",
            "Line 3 with different content",
            "Line 4 with repeated text repeated",
            "Line 5 with final line"
        };
        textBuffer->load(lines);
    }
    
    std::unique_ptr<TextBuffer> textBuffer;
    std::shared_ptr<FileManager> fileManager;
    std::unique_ptr<Editor> editor;
};

// Test enabling/disabling multi-cursor mode
TEST_F(EditorMultiCursorTest, EnableDisableMultiCursor) {
    // Initially, multi-cursor should be disabled
    EXPECT_FALSE(editor->isMultiCursorEnabled());
    
    // Enable multi-cursor
    editor->setMultiCursorEnabled(true);
    EXPECT_TRUE(editor->isMultiCursorEnabled());
    
    // Disable multi-cursor
    editor->setMultiCursorEnabled(false);
    EXPECT_FALSE(editor->isMultiCursorEnabled());
}

// Test adding and removing cursors
TEST_F(EditorMultiCursorTest, AddRemoveCursors) {
    // Enable multi-cursor mode
    editor->setMultiCursorEnabled(true);
    
    // Initially should have only one cursor
    EXPECT_EQ(editor->getCursorCount(), 1);
    
    // Add some cursors
    editor->addCursor(1, 5);
    editor->addCursor(2, 10);
    editor->addCursor(3, 15);
    
    // Should now have 4 cursors (1 primary + 3 added)
    EXPECT_EQ(editor->getCursorCount(), 4);
    
    // Remove a cursor
    editor->removeCursor(2, 10);
    EXPECT_EQ(editor->getCursorCount(), 3);
    
    // Remove all secondary cursors
    editor->removeAllSecondaryCursors();
    EXPECT_EQ(editor->getCursorCount(), 1);
}

// Test adding cursors at all occurrences of text
TEST_F(EditorMultiCursorTest, AddCursorsAtAllOccurrences) {
    // Enable multi-cursor mode
    editor->setMultiCursorEnabled(true);
    
    // Add cursors at all occurrences of "repeated"
    size_t added = editor->addCursorsAtAllOccurrences("repeated", true);
    
    // Should have added 4 new cursors (2 occurrences in line 1, 2 in line 3)
    EXPECT_EQ(added, 4);
    EXPECT_EQ(editor->getCursorCount(), 5); // 1 primary + 4 added
    
    // Clear all secondary cursors
    editor->removeAllSecondaryCursors();
    EXPECT_EQ(editor->getCursorCount(), 1);
    
    // Test case-insensitive search
    added = editor->addCursorsAtAllOccurrences("REPEATED", false); // case-insensitive
    EXPECT_EQ(added, 4);
    EXPECT_EQ(editor->getCursorCount(), 5);
}

// Test adding cursors at a specific column
TEST_F(EditorMultiCursorTest, AddCursorsAtColumn) {
    // Enable multi-cursor mode
    editor->setMultiCursorEnabled(true);
    
    // Add cursors at column 5 for lines 1-3
    size_t added = editor->addCursorsAtColumn(1, 3, 5);
    
    // Should have added 3 new cursors
    EXPECT_EQ(added, 3);
    EXPECT_EQ(editor->getCursorCount(), 4); // 1 primary + 3 added
}

// Test cursor movement with multiple cursors
TEST_F(EditorMultiCursorTest, CursorMovement) {
    // Enable multi-cursor mode
    editor->setMultiCursorEnabled(true);
    
    // Set initial cursor position
    editor->setCursor(1, 5);
    
    // Add additional cursors
    editor->addCursor(2, 5);
    editor->addCursor(3, 5);
    
    // Move all cursors right
    editor->moveCursorRight();
    
    // Verify cursor count remains the same
    EXPECT_EQ(editor->getCursorCount(), 3);
    
    // Move all cursors left
    editor->moveCursorLeft();
    
    // Move all cursors down
    editor->moveCursorDown();
    
    // Move all cursors up
    editor->moveCursorUp();
}

// Test selection operations with multiple cursors
TEST_F(EditorMultiCursorTest, SelectionOperations) {
    // Enable multi-cursor mode
    editor->setMultiCursorEnabled(true);
    
    // Set initial cursor position
    editor->setCursor(1, 5);
    
    // Add additional cursors
    editor->addCursor(2, 5);
    editor->addCursor(3, 5);
    
    // Start selection for all cursors
    editor->startSelection();
    
    // Move cursors to create selections
    editor->moveCursorRight();
    editor->moveCursorRight();
    editor->moveCursorRight();
    
    // Update selection for all cursors
    editor->updateSelection();
    
    // End selection
    editor->endSelection();
    
    // Should have selections active
    EXPECT_TRUE(editor->hasSelection());
    
    // Get selected text (should include text from all selections)
    std::string selectedText = editor->getSelectedText();
    EXPECT_FALSE(selectedText.empty());
    
    // Clear all selections
    editor->clearSelection();
    EXPECT_FALSE(editor->hasSelection());
}

// Test setting selection range with multiple cursors
TEST_F(EditorMultiCursorTest, SetSelectionRange) {
    // Enable multi-cursor mode
    editor->setMultiCursorEnabled(true);
    
    // Set a selection range for the primary cursor
    editor->setSelectionRange(1, 5, 1, 15);
    
    // Should have a selection
    EXPECT_TRUE(editor->hasSelection());
    
    // Get selected text
    std::string selectedText = editor->getSelectedText();
    EXPECT_FALSE(selectedText.empty());
    
    // Clear selection
    editor->clearSelection();
    EXPECT_FALSE(editor->hasSelection());
}

// Test backward compatibility when multi-cursor is disabled
TEST_F(EditorMultiCursorTest, BackwardCompatibility) {
    // Make sure multi-cursor is disabled
    editor->setMultiCursorEnabled(false);
    
    // Set cursor position
    editor->setCursor(1, 5);
    
    // Start selection
    editor->startSelection();
    editor->moveCursorRight();
    editor->moveCursorRight();
    editor->updateSelection();
    
    // Should have a selection
    EXPECT_TRUE(editor->hasSelection());
    
    // Get selected text
    std::string selectedText = editor->getSelectedText();
    EXPECT_FALSE(selectedText.empty());
    
    // Clear selection
    editor->clearSelection();
    EXPECT_FALSE(editor->hasSelection());
}

// Main test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 