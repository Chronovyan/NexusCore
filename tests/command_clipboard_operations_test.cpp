#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include <memory>
#include <string>
#include <iostream>

// Base fixture for clipboard operation tests
class ClipboardOperationsTestBase : public ::testing::Test {
protected:
    TestEditor editor;
    
    void SetUp() override {
        editor.getBuffer().clear(false); // Clear without adding an empty line
        // Save and clear clipboard at the beginning of each test
        originalClipboard_ = editor.getClipboardText();
        editor.setClipboardText("");
    }
    
    void TearDown() override {
        // Restore original clipboard content after each test
        editor.setClipboardText(originalClipboard_);
    }
    
private:
    std::string originalClipboard_;
};

// CopyCommand test fixture
class CopyCommandTest : public ClipboardOperationsTestBase {};

// Test copying a simple text selection within a single line
TEST_F(CopyCommandTest, CopySingleLineSelection) {
    // Setup buffer and selection
    editor.getBuffer().addLine("Line one for copy.");
    editor.setCursor(0, 5);
    editor.setSelectionRange(0, 5, 0, 8); // Select "one"
    
    // Execute the command
    auto copyCmd = std::make_unique<CopyCommand>();
    copyCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ("one", editor.getClipboardText()) << "Clipboard should contain selected text";
    ASSERT_EQ("Line one for copy.", editor.getBuffer().getLine(0)) << "Buffer should remain unchanged";
    
    // Test undo restores previous clipboard content
    editor.setClipboardText("SomethingElse");
    copyCmd->undo(editor);
    ASSERT_EQ("", editor.getClipboardText()) << "Undo should restore original clipboard content";
}

// Test copying a multi-line selection
TEST_F(CopyCommandTest, CopyMultiLineSelection) {
    // Setup buffer and selection
    editor.getBuffer().addLine("First line of multi-copy");
    editor.getBuffer().addLine("Second line");
    editor.setCursor(0, 0);
    editor.setSelectionRange(0, 0, 1, editor.getBuffer().getLine(1).length()); // Select both lines
    
    // Expected clipboard content with newline
    std::string expectedClipboard = "First line of multi-copy\nSecond line";
    
    // Execute the command
    auto copyCmd = std::make_unique<CopyCommand>();
    copyCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(expectedClipboard, editor.getClipboardText()) << "Clipboard should contain multi-line selection";
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should remain unchanged";
    ASSERT_EQ("First line of multi-copy", editor.getBuffer().getLine(0)) << "First line should remain unchanged";
    ASSERT_EQ("Second line", editor.getBuffer().getLine(1)) << "Second line should remain unchanged";
}

// PasteCommand test fixture
class PasteCommandTest : public ClipboardOperationsTestBase {};

// Test pasting text within a single line
TEST_F(PasteCommandTest, PasteSingleLineInMiddle) {
    // Setup buffer and cursor
    editor.getBuffer().addLine("Line two, paste here.");
    editor.setCursor(0, 10); // Position cursor after "Line two, "
    
    // Set clipboard content and execute command
    editor.setClipboardText("one");
    auto pasteCmd = std::make_unique<PasteCommand>();
    pasteCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ("Line two, onepaste here.", editor.getBuffer().getLine(0)) << "Line should contain pasted text";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should remain on the same line";
    ASSERT_EQ(13, editor.getCursorCol()) << "Cursor should be after the pasted text";
    
    // Test undo removes the pasted text
    pasteCmd->undo(editor);
    ASSERT_EQ("Line two, paste here.", editor.getBuffer().getLine(0)) << "Undo should restore original line";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be restored";
    ASSERT_EQ(10, editor.getCursorCol()) << "Cursor column should be restored";
}

// Test pasting multi-line text
TEST_F(PasteCommandTest, PasteMultiLine) {
    // Setup buffer and cursor
    editor.getBuffer().addLine("Third line for pasting");
    editor.setCursor(0, 6); // Position cursor after "Third "
    
    // Set multi-line clipboard content and execute command
    std::string multiLineText = "First line of multi-copy\nSecond line";
    editor.setClipboardText(multiLineText);
    auto pasteCmd = std::make_unique<PasteCommand>();
    pasteCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have two lines after multi-line paste";
    ASSERT_EQ("Third First line of multi-copy", editor.getBuffer().getLine(0)) << "First line should be merged with first part of paste";
    ASSERT_EQ("Second lineline for pasting", editor.getBuffer().getLine(1)) << "Second line should be merged with remainder of original line";
    ASSERT_EQ(1, editor.getCursorLine()) << "Cursor should be on the last pasted line";
    ASSERT_EQ(11, editor.getCursorCol()) << "Cursor should be at the end of the pasted content";
    
    // Test undo removes the pasted text
    pasteCmd->undo(editor);
    ASSERT_EQ(1, editor.getBuffer().lineCount()) << "Buffer should have one line after undo";
    ASSERT_EQ("Third line for pasting", editor.getBuffer().getLine(0)) << "Original line should be restored";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be restored";
    ASSERT_EQ(6, editor.getCursorCol()) << "Cursor column should be restored";
}

