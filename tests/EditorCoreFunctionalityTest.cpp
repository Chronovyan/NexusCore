#include "gtest/gtest.h"
#include "../src/Editor.h"
#include "../src/TextBuffer.h"
#include "../src/CommandManager.h"
#include "../src/SyntaxHighlightingManager.h"
#include "MockAIAgentOrchestrator.h"
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>

class EditorCoreFunctionalityTest : public ::testing::Test {
protected:
    void SetUp() override {
        try {
            // Create dependencies
            textBuffer = std::make_shared<TextBuffer>();
            commandManager = std::make_shared<CommandManager>();
            syntaxHighlightingManager = std::make_shared<SyntaxHighlightingManager>();
            
            // Create editor with dependencies
            editor = std::make_unique<Editor>(
                textBuffer,
                commandManager,
                syntaxHighlightingManager
            );
            
            // Set up mock AI agent orchestrator
            auto mockOrchestrator = std::make_shared<MockAIAgentOrchestrator>();
            editor->setAIAgentOrchestrator(mockOrchestrator);
            
            // Clear the buffer to ensure a clean state
            textBuffer->clear(true);
        } catch (const std::exception& e) {
            FAIL() << "Exception in SetUp: " << e.what();
        } catch (...) {
            FAIL() << "Unknown exception in SetUp";
        }
    }

    void TearDown() override {
        if (editor) {
            editor->getBuffer().clear(true);
        }
        editor.reset();
    }

    std::shared_ptr<TextBuffer> textBuffer;
    std::shared_ptr<CommandManager> commandManager;
    std::shared_ptr<SyntaxHighlightingManager> syntaxHighlightingManager;
    std::unique_ptr<Editor> editor;
};

// 1. Basic Text Insertion and Deletion
TEST_F(EditorCoreFunctionalityTest, InsertAndDeleteText) {
    // Test basic text insertion
    editor->typeText("Hello");
    EXPECT_EQ(editor->getCurrentLineText(), "Hello");
    EXPECT_EQ(editor->getCursorLine(), 0);
    EXPECT_EQ(editor->getCursorCol(), 5);
    
    // Test cursor movement and insertion
    editor->moveCursorLeft();
    editor->moveCursorLeft();
    editor->typeText("p ");
    EXPECT_EQ(editor->getCurrentLineText(), "Help lo");
    
    // Test backspace
    editor->backspace();
    EXPECT_EQ(editor->getCurrentLineText(), "Helplo");
    
    // Test delete
    editor->moveCursorToLineStart();
    editor->moveCursorRight(); // Move to 'e'
    editor->deleteForward();
    EXPECT_EQ(editor->getCurrentLineText(), "Hlplo");
}

// 2. Multi-line Operations
TEST_F(EditorCoreFunctionalityTest, MultiLineOperations) {
    // Insert multiple lines
    editor->typeText("First line\nSecond line\nThird line");
    
    // Verify line count
    EXPECT_EQ(editor->getBuffer().getLineCount(), 3);
    
    // Move between lines
    editor->moveCursorUp();
    EXPECT_EQ(editor->getCursorLine(), 1);
    EXPECT_EQ(editor->getCurrentLineText(), "Second line");
    
    // Insert at beginning of line
    editor->moveCursorToLineStart();
    editor->typeText("The ");
    EXPECT_EQ(editor->getCurrentLineText(), "The Second line");
    
    // Test new line insertion
    editor->moveCursorToLineEnd();
    editor->newLine();
    editor->typeText("New line");
    EXPECT_EQ(editor->getBuffer().getLineCount(), 4);
    EXPECT_EQ(editor->getCurrentLineText(), "New line");
}

// 3. Selection and Clipboard Operations
TEST_F(EditorCoreFunctionalityTest, SelectionAndClipboard) {
    // Set up test content
    editor->typeText("This is a test string for selection");
    
    // Select a word
    editor->moveCursorToLineStart();
    editor->moveCursorRight(5); // Move to 'i' in "This"
    editor->startSelection();
    editor->moveCursorRight(4); // Select "is a"
    EXPECT_TRUE(editor->hasSelection());
    EXPECT_EQ(editor->getSelectedText(), "is a");
    
    // Test copy and paste
    editor->copySelection();
    editor->moveCursorToLineEnd();
    editor->typeText(" ");
    editor->pasteAtCursor();
    EXPECT_EQ(editor->getCurrentLineText(), "This is a test string for selection is a");
    
    // Test cut
    editor->setSelectionRange(0, 5, 0, 9); // Select first "is a"
    editor->cutSelection();
    EXPECT_EQ(editor->getCurrentLineText(), "This  test string for selection is a");
    
    // Test select all
    editor->selectAll();
    EXPECT_TRUE(editor->hasSelection());
    EXPECT_EQ(editor->getSelectedText(), "This  test string for selection is a");
}

