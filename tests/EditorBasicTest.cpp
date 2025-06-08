#include "gtest/gtest.h"
#include "../src/Editor.h"
#include <string>
#include <vector>
#include <memory>
#include "../src/TextBuffer.h"

class EditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        editor = std::make_unique<Editor>();
        // Ensure we start with a clean state
        editor->selectAll();
        editor->deleteSelection();
    }
    
    std::unique_ptr<Editor> editor;
};

TEST_F(EditorTest, InitialState) {
    // Verify initial state of the editor
    EXPECT_EQ(editor->getCursorLine(), 0);
    EXPECT_EQ(editor->getCursorCol(), 0);
    EXPECT_FALSE(editor->hasSelection());
    // Get buffer through getBuffer() and check line count
    const auto& buffer = editor->getBuffer();
    EXPECT_EQ(buffer.getLineCount(), 1);
    EXPECT_TRUE(editor->getCurrentLineText().empty());
    (void)buffer; // Suppress unused variable warning
}

TEST_F(EditorTest, InsertTextBasic) {
    // Test inserting text
    editor->typeText("Hello, World!");
    
    // Verify cursor position after insertion
    EXPECT_EQ(editor->getCursorLine(), 0);
    EXPECT_EQ(editor->getCursorCol(), 13);
    
    // Verify the text was inserted
    EXPECT_EQ(editor->getCurrentLineText(), "Hello, World!");
    const auto& buffer = editor->getBuffer();
    EXPECT_EQ(buffer.getLineCount(), 1);
    (void)buffer; // Suppress unused variable warning
}

TEST_F(EditorTest, InsertTextWithNewlines) {
    // Test inserting text with newlines
    editor->typeText("Line 1\nLine 2\nLine 3");
    
    // Verify cursor position
    EXPECT_EQ(editor->getCursorLine(), 2);
    EXPECT_EQ(editor->getCursorCol(), 6); // After "Line 3"
    
    // Verify content of each line
    editor->setCursor(0, 0);
    EXPECT_EQ(editor->getCurrentLineText(), "Line 1");
    editor->moveCursorDown();
    EXPECT_EQ(editor->getCurrentLineText(), "Line 2");
    editor->moveCursorDown();
    EXPECT_EQ(editor->getCurrentLineText(), "Line 3");
    
    // Verify line count through the buffer
    const auto& buffer = editor->getBuffer();
    EXPECT_EQ(buffer.getLineCount(), 3);
    (void)buffer; // Suppress unused variable warning
}

TEST_F(EditorTest, CursorMovement) {
    // Set up multi-line content
    editor->typeText("First line\nSecond line\nThird line");
    
    // Move to start of document
    editor->moveCursorToBufferStart();
    EXPECT_EQ(editor->getCursorLine(), 0);
    EXPECT_EQ(editor->getCursorCol(), 0);
    
    // Move to end of document
    editor->moveCursorToBufferEnd();
    EXPECT_EQ(editor->getCursorLine(), 2);
    EXPECT_EQ(editor->getCursorCol(), 10); // After "Third line"
    
    // Move up and verify position
    editor->moveCursorUp();
    EXPECT_EQ(editor->getCursorLine(), 1);
    EXPECT_EQ(editor->getCursorCol(), 10); // Cursor should stay at column 10
    
    // Move to start of line
    editor->moveCursorToLineStart();
    EXPECT_EQ(editor->getCursorCol(), 0);
    
    // Move to end of line
    editor->moveCursorToLineEnd();
    EXPECT_EQ(editor->getCursorCol(), 11); // "Second line" has 11 characters
    
    // Move right (should not go beyond line end)
    editor->moveCursorRight();
    EXPECT_EQ(editor->getCursorCol(), 11); // No change
    
    // Move left
    editor->moveCursorLeft();
    EXPECT_EQ(editor->getCursorCol(), 10);
}

TEST_F(EditorTest, DeleteOperations) {
    // Set up test content
    editor->typeText("Testing delete operations");
    
    // Move cursor to before the last word
    for (int i = 0; i < 10; ++i) {
        editor->moveCursorLeft();
    }
    
    // Delete forward (delete 'operati')
    for (int i = 0; i < 6; ++i) {
        editor->deleteForward();
    }
    
    EXPECT_EQ(editor->getCurrentLineText(), "Testing delete ons");
    
    // Move to start and delete first character
    editor->moveCursorToLineStart();
    editor->deleteForward();
    EXPECT_EQ(editor->getCurrentLineText(), "esting delete ons");
    
    // Delete last character
    editor->moveCursorToLineEnd();
    editor->deleteCharacter();
    std::string line = editor->getCurrentLineText();
    EXPECT_EQ(line, "esting delete on");
}

