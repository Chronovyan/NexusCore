#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <deque>
#include <imgui.h>

namespace ai_editor {

// Forward declarations
struct TextOperation;
struct EditorTheme;

class EnhancedEditorWindow {
public:
    EnhancedEditorWindow();
    ~EnhancedEditorWindow();

    // Initialize the editor window
    bool initialize();
    
    // Render the editor window
    void render(bool* p_open = nullptr);
    
    // File operations
    bool loadFile(const std::string& filename);
    bool saveFile(const std::string& filename);
    bool saveCurrentFile();
    
    // Content management
    void setText(const std::string& text);
    std::string getText() const;
    
    // Language support
    void setLanguage(const std::string& language);
    std::string getLanguage() const { return currentLanguage_; }
    
    // AI Assistant integration
    void sendQueryToAssistant(const std::string& query);
    void applyAssistantSuggestion(int suggestionIndex);

private:
    // Text content and state
    std::vector<std::string> lines_;
    std::string filename_;
    std::string currentLanguage_;
    bool isModified_ = false;
    
    // Editor state
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
    bool hasSelection_ = false;
    int selectionStartLine_ = 0;
    int selectionStartCol_ = 0;
    int selectionEndLine_ = 0;
    int selectionEndCol_ = 0;
    
    // UI state
    bool showLineNumbers_ = true;
    bool enableSyntaxHighlighting_ = true;
    bool showAIPanel_ = true;
    float editorPanelWidth_ = 0.7f; // 70% of window width
    
    // UI components rendering
    void renderMenuBar();
    void renderEditor();
    void renderStatusBar();
    void renderAIAssistantPanel();
    
    // Syntax highlighting
    struct SyntaxHighlightingRule {
        std::string pattern;
        ImVec4 color;
        bool isRegex = false;
    };
    
    struct SyntaxHighlightingLanguage {
        std::string name;
        std::vector<SyntaxHighlightingRule> rules;
        std::unordered_map<std::string, ImVec4> keywords;
        std::string lineCommentStart;
        std::pair<std::string, std::string> blockComment;
        ImVec4 defaultColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    };
    
    std::unordered_map<std::string, SyntaxHighlightingLanguage> languages_;
    void initializeLanguages();
    void renderLineWithSyntaxHighlighting(const std::string& line, int lineNumber);
    
    // Text editing helpers
    void insertCharacterAtCursor(char c);
    void deleteCharacterAtCursor(bool isBackspace = true);
    void handleTextInput();
    void handleKeyPress(ImGuiKey key, bool shift, bool ctrl);
    
    // AI Assistant state
    struct AIAssistantState {
        std::string currentQuery;
        std::deque<std::pair<std::string, std::string>> chatHistory; // <query, response> pairs
        std::vector<std::string> suggestions;
        bool isThinking = false;
        char inputBuffer[1024] = "";
    };
    
    AIAssistantState aiState_;
    void generateMockAIResponse(const std::string& query);
    
    // Helpers
    void splitIntoLines(const std::string& text);
    std::string joinLines() const;
    void setModified(bool modified);
    std::string getSelectedText() const;
    void handleFileDropped(const std::string& filename);
};

} // namespace ai_editor 