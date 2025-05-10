#include "gtest/gtest.h"
#include "EditorCommands.h" // For InsertCommand, etc.
#include "TextBuffer.h"     // Commands operate on TextBuffer
#include "Editor.h"         // For Editor object
#include "EditorError.h"    // For any exceptions commands might throw (though less common for execute/undo)
#include <string>
#include <vector>

// Test fixture for Editor Command tests
class EditorCommandsTest : public ::testing::Test {
protected:
    Editor editor; // Each test gets a fresh Editor
    // TextBuffer& buffer; // Reference to the editor's buffer, if needed directly

    // EditorCommandsTest() : buffer(editor.getBuffer()) {} // Initialize reference if used

    // You can add common setup/teardown if needed
    // For example, CommandManager can be instantiated here if tests require it.
    // CommandManager cmdManager; 
};

// --- InsertArbitraryTextCommand Tests ---

TEST_F(EditorCommandsTest, InsertArbitraryTextExecuteAndUndo) {
    TextBuffer& buffer = editor.getBuffer(); // Get ref to buffer for convenience
    // Initial state: buffer has one empty line: [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");

    // 1. Insert "Hello" at (0,0)
    std::string textToInsert = "Hello";
    size_t lineIndex = 0;
    size_t colIndex = 0;
    
    InsertArbitraryTextCommand insertCmd(lineIndex, colIndex, textToInsert);
    
    // Execute
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.lineCount(), 1);

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), ""); // Should revert to original empty string
    EXPECT_EQ(buffer.lineCount(), 1);
}

TEST_F(EditorCommandsTest, InsertArbitraryTextInMiddleAndUndo) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Horld"); // Initial state: ["Horld"]
    
    std::string textToInsert = "ell";
    size_t lineIndex = 0;
    size_t colIndex = 1; // Insert after 'H'
    
    InsertArbitraryTextCommand insertCmd(lineIndex, colIndex, textToInsert);
    
    // Execute
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hellorld");

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Horld");
}

TEST_F(EditorCommandsTest, InsertArbitraryTextAtEndOfLineAndUndo) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello"); // Initial state: ["Hello"]
    
    std::string textToInsert = " World";
    size_t lineIndex = 0;
    size_t colIndex = 5; // Insert at the end of "Hello"
    
    InsertArbitraryTextCommand insertCmd(lineIndex, colIndex, textToInsert);
    
    // Execute
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello World");

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello");
}

TEST_F(EditorCommandsTest, InsertArbitraryTextIntoNewLineAndUndo) {
    TextBuffer& buffer = editor.getBuffer();
    // Starts with [""]
    buffer.addLine("Second Line"); // Buffer: ["", "Second Line"]
    ASSERT_EQ(buffer.lineCount(), 2);
    
    std::string textToInsert = "Content";
    size_t lineIndex = 1; 
    size_t colIndex = 0; 
    
    InsertArbitraryTextCommand insertCmd(lineIndex, colIndex, textToInsert);
    
    // Execute
    insertCmd.execute(editor);
    // This expectation needs to be TextBuffer::insertString behavior consistent
    // insertString("Content", 1, 0) on ["", "Second Line"] -> ["", "ContentSecond Line"]
    EXPECT_EQ(buffer.getLine(1), "ContentSecond Line");

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(1), "Second Line");
    EXPECT_EQ(buffer.lineCount(), 2); // Ensure line count is stable
    EXPECT_EQ(buffer.getLine(0), "");   // Ensure other lines are unaffected
}

TEST_F(EditorCommandsTest, InsertArbitraryTextEmptyStringAndUndo) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Text"); // Initial state: ["Text"]
    
    std::string textToInsert = "";
    size_t lineIndex = 0;
    size_t colIndex = 2; // Insert in "Te|xt"
    
    InsertArbitraryTextCommand insertCmd(lineIndex, colIndex, textToInsert);
    
    // Execute (inserting empty string should not change the line)
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Text");

    // Undo (should also result in no change)
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Text");
}

// --- DeleteCharCommand Tests ---

