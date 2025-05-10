#include "TestFramework.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "../src/CommandManager.h"
#include <vector>
#include <string>

// Test basic execute, undo, redo with CommandManager
TestResult testCommandManager_ExecuteUndoRedo() {
    TestEditor editor;
    CommandManager cmdManager;
    std::string msg;

    editor.getBuffer().clear(false);  // Clear without adding an empty line
    editor.getBuffer().addLine("Initial state.");
    editor.setCursor(0, 0);

    // 1. Execute an InsertTextCommand
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("Test "), editor);
    // Expected: "Test Initial state.", cursor [0,5]
    if (editor.getBuffer().getLine(0) != "Test Initial state." || editor.getCursorLine() != 0 || editor.getCursorCol() != 5) {
        return TestResult(false, "CM Execute: InsertText failed. Got: L0:'" + editor.getBuffer().getLine(0) + "' Cursor:[" + std::to_string(editor.getCursorLine()) + "," + std::to_string(editor.getCursorCol()) + "]");
    }

    // 2. Execute a NewLineCommand instead of AddLineCommand for splitting
    // Cursor is at [0,5] ("Test |Initial state.")
    cmdManager.executeCommand(std::make_unique<NewLineCommand>(), editor);
    // Expected: L0: "Test ", L1: "Initial state.", cursor [1,0]
    if (editor.getBuffer().lineCount() != 2 || editor.getBuffer().getLine(0) != "Test " || editor.getBuffer().getLine(1) != "Initial state." || editor.getCursorLine() != 1 || editor.getCursorCol() != 0) {
        return TestResult(false, "CM Execute: NewLine failed. Got: L0:'" + editor.getBuffer().getLine(0) + "' L1:'" + 
            (editor.getBuffer().lineCount() > 1 ? editor.getBuffer().getLine(1) : "<NO LINE>") + 
            "' Cursor:[" + std::to_string(editor.getCursorLine()) + "," + std::to_string(editor.getCursorCol()) + "]");
    }

    // 3. Undo NewLineCommand
    cmdManager.undo(editor);
    // Expected: "Test Initial state.", cursor [0,5] (state after InsertText)
    if (editor.getBuffer().lineCount() != 1 || editor.getBuffer().getLine(0) != "Test Initial state." || editor.getCursorLine() != 0 || editor.getCursorCol() != 5) {
        return TestResult(false, "CM Undo (NewLine): Failed. Got: L0:'" + editor.getBuffer().getLine(0) + "' Cursor:[" + std::to_string(editor.getCursorLine()) + "," + std::to_string(editor.getCursorCol()) + "]");
    }

    // 4. Undo InsertTextCommand
    cmdManager.undo(editor);
    // Expected: "Initial state.", cursor [0,0]
    if (editor.getBuffer().getLine(0) != "Initial state." || editor.getCursorLine() != 0 || editor.getCursorCol() != 0) {
        return TestResult(false, "CM Undo (InsertText): Failed. Got: L0:'" + editor.getBuffer().getLine(0) + "' Cursor:[" + std::to_string(editor.getCursorLine()) + "," + std::to_string(editor.getCursorCol()) + "]");
    }

    // 5. Redo InsertTextCommand
    cmdManager.redo(editor);
    // Expected: "Test Initial state.", cursor [0,5]
    if (editor.getBuffer().getLine(0) != "Test Initial state." || editor.getCursorLine() != 0 || editor.getCursorCol() != 5) {
        return TestResult(false, "CM Redo (InsertText): Failed.");
    }

    // 6. Redo NewLineCommand
    cmdManager.redo(editor);
    // Expected: L0: "Test ", L1: "Initial state.", cursor [1,0]
    if (editor.getBuffer().lineCount() != 2 || editor.getBuffer().getLine(0) != "Test " || editor.getBuffer().getLine(1) != "Initial state." || editor.getCursorLine() != 1 || editor.getCursorCol() != 0) {
        return TestResult(false, "CM Redo (NewLine): Failed.");
    }
    
    return TestResult(true, "");
}

// Test that redo stack is cleared after a new command is executed
TestResult testCommandManager_RedoStackClearing() {
    TestEditor editor;
    CommandManager cmdManager;
    std::string msg;

    editor.getBuffer().clear(false);  // Clear without adding an empty line
    editor.getBuffer().addLine("Content.");
    editor.setCursor(0,0);

    // Cmd 1
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("A"), editor); // "AContent." C:[0,1]
    // Cmd 2
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("B"), editor); // "ABContent." C:[0,2]

    cmdManager.undo(editor); // Undo Cmd2. State: "AContent." C:[0,1]. Cmd2 on redo stack.
    if (editor.getBuffer().getLine(0) != "AContent." || !cmdManager.canRedo()) {
         return TestResult(false, "CM RedoStack (Setup): Undo or canRedo failed.");
    }

    // Cmd 3 (New command)
    cmdManager.executeCommand(std::make_unique<InsertTextCommand>("C"), editor); // "ACContent." C:[0,2]
                                                                                // Redo stack (Cmd2) should be cleared.
    
    if (editor.getBuffer().getLine(0) != "ACContent.") {
        return TestResult(false, "CM RedoStack (Execute New): Buffer content incorrect.");
    }
    if (cmdManager.canRedo()) { // Redo stack should be empty
        return TestResult(false, "CM RedoStack: Redo stack not cleared after new command.");
    }

    // Try to redo - should do nothing
    cmdManager.redo(editor);
     if (editor.getBuffer().getLine(0) != "ACContent." || editor.getCursorCol() != 2) { // State should remain unchanged
        return TestResult(false, "CM RedoStack: Redo after stack clear changed state.");
    }

    // Undo Cmd3
    cmdManager.undo(editor); // "AContent." C:[0,1]
    if (editor.getBuffer().getLine(0) != "AContent." || editor.getCursorCol() != 1) {
        return TestResult(false, "CM RedoStack: Undo of Cmd3 failed.");
    }
     if (cmdManager.canRedo()) { // Cmd3 should be on redo stack
        //This check is good.
    } else {
        return TestResult(false, "CM RedoStack: Cmd3 not on redo stack after undo.");
    }


    return TestResult(true, "");
}


int main() {
    TestFramework tf;
    tf.registerTest("CommandManager Execute, Undo, Redo", testCommandManager_ExecuteUndoRedo);
    tf.registerTest("CommandManager Redo Stack Clearing", testCommandManager_RedoStackClearing);
    // Add other CommandManager tests here

    tf.runAllTests();
    return 0;
} 