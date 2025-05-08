#include <iostream>
#include "TestFramework.h"

// Forward declarations of test functions from BasicEditorTests.cpp
extern TestResult testBasicLineOperations();
extern TestResult testCursorMovement();
extern TestResult testTextEditing();
extern TestResult testSelectionOperations();

// Forward declarations of test functions from AdvancedEditorTests.cpp
extern TestResult testComplexEditingWithCheckpoints();
extern TestResult testLargeFileEditing();

// Forward declarations of test functions from FutureFeatureTests.cpp
extern TestResult testUndoRedo();
extern TestResult testSearch();
extern TestResult testReplace();
extern TestResult testSyntaxHighlighting();

// Forward declarations of test functions from UndoRedoTest.cpp
extern TestResult testBasicUndoRedo();
extern TestResult testTextEditingUndoRedo();
extern TestResult testLineOperationsUndoRedo();

// Main function to run all tests
int main() {
    TestFramework framework;
    
    // Register basic tests
    framework.registerTest("Basic Line Operations", testBasicLineOperations);
    framework.registerTest("Cursor Movement", testCursorMovement);
    framework.registerTest("Text Editing", testTextEditing);
    framework.registerTest("Selection Operations", testSelectionOperations);
    
    // Register advanced tests
    framework.registerTest("Complex Editing with Checkpoints", testComplexEditingWithCheckpoints);
    framework.registerTest("Large File Editing", testLargeFileEditing);
    
    // Register future feature tests
    framework.registerTest("Undo/Redo Operations (Future)", testUndoRedo);
    
    // Register detailed undo/redo tests
    framework.registerTest("Basic Undo/Redo Operations", testBasicUndoRedo);
    framework.registerTest("Text Editing Undo/Redo", testTextEditingUndoRedo);
    framework.registerTest("Line Operations Undo/Redo", testLineOperationsUndoRedo);
    
    std::cout << "======= Running All Tests for Coverage Analysis =======" << std::endl;
    
    // Run all tests
    framework.runAllTests();
    
    return 0;
} 