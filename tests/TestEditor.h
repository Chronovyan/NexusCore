#ifndef TEST_EDITOR_H
#define TEST_EDITOR_H

#include "../src/Editor.h"

// Test-specific editor class that allows unrestricted cursor positioning
class TestEditor : public Editor {
public:
    TestEditor() : Editor() {}

    // Override setCursor to bypass validation in tests
    void setCursor(size_t line, size_t col) {
        // Direct access to protected member variables of Editor
        cursorLine_ = line;
        cursorCol_ = col;
        // No validation call - allows any position for testing
    }

    // Enable access to protected members for test verification
    using Editor::cursorLine_;
    using Editor::cursorCol_;
    using Editor::hasSelection_;
    using Editor::selectionStartLine_;
    using Editor::selectionStartCol_;
    using Editor::selectionEndLine_;
    using Editor::selectionEndCol_;
    using Editor::clipboard_;

    // Access methods for testing selection
    bool hasSelection() const {
        return Editor::hasSelection();
    }

    std::string getSelectedText() const {
        return Editor::getSelectedText();
    }
    
    // Access methods for syntax highlighting
    bool isSyntaxHighlightingEnabled() const {
        return Editor::isSyntaxHighlightingEnabled();
    }
    
    void enableSyntaxHighlighting(bool enable = true) {
        Editor::enableSyntaxHighlighting(enable);
    }
    
    void setFilename(const std::string& filename) {
        Editor::setFilename(filename);
    }
    
    std::string getFilename() const {
        return Editor::getFilename();
    }
    
    SyntaxHighlighter* getCurrentHighlighter() const {
        return Editor::getCurrentHighlighter();
    }
    
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const {
        return Editor::getHighlightingStyles();
    }
};

#endif // TEST_EDITOR_H 