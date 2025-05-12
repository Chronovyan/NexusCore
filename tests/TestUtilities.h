#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "TestEditor.h"

namespace test_utils {

// Helper class for all editor command tests
class EditorCommandTestBase : public ::testing::Test {
protected:
    TestEditor editor;
    
    void SetUp() override {
        editor.getBuffer().clear(false); // Clear without adding an empty line
    }
    
    void TearDown() override {
        // Base implementation - nothing to do here
    }
    
    // Helper method to set up the buffer with multiple lines
    void setBufferLines(const std::vector<std::string>& lines) {
        editor.getBuffer().clear(false);
        for (const auto& line : lines) {
            editor.getBuffer().addLine(line);
        }
    }
    
    // Helper method to set up a buffer with a single line
    void setBufferContent(const std::string& content) {
        editor.getBuffer().clear(false);
        editor.getBuffer().addLine(content);
    }
    
    // Helper method to position cursor and optionally set selection
    void positionCursor(size_t line, size_t col, 
                        bool setSelection = false,
                        size_t selStartLine = 0, size_t selStartCol = 0,
                        size_t selEndLine = 0, size_t selEndCol = 0) {
        editor.setCursor(line, col);
        if (setSelection) {
            editor.setSelectionRange(selStartLine, selStartCol, selEndLine, selEndCol);
        } else {
            editor.clearSelection();
        }
    }
    
    // Helper method to verify buffer content matches expected lines
    void verifyBufferContent(const std::vector<std::string>& expectedLines) {
        ASSERT_EQ(expectedLines.size(), editor.getBuffer().lineCount()) 
            << "Line count should match expected";
        
        for (size_t i = 0; i < expectedLines.size(); ++i) {
            ASSERT_EQ(expectedLines[i], editor.getBuffer().getLine(i))
                << "Line " << i << " content should match expected";
        }
    }
    
    // Helper method to verify cursor position
    void verifyCursorPosition(size_t expectedLine, size_t expectedCol) {
        ASSERT_EQ(expectedLine, editor.getCursorLine()) 
            << "Cursor line should be at expected position";
        ASSERT_EQ(expectedCol, editor.getCursorCol()) 
            << "Cursor column should be at expected position";
    }
    
    // Helper method to verify selection range
    void verifySelection(bool shouldHaveSelection,
                         size_t expectedStartLine = 0, size_t expectedStartCol = 0,
                         size_t expectedEndLine = 0, size_t expectedEndCol = 0) {
        ASSERT_EQ(shouldHaveSelection, editor.hasSelection())
            << (shouldHaveSelection ? "Should have selection" : "Should not have selection");
        
        if (shouldHaveSelection) {
            ASSERT_EQ(expectedStartLine, editor.getSelectionStartLine()) 
                << "Selection start line should match expected";
            ASSERT_EQ(expectedStartCol, editor.getSelectionStartCol())
                << "Selection start column should match expected";
            ASSERT_EQ(expectedEndLine, editor.getSelectionEndLine())
                << "Selection end line should match expected";
            ASSERT_EQ(expectedEndCol, editor.getSelectionEndCol())
                << "Selection end column should match expected";
        }
    }
    
    // Helper method to verify clipboard content
    void verifyClipboard(const std::string& expectedContent) {
        ASSERT_EQ(expectedContent, editor.getClipboardText())
            << "Clipboard content should match expected";
    }
};

// Helper class for clipboard operation tests
class ClipboardOperationsTestBase : public EditorCommandTestBase {
protected:
    void SetUp() override {
        EditorCommandTestBase::SetUp();
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

} // namespace test_utils 