TEST_F(EditorCommandsTest, DeleteCharCommand_BackspaceInMiddle) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 3); // Cursor after 'l': He|llo -> Hel|lo

    DeleteCharCommand backspaceCmd(true); // isBackspace = true

    // Execute
    backspaceCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Helo");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 2); // Cursor moves left: He|lo

    // Undo
    backspaceCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 3); // Cursor restored
}

TEST_F(EditorCommandsTest, DeleteCharCommand_ForwardDeleteInMiddle) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 2); // Cursor before second 'l': He|llo

    DeleteCharCommand forwardDeleteCmd(false); // isBackspace = false

    // Execute
    forwardDeleteCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Helo");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 2); // Cursor stays: He|lo

    // Undo
    forwardDeleteCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 2); // Cursor restored
}

TEST_F(EditorCommandsTest, DeleteCharCommand_BackspaceAtLineStart_JoinsLines) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "First");
    buffer.addLine("Second"); // Buffer: ["First", "Second"]
    editor.setCursor(1, 0);    // Cursor at start of "Second"

    DeleteCharCommand backspaceCmd(true);

    // Execute
    backspaceCmd.execute(editor);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "FirstSecond");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor at end of "First"

    // Undo
    backspaceCmd.undo(editor);
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "First");
    EXPECT_EQ(buffer.getLine(1), "Second");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor restored
}

TEST_F(EditorCommandsTest, DeleteCharCommand_ForwardDeleteAtLineEnd_JoinsLines) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "First");
    buffer.addLine("Second"); // Buffer: ["First", "Second"]
    editor.setCursor(0, 5);    // Cursor at end of "First"

    DeleteCharCommand forwardDeleteCmd(false);

    // Execute
    forwardDeleteCmd.execute(editor);
    EXPECT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "FirstSecond");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor stays at end of "First"

    // Undo
    forwardDeleteCmd.undo(editor);
    EXPECT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "First");
    EXPECT_EQ(buffer.getLine(1), "Second");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor restored
}

TEST_F(EditorCommandsTest, DeleteCharCommand_BackspaceAtBufferStart) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 0); // Cursor at start of buffer

    DeleteCharCommand backspaceCmd(true);
    std::string originalLine = buffer.getLine(0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    // Execute (should do nothing)
    backspaceCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);

    // Undo (should also do nothing to the buffer state)
    backspaceCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, DeleteCharCommand_ForwardDeleteAtBufferEnd) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 5); // Cursor at end of buffer (single line)

    DeleteCharCommand forwardDeleteCmd(false);
    std::string originalLineText = buffer.getLine(0);
    size_t originalEditorCursorLine = editor.getCursorLine();
    size_t originalEditorCursorCol = editor.getCursorCol();

    // Execute (should do nothing)
    forwardDeleteCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), originalLineText);
    EXPECT_EQ(editor.getCursorLine(), originalEditorCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalEditorCursorCol);

    // Undo (should also do nothing)
    forwardDeleteCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), originalLineText);
    EXPECT_EQ(editor.getCursorLine(), originalEditorCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalEditorCursorCol);
}

// --- CutCommand Tests ---

TEST_F(EditorCommandsTest, CutCommand_CutsSelectedTextAndUpdatesClipboard) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello World");
    editor.setSelectionRange(0, 6, 0, 11); // Select "World"
    editor.setCursor(0, 11); // Cursor at end of selection for consistency with typical UX
    
    std::string initialClipboard = "InitialClipboardContent";
    editor.setClipboardText(initialClipboard);

    CutCommand cutCmd;

    // Store pre-cut state for thorough undo check
    size_t preCutCursorLine = editor.getCursorLine();
    size_t preCutCursorCol = editor.getCursorCol();
    size_t preCutSelStartLine = editor.getSelectionStartLine();
    size_t preCutSelStartCol = editor.getSelectionStartCol();
    size_t preCutSelEndLine = editor.getSelectionEndLine();
    size_t preCutSelEndCol = editor.getSelectionEndCol();
    bool preCutHasSelection = editor.hasSelection();

    // Execute Cut
    cutCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello ");
    EXPECT_EQ(editor.getClipboardText(), "World");
    EXPECT_FALSE(editor.hasSelection());
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 6); // Cursor should be where selection started

    // Undo Cut
    cutCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello World");
    EXPECT_EQ(editor.getClipboardText(), initialClipboard);
    EXPECT_EQ(editor.hasSelection(), preCutHasSelection);
    EXPECT_EQ(editor.getSelectionStartLine(), preCutSelStartLine);
    EXPECT_EQ(editor.getSelectionStartCol(), preCutSelStartCol);
    EXPECT_EQ(editor.getSelectionEndLine(), preCutSelEndLine);
    EXPECT_EQ(editor.getSelectionEndCol(), preCutSelEndCol);
    EXPECT_EQ(editor.getCursorLine(), preCutCursorLine); // Cursor should be restored to its pre-command state
    EXPECT_EQ(editor.getCursorCol(), preCutCursorCol);
}

