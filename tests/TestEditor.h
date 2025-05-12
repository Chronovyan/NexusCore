#ifndef TEST_EDITOR_H
#define TEST_EDITOR_H

#include "../src/Editor.h"
#include "TestSyntaxHighlightingManager.h"

// Test-specific editor class that allows unrestricted cursor positioning and uses TestSyntaxHighlightingManager
class TestEditor : public Editor {
public:
    TestEditor() : Editor() {
        // Replace the production SyntaxHighlightingManager with our test version
        testSyntaxHighlightingManager_ = std::make_unique<TestSyntaxHighlightingManager>();
        testSyntaxHighlightingManager_->setBuffer(&buffer_);
        testSyntaxHighlightingManager_->setEnabled(syntaxHighlightingEnabled_);
    }

    // Override setCursor to bypass validation in tests
    void setCursor(size_t line, size_t col) override {
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

    // Override getHighlightingStyles to use TestSyntaxHighlightingManager
    std::vector<std::vector<SyntaxStyle>> getHighlightingStyles() const override {
        if (!syntaxHighlightingEnabled_ || !currentHighlighter_) {
            return std::vector<std::vector<SyntaxStyle>>(buffer_.lineCount());
        }
        
        size_t startLine = topVisibleLine_;
        size_t endLine = std::min(buffer_.lineCount(), topVisibleLine_ + viewableLines_) - 1;
        
        testSyntaxHighlightingManager_->setVisibleRange(startLine, endLine);
        return testSyntaxHighlightingManager_->getHighlightingStyles(startLine, endLine);
    }
    
    // Override detectAndSetHighlighter to use TestSyntaxHighlightingManager
    void detectAndSetHighlighter() override {
        currentHighlighter_ = nullptr;
        
        if (filename_.empty() || !syntaxHighlightingEnabled_) {
            testSyntaxHighlightingManager_->setHighlighter(nullptr);
            return;
        }
        
        try {
            currentHighlighter_ = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension(filename_);
            testSyntaxHighlightingManager_->setHighlighter(currentHighlighter_);
        } catch (...) {
            currentHighlighter_ = nullptr;
            testSyntaxHighlightingManager_->setHighlighter(nullptr);
        }
    }
    
    // Override enableSyntaxHighlighting to use TestSyntaxHighlightingManager
    void enableSyntaxHighlighting(bool enable = true) override {
        syntaxHighlightingEnabled_ = enable;
        testSyntaxHighlightingManager_->setEnabled(enable);
    }
    
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
    
    void setFilename(const std::string& filename) {
        Editor::setFilename(filename);
    }
    
    std::string getFilename() const {
        return Editor::getFilename();
    }
    
    std::shared_ptr<SyntaxHighlighter> getCurrentHighlighter() const {
        return Editor::getCurrentHighlighter();
    }

private:
    std::unique_ptr<TestSyntaxHighlightingManager> testSyntaxHighlightingManager_;
};

#endif // TEST_EDITOR_H 