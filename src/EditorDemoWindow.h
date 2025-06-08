#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <deque>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <memory>
#include <functional>
#include <imgui.h>
#include <regex>
#include <map>
#include <unordered_map>
#include <stack>
#include <filesystem>

namespace ai_editor {

// Forward declarations
struct TextSelection;

// Define TextOperation structure fully instead of forward declaring it
struct TextOperation {
    enum class Type { INSERT, DELETE, REPLACE };
    Type type;
    int line;
    int column;
    std::string text; // Text inserted, deleted, or replaced
    std::string replacedText; // Only used for REPLACE operations

    // For multi-line operations
    int endLine = -1;
    int endColumn = -1;
};

/**
 * @class EditorDemoWindow
 * @brief A simplified demonstration window for the AI First Editor
 * 
 * This class encapsulates a basic ImGui text editor window without all the
 * dependencies of the full AI First Editor.
 */
class EditorDemoWindow {
public:
    /**
     * @brief Constructor
     */
    EditorDemoWindow();
    ~EditorDemoWindow();
    
    // Core functionality
    ~EditorDemoWindow() = default;
    
    /**
     * @brief Initialize the editor window
     */
    bool initialize();
    
    /**
     * @brief Render the editor demo window
     * 
     * @param p_open Pointer to boolean controlling window visibility
     */
    void render(bool* p_open = nullptr);
    
    /**
     * @brief Set demo code to display in the editor
     * 
     * @param code The demo code to display
     * @param language The language of the code
     */
    void setDemoCode(const std::string& code, const std::string& language);
    
    /**
     * @brief Load a file into the editor
     * 
     * @param filename The path to the file to load
     * @return True if the file was loaded successfully
     */
    bool loadFile(const std::string& filename);
    
    /**
     * @brief Save current content to a file
     * 
     * @param filename The path to save to
     * @return True if the file was saved successfully
     */
    bool saveFile(const std::string& filename);
    
    /**
     * @brief Get the current content of the editor
     * 
     * @return The text content of the editor
     */
    std::string getEditorContent() const;
    
    /**
     * @brief Set the title of the demo window
     * 
     * @param title The window title
     */
    void setWindowTitle(const std::string& title) { windowTitle_ = title; }
    
    /**
     * @brief Check if there are undoable operations
     * 
     * @return True if undo is available
     */
    bool canUndo() const { return !undoStack_.empty(); }
    
    /**
     * @brief Check if there are redoable operations
     * 
     * @return True if redo is available
     */
    bool canRedo() const { return !redoStack_.empty(); }
    
    /**
     * @brief Undo the last operation
     */
    void undo();
    
    /**
     * @brief Redo the last undone operation
     */
    void redo();
    
    /**
     * @brief Search for text in the editor
     * 
     * @param searchText Text to search for
     * @param caseSensitive Whether the search is case sensitive
     * @param wholeWord Whether to match whole words only
     * @return True if the text was found
     */
    bool search(const std::string& searchText, bool caseSensitive, bool wholeWord = false);
    
    /**
     * @brief Find the next occurrence of the search text
     * 
     * @return True if another occurrence was found
     */
    bool findNext();
    
    /**
     * @brief Replace the current search match with new text
     * 
     * @param replaceText Text to replace with
     * @return True if replacement was made
     */
    bool replace(const std::string& replaceText);
    
    /**
     * @brief Replace all occurrences of the search text
     * 
     * @param searchText Text to search for
     * @param replaceText Text to replace with
     * @param caseSensitive Whether the search is case sensitive
     * @param wholeWord Whether to match whole words only
     * @return Number of replacements made
     */
    int replaceAll(const std::string& searchText, const std::string& replaceText, 
                   bool caseSensitive, bool wholeWord = false);

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
    void addNewTab(const std::string& filename = "");
    void closeTab(int tabIndex);
    void switchToTab(int tabIndex);
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

    bool isOpen() const { return isOpen_; }
    void setOpen(bool open) { isOpen_ = open; }

    // Test class friend declaration
    friend class EditorDemoWindowTest;

    // Public getter methods for testing purposes
    std::vector<TabState> getTabs() const { return tabs_; }
    int getActiveTabIndex() const { return activeTabIndex_; }
    std::string getWindowTitle() const { return windowTitle_; }
    ImVec2 getWindowSize() const { return windowSize_; }
    ImVec2 getWindowPos() const { return windowPos_; }
    std::vector<std::string> getLines() const { return lines_; }
    std::string getCurrentLanguage() const { return currentLanguage_; }
    std::string getCurrentFilePath() const { return currentFilePath_; }
    bool getIsModified() const { return isModified_; }
    bool getIsOpen() const { return isOpen_; }

private:
    // Tab and file management
    struct TabState {
        std::string filename;
        std::string displayName;
        std::string language;
        std::vector<std::string> lines;
        bool isModified = false;
        
        // Cursor state
        int cursorLine = 0;
        int cursorColumn = 0;
        
        // Selection state
        bool hasSelection = false;
        int selectionStartLine = 0;
        int selectionStartCol = 0;
        int selectionEndLine = 0;
        int selectionEndCol = 0;
        
        // Code folding state
        std::map<int, bool> foldedLines; // Line number -> folded state
        
        // Undo/redo stacks
        std::deque<TextOperation> undoStack;
        std::deque<TextOperation> redoStack;
        
        // Create a unique ID for ImGui
        std::string GetID() const {
            return filename.empty() ? "untitled" + std::to_string(reinterpret_cast<uintptr_t>(this)) : filename;
        }
    };
    