TEST_F(EditorCommandsTest, CutCommand_NoSelection_DoesNothing) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello World");
    editor.clearSelection();
    editor.setCursor(0, 5); // Cursor somewhere in the middle

    std::string initialClipboard = "InitialClipboardContent";
    editor.setClipboardText(initialClipboard);
    std::string originalLine = buffer.getLine(0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    CutCommand cutCmd;

    // Execute Cut (no selection)
    cutCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getClipboardText(), initialClipboard);
    EXPECT_FALSE(editor.hasSelection());
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);

    // Undo Cut (should also do nothing)
    cutCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getClipboardText(), initialClipboard);
    EXPECT_FALSE(editor.hasSelection());
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

// --- PasteCommand Tests ---

TEST_F(EditorCommandsTest, PasteCommand_PastesSingleLineText) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello ");
    editor.setCursor(0, 6); // Cursor at end of "Hello "
    
    std::string textToPaste = "World!";
    editor.setClipboardText(textToPaste);

    PasteCommand pasteCmd;

    // Execute Paste
    pasteCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello World!");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 12); // Cursor at end of pasted text

    // Undo Paste
    pasteCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello ");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 6); // Cursor restored
}

TEST_F(EditorCommandsTest, PasteCommand_PastesMultiLineText) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line1");
    editor.setCursor(0, 5); // Cursor at end of "Line1"

    std::string textToPaste = "Multi\nLine\nText";
    editor.setClipboardText(textToPaste);

    PasteCommand pasteCmd;

    // Execute Paste
    pasteCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line1Multi");
    EXPECT_EQ(buffer.getLine(1), "Line");
    EXPECT_EQ(buffer.getLine(2), "Text");
    EXPECT_EQ(editor.getCursorLine(), 2);
    EXPECT_EQ(editor.getCursorCol(), 4); // Cursor at end of "Text"

    // Undo Paste
    pasteCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Line1");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor restored
}

TEST_F(EditorCommandsTest, PasteCommand_PasteEmptyClipboard_DoesNothing) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 2); // Cursor at H|ello
    
    editor.setClipboardText(""); // Empty clipboard
    std::string originalLine = buffer.getLine(0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    PasteCommand pasteCmd;

    // Execute Paste (empty clipboard)
    pasteCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);

    // Undo Paste (should also do nothing)
    pasteCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

// --- InsertTextCommand Tests ---

TEST_F(EditorCommandsTest, InsertTextCommand_InsertsTextAtCursor) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello orld");
    editor.setCursor(0, 6); // Cursor between " " and "o": Hello |orld

    InsertTextCommand insertCmd("W");

    // Execute
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello World");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 7); // Cursor after "W": Hello W|orld

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello orld");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 6); // Cursor restored
}

TEST_F(EditorCommandsTest, InsertTextCommand_InsertsMultiCharTextAtCursor) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Heo");
    editor.setCursor(0, 2); // Cursor after "e": He|o

    InsertTextCommand insertCmd("ll");

    // Execute
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 4); // Cursor after "ll": Hell|o

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), "Heo");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 2); // Cursor restored
}

