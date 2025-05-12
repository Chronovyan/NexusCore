#ifndef TEST_DECLARATIONS_H
#define TEST_DECLARATIONS_H

#include "TestFramework.h"

// Command Manager tests (CommandManagerTests.cpp)
TestResult testCommandManager_ExecuteUndoRedo();
TestResult testCommandManager_RedoStackClearing();

// Comprehensive Undo/Redo tests (ComprehensiveUndoRedoTest.cpp)
TestResult testUndoRedoBasicAddLines();
TestResult testUndoRedoTextInsertion();
TestResult testUndoRedoDeletion();
TestResult testUndoRedoNewLine();
TestResult testRedoStackClearing();
TestResult testComplexUndoRedoSequence();

// Selection and clipboard tests (SelectionClipboardTest.cpp)
TestResult testBasicSelection();
TestResult testClipboardOperations();
TestResult testMultiLineSelection();
TestResult testWordSelection();
TestResult testDeleteWord();

// Exit tests (ExitTest.cpp)
TestResult testExitCommands();
TestResult testExitWithUnsavedChanges();

#endif // TEST_DECLARATIONS_H 