#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"
#include "TestFramework.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "../src/CommandManager.h"

// Test selection operations
TestResult testBasicSelection() {
    TestEditor editor;
    
    // Setup
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("The quick brown fox jumps over the lazy dog");
    editor.setCursor(0, 4); // Position at the start of "quick"
    
    // Test selection start
    editor.setSelectionStart();
    if (!editor.hasSelection() || editor.getSelectedText() != "") {
        return TestResult(false, "Selection start failed - empty selection should exist");
    }
    
    // Move cursor to create selection
    for (int i = 0; i < 5; i++) {
        editor.moveCursorRight();
    }
    editor.setSelectionEnd();
    
    // Verify selection
    if (!editor.hasSelection()) {
        return TestResult(false, "No selection after setSelectionEnd");
    }
    
    if (editor.getSelectedText() != "quick") {
        return TestResult(false, "Incorrect selection text. Expected: 'quick', Got: '" + 
                         editor.getSelectedText() + "'");
    }
    
    // Test selection clear
    editor.clearSelection();
    if (editor.hasSelection()) {
        return TestResult(false, "Selection not cleared after clearSelection");
    }
    
    return TestResult(true, "Basic selection test passed");
}

// Test clipboard operations (copy, cut, paste)
TestResult testClipboardOperations() {
    TestEditor editor;
    CommandManager cmdManager;
    
    // Setup
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("The quick brown fox");
    editor.setCursor(0, 4); // Position at the start of "quick"
    
    // Create selection
    editor.setSelectionStart();
    for (int i = 0; i < 5; i++) {
        editor.moveCursorRight();
    }
    editor.setSelectionEnd();
    
    // Test copy
    cmdManager.executeCommand(std::make_unique<CopyCommand>(), editor);
    if (editor.getClipboardText() != "quick") {
        return TestResult(false, "Copy failed. Expected clipboard: 'quick', Got: '" + 
                         editor.getClipboardText() + "'");
    }
    
    // Verify selection still exists after copy
    if (!editor.hasSelection()) {
        return TestResult(false, "Selection was cleared after copy");
    }
    
    // Test cut
    cmdManager.executeCommand(std::make_unique<CutCommand>(), editor);
    if (editor.getClipboardText() != "quick") {
        return TestResult(false, "Cut failed to put text in clipboard. Got: '" + editor.getClipboardText() + "'");
    }
    
    if (editor.hasSelection()) {
        return TestResult(false, "Selection not cleared after cut");
    }
    
    if (editor.getBuffer().getLine(0) != "The  brown fox") {
        return TestResult(false, "Cut failed to remove text. Got: '" + 
                         editor.getBuffer().getLine(0) + "'");
    }
    
    // Test paste
    cmdManager.executeCommand(std::make_unique<PasteCommand>(), editor);
    if (editor.getBuffer().getLine(0) != "The quick brown fox") {
        return TestResult(false, "Paste failed. Got: '" + 
                         editor.getBuffer().getLine(0) + "'");
    }
    
    return TestResult(true, "Clipboard operations test passed");
}

// Test selection across multiple lines
TestResult testMultiLineSelection() {
    TestEditor editor;
    CommandManager cmdManager;
    
    // Setup
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("First line");
    editor.getBuffer().addLine("Second line");
    editor.setCursor(0, 6); // After "First"
    
    // Create selection across lines
    editor.setSelectionStart();
    editor.moveCursorDown();
    editor.moveCursorRight();
    editor.moveCursorRight();
    editor.moveCursorRight();
    editor.setSelectionEnd();
    
    // Verify selection spans multiple lines
    std::string selectedText = editor.getSelectedText();
    if (selectedText != " line\nSec") {
        return TestResult(false, "Multi-line selection failed. Expected: ' line\\nSec', Got: '" + 
                         selectedText + "'");
    }
    
    // Test copy and paste of multi-line selection
    cmdManager.executeCommand(std::make_unique<CopyCommand>(), editor);
    editor.clearSelection();
    editor.setCursor(1, 9); // End of "Second line"
    cmdManager.executeCommand(std::make_unique<PasteCommand>(), editor);
    
    if (editor.getBuffer().lineCount() != 3 || 
        editor.getBuffer().getLine(1) != "Second line line" || 
        editor.getBuffer().getLine(2) != "Sec") {
        std::string debugBuffer;
        for(size_t i=0; i < editor.getBuffer().lineCount(); ++i) {
            debugBuffer += "L" + std::to_string(i) + ": '" + editor.getBuffer().getLine(i) + "'\\n";
        }
        return TestResult(false, "Multi-line paste failed. Buffer:\\n" + debugBuffer);
    }
    
    return TestResult(true, "Multi-line selection test passed");
}

// Test word selection
TestResult testWordSelection() {
    TestEditor editor;
    
    // Setup
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("The quick brown fox");
    editor.setCursor(0, 5); // Middle of "quick"
    
    // Select word
    editor.selectWord();
    
    // Verify selection
    if (!editor.hasSelection()) {
        return TestResult(false, "No selection after selectWord");
    }
    
    if (editor.getSelectedText() != "quick") {
        return TestResult(false, "Word selection failed. Expected: 'quick', Got: '" + 
                         editor.getSelectedText() + "'");
    }
    
    return TestResult(true, "Word selection test passed");
}

// Test delete word
TestResult testDeleteWord() {
    TestEditor editor;
    
    // Setup
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("The quick brown fox");
    editor.setCursor(0, 4); // Start of "quick"
    
    // Delete word
    editor.deleteWord();
    
    // Verify word was deleted
    if (editor.getBuffer().getLine(0) != "The  brown fox") {
        return TestResult(false, "Delete word failed. Expected: 'The  brown fox', Got: '" + 
                         editor.getBuffer().getLine(0) + "'");
    }
    
    return TestResult(true, "Delete word test passed");
}

int main() {
    TestFramework runner;
    runner.registerTest("Basic Selection", testBasicSelection);
    runner.registerTest("Clipboard Operations", testClipboardOperations);
    runner.registerTest("Multi-Line Selection & Paste", testMultiLineSelection);
    runner.registerTest("Word Selection", testWordSelection);
    runner.registerTest("Delete Word", testDeleteWord);
    runner.runAllTests();
    return 0;
}