TEST_F(EditorCommandsTest, InsertTextCommand_EmptyText_DoesNothing) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 2); // H|ello -> He|llo

    std::string originalLine = buffer.getLine(0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    InsertTextCommand insertCmd("");

    // Execute
    insertCmd.execute(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);

    // Undo
    insertCmd.undo(editor);
    EXPECT_EQ(buffer.getLine(0), originalLine);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

// --- NewLineCommand Tests ---

TEST_F(EditorCommandsTest, NewLineCommand_SplitsLineInMiddle) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "HelloWorld");
    editor.setCursor(0, 5); // Cursor between "Hello" and "World"

    NewLineCommand newLineCmd;

    // Execute
    newLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.getLine(1), "World");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor at start of "World"

    // Undo
    newLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "HelloWorld");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor restored
}

TEST_F(EditorCommandsTest, NewLineCommand_AtEndOfLine_AddsEmptyLineAfter) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 5); // Cursor at end of "Hello"

    NewLineCommand newLineCmd;

    // Execute
    newLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.getLine(1), "");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor at start of new empty line

    // Undo
    newLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor restored
}

TEST_F(EditorCommandsTest, NewLineCommand_AtStartOfLine_AddsEmptyLineBefore) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 0); // Cursor at start of "Hello"

    NewLineCommand newLineCmd;

    // Execute
    newLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor at start of "Hello" on the new line

    // Undo
    newLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor restored
}

TEST_F(EditorCommandsTest, NewLineCommand_OnDefaultEmptyBuffer_SplitsToTwoEmptyLines) {
    TextBuffer& buffer = editor.getBuffer(); // Starts with [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");
    editor.setCursor(0, 0);

    NewLineCommand newLineCmd;

    // Execute
    newLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    newLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 0);
}

// --- AddLineCommand Tests ---

// Tests for AddLineCommand() (split behavior)
TEST_F(EditorCommandsTest, AddLineCommand_Default_SplitsLineInMiddle) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "HelloWorld");
    editor.setCursor(0, 5); // Cursor between "Hello" and "World"

    AddLineCommand addLineCmd; // Default constructor

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.getLine(1), "World");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor at start of "World"

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "HelloWorld");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor restored
}

TEST_F(EditorCommandsTest, AddLineCommand_Default_AtEndOfLine_AddsEmptyLineAfter) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 5); // Cursor at end of "Hello"

    AddLineCommand addLineCmd; // Default constructor

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(buffer.getLine(1), "");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor at start of new empty line

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 5); // Cursor restored
}

TEST_F(EditorCommandsTest, AddLineCommand_Default_AtStartOfLine_AddsEmptyLineBefore) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Hello");
    editor.setCursor(0, 0); // Cursor at start of "Hello"

    AddLineCommand addLineCmd; // Default constructor

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor at start of "Hello" on the new line

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Hello");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 0); // Cursor restored
}

TEST_F(EditorCommandsTest, AddLineCommand_Default_OnDefaultEmptyBuffer_SplitsToTwoEmptyLines) {
    TextBuffer& buffer = editor.getBuffer(); // Starts with [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");
    editor.setCursor(0, 0);

    AddLineCommand addLineCmd; // Default constructor

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(buffer.getLine(1), "");
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 0);
}

// Tests for AddLineCommand(const std::string& text) (add to end behavior)
TEST_F(EditorCommandsTest, AddLineCommand_WithText_AddsLineToEnd) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1"); // Buffer: ["Line 0", "Line 1"]
    editor.setCursor(0, 2); // Cursor somewhere on Line 0
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    AddLineCommand addLineCmd("New Last Line");

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    EXPECT_EQ(buffer.getLine(2), "New Last Line");
    EXPECT_EQ(editor.getCursorLine(), 2); // Cursor at start of new line
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, AddLineCommand_WithText_ToEmptyBuffer) {
    TextBuffer& buffer = editor.getBuffer(); // Starts with [""]
    ASSERT_EQ(buffer.lineCount(), 1);
    ASSERT_EQ(buffer.getLine(0), "");
    editor.setCursor(0, 0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    AddLineCommand addLineCmd("First Line");

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), ""); // Original empty line remains
    EXPECT_EQ(buffer.getLine(1), "First Line");
    EXPECT_EQ(editor.getCursorLine(), 1); // Cursor at start of new line
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, AddLineCommand_WithEmptyText_AddsEmptyLineToEnd) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0"); // Buffer: ["Line 0"]
    editor.setCursor(0, 2); // Cursor somewhere on Line 0
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    AddLineCommand addLineCmd(""); // Add empty line

    // Execute
    addLineCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "");
    EXPECT_EQ(editor.getCursorLine(), 1); // Cursor at start of new empty line
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    addLineCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