TEST_F(EditorTest, UndoRedo) {
    // Test undo/redo
    editor->typeText("First");
    editor->typeText(" ");
    editor->typeText("line");
    
    // Undo last insertion
    editor->undo();
    std::string line = editor->getCurrentLineText();
    EXPECT_EQ(line, "First ");
    
    // Undo again
    editor->undo();
    line = editor->getCurrentLineText();
    EXPECT_EQ(line, "First");
    
    // Redo
    editor->redo();
    line = editor->getCurrentLineText();
    EXPECT_EQ(line, "First ");
    
    // Redo again
    editor->redo();
    line = editor->getCurrentLineText();
    EXPECT_EQ(line, "First line");
    
    // No more redos
    EXPECT_FALSE(editor->canRedo());
}

TEST_F(EditorTest, Selection) {
    // Set up test content
    editor->typeText("This is a test string");
    
    // Select first 10 characters
    editor->setCursor(0, 0);
    editor->setSelectionStart();
    editor->setCursor(0, 10);
    editor->setSelectionEnd();
    
    // Verify selection
    EXPECT_TRUE(editor->hasSelection());
    
    // Get selected text
    std::string selectedText = editor->getSelectedText();
    EXPECT_EQ(selectedText, "This is a ");
    
    // Clear selection
    editor->clearSelection();
    EXPECT_FALSE(editor->hasSelection());
}

TEST_F(EditorTest, CopyPaste) {
    // Test copy and paste
    editor->typeText("Copy this text");
    editor->moveCursorToBufferStart();
    
    // Select and copy first word
    editor->setCursor(0, 0);
    editor->setSelectionStart();
    editor->setCursor(0, 4);  // Select "Copy"
    editor->setSelectionEnd();
    editor->copySelection();
    
    // Paste at start
    editor->setCursor(0, 0);
    editor->pasteAtCursor();
    std::string line = editor->getCurrentLineText();
    EXPECT_EQ(line, "CopyCopy this text");
    
    // Cut and paste
    editor->setCursor(0, 0);
    // Select first word
    editor->setSelectionStart();
    editor->setCursor(0, 4);  // Select first "Copy"
    editor->setSelectionEnd();
    editor->cutSelection();
    line = editor->getCurrentLineText();
    EXPECT_EQ(line, "Copy this text");
    
    // Paste at start
    editor->setCursor(0, 0);
    editor->pasteAtCursor();
    line = editor->getCurrentLineText();
    EXPECT_EQ(line, "CopyCopy this text");
}

TEST_F(EditorTest, EdgeCases) {
    // Test empty buffer
    editor->selectAll();
    editor->deleteSelection();
    const auto& buffer = editor->getBuffer();
    EXPECT_EQ(buffer.getLineCount(), 1);
    std::string emptyLine = editor->getCurrentLineText();
    EXPECT_TRUE(emptyLine.empty());
    
    // Test cursor movement in empty buffer
    editor->moveCursorRight();
    editor->moveCursorLeft();
    editor->moveCursorUp();
    editor->moveCursorDown();
    
    // Test delete operations on empty buffer
    editor->deleteCharacter();
    editor->deleteForward();
    
    // Test undo/redo with no history
    EXPECT_FALSE(editor->canUndo());
    EXPECT_FALSE(editor->canRedo());
    editor->undo(); // Should be safe to call
    editor->redo(); // Should be safe to call
    
    // Test selection in empty buffer
    editor->setSelectionStart();
    editor->setSelectionEnd();
    EXPECT_FALSE(editor->hasSelection());
    editor->clearSelection();
}

TEST_F(EditorTest, MultiLineOperations) {
    // Set up multi-line content
    editor->typeText("Line 1\nLine 2\nLine 3");
    
    // Move to start of second line
    editor->moveCursorUp();
    editor->moveCursorToLineStart();
    
    // Insert new line above
    editor->newLine();
    const auto& buffer = editor->getBuffer();
    EXPECT_EQ(buffer.getLineCount(), 4);
    EXPECT_EQ(editor->getCursorLine(), 1);
    
    // Move to the empty line and get its content
    editor->setCursor(1, 0);
    std::string line = editor->getCurrentLineText();
    EXPECT_TRUE(line.empty());
    
    // Delete current line - first move to the line we want to delete
    editor->setCursor(2, 0);
    editor->deleteLine(2);
    EXPECT_EQ(buffer.getLineCount(), 3);
    
    // Verify content by moving to each line and checking
    editor->setCursor(0, 0);
    EXPECT_EQ(editor->getCurrentLineText(), "Line 1");
    editor->moveCursorDown();
    EXPECT_EQ(editor->getCurrentLineText(), "");
    editor->moveCursorDown();
    EXPECT_EQ(editor->getCurrentLineText(), "Line 2");
    
    // Join lines - move to first line and join with next
    editor->setCursor(0, 0);
    editor->moveCursorToLineEnd();
    editor->joinWithNextLine();
    EXPECT_EQ(buffer.getLineCount(), 2);
    
    // Verify joined line content
    editor->setCursor(0, 0);
    std::string joinedLine = editor->getCurrentLineText();
    EXPECT_EQ(joinedLine, "Line 1");
    editor->moveCursorDown();
    EXPECT_EQ(editor->getCurrentLineText(), "Line 2");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
