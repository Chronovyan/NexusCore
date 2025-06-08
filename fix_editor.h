#pragma once

// Core editor components
#include "core/Document.h"
#include "tabs/TabState.h"
#include "syntax/SyntaxHighlighter.h"
#include "interfaces/ICodebaseIndex.hpp"
#include "undo/TextOperation.h"
#include <memory>

// Forward declarations
namespace search {
    class SearchManager;
}

// Forward declarations for editor types
namespace ai_editor {
    struct EditorSettings;
    
    // Search state for find/replace operations
    struct SearchState {
        std::string searchText;
        std::string replaceText;
        bool caseSensitive = false;
        bool wholeWord = false;
        bool useRegex = false;
        bool matchBrackets = false;
        bool wrapAround = true;
        int currentMatchLine = -1;
        int currentMatchCol = -1;
        std::vector<std::pair<int, int>> matches; // line, column pairs
    };
    
    struct LanguageDefinition;
}

// Standard Library
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <regex>
#include <map>

// Forward declarations
class TabState;

namespace ai_editor {

/**
 * @class EditorDemoWindow
 * @brief A simplified demonstration window for the AI First Editor
 *
 * This class encapsulates a basic ImGui text editor window without all the
 * dependencies of the full AI First Editor.
 */
class EditorDemoWindow {
    friend class EditorDemoWindowTest; // Allow test class to access private members

    // Core components
    std::unique_ptr<SyntaxHighlighter> syntaxHighlighter_;
    std::unique_ptr<search::SearchManager> searchManager_;

    // Editor state
    std::vector<std::string> lines_;
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
    bool hasSelection_ = false;
    int selectionStartLine_ = 0;
    int selectionStartCol_ = 0;
    int selectionEndLine_ = 0;
    int selectionEndCol_ = 0;

    // Tab management
    std::unique_ptr<TabManager> tabManager_;
    int activeTabIndex_ = -1;

    // Window state
    std::string currentLanguage_ = "text";
    std::string currentFilePath_ = "";
    bool isModified_ = false;
    bool isOpen_ = true;

    // Window properties
    std::string windowTitle_ = "AI-First Text Editor";
    ImVec2 windowSize_ = ImVec2(1280, 720);
    ImVec2 windowPos_ = ImVec2(100, 100);

    // Viewport state
    int viewportStartLine_ = 0;
    int viewportHeight_ = 30; // Default viewport height in lines

    // Code folding state
    std::map<int, bool> foldedLines_;

    // Undo/Redo stacks
    std::deque<TextOperation> undoStack_;
    std::deque<TextOperation> redoStack_;

    // Search state
    ai_editor::SearchState searchState_;

    // UI state
    char statusBuffer_[256] = "Ready";
    char newTabName_[256] = "";
    bool showSearchPanel_ = false;
    bool searchCaseSensitive_ = false;
    bool searchWholeWord_ = false;
    bool searchUseRegex_ = false;
    char searchText_[256] = "";
    char replaceText_[256] = "";

    // Editor settings
    ai_editor::EditorSettings settings_;
    bool showSettingsDialog_ = false;

    // UI settings
    float fontSize_ = 16.0f;
    bool showWhitespace_ = false;
    bool wordWrap_ = true;
    bool showLineNumbers_ = true;

    // Color settings
    ImVec4 backgroundColor_ = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    ImVec4 textColor_ = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    ImVec4 cursorColor_ = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 selectionColor_ = ImVec4(0.26f, 0.59f, 0.98f, 0.4f);

    // Syntax highlighting
    std::unordered_map<std::string, ai_editor::LanguageDefinition> languages_;

public:
    /**
     * @brief Constructor
     */
    EditorDemoWindow();
    ~EditorDemoWindow() = default;

    /**
     * @brief Initialize the editor window
     */
    bool initialize();

    /**
     * @brief Render the editor demo window
     * @param p_open Pointer to boolean controlling window visibility
     */
    void render(bool* p_open = nullptr);

    // File operations
    bool loadFile(const std::string& filename);
    bool saveFile(const std::string& filename);
    std::string getEditorContent() const;
    void setDemoCode(const std::string& code, const std::string& language);

    // Editor state
    void setWindowTitle(const std::string& title) { windowTitle_ = title; }
    std::string getWindowTitle() const { return windowTitle_; }
    bool isOpen() const { return isOpen_; }
    void setOpen(bool open) { isOpen_ = open; }

    // Window properties
    ImVec2 getWindowSize() const { return windowSize_; }
    ImVec2 getWindowPos() const { return windowPos_; }

    // Editor state getters
    std::string getCurrentLanguage() const { return currentLanguage_; }
    std::string getCurrentFilePath() const { return currentFilePath_; }
    bool getIsModified() const { return isModified_; }