// --- DeleteLineCommand Tests ---

TEST_F(EditorCommandsTest, DeleteLineCommand_DeletesMiddleLine) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1 (to delete)");
    buffer.addLine("Line 2"); // Buffer: ["Line 0", "Line 1 (to delete)", "Line 2"]
    editor.setCursor(0, 1); // Set original cursor
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    DeleteLineCommand deleteCmd(1); // Delete line at index 1

    // Execute
    deleteCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 2");
    EXPECT_EQ(editor.getCursorLine(), 1); // Cursor should be at start of the line that took the deleted one's place
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    deleteCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 1 (to delete)");
    EXPECT_EQ(buffer.getLine(2), "Line 2");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, DeleteLineCommand_DeletesFirstLine) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0 (to delete)");
    buffer.addLine("Line 1");
    buffer.addLine("Line 2");
    editor.setCursor(1, 1); // Set original cursor
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    DeleteLineCommand deleteCmd(0); // Delete line at index 0

    // Execute
    deleteCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Line 1");
    EXPECT_EQ(buffer.getLine(1), "Line 2");
    EXPECT_EQ(editor.getCursorLine(), 0); // Cursor should be at start of the new first line
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    deleteCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 0 (to delete)");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    EXPECT_EQ(buffer.getLine(2), "Line 2");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, DeleteLineCommand_DeletesLastLine) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1");
    buffer.addLine("Line 2 (to delete)");
    editor.setCursor(0, 1); // Set original cursor
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    DeleteLineCommand deleteCmd(2); // Delete line at index 2

    // Execute
    deleteCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 2);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    // Cursor logic: if lineIndex_ (2) >= buffer.lineCount() (now 2), set to (lineCount -1, 0) -> (1,0)
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    deleteCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 1");
    EXPECT_EQ(buffer.getLine(2), "Line 2 (to delete)");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, DeleteLineCommand_DeletesOnlyLine_LeavesOneEmptyLine) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Only Line (to delete)");
    editor.setCursor(0, 0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();

    DeleteLineCommand deleteCmd(0);

    // Execute
    deleteCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "");
    EXPECT_EQ(editor.getCursorLine(), 0); // Cursor remains at (0,0) on the new empty line
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    deleteCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Only Line (to delete)");
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor restored
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

TEST_F(EditorCommandsTest, DeleteLineCommand_OutOfBounds_DoesNothing) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1");
    editor.setCursor(0, 0);
    size_t originalCursorLine = editor.getCursorLine();
    size_t originalCursorCol = editor.getCursorCol();
    size_t originalLineCount = buffer.lineCount();
    std::string originalLine0 = buffer.getLine(0);
    std::string originalLine1 = buffer.getLine(1);

    DeleteLineCommand deleteCmd(5); // Index 5 is out of bounds for a 2-line buffer

    // Execute (should do nothing to buffer due to internal try-catch)
    deleteCmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), originalLineCount);
    EXPECT_EQ(buffer.getLine(0), originalLine0);
    EXPECT_EQ(buffer.getLine(1), originalLine1);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine); // Cursor should not change
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);

    // Undo (should also do nothing as execute didn't change state)
    deleteCmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), originalLineCount);
    EXPECT_EQ(buffer.getLine(0), originalLine0);
    EXPECT_EQ(buffer.getLine(1), originalLine1);
    EXPECT_EQ(editor.getCursorLine(), originalCursorLine);
    EXPECT_EQ(editor.getCursorCol(), originalCursorCol);
}

// --- ReplaceLineCommand Tests ---