// CutCommand test fixture
class CutCommandTest : public ClipboardOperationsTestBase {};

// Test cutting text from a single line
TEST_F(CutCommandTest, CutSingleLineSelection) {
    // Setup buffer and selection
    editor.getBuffer().addLine("Cut this part out.");
    editor.setCursor(0, 4);
    editor.setSelectionRange(0, 4, 0, 9); // Select "this "
    
    // Execute the command
    auto cutCmd = std::make_unique<CutCommand>();
    cutCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ("this ", editor.getClipboardText()) << "Clipboard should contain cut text";
    ASSERT_EQ("Cut part out.", editor.getBuffer().getLine(0)) << "Line should have text removed";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should remain on the same line";
    ASSERT_EQ(4, editor.getCursorCol()) << "Cursor should be at cut position";
    
    // Test undo restores the cut text and selection
    cutCmd->undo(editor);
    ASSERT_EQ("", editor.getClipboardText()) << "Clipboard should be restored to original state";
    ASSERT_EQ("Cut this part out.", editor.getBuffer().getLine(0)) << "Original text should be restored";
    // The cursor should be at the end of the restored selection
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor line should be at end of restored selection";
    ASSERT_EQ(9, editor.getCursorCol()) << "Cursor column should be at end of restored selection";
}

// Test cutting multi-line text
TEST_F(CutCommandTest, CutMultiLineSelection) {
    // Setup buffer and selection
    editor.getBuffer().addLine("First line to cut from");
    editor.getBuffer().addLine("Second line entirely cut");
    editor.getBuffer().addLine("Third line, cut some too");
    editor.getBuffer().addLine("Fourth line stays");
    
    editor.setCursor(0, 6);
    editor.setSelectionRange(0, 6, 2, 6); // Select "line to cut from\nSecond line entirely cut\nThird "
    std::string expectedCutText = "line to cut from\nSecond line entirely cut\nThird ";
    
    // Execute the command
    auto cutCmd = std::make_unique<CutCommand>();
    cutCmd->execute(editor);
    
    // Verify state after execution
    ASSERT_EQ(expectedCutText, editor.getClipboardText()) << "Clipboard should contain multi-line cut text";
    ASSERT_EQ(2, editor.getBuffer().lineCount()) << "Buffer should have two lines after multi-line cut";
    ASSERT_EQ("First line, cut some too", editor.getBuffer().getLine(0)) << "First line should be merged with remainder of third line";
    ASSERT_EQ("Fourth line stays", editor.getBuffer().getLine(1)) << "Fourth line should remain unchanged";
    ASSERT_EQ(0, editor.getCursorLine()) << "Cursor should be on the first line";
    ASSERT_EQ(6, editor.getCursorCol()) << "Cursor should be at cut position";
    
    // Test undo restores the cut text and selection
    cutCmd->undo(editor);
    ASSERT_EQ("", editor.getClipboardText()) << "Clipboard should be restored to original state";
    ASSERT_EQ(4, editor.getBuffer().lineCount()) << "Buffer should have four lines after undo";
    ASSERT_EQ("First line to cut from", editor.getBuffer().getLine(0)) << "First line should be restored";
    ASSERT_EQ("Second line entirely cut", editor.getBuffer().getLine(1)) << "Second line should be restored";
    ASSERT_EQ("Third line, cut some too", editor.getBuffer().getLine(2)) << "Third line should be restored";
    ASSERT_EQ("Fourth line stays", editor.getBuffer().getLine(3)) << "Fourth line should remain unchanged";
    // The cursor should be at the end of the restored selection
    ASSERT_EQ(2, editor.getCursorLine()) << "Cursor line should be at end of restored selection";
    ASSERT_EQ(6, editor.getCursorCol()) << "Cursor column should be at end of restored selection";
} 