// 4. Undo/Redo Operations
TEST_F(EditorCoreFunctionalityTest, UndoRedoOperations) {
    // Initial text
    editor->typeText("Initial text");
    
    // Make some changes
    editor->selectAll();
    editor->typeText("New ");
    EXPECT_EQ(editor->getCurrentLineText(), "New ");
    
    // Undo
    EXPECT_TRUE(editor->canUndo());
    editor->undo();
    EXPECT_EQ(editor->getCurrentLineText(), "Initial text");
    
    // Redo
    EXPECT_TRUE(editor->canRedo());
    editor->redo();
    EXPECT_EQ(editor->getCurrentLineText(), "New ");
    
    // Test multiple undos/redos
    editor->typeText("text with more changes");
    editor->undo();
    editor->undo();
    EXPECT_EQ(editor->getCurrentLineText(), "Initial text");
    editor->redo();
    editor->redo();
    EXPECT_EQ(editor->getCurrentLineText(), "New text with more changes");
}

// 5. Word Navigation and Manipulation
TEST_F(EditorCoreFunctionalityTest, WordNavigation) {
    editor->typeText("This is a test string with multiple words");
    
    // Move to start of previous word
    editor->moveCursorToLineEnd();
    editor->moveCursorToPrevWord();
    EXPECT_EQ(editor->getCursorCol(), 34); // Before 'words'
    
    // Delete previous word
    editor->deleteWord();
    EXPECT_EQ(editor->getCurrentLineText(), "This is a test string with multiple ");
    
    // Move to next word and delete
    editor->moveCursorToPrevWord();
    editor->moveCursorToNextWord();
    editor->deleteWord();
    EXPECT_EQ(editor->getCurrentLineText(), "This is a test string with ");
}

// 6. Line Operations
TEST_F(EditorCoreFunctionalityTest, LineOperations) {
    // Set up multiple lines
    editor->typeText("First line\nSecond line\nThird line");
    
    // Delete middle line
    editor->moveCursorToLineStart();
    editor->moveCursorDown();
    editor->deleteLine();
    
    // Verify line was deleted
    EXPECT_EQ(editor->getBuffer().getLineCount(), 2);
    EXPECT_EQ(editor->getCurrentLineText(), "Third line");
    
    // Insert line above
    editor->moveCursorUp();
    editor->insertLine(editor->getCursorLine(), "New second line");
    EXPECT_EQ(editor->getBuffer().getLineCount(), 3);
    EXPECT_EQ(editor->getCurrentLineText(), "New second line");
    
    // Join lines
    editor->moveCursorToLineEnd();
    editor->joinWithNextLine();
    EXPECT_EQ(editor->getBuffer().getLineCount(), 2);
    EXPECT_EQ(editor->getCurrentLineText(), "New second lineThird line");
}

// 7. Search Operations
TEST_F(EditorCoreFunctionalityTest, SearchOperations) {
    editor->typeText("This is a test string with test data for testing");
    
    // Search forward
    EXPECT_TRUE(editor->search("test", true, true));
    EXPECT_EQ(editor->getCursorCol(), 10); // First occurrence of "test"
    
    // Search next
    EXPECT_TRUE(editor->searchNext());
    EXPECT_EQ(editor->getCursorCol(), 22); // Second occurrence of "test"
    
    // Search previous
    EXPECT_TRUE(editor->searchPrevious());
    EXPECT_EQ(editor->getCursorCol(), 10); // Back to first occurrence
    
    // Test case sensitivity
    EXPECT_FALSE(editor->search("TEST", true, true)); // Case-sensitive search should fail
    EXPECT_TRUE(editor->search("TEST", false, true)); // Case-insensitive search should pass
}

// 8. Replace Operations
TEST_F(EditorCoreFunctionalityTest, ReplaceOperations) {
    editor->typeText("This is a test string with test data");
    
    // Single replace
    editor->moveCursorToBufferStart();
    EXPECT_TRUE(editor->replace("test", "demo", true));
    EXPECT_EQ(editor->getCurrentLineText(), "This is a demo string with test data");
    
    // Replace all
    EXPECT_EQ(editor->replaceAll("test", "demo", true), 1);
    EXPECT_EQ(editor->getCurrentLineText(), "This is a demo string with demo data");
    
    // Test undo of replace all
    editor->undo();
    EXPECT_EQ(editor->getCurrentLineText(), "This is a test string with test data");
}

// 9. Edge Cases
TEST_F(EditorCoreFunctionalityTest, EdgeCases) {
    // Test empty editor
    EXPECT_EQ(editor->getCurrentLineText(), "");
    EXPECT_EQ(editor->getCursorLine(), 0);
    EXPECT_EQ(editor->getCursorCol(), 0);
    
    // Test cursor movement in empty editor
    editor->moveCursorRight();
    editor->moveCursorLeft();
    editor->moveCursorUp();
    editor->moveCursorDown();
    
    // Test delete/backspace in empty editor
    editor->deleteForward();
    editor->backspace();
    
    // Test selection in empty editor
    editor->startSelection();
    editor->updateSelection();
    EXPECT_FALSE(editor->hasSelection());
    
    // Test undo/redo with no operations
    EXPECT_FALSE(editor->canUndo());
    EXPECT_FALSE(editor->canRedo());
    
    // Test new line in empty editor
    editor->newLine();
    EXPECT_EQ(editor->getBuffer().getLineCount(), 2);
    EXPECT_EQ(editor->getCursorLine(), 1);
    EXPECT_EQ(editor->getCursorCol(), 0);
}