TEST_F(EditorCommandsTest, ReplaceLineCommand_ReplacesMiddleLine) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1 Original");
    buffer.addLine("Line 2");
    // Buffer: ["Line 0", "Line 1 Original", "Line 2"]
    size_t lineIndexToReplace = 1;
    std::string newLineText = "Line 1 Replaced";

    // Save editor cursor state if we care about restoring it precisely for this command type
    // For ReplaceLine, current impl sets cursor to start of replaced line.
    // editor.setCursor(0,1); // e.g. place cursor somewhere else
    // size_t preOpCursorLine = editor.getCursorLine();
    // size_t preOpCursorCol = editor.getCursorCol();

    ReplaceLineCommand cmd(lineIndexToReplace, newLineText);

    // Execute
    cmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), newLineText);
    EXPECT_EQ(buffer.getLine(2), "Line 2");
    EXPECT_EQ(editor.getCursorLine(), lineIndexToReplace);
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    cmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(0), "Line 0");
    EXPECT_EQ(buffer.getLine(1), "Line 1 Original");
    EXPECT_EQ(buffer.getLine(2), "Line 2");
    EXPECT_EQ(editor.getCursorLine(), lineIndexToReplace);
    EXPECT_EQ(editor.getCursorCol(), 0);
    // If we were restoring preOpCursor: 
    // EXPECT_EQ(editor.getCursorLine(), preOpCursorLine);
    // EXPECT_EQ(editor.getCursorCol(), preOpCursorCol);
}

TEST_F(EditorCommandsTest, ReplaceLineCommand_ReplacesOnlyLine) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Original Only Line");
    size_t lineIndexToReplace = 0;
    std::string newLineText = "Replaced Only Line";

    ReplaceLineCommand cmd(lineIndexToReplace, newLineText);

    // Execute
    cmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), newLineText);
    EXPECT_EQ(editor.getCursorLine(), lineIndexToReplace);
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    cmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 1);
    EXPECT_EQ(buffer.getLine(0), "Original Only Line");
    EXPECT_EQ(editor.getCursorLine(), lineIndexToReplace);
    EXPECT_EQ(editor.getCursorCol(), 0);
}

TEST_F(EditorCommandsTest, ReplaceLineCommand_ReplaceWithEmptyString) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1 To Be Emptied");
    buffer.addLine("Line 2");
    size_t lineIndexToReplace = 1;
    std::string newLineText = "";

    ReplaceLineCommand cmd(lineIndexToReplace, newLineText);

    // Execute
    cmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(1), newLineText);
    EXPECT_EQ(editor.getCursorLine(), lineIndexToReplace);
    EXPECT_EQ(editor.getCursorCol(), 0);

    // Undo
    cmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), 3);
    EXPECT_EQ(buffer.getLine(1), "Line 1 To Be Emptied");
    EXPECT_EQ(editor.getCursorLine(), lineIndexToReplace);
    EXPECT_EQ(editor.getCursorCol(), 0);
}

TEST_F(EditorCommandsTest, ReplaceLineCommand_OutOfBounds_DoesNothing) {
    TextBuffer& buffer = editor.getBuffer();
    buffer.replaceLine(0, "Line 0");
    buffer.addLine("Line 1");
    // Buffer: ["Line 0", "Line 1"]
    size_t originalLineCount = buffer.lineCount();
    std::string originalLine0 = buffer.getLine(0);
    std::string originalLine1 = buffer.getLine(1);
    editor.setCursor(0,0); // Set some initial cursor
    size_t preOpCursorLine = editor.getCursorLine();
    size_t preOpCursorCol = editor.getCursorCol();

    ReplaceLineCommand cmd(5, "This should not appear"); // Index 5 is out of bounds

    // Execute
    cmd.execute(editor);
    ASSERT_EQ(buffer.lineCount(), originalLineCount);
    EXPECT_EQ(buffer.getLine(0), originalLine0);
    EXPECT_EQ(buffer.getLine(1), originalLine1);
    EXPECT_EQ(editor.getCursorLine(), preOpCursorLine); // Cursor should not change if no-op
    EXPECT_EQ(editor.getCursorCol(), preOpCursorCol);

    // Undo (should also do nothing)
    cmd.undo(editor);
    ASSERT_EQ(buffer.lineCount(), originalLineCount);
    EXPECT_EQ(buffer.getLine(0), originalLine0);
    EXPECT_EQ(buffer.getLine(1), originalLine1);
    EXPECT_EQ(editor.getCursorLine(), preOpCursorLine);
    EXPECT_EQ(editor.getCursorCol(), preOpCursorCol);
}

// TODO: Add tests for other commands:
// ... existing code ... 