    // Viewport
    int getViewportStartLine() const { return viewportStartLine_; }
    int getViewportHeight() const { return viewportHeight_; }

    // Tab management
    void addNewTab(const std::string& filename = "");
    void closeTab(int tabIndex);
    void switchToTab(int tabIndex);
    const std::vector<std::shared_ptr<TabState>>& getTabs() const { 
        static const std::vector<std::shared_ptr<TabState>> emptyTabs;
        return tabManager_ ? tabManager_->getTabs() : emptyTabs; 
    }
    int getActiveTabIndex() const { return activeTabIndex_; }

    // Cursor and selection
    void setCursor(int line, int column);
    std::pair<int, int> getCursorPosition() const;
    bool hasSelection() const { return hasSelection_; }
    void getSelection(int& startLine, int& startCol, int& endLine, int& endCol) const;

    // Search and replace
    bool search(const std::string& searchText, bool caseSensitive, bool wholeWord = false);
    bool findNext();
    bool replace(const std::string& replaceText);
    int replaceAll(const std::string& searchText, const std::string& replaceText, 
                      bool caseSensitive, bool wholeWord = false);

    // Undo/Redo
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

    // Settings structure for editor configuration
    struct EditorSettings {
        bool showLineNumbers = true;
        bool enableSyntaxHighlighting = true;
        bool enableAutoIndent = true;
        bool enableWordWrap = false;
        bool showFoldingMarkers = true;
        bool enableAutoComplete = true;
        ImVec4 currentLineBackgroundColor = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
        ImVec4 selectedTextBackgroundColor = ImVec4(0.2f, 0.4f, 0.8f, 0.5f);
        float tabSize = 4.0f;
        float fontSize = 14.0f;
        std::string fontName = "Consolas";
    };

    // File handling functions
    void newFile();
    void openFile();
    bool saveCurrentFile();
    bool saveFileAs();
    
    // Tab management
    void storeTabState();
    void updateTabState();
    
    // Syntax highlighting
    struct SyntaxRule {
        std::string pattern;
        ImVec4 color;
        bool isRegex;
    };
    
    struct LanguageDefinition {
        std::string name;
        std::vector<std::string> extensions;
        std::vector<SyntaxRule> rules;
        std::unordered_map<std::string, ImVec4> keywords;
        std::string lineCommentStart;
        std::pair<std::string, std::string> blockComment;
        std::vector<std::pair<char, char>> brackets;
        std::vector<std::string> preprocessors;
    };

    // Test class friend declaration
    friend class EditorDemoWindowTest;

    // Public getter methods for testing purposes
    int getActiveTabIndex() const { return activeTabIndex_; }
    std::string getWindowTitle() const { return windowTitle_; }
    ImVec2 getWindowSize() const { return windowSize_; }
    ImVec2 getWindowPos() const { return windowPos_; }
    std::vector<std::string> getLines() const { return lines_; }
    std::string getCurrentLanguage() const { return currentLanguage_; }
    std::string getCurrentFilePath() const { return currentFilePath_; }
    bool getIsModified() const { return isModified_; }
    bool getIsOpen() const { return isOpen_; }

    // Render helper methods
    void renderMenuBar(bool* p_open = nullptr);
    void renderTabBar();
    void renderStatusBar();
    void renderEditorContent();
    void renderSearchReplace();
    void renderSettingsDialog();
    void handleKeyboardShortcuts();
    
    // File dialog helpers
    std::string showOpenFileDialog();
    std::string showSaveFileDialog();
    void renderFileDialog(bool isOpen, bool& result);
    
    // Code folding
    bool isFoldable(int line) const;
    void toggleFold(int line);
    bool isFolded(int line) const;
    int getNextVisibleLine(int line) const;
    void renderFoldingMarker(int line, bool isFoldable, bool isFolded) const;
    
    // Demo examples
    void loadCppExample();
    void loadPythonExample();
    void loadJavaScriptExample();
    
    // Helper methods
    void ensureCursorVisible();
    void moveCursor(int line, int column, bool select = false);
    void insertTextAtCursor(const std::string& text);
    void deleteSelection();
    void copyToClipboard(const std::string& text);
    std::string getSelectedText() const;
    void setLanguageFromFilename(const std::string& filename);
    
    // Syntax highlighting
    void initializeLanguages();
    void applySyntaxHighlighting(const std::string& text, const std::string& language, int lineNumber);
    
    // Text operations
    void recordOperation(const TextOperation& op);
    void applyOperation(const TextOperation& op, bool isUndo);
    
    // Search helpers
    bool findNextMatch(int startLine, int startPos, const std::string& searchText, 
                          bool caseSensitive, bool wholeWord, bool forward = true);
    static bool isWordCharacter(char c);
    
    // Settings persistence
    void loadSettings();
    void saveSettings() const;
    void applySettings();
};

} // namespace ai_editor
