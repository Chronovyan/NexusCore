#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>
#include <iostream>

// CopyCommand test fixture
class CopyCommandTest : public test_utils::ClipboardOperationsTestBase {};

// Test copying a simple text selection within a single line
TEST_F(CopyCommandTest, CopySingleLineSelection) {
    // Setup buffer and selection
    setBufferContent("Line one for copy.");
    positionCursor(0, 5, true, 0, 5, 0, 8); // Select "one"
    
    // Execute the command
    auto copyCmd = std::make_unique<CopyCommand>();
    copyCmd->execute(editor);
    
    // Verify state after execution
    verifyClipboard("one");
    verifyBufferContent({"Line one for copy."});
    
    // Test undo restores previous clipboard content
    editor.setClipboardText("SomethingElse");
    copyCmd->undo(editor);
    verifyClipboard("");
}

// Test copying a multi-line selection
TEST_F(CopyCommandTest, CopyMultiLineSelection) {
    // Setup buffer and selection
    setBufferLines({
        "First line of multi-copy",
        "Second line"
    });
    
    // Select both lines
    positionCursor(0, 0, true, 0, 0, 1, editor.getBuffer().getLine(1).length());
    
    // Expected clipboard content with newline
    std::string expectedClipboard = "First line of multi-copy\nSecond line";
    
    // Execute the command
    auto copyCmd = std::make_unique<CopyCommand>();
    copyCmd->execute(editor);
    
    // Verify state after execution
    verifyClipboard(expectedClipboard);
    verifyBufferContent({
        "First line of multi-copy",
        "Second line"
    });
}

// PasteCommand test fixture
class PasteCommandTest : public test_utils::ClipboardOperationsTestBase {};

// Test pasting text within a single line
TEST_F(PasteCommandTest, PasteSingleLineInMiddle) {
    // Setup buffer and cursor
    setBufferContent("Line two, paste here.");
    positionCursor(0, 10); // Position cursor after "Line two, "
    
    // Set clipboard content and execute command
    editor.setClipboardText("one");
    auto pasteCmd = std::make_unique<PasteCommand>();
    pasteCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({"Line two, onepaste here."});
    verifyCursorPosition(0, 13);
    
    // Test undo removes the pasted text
    pasteCmd->undo(editor);
    verifyBufferContent({"Line two, paste here."});
    verifyCursorPosition(0, 10);
}

// Test pasting multi-line text
TEST_F(PasteCommandTest, PasteMultiLine) {
    // Setup buffer and cursor
    setBufferContent("Third line for pasting");
    positionCursor(0, 6); // Position cursor after "Third "
    
    // Set multi-line clipboard content and execute command
    std::string multiLineText = "First line of multi-copy\nSecond line";
    editor.setClipboardText(multiLineText);
    auto pasteCmd = std::make_unique<PasteCommand>();
    pasteCmd->execute(editor);
    
    // Verify state after execution
    verifyBufferContent({
        "Third First line of multi-copy",
        "Second lineline for pasting"
    });
    verifyCursorPosition(1, 11);
    
    // Test undo removes the pasted text
    pasteCmd->undo(editor);
    verifyBufferContent({"Third line for pasting"});
    verifyCursorPosition(0, 6);
}

// CutCommand test fixture
class CutCommandTest : public test_utils::ClipboardOperationsTestBase {};

// Test cutting text from a single line
TEST_F(CutCommandTest, CutSingleLineSelection) {
    // Setup buffer and selection
    setBufferContent("Cut this part out.");
    positionCursor(0, 4, true, 0, 4, 0, 9); // Select "this "
    
    // Execute the command
    auto cutCmd = std::make_unique<CutCommand>();
    cutCmd->execute(editor);
    
    // Verify state after execution
    verifyClipboard("this ");
    verifyBufferContent({"Cut part out."});
    verifyCursorPosition(0, 4);
    
    // Test undo restores the cut text and selection
    cutCmd->undo(editor);
    verifyClipboard("");
    verifyBufferContent({"Cut this part out."});
    verifyCursorPosition(0, 9);
}

// Test cutting multi-line text
TEST_F(CutCommandTest, CutMultiLineSelection) {
    // Setup buffer and selection
    setBufferLines({
        "First line to cut from",
        "Second line entirely cut",
        "Third line, cut some too",
        "Fourth line stays"
    });
    
    // Select "line to cut from\nSecond line entirely cut\nThird "
    positionCursor(0, 6, true, 0, 6, 2, 6);
    std::string expectedCutText = "line to cut from\nSecond line entirely cut\nThird ";
    
    // Execute the command
    auto cutCmd = std::make_unique<CutCommand>();
    cutCmd->execute(editor);
    
    // Verify state after execution
    verifyClipboard(expectedCutText);
    verifyBufferContent({
        "First line, cut some too",
        "Fourth line stays"
    });
    verifyCursorPosition(0, 6);
    
    // Test undo restores the cut text and selection
    cutCmd->undo(editor);
    verifyClipboard("");
    verifyBufferContent({
        "First line to cut from",
        "Second line entirely cut",
        "Third line, cut some too",
        "Fourth line stays"
    });
    verifyCursorPosition(2, 6);
} 