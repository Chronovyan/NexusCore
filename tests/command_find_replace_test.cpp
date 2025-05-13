#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "../src/Editor.h"
#include "../src/EditorCommands.h"
#include "TestEditor.h"
#include "TestUtilities.h"

// SearchCommand tests
class SearchCommandTest : public test_utils::EditorCommandTestBase {
protected:
    void SetUp() override {
        EditorCommandTestBase::SetUp();
        // Setup buffer with text that contains multiple search targets
        setBufferLines({
            "Search for word, then search for WORD again.",
            "Another word here.",
            "No targets on this line."
        });
        
        // Reset cursor to beginning
        positionCursor(0, 0);
    }
};

// Test case-sensitive search finding first occurrence
TEST_F(SearchCommandTest, CaseSensitiveFirstMatch) {
    auto searchCmd = std::make_unique<SearchCommand>("word", true);
    searchCmd->execute(editor);
    
    // Verify search was successful
    EXPECT_TRUE(searchCmd->wasSuccessful());
    
    // Verify correct text is selected
    verifySelection(true, 0, 11, 0, 15);
    
    // Verify cursor is at start of selection (modified expectation)
    verifyCursorPosition(0, 11);
    
    // Verify undo restores original state
    searchCmd->undo(editor);
    verifySelection(false);
    verifyCursorPosition(0, 0);
}

// Test case-sensitive search finding next occurrence
TEST_F(SearchCommandTest, CaseSensitiveNextMatch) {
    // First search
    auto searchCmd1 = std::make_unique<SearchCommand>("word", true);
    searchCmd1->execute(editor);
    
    // Debug prints for first search result
    std::cout << "After first search - Cursor: " << editor.getCursorLine() << "," << editor.getCursorCol() 
              << " | Selection: " << editor.getSelectionStartLine() << "," << editor.getSelectionStartCol()
              << " to " << editor.getSelectionEndLine() << "," << editor.getSelectionEndCol() << std::endl;
    
    // Move cursor manually past the first match to test finding the next match
    editor.setCursor(0, 15); // Move cursor to end of first match
    
    // Second search from current position
    auto searchCmd2 = std::make_unique<SearchCommand>("word", true);
    searchCmd2->execute(editor);
    
    // Debug prints for second search result
    std::cout << "After second search - Cursor: " << editor.getCursorLine() << "," << editor.getCursorCol() 
              << " | Selection: " << editor.getSelectionStartLine() << "," << editor.getSelectionStartCol()
              << " to " << editor.getSelectionEndLine() << "," << editor.getSelectionEndCol() << std::endl;
    
    // Verify search found second occurrence
    EXPECT_TRUE(searchCmd2->wasSuccessful());
    verifySelection(true, 1, 8, 1, 12);
    
    // Verify cursor is at start of selection
    verifyCursorPosition(1, 8);
}

// Test case-insensitive search
TEST_F(SearchCommandTest, CaseInsensitiveSearch) {
    auto searchCmd = std::make_unique<SearchCommand>("WORD", false);
    searchCmd->execute(editor);
    
    // Verify search was successful and found "word" (lowercase)
    EXPECT_TRUE(searchCmd->wasSuccessful());
    verifySelection(true, 0, 11, 0, 15);
    
    // Verify cursor is at start of first selection
    verifyCursorPosition(0, 11);
    
    // Move cursor manually past the first match to test finding the next match
    editor.setCursor(0, 15); // Move cursor to end of first match
    
    // Second search finds uppercase WORD
    auto searchCmd2 = std::make_unique<SearchCommand>("WORD", false);
    searchCmd2->execute(editor);
    
    EXPECT_TRUE(searchCmd2->wasSuccessful());
    verifySelection(true, 0, 33, 0, 37);
    
    // Verify cursor is at start of second selection
    verifyCursorPosition(0, 33);
}

