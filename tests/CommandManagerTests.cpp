#include <gtest/gtest.h>
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "../src/CommandManager.h"
#include <vector>
#include <string>

// Test basic execute, undo, redo with CommandManager
TEST(CommandManagerTest, ExecuteUndoRedo) {
    TestEditor editor;
    CommandManager cmdManager;

    editor.getBuffer().clear(false);  // Clear without adding an empty line
    editor.getBuffer().addLine("Initial state.");
    editor.setCursor(0, 0);

    // 1. Execute an InsertTextCommand
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("Test "), editor);
    // Expected: "Test Initial state.", cursor [0,5]
    ASSERT_EQ(editor.getBuffer().getLine(0), "Test Initial state.");
    ASSERT_EQ(editor.getCursorLine(), 0);
    ASSERT_EQ(editor.getCursorCol(), 5);

    // 2. Execute a NewLineCommand instead of AddLineCommand for splitting
    // Cursor is at [0,5] ("Test |Initial state.")
    cmdManager.executeCommand(std::make_unique<NewLineCommand>(), editor);
    // Expected: L0: "Test ", L1: "Initial state.", cursor [1,0]
    ASSERT_EQ(editor.getBuffer().lineCount(), 2);
    ASSERT_EQ(editor.getBuffer().getLine(0), "Test ");
    ASSERT_EQ(editor.getBuffer().getLine(1), "Initial state.");
    ASSERT_EQ(editor.getCursorLine(), 1);
    ASSERT_EQ(editor.getCursorCol(), 0);

    // 3. Undo NewLineCommand
    cmdManager.undo(editor);
    // Expected: "Test Initial state.", cursor [0,5] (state after InsertText)
    ASSERT_EQ(editor.getBuffer().lineCount(), 1);
    ASSERT_EQ(editor.getBuffer().getLine(0), "Test Initial state.");
    ASSERT_EQ(editor.getCursorLine(), 0);
    ASSERT_EQ(editor.getCursorCol(), 5);

    // 4. Undo InsertTextCommand
    cmdManager.undo(editor);
    // Expected: "Initial state.", cursor [0,0]
    ASSERT_EQ(editor.getBuffer().getLine(0), "Initial state.");
    ASSERT_EQ(editor.getCursorLine(), 0);
    ASSERT_EQ(editor.getCursorCol(), 0);

    // 5. Redo InsertTextCommand
    cmdManager.redo(editor);
    // Expected: "Test Initial state.", cursor [0,5]
    ASSERT_EQ(editor.getBuffer().getLine(0), "Test Initial state.");
    ASSERT_EQ(editor.getCursorLine(), 0);
    ASSERT_EQ(editor.getCursorCol(), 5);

    // 6. Redo NewLineCommand
    cmdManager.redo(editor);
    // Expected: L0: "Test ", L1: "Initial state.", cursor [1,0]
    ASSERT_EQ(editor.getBuffer().lineCount(), 2);
    ASSERT_EQ(editor.getBuffer().getLine(0), "Test ");
    ASSERT_EQ(editor.getBuffer().getLine(1), "Initial state.");
    ASSERT_EQ(editor.getCursorLine(), 1);
    ASSERT_EQ(editor.getCursorCol(), 0);
}

// Test that redo stack is cleared after a new command is executed
TEST(CommandManagerTest, RedoStackClearing) {
    TestEditor editor;
    CommandManager cmdManager;

    editor.getBuffer().clear(false);  // Clear without adding an empty line
    editor.getBuffer().addLine("Content.");
    editor.setCursor(0,0);

    // Cmd 1
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("A"), editor); // "AContent." C:[0,1]
    // Cmd 2
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("B"), editor); // "ABContent." C:[0,2]

    cmdManager.undo(editor); // Undo Cmd2. State: "AContent." C:[0,1]. Cmd2 on redo stack.
    ASSERT_EQ(editor.getBuffer().getLine(0), "AContent.");
    ASSERT_TRUE(cmdManager.canRedo());

    // Cmd 3 (New command)
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("C"), editor); // "ACContent." C:[0,2]
                                                                                // Redo stack (Cmd2) should be cleared.
    
    ASSERT_EQ(editor.getBuffer().getLine(0), "ACContent.");
    ASSERT_FALSE(cmdManager.canRedo());

    // Try to redo - should do nothing
    cmdManager.redo(editor);
    ASSERT_EQ(editor.getBuffer().getLine(0), "ACContent.");
    ASSERT_EQ(editor.getCursorCol(), 2);

    // Undo Cmd3
    cmdManager.undo(editor); // "AContent." C:[0,1]
    ASSERT_EQ(editor.getBuffer().getLine(0), "AContent.");
    ASSERT_EQ(editor.getCursorCol(), 1);
    ASSERT_TRUE(cmdManager.canRedo());
}