    // Window properties
    std::string windowTitle_ = "AI First Editor Demo";
    ImVec2 windowSize_ = ImVec2(800, 600);
    ImVec2 windowPos_ = ImVec2(100, 100);
    
    // Tab management
    std::vector<TabState> tabs_;
    int activeTabIndex_ = -1;
    bool showTabBar_ = true;
    char newTabName_[256] = "";
    
    // Editor content
    std::vector<std::string> lines_;
    std::string currentLanguage_ = "text";
    std::string currentFilePath_ = "";
    bool isModified_ = false;
    bool isOpen_ = true;
    
    // UI state
    char statusBuffer_[256] = "Ready";
    char searchBuffer_[256] = "";
    char replaceBuffer_[256] = "";
    bool caseSensitiveSearch_ = true;
    bool wholeWordSearch_ = false;
    bool regexSearch_ = false;
    bool isSearchFocused_ = false;
    ImVec2 textAreaSize_ = ImVec2(0, 0);
    ImVec2 textCursorPos_ = ImVec2(0, 0);
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
    
    // Text selection
    bool hasSelection_ = false;
    int selectionStartLine_ = 0;
    int selectionStartCol_ = 0;
    int selectionEndLine_ = 0;
    int selectionEndCol_ = 0;
    
    // Code folding
    std::map<int, bool> foldedLines_; // Line number -> folded state
    bool showFoldingMarkers_ = true;
    
    // Search state
    struct SearchState {
        std::string searchText;
        int currentLine = 0;
        int currentPos = 0;
        bool caseSensitive = true;
        bool wholeWord = false;
        bool isRegex = false;
        bool hasMatch = false;
        int matchLine = -1;
        int matchStartPos = -1;
        int matchEndPos = -1;
    };
    SearchState searchState_;
    
    // Undo/Redo stacks
    std::deque<TextOperation> undoStack_;
    std::deque<TextOperation> redoStack_;
    
    // Syntax highlighting
    struct SyntaxHighlightingRule {
        std::string pattern;
        ImVec4 color;
        bool isRegex = false;
    };
    struct SyntaxHighlightingLanguage {
        std::vector<SyntaxHighlightingRule> rules;
        std::string lineCommentStart;
        std::string blockCommentStart;
        std::string blockCommentEnd;
        ImVec4 defaultColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 commentColor = ImVec4(0.0f, 0.7f, 0.0f, 1.0f);
        ImVec4 stringColor = ImVec4(0.8f, 0.0f, 0.0f, 1.0f);
        ImVec4 numberColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
        ImVec4 keywordColor = ImVec4(0.0f, 0.0f, 0.8f, 1.0f);
        ImVec4 preprocessorColor = ImVec4(0.8f, 0.4f, 0.0f, 1.0f);
    };
    std::unordered_map<std::string, SyntaxHighlightingLanguage> languages_;
    
    // Settings
    EditorSettings settings_;
    bool showSettingsDialog_ = false;
    
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
    
    // Tab management
    void addNewTab(const std::string& filename = "");
    void closeTab(int tabIndex);
    void switchToTab(int tabIndex);
    void saveCurrentTab();
    void saveTabAs(int tabIndex);
    void updateTabState();
    void storeTabState();
    
    // Code folding
    bool isFoldable(int line) const;
    void toggleFold(int line);
    bool isFolded(int line) const;
    int getNextVisibleLine(int line) const;
    void renderFoldingMarker(int line, bool isFoldable, bool isFolded);
    
    // Demo examples
    void loadCppExample();
    void loadPythonExample();
    void loadJavaScriptExample();
    
    // Text editing helpers
    void insertCharacterAtCursor(char c);
    void insertTextAtCursor(const std::string& text);
    void deleteCharacterAtCursor(bool isBackspace);
    void handleKeyPress(ImGuiKey key, bool shift, bool ctrl);
    void handleTextInput();
    void handleMouseInput();
    void setCursorPosition(int line, int column);
    void moveCursor(int lineOffset, int columnOffset);
    void handleEnter();
    void handleTab();
    void handleDelete();
    void handleBackspace();
    void startSelection();
    void updateSelection();
    void clearSelection();
    std::string getSelectedText() const;
    void deleteSelection();
    
    // Clipboard operations
    void cutSelection();
    void copySelection();
    void pasteAtCursor();
    
    // Undo/Redo helpers
    void recordOperation(TextOperation operation);
    void clearUndoRedo();
    
    // Syntax highlighting helpers
    void initializeSyntaxHighlighting();
    void renderLineWithSyntaxHighlighting(const std::string& line, int lineIndex);
    bool isInsideComment(const std::string& line, int pos);
    bool isInsideString(const std::string& line, int pos);
    
    // Regex search helpers
    bool regexSearch(bool forward);
    std::vector<std::pair<int, int>> findAllRegexMatches(const std::string& text, const std::string& pattern, bool caseSensitive);
    
    // Internal helpers
    void splitIntoLines(const std::string& text);
    std::string joinLines() const;
    void setModified(bool modified);
    void ensureValidCursorPosition();
    std::string getCurrentLine() const;
    char getCharacterAt(int line, int column) const;
    bool isWordCharacter(char c) const;
    std::string getCurrentWord() const;
    std::pair<int, int> findWordBoundary(int line, int column) const;
    
    // Settings helpers
    void loadSettings();
    void saveSettings();
    void applySettings();
};

} // namespace ai_editor 