// Test search with no matches
TEST_F(SearchCommandTest, NoMatches) {
    auto searchCmd = std::make_unique<SearchCommand>("nonexistent", true);
    searchCmd->execute(editor);
    
    // Verify search was unsuccessful
    EXPECT_FALSE(searchCmd->wasSuccessful());
    
    // Verify no selection was created
    verifySelection(false);
    
    // Verify cursor position is unchanged
    verifyCursorPosition(0, 0);
}

// ReplaceAllCommand tests
class ReplaceAllCommandTest : public test_utils::EditorCommandTestBase {
protected:
    void SetUp() override {
        EditorCommandTestBase::SetUp();
        
        // Setup buffer with text containing multiple instances to replace
        setBufferLines({
            "Replace word here, and word there, and even WORD here.",
            "Another word to replace.",
            "No target here."
        });
        
        // Reset cursor to beginning
        positionCursor(0, 0);
    }
};

// Test case-sensitive replacement
TEST_F(ReplaceAllCommandTest, CaseSensitiveReplace) {
    auto replaceAllCmd = std::make_unique<ReplaceAllCommand>("word", "token", true);
    replaceAllCmd->execute(editor);
    
    // Verify replacement was successful
    EXPECT_TRUE(replaceAllCmd->wasSuccessful());
    
    // Verify content was correctly modified (case-sensitive)
    verifyBufferContent({
        "Replace token here, and token there, and even WORD here.",
        "Another token to replace.",
        "No target here."
    });
    
    // Verify undo restores original content
    replaceAllCmd->undo(editor);
    verifyBufferContent({
        "Replace word here, and word there, and even WORD here.",
        "Another word to replace.",
        "No target here."
    });
    
    // Verify undo restores cursor position
    verifyCursorPosition(0, 0);
}

// Test case-insensitive replacement
TEST_F(ReplaceAllCommandTest, CaseInsensitiveReplace) {
    auto replaceAllCmd = std::make_unique<ReplaceAllCommand>("WORD", "phrase", false);
    replaceAllCmd->execute(editor);
    
    // Verify replacement was successful
    EXPECT_TRUE(replaceAllCmd->wasSuccessful());
    
    // Verify content was correctly modified (all case variations replaced)
    verifyBufferContent({
        "Replace phrase here, and phrase there, and even phrase here.",
        "Another phrase to replace.",
        "No target here."
    });
    
    // Verify undo restores original content
    replaceAllCmd->undo(editor);
    verifyBufferContent({
        "Replace word here, and word there, and even WORD here.",
        "Another word to replace.",
        "No target here."
    });
}

// Test replacement with non-existing term
TEST_F(ReplaceAllCommandTest, NoMatchesReplace) {
    auto replaceAllCmd = std::make_unique<ReplaceAllCommand>("nonexistent", "stuff", true);
    replaceAllCmd->execute(editor);
    
    // The operation might still report successful even with 0 replacements
    // What's important is that content and cursor remain unchanged
    verifyBufferContent({
        "Replace word here, and word there, and even WORD here.",
        "Another word to replace.",
        "No target here."
    });
    
    // Cursor position should be unchanged
    verifyCursorPosition(0, 0);
}

// Empty replacement test
TEST_F(ReplaceAllCommandTest, EmptyReplacement) {
    auto replaceAllCmd = std::make_unique<ReplaceAllCommand>("word", "", true);
    replaceAllCmd->execute(editor);
    
    // Verify replacement was successful
    EXPECT_TRUE(replaceAllCmd->wasSuccessful());
    
    // Verify content was correctly modified (words removed)
    verifyBufferContent({
        "Replace  here, and  there, and even WORD here.",
        "Another  to replace.",
        "No target here."
    });
    
    // Verify undo restores original content
    replaceAllCmd->undo(editor);
    verifyBufferContent({
        "Replace word here, and word there, and even WORD here.",
        "Another word to replace.",
        "No target here."
    });
} 