#include "gtest/gtest.h"
#include "../src/TextBuffer.h"
#include <string>
#include <vector>
#include <memory>

class TextBufferSelectionUndoTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<TextBuffer>();
        // Start with some initial text
        buffer->addLine("The quick brown fox jumps over the lazy dog");
    }
    
    void TearDown() override {
        buffer.reset();
    }
    
    std::unique_ptr<TextBuffer> buffer;
};

// Selection Tests
TEST_F(TextBufferSelectionUndoTest, BasicSelection) {
    // Select "quick"
    buffer->setSelection(4, 5);
    auto [start, end] = buffer->getSelection();
    EXPECT_EQ(start, 4);
    EXPECT_EQ(end, 9);
    EXPECT_EQ(buffer->getSelectedText(), "quick");
}

TEST_F(TextBufferSelectionUndoTest, SelectionDeletion) {
    // Select and delete "quick"
    buffer->setSelection(4, 9);
    buffer->deleteSelectedText();
    EXPECT_EQ(buffer->getText(), "The  brown fox jumps over the lazy dog");
    EXPECT_FALSE(buffer->hasSelection());
}

TEST_F(TextBufferSelectionUndoTest, SelectionReplacement) {
    // Select "quick" and replace with "slow"
    buffer->setSelection(4, 9);
    buffer->replaceSelectedText("slow");
    EXPECT_EQ(buffer->getText(), "The slow brown fox jumps over the lazy dog");
    EXPECT_FALSE(buffer->hasSelection());
}

TEST_F(TextBufferSelectionUndoTest, MultiLineSelection) {
    buffer->insertText("\nSecond line\nThird line");
    
    // Select from "brown" to "line"
    buffer->setSelection(10, 30);
    EXPECT_EQ(buffer->getSelectedText(), "brown fox jumps over the lazy dog\nSecond line");
    
    // Get the selection line range
    // If needed, we can add it later
}

// Undo/Redo Tests
TEST_F(TextBufferSelectionUndoTest, BasicUndoRedo) {
    // Initial insertion is already in the undo stack
    std::string initialText = buffer->getText();
    
    // Make a change
    buffer->insertText("!!!");
    EXPECT_NE(buffer->getText(), initialText);
    
    // Undo
    EXPECT_TRUE(buffer->canUndo());
    buffer->undo();
    EXPECT_EQ(buffer->getText(), initialText);
    
    // Redo
    EXPECT_TRUE(buffer->canRedo());
    buffer->redo();
    EXPECT_NE(buffer->getText(), initialText);
}

TEST_F(TextBufferSelectionUndoTest, MultipleUndoRedo) {
    std::string state1 = buffer->getText();
    
    // Make multiple changes
    buffer->insertText("!!!");
    std::string state2 = buffer->getText();
    buffer->insertText("???");
    std::string state3 = buffer->getText();
    
    // Undo twice
    buffer->undo();
    EXPECT_EQ(buffer->getText(), state2);
    
    buffer->undo();
    EXPECT_EQ(buffer->getText(), state1);
    
    // Redo twice
    buffer->redo();
    EXPECT_EQ(buffer->getText(), state2);
    
    buffer->redo();
    EXPECT_EQ(buffer->getText(), state3);
}

TEST_F(TextBufferSelectionUndoTest, SelectionPreservedAfterUndo) {
    // Make a change with selection
    buffer->setSelection(4, 9);
    buffer->replaceSelectedText("swift");
    
    // Undo should restore the original text and selection
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "The quick brown fox jumps over the lazy dog");
    
    auto [start, end] = buffer->getSelection();
    EXPECT_EQ(start, 4);
    EXPECT_EQ(end, 9);
}

TEST_F(TextBufferSelectionUndoTest, CursorPositionAfterUndo) {
    // Move cursor to end and insert text
    buffer->moveCursorTo(buffer->getText().length());
    buffer->insertText("!!!");
    
    // Undo should restore cursor position
    size_t posBeforeUndo = buffer->getCursorPosition();
    buffer->undo();
    size_t posAfterUndo = buffer->getCursorPosition();
    
    EXPECT_LT(posAfterUndo, posBeforeUndo);
    EXPECT_EQ(buffer->getText(), "The quick brown fox jumps over the lazy dog");
}

TEST_F(TextBufferSelectionUndoTest, BatchOperations) {
    // Start a compound edit
    buffer->beginUndoGroup();
    
    // Make multiple changes that should be undone together
    buffer->insertText("!!!");
    buffer->setSelection(0, 4);
    buffer->deleteSelectedText();
    
    // End the compound edit
    buffer->endUndoGroup();
    
    std::string stateAfterEdit = buffer->getText();
    
    // Undo should undo all changes in the group
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "The quick brown fox jumps over the lazy dog");
    
    // Redo should redo all changes in the group
    buffer->redo();
    EXPECT_EQ(buffer->getText(), stateAfterEdit);
}

TEST_F(TextBufferSelectionUndoTest, UndoRedoWithMultipleSelections) {
    // Clear initial text
    buffer->clear(true); // Clear but keep one empty line
    
    // Insert text with multiple cursors (simulated by multiple operations)
    buffer->beginUndoGroup();
    buffer->insertText("one");
    buffer->insertText("\ntwo");
    buffer->insertText("\nthree");
    buffer->endUndoGroup();
    
    // Make changes to multiple lines
    buffer->beginUndoGroup();
    buffer->moveCursorTo(0);
    buffer->insertText("*");
    buffer->moveCursorTo(5);
    buffer->insertText("\"");
    buffer->moveCursorTo(10);
    buffer->insertText("\"");
    buffer->endUndoGroup();
    
    std::string modifiedText = buffer->getText();
    
    // Undo should undo all cursor operations together
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "one\ntwo\nthree");
    
    // Redo should redo all cursor operations
    buffer->redo();
    EXPECT_EQ(buffer->getText(), modifiedText);
}

TEST_F(TextBufferSelectionUndoTest, UndoStackLimit) {
    // Set a small undo limit for testing
    const size_t undoLimit = 3;
    buffer->clear(true); // Clear but keep one empty line
    buffer->setUndoLimit(undoLimit);
    
    // Make more changes than the undo limit
    for (int i = 0; i < undoLimit + 2; i++) {
        buffer->insertText(std::to_string(i));
    }
    
    // Should only be able to undo up to the limit
    for (size_t i = 0; i < undoLimit; i++) {
        EXPECT_TRUE(buffer->canUndo());
        buffer->undo();
    }
    
    // No more undos should be available
    EXPECT_FALSE(buffer->canUndo());
}

TEST_F(TextBufferSelectionUndoTest, ClearUndoRedoStack) {
    // Make some changes
    buffer->insertText("!!!");
    buffer->insertText("???");
    
    // Clear the undo stack
    buffer->clearUndoStack();
    
    // Should not be able to undo
    EXPECT_FALSE(buffer->canUndo());
    EXPECT_FALSE(buffer->canRedo());
    
    // New changes should work normally
    buffer->insertText("###");
    EXPECT_TRUE(buffer->canUndo());
}

TEST_F(TextBufferSelectionUndoTest, UndoRedoWithLineOperations) {
    // Test with line operations
    buffer->clear(true); // Clear but keep one empty line
    buffer->addLine("Line 1");
    buffer->addLine("Line 2");
    
    // Delete a line
    buffer->setSelection(6, 11);  // Select "line2"
    buffer->deleteSelectedText();
    
    // Undo should restore the line
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "Line 1\nLine 2");
    
    // Test with line insertion
    buffer->moveCursorTo(6);  // Start of line2
    buffer->insertText("new line\n");
    
    // Undo should remove the line
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "line1\nline2\nline3");
}

TEST_F(TextBufferSelectionUndoTest, UndoRedoWithMultipleCursors) {
    // Clear and set up test text
    buffer->clear(true);
    buffer->insertText("word word word");
    
    // Simulate multiple cursors at each space
    buffer->beginUndoGroup();
    buffer->moveCursorTo(4);
    buffer->insertText("X");
    buffer->moveCursorTo(10);
    buffer->insertText("Y");
    buffer->endUndoGroup();
    
    // Undo should remove both insertions
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "word word word");
    
    // Redo should restore both insertions
    buffer->redo();
    EXPECT_EQ(buffer->getText(), "wordX wordY word");
}

TEST_F(TextBufferSelectionUndoTest, UndoRedoWithSelectionModification) {
    // Test that modifying a selection and undoing preserves the selection
    buffer->setSelection(4, 9);  // Select "quick"
    buffer->replaceSelectedText("fast");
    
    // Undo should restore "quick" and the selection
    buffer->undo();
    EXPECT_EQ(buffer->getText(), "The quick brown fox jumps over the lazy dog");
    
    auto [start, end] = buffer->getSelection();
    EXPECT_EQ(start, 4);
    EXPECT_EQ(end, 9);
    EXPECT_EQ(buffer->getSelectedText(), "quick");
}
