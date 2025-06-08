#include "EnhancedEditorWindow.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

namespace ai_editor {

EnhancedEditorWindow::EnhancedEditorWindow() {
    // Initialize with an empty document
    lines_.push_back("");
    
    // Initialize languages
    initializeLanguages();
}

EnhancedEditorWindow::~EnhancedEditorWindow() {
}

bool EnhancedEditorWindow::initialize() {
    // Additional initialization if needed
    return true;
}

void EnhancedEditorWindow::render(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    
    if (!ImGui::Begin("AI-Enhanced Text Editor", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    
    // Render menu bar
    renderMenuBar();
    
    // Calculate the panel sizes based on the window width
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float editorWidth = windowWidth * editorPanelWidth_;
    float assistantWidth = windowWidth - editorWidth - ImGui::GetStyle().ItemSpacing.x;
    
    // Create a split layout with editor on the left and AI panel on the right
    ImGui::BeginChild("EditorPanel", ImVec2(editorWidth, 0), true);
    renderEditor();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    if (showAIPanel_) {
        ImGui::BeginChild("AIPanel", ImVec2(assistantWidth, 0), true);
        renderAIAssistantPanel();
        ImGui::EndChild();
    }
    
    // Render status bar at the bottom
    renderStatusBar();
    
    ImGui::End();
}

bool EnhancedEditorWindow::loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    setText(buffer.str());
    filename_ = filename;
    
    // Detect language based on file extension
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = filename.substr(dotPos + 1);
        if (ext == "cpp" || ext == "h" || ext == "hpp") {
            setLanguage("cpp");
        } else if (ext == "py") {
            setLanguage("python");
        } else if (ext == "js") {
            setLanguage("javascript");
        } else {
            setLanguage("text");
        }
    }
    
    setModified(false);
    
    return true;
}

bool EnhancedEditorWindow::saveFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content = joinLines();
    file << content;
    file.close();
    
    filename_ = filename;
    setModified(false);
    
    return true;
}

bool EnhancedEditorWindow::saveCurrentFile() {
    if (filename_.empty()) {
        return false;
    }
    return saveFile(filename_);
}

void EnhancedEditorWindow::setText(const std::string& text) {
    splitIntoLines(text);
    cursorLine_ = 0;
    cursorColumn_ = 0;
    hasSelection_ = false;
    setModified(false);
}

std::string EnhancedEditorWindow::getText() const {
    return joinLines();
}

void EnhancedEditorWindow::setLanguage(const std::string& language) {
    if (languages_.find(language) != languages_.end()) {
        currentLanguage_ = language;
    } else {
        currentLanguage_ = "text"; // Default to plain text
    }
}

void EnhancedEditorWindow::splitIntoLines(const std::string& text) {
    lines_.clear();
    std::istringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines_.push_back(line);
    }
    
    if (lines_.empty()) {
        lines_.push_back("");
    }
}

std::string EnhancedEditorWindow::joinLines() const {
    std::stringstream result;
    
    for (size_t i = 0; i < lines_.size(); ++i) {
        result << lines_[i];
        if (i < lines_.size() - 1) {
            result << "\n";
        }
    }
    
    return result.str();
}

void EnhancedEditorWindow::setModified(bool modified) {
    isModified_ = modified;
}

std::string EnhancedEditorWindow::getSelectedText() const {
    if (!hasSelection_) {
        return "";
    }
    
    std::stringstream result;
    
    for (int line = selectionStartLine_; line <= selectionEndLine_; ++line) {
        int startCol = (line == selectionStartLine_) ? selectionStartCol_ : 0;
        int endCol = (line == selectionEndLine_) ? selectionEndCol_ : static_cast<int>(lines_[line].length());
        
        if (line < lines_.size()) {
            result << lines_[line].substr(startCol, endCol - startCol);
            if (line < selectionEndLine_) {
                result << "\n";
            }
        }
    }
    
    return result.str();
}

void EnhancedEditorWindow::handleFileDropped(const std::string& filename) {
    loadFile(filename);
}

void EnhancedEditorWindow::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                setText("");
                filename_.clear();
            }
            
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                // Open file dialog would go here
                // For now, we'll mock loading a sample file
                loadFile("sample.cpp");
            }
            
            if (ImGui::MenuItem("Save", "Ctrl+S", false, !filename_.empty())) {
                saveCurrentFile();
            }
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                // Save As dialog would go here
                // For now, we'll mock saving to a hardcoded filename
                saveFile("sample_saved.cpp");
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                if (p_open) *p_open = false;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {} // Not implemented yet
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {} // Not implemented yet
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Cut", "Ctrl+X", false, hasSelection_)) {
                // Cut implementation
            }
            
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection_)) {
                // Copy implementation
            }
            
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                // Paste implementation
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                // Select all implementation
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Line Numbers", nullptr, &showLineNumbers_);
            ImGui::MenuItem("Syntax Highlighting", nullptr, &enableSyntaxHighlighting_);
            ImGui::MenuItem("Show AI Assistant", nullptr, &showAIPanel_);
            
            ImGui::Separator();
            
            if (ImGui::BeginMenu("Language")) {
                if (ImGui::MenuItem("C++", nullptr, currentLanguage_ == "cpp")) {
                    setLanguage("cpp");
                }
                if (ImGui::MenuItem("Python", nullptr, currentLanguage_ == "python")) {
                    setLanguage("python");
                }
                if (ImGui::MenuItem("JavaScript", nullptr, currentLanguage_ == "javascript")) {
                    setLanguage("javascript");
                }
                if (ImGui::MenuItem("Plain Text", nullptr, currentLanguage_ == "text")) {
                    setLanguage("text");
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("AI")) {
            if (ImGui::MenuItem("Ask Question", "Ctrl+Space")) {
                // Focus on AI input field
            }
            
            if (ImGui::MenuItem("Generate Code", "Ctrl+G")) {
                sendQueryToAssistant("Generate a function to calculate the Fibonacci sequence");
            }
            
            if (ImGui::MenuItem("Explain Selection", nullptr, false, hasSelection_)) {
                std::string selectedText = getSelectedText();
                if (!selectedText.empty()) {
                    sendQueryToAssistant("Explain this code: " + selectedText);
                }
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void EnhancedEditorWindow::renderEditor() {
    // Line numbers column width calculation
    float lineNumberWidth = 0.0f;
    if (showLineNumbers_) {
        int lineCount = static_cast<int>(lines_.size());
        int digits = 1;
        while (lineCount >= 10) {
            lineCount /= 10;
            digits++;
        }
        lineNumberWidth = ImGui::CalcTextSize("0").x * (digits + 1);
    }
    
    // Editor area
    ImGui::BeginChild("LineNumbers", ImVec2(lineNumberWidth, 0), false);
    
    float lineHeight = ImGui::GetTextLineHeight();
    ImVec2 cursorStartPos = ImGui::GetCursorScreenPos();
    
    for (int i = 0; i < static_cast<int>(lines_.size()); i++) {
        ImGui::Text("%*d", 3, i + 1);
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("EditorContent", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Handle keyboard input for cursor movement and text editing
    handleTextInput();
    
    // Render each line with syntax highlighting
    for (int i = 0; i < static_cast<int>(lines_.size()); i++) {
        if (enableSyntaxHighlighting_ && currentLanguage_ != "text") {
            renderLineWithSyntaxHighlighting(lines_[i], i);
        } else {
            // Render without syntax highlighting
            ImGui::TextUnformatted(lines_[i].c_str());
        }
        
        // Highlight the current line
        if (i == cursorLine_) {
            ImVec2 lineStart = ImGui::GetItemRectMin();
            ImVec2 lineEnd = ImGui::GetItemRectMax();
            ImVec2 windowPos = ImGui::GetWindowPos();
            lineStart.x = windowPos.x;
            lineEnd.x = windowPos.x + ImGui::GetWindowWidth();
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(lineStart, lineEnd, ImColor(0.2f, 0.2f, 0.2f, 0.3f));
        }
    }
    
    ImGui::EndChild();
}

void EnhancedEditorWindow::renderStatusBar() {
    ImGui::Separator();
    
    ImGui::Text("Line: %d, Col: %d | %s | %s%s", 
                cursorLine_ + 1, 
                cursorColumn_ + 1, 
                currentLanguage_.c_str(),
                filename_.empty() ? "Untitled" : filename_.c_str(),
                isModified_ ? " *" : "");
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 150);
    
    float splitRatio = editorPanelWidth_ * 100.0f;
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##Split", &splitRatio, 10.0f, 90.0f, "%.0f%%")) {
        editorPanelWidth_ = splitRatio / 100.0f;
    }
}

void EnhancedEditorWindow::handleTextInput() {
    // Process key input for text editing
    ImGuiIO& io = ImGui::GetIO();
    
    // Handle common keyboard shortcuts
    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;
    
    // Cursor movement keys
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        handleKeyPress(ImGuiKey_LeftArrow, shift, ctrl);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        handleKeyPress(ImGuiKey_RightArrow, shift, ctrl);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        handleKeyPress(ImGuiKey_UpArrow, shift, ctrl);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        handleKeyPress(ImGuiKey_DownArrow, shift, ctrl);
    }
    
    // Text editing keys
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        deleteCharacterAtCursor(true);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        deleteCharacterAtCursor(false);
    }
    
    // Handle character input
    if (!io.KeyCtrl && !io.KeyAlt) {
        for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
            char c = (char)io.InputQueueCharacters[i];
            if (c != 0 && (c == '\n' || c >= 32)) {
                insertCharacterAtCursor(c);
            }
        }
    }
    
    // File operations shortcuts
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        saveCurrentFile();
    }
    
    // AI Assistant shortcut
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Space)) {
        // Focus on AI input field
    }
}

void EnhancedEditorWindow::insertCharacterAtCursor(char c) {
    if (c == '\n') {
        // Handle new line
        std::string currentLine = lines_[cursorLine_];
        std::string rightPart = cursorColumn_ < currentLine.length() ? 
                                currentLine.substr(cursorColumn_) : "";
        
        lines_[cursorLine_] = currentLine.substr(0, cursorColumn_);
        lines_.insert(lines_.begin() + cursorLine_ + 1, rightPart);
        
        cursorLine_++;
        cursorColumn_ = 0;
    } else {
        // Insert character at cursor position
        if (cursorLine_ >= lines_.size()) {
            lines_.push_back("");
        }
        
        if (cursorColumn_ > lines_[cursorLine_].length()) {
            cursorColumn_ = static_cast<int>(lines_[cursorLine_].length());
        }
        
        lines_[cursorLine_].insert(cursorColumn_, 1, c);
        cursorColumn_++;
    }
    
    setModified(true);
}

void EnhancedEditorWindow::deleteCharacterAtCursor(bool isBackspace) {
    if (isBackspace) {
        // Backspace: delete character before cursor
        if (cursorColumn_ > 0) {
            lines_[cursorLine_].erase(cursorColumn_ - 1, 1);
            cursorColumn_--;
            setModified(true);
        } else if (cursorLine_ > 0) {
            // At beginning of line, merge with previous line
            cursorColumn_ = static_cast<int>(lines_[cursorLine_ - 1].length());
            lines_[cursorLine_ - 1] += lines_[cursorLine_];
            lines_.erase(lines_.begin() + cursorLine_);
            cursorLine_--;
            setModified(true);
        }
    } else {
        // Delete: delete character at cursor
        if (cursorColumn_ < lines_[cursorLine_].length()) {
            lines_[cursorLine_].erase(cursorColumn_, 1);
            setModified(true);
        } else if (cursorLine_ < lines_.size() - 1) {
            // At end of line, merge with next line
            lines_[cursorLine_] += lines_[cursorLine_ + 1];
            lines_.erase(lines_.begin() + cursorLine_ + 1);
            setModified(true);
        }
    }
}

void EnhancedEditorWindow::handleKeyPress(ImGuiKey key, bool shift, bool ctrl) {
    switch (key) {
        case ImGuiKey_LeftArrow:
            if (ctrl) {
                // Move to the beginning of the word
                while (cursorColumn_ > 0 && !isalnum(lines_[cursorLine_][cursorColumn_ - 1])) {
                    cursorColumn_--;
                }
                while (cursorColumn_ > 0 && isalnum(lines_[cursorLine_][cursorColumn_ - 1])) {
                    cursorColumn_--;
                }
            } else {
                if (cursorColumn_ > 0) {
                    cursorColumn_--;
                } else if (cursorLine_ > 0) {
                    cursorLine_--;
                    cursorColumn_ = static_cast<int>(lines_[cursorLine_].length());
                }
            }
            break;
            
        case ImGuiKey_RightArrow:
            if (ctrl) {
                // Move to the end of the word
                while (cursorColumn_ < lines_[cursorLine_].length() && !isalnum(lines_[cursorLine_][cursorColumn_])) {
                    cursorColumn_++;
                }
                while (cursorColumn_ < lines_[cursorLine_].length() && isalnum(lines_[cursorLine_][cursorColumn_])) {
                    cursorColumn_++;
                }
            } else {
                if (cursorColumn_ < lines_[cursorLine_].length()) {
                    cursorColumn_++;
                } else if (cursorLine_ < lines_.size() - 1) {
                    cursorLine_++;
                    cursorColumn_ = 0;
                }
            }
            break;
            
        case ImGuiKey_UpArrow:
            if (cursorLine_ > 0) {
                cursorLine_--;
                cursorColumn_ = std::min(cursorColumn_, static_cast<int>(lines_[cursorLine_].length()));
            }
            break;
            
        case ImGuiKey_DownArrow:
            if (cursorLine_ < lines_.size() - 1) {
                cursorLine_++;
                cursorColumn_ = std::min(cursorColumn_, static_cast<int>(lines_[cursorLine_].length()));
            }
            break;
    }
}

void EnhancedEditorWindow::initializeLanguages() {
    // C++ language definition
    SyntaxHighlightingLanguage cpp;
    cpp.name = "cpp";
    cpp.lineCommentStart = "//";
    cpp.blockComment = std::make_pair("/*", "*/");
    
    // Keywords
    const char* cppKeywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double",
        "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
        "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
        "union", "unsigned", "void", "volatile", "while", "class", "namespace", "template",
        "new", "this", "delete", "public", "protected", "private", "virtual", "friend",
        "inline", "explicit", "operator", "bool", "try", "catch", "throw", "using", "true",
        "false", "nullptr"
    };
    
    for (const char* keyword : cppKeywords) {
        cpp.keywords[keyword] = ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Blue for keywords
    }
    
    // Rules
    cpp.rules.push_back({R"(//.*$)", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), true}); // Line comments
    cpp.rules.push_back({R"(/\*.*?\*/)", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), true}); // Block comments
    cpp.rules.push_back({R"(".*?")", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Strings
    cpp.rules.push_back({R"('.'|'\\.')", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Char literals
    cpp.rules.push_back({R"(\b[0-9]+\b)", ImVec4(0.0f, 0.7f, 0.7f, 1.0f), true}); // Numbers
    cpp.rules.push_back({R"(\b[A-Z_][A-Z0-9_]+\b)", ImVec4(0.5f, 0.7f, 0.5f, 1.0f), true}); // Constants
    cpp.rules.push_back({R"(\w+\s*\()", ImVec4(0.8f, 0.8f, 0.0f, 1.0f), true}); // Functions
    
    // Set the default color for C++
    cpp.defaultColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Almost white
    
    languages_["cpp"] = cpp;
    
    // Python language definition
    SyntaxHighlightingLanguage python;
    python.name = "python";
    python.lineCommentStart = "#";
    
    // Keywords
    const char* pythonKeywords[] = {
        "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else",
        "except", "False", "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "None", "nonlocal", "not", "or", "pass", "raise", "return", "True", "try",
        "while", "with", "yield"
    };
    
    for (const char* keyword : pythonKeywords) {
        python.keywords[keyword] = ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Blue for keywords
    }
    
    // Rules
    python.rules.push_back({R"(#.*$)", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), true}); // Comments
    python.rules.push_back({R"(""".*?""")", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), true}); // Doc strings
    python.rules.push_back({R"(".*?")", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Strings
    python.rules.push_back({R"('.*?')", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Strings
    python.rules.push_back({R"(\b[0-9]+\b)", ImVec4(0.0f, 0.7f, 0.7f, 1.0f), true}); // Numbers
    python.rules.push_back({R"(\b[A-Z_][A-Z0-9_]+\b)", ImVec4(0.5f, 0.7f, 0.5f, 1.0f), true}); // Constants
    python.rules.push_back({R"(def\s+(\w+))", ImVec4(0.8f, 0.8f, 0.0f, 1.0f), true}); // Function definitions
    
    // Set the default color for Python
    python.defaultColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Almost white
    
    languages_["python"] = python;
    
    // JavaScript language definition
    SyntaxHighlightingLanguage javascript;
    javascript.name = "javascript";
    javascript.lineCommentStart = "//";
    javascript.blockComment = std::make_pair("/*", "*/");
    
    // Keywords
    const char* jsKeywords[] = {
        "break", "case", "catch", "class", "const", "continue", "debugger", "default",
        "delete", "do", "else", "export", "extends", "false", "finally", "for", "function",
        "if", "import", "in", "instanceof", "new", "null", "return", "super", "switch",
        "this", "throw", "true", "try", "typeof", "var", "void", "while", "with", "let",
        "static", "yield", "async", "await"
    };
    
    for (const char* keyword : jsKeywords) {
        javascript.keywords[keyword] = ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Blue for keywords
    }
    
    // Rules
    javascript.rules.push_back({R"(//.*$)", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), true}); // Line comments
    javascript.rules.push_back({R"(/\*.*?\*/)", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), true}); // Block comments
    javascript.rules.push_back({R"(".*?")", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Double-quoted strings
    javascript.rules.push_back({R"('.*?')", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Single-quoted strings
    javascript.rules.push_back({R"(`.*?`)", ImVec4(0.9f, 0.6f, 0.0f, 1.0f), true}); // Template strings
    javascript.rules.push_back({R"(\b[0-9]+\b)", ImVec4(0.0f, 0.7f, 0.7f, 1.0f), true}); // Numbers
    javascript.rules.push_back({R"(function\s+(\w+))", ImVec4(0.8f, 0.8f, 0.0f, 1.0f), true}); // Function definitions
    javascript.rules.push_back({R"(\b[A-Z_][A-Z0-9_]+\b)", ImVec4(0.5f, 0.7f, 0.5f, 1.0f), true}); // Constants
    
    // Set the default color for JavaScript
    javascript.defaultColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Almost white
    
    languages_["javascript"] = javascript;
    
    // Plain text (default)
    SyntaxHighlightingLanguage text;
    text.name = "text";
    text.defaultColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Almost white
    languages_["text"] = text;
    
    // Set default language
    currentLanguage_ = "text";
}

void EnhancedEditorWindow::renderLineWithSyntaxHighlighting(const std::string& line, int lineNumber) {
    if (languages_.find(currentLanguage_) == languages_.end()) {
        ImGui::TextUnformatted(line.c_str());
        return;
    }
    
    const SyntaxHighlightingLanguage& lang = languages_[currentLanguage_];
    
    // Simple approach: Break the line into colored spans
    struct ColoredSpan {
        int start;
        int end;
        ImVec4 color;
    };
    
    std::vector<ColoredSpan> spans;
    
    // Apply keyword coloring
    for (const auto& kw : lang.keywords) {
        size_t pos = 0;
        while ((pos = line.find(kw.first, pos)) != std::string::npos) {
            // Make sure it's a whole word
            bool isWordStart = (pos == 0 || !isalnum(line[pos - 1]));
            bool isWordEnd = (pos + kw.first.length() == line.length() || 
                             !isalnum(line[pos + kw.first.length()]));
                             
            if (isWordStart && isWordEnd) {
                spans.push_back({static_cast<int>(pos), static_cast<int>(pos + kw.first.length()), kw.second});
            }
            pos += kw.first.length();
        }
    }
    
    // Apply regex-based rules
    for (const auto& rule : lang.rules) {
        if (rule.isRegex) {
            try {
                std::regex regex(rule.pattern);
                auto words_begin = std::sregex_iterator(line.begin(), line.end(), regex);
                auto words_end = std::sregex_iterator();
                
                for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                    std::smatch match = *i;
                    spans.push_back({
                        static_cast<int>(match.position()), 
                        static_cast<int>(match.position() + match.length()), 
                        rule.color
                    });
                }
            } catch (const std::regex_error&) {
                // Ignore regex errors
            }
        }
    }
    
    // Sort spans by start position
    std::sort(spans.begin(), spans.end(), [](const ColoredSpan& a, const ColoredSpan& b) {
        return a.start < b.start;
    });
    
    // Merge overlapping spans (prioritizing later rules)
    for (size_t i = 0; i < spans.size(); i++) {
        for (size_t j = i + 1; j < spans.size(); j++) {
            if (spans[j].start <= spans[i].end) {
                if (spans[j].end <= spans[i].end) {
                    // j is fully contained in i, remove j
                    spans.erase(spans.begin() + j);
                    j--;
                } else {
                    // Partial overlap, truncate i
                    spans[i].end = spans[j].start;
                }
            }
        }
    }
    
    // Render with the determined colors
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    int lastPos = 0;
    for (const auto& span : spans) {
        if (span.start > lastPos) {
            // Render text between colored spans
            ImGui::TextColored(lang.defaultColor, "%.*s", span.start - lastPos, line.c_str() + lastPos);
            ImGui::SameLine(0.0f, 0.0f);
        }
        
        // Render colored span
        ImGui::TextColored(span.color, "%.*s", span.end - span.start, line.c_str() + span.start);
        ImGui::SameLine(0.0f, 0.0f);
        
        lastPos = span.end;
    }
    
    // Render remaining text
    if (lastPos < line.length()) {
        ImGui::TextColored(lang.defaultColor, "%s", line.c_str() + lastPos);
    } else if (lastPos == 0) {
        // Empty or uncolored line
        ImGui::TextColored(lang.defaultColor, "%s", line.c_str());
    }
    
    ImGui::PopStyleVar();
}

void EnhancedEditorWindow::renderAIAssistantPanel() {
    ImGui::Text("AI Assistant");
    ImGui::Separator();
    
    // Chat history display
    ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 4), true);
    
    for (const auto& chat : aiState_.chatHistory) {
        // User query
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
        ImGui::TextWrapped("You: %s", chat.first.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // AI response
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::TextWrapped("AI: %s", chat.second.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Separator();
    }
    
    // Show "thinking" animation if the AI is processing
    if (aiState_.isThinking) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        
        // Animate dots based on time
        float time = ImGui::GetTime();
        int dots = (int)(fmodf(time, 1.0f) * 3.0f);
        std::string thinking = "AI is thinking";
        for (int i = 0; i < dots; i++) {
            thinking += ".";
        }
        ImGui::TextWrapped("%s", thinking.c_str());
        
        ImGui::PopStyleColor();
    }
    
    ImGui::SetScrollHereY(1.0f); // Auto-scroll to bottom
    ImGui::EndChild();
    
    // Suggestions section (if any)
    if (!aiState_.suggestions.empty()) {
        ImGui::Text("Suggestions:");
        ImGui::BeginChild("Suggestions", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2), true);
        
        for (size_t i = 0; i < aiState_.suggestions.size(); i++) {
            if (ImGui::Selectable(aiState_.suggestions[i].c_str(), false)) {
                applyAssistantSuggestion(static_cast<int>(i));
            }
        }
        
        ImGui::EndChild();
    }
    
    // Input box
    ImGui::Text("Ask the AI:");
    ImGui::SetNextItemWidth(-1.0f);
    bool inputSubmitted = ImGui::InputText("##AIInput", aiState_.inputBuffer, sizeof(aiState_.inputBuffer), 
                                           ImGuiInputTextFlags_EnterReturnsTrue);
    
    ImGui::SameLine();
    if (ImGui::Button("Send") || inputSubmitted) {
        if (strlen(aiState_.inputBuffer) > 0) {
            sendQueryToAssistant(aiState_.inputBuffer);
            aiState_.inputBuffer[0] = '\0'; // Clear input
        }
    }
}

void EnhancedEditorWindow::sendQueryToAssistant(const std::string& query) {
    if (query.empty()) {
        return;
    }
    
    aiState_.currentQuery = query;
    aiState_.isThinking = true;
    
    // In a real implementation, this would send the query to an actual AI service
    // For the mock, we'll generate a response after a short delay
    
    // In a real implementation, you would have a separate thread or async call
    // For this demo, we'll just pretend to think and immediately generate a response
    generateMockAIResponse(query);
}

void EnhancedEditorWindow::generateMockAIResponse(const std::string& query) {
    // Store the query in chat history
    aiState_.chatHistory.push_back(std::make_pair(query, "")); // Empty response initially
    
    // Generate a mock response based on the query
    std::string response;
    std::vector<std::string> suggestions;
    
    // Lowercase the query for easier pattern matching
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    if (lowerQuery.find("hello") != std::string::npos || 
        lowerQuery.find("hi") != std::string::npos) {
        response = "Hello! I'm your AI coding assistant. How can I help you with your code today?";
    }
    else if (lowerQuery.find("fibonacci") != std::string::npos) {
        response = "Here's a simple function to calculate Fibonacci numbers:";
        
        if (currentLanguage_ == "cpp") {
            suggestions.push_back(
                "int fibonacci(int n) {\n"
                "    if (n <= 1) return n;\n"
                "    return fibonacci(n-1) + fibonacci(n-2);\n"
                "}"
            );
            
            suggestions.push_back(
                "// Iterative solution (more efficient)\n"
                "int fibonacci(int n) {\n"
                "    if (n <= 1) return n;\n"
                "    int prev = 0, curr = 1;\n"
                "    for (int i = 2; i <= n; i++) {\n"
                "        int next = prev + curr;\n"
                "        prev = curr;\n"
                "        curr = next;\n"
                "    }\n"
                "    return curr;\n"
                "}"
            );
        } else if (currentLanguage_ == "python") {
            suggestions.push_back(
                "def fibonacci(n):\n"
                "    if n <= 1:\n"
                "        return n\n"
                "    return fibonacci(n-1) + fibonacci(n-2)"
            );
            
            suggestions.push_back(
                "# Iterative solution (more efficient)\n"
                "def fibonacci(n):\n"
                "    if n <= 1:\n"
                "        return n\n"
                "    prev, curr = 0, 1\n"
                "    for i in range(2, n+1):\n"
                "        prev, curr = curr, prev + curr\n"
                "    return curr"
            );
        } else if (currentLanguage_ == "javascript") {
            suggestions.push_back(
                "function fibonacci(n) {\n"
                "    if (n <= 1) return n;\n"
                "    return fibonacci(n-1) + fibonacci(n-2);\n"
                "}"
            );
            
            suggestions.push_back(
                "// Iterative solution (more efficient)\n"
                "function fibonacci(n) {\n"
                "    if (n <= 1) return n;\n"
                "    let prev = 0, curr = 1;\n"
                "    for (let i = 2; i <= n; i++) {\n"
                "        const next = prev + curr;\n"
                "        prev = curr;\n"
                "        curr = next;\n"
                "    }\n"
                "    return curr;\n"
                "}"
            );
        }
    }
    else if (lowerQuery.find("explain") != std::string::npos) {
        if (lowerQuery.find("loop") != std::string::npos || 
            lowerQuery.find("for") != std::string::npos) {
            response = "Loops are control flow structures that allow you to repeat a block of code multiple times.\n\n"
                      "Common loop types:\n"
                      "- For loops: Execute a block of code a specific number of times\n"
                      "- While loops: Execute a block of code as long as a condition is true\n"
                      "- Do-while loops: Similar to while loops but always execute at least once\n\n"
                      "Loops are essential for iterating over collections, processing data, or implementing algorithms that require repetition.";
        } else {
            response = "I'd be happy to explain this code or concept. Could you provide more specific details about what you'd like me to explain?";
        }
    }
    else if (lowerQuery.find("sort") != std::string::npos) {
        response = "Here's a quick implementation of a sorting algorithm:";
        
        if (currentLanguage_ == "cpp") {
            suggestions.push_back(
                "// Bubble sort implementation\n"
                "void bubbleSort(int arr[], int n) {\n"
                "    for (int i = 0; i < n-1; i++) {\n"
                "        for (int j = 0; j < n-i-1; j++) {\n"
                "            if (arr[j] > arr[j+1]) {\n"
                "                // Swap elements\n"
                "                int temp = arr[j];\n"
                "                arr[j] = arr[j+1];\n"
                "                arr[j+1] = temp;\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "}"
            );
        } else if (currentLanguage_ == "python") {
            suggestions.push_back(
                "# Bubble sort implementation\n"
                "def bubble_sort(arr):\n"
                "    n = len(arr)\n"
                "    for i in range(n):\n"
                "        for j in range(0, n-i-1):\n"
                "            if arr[j] > arr[j+1]:\n"
                "                # Swap elements\n"
                "                arr[j], arr[j+1] = arr[j+1], arr[j]\n"
                "    return arr"
            );
        } else if (currentLanguage_ == "javascript") {
            suggestions.push_back(
                "// Bubble sort implementation\n"
                "function bubbleSort(arr) {\n"
                "    const n = arr.length;\n"
                "    for (let i = 0; i < n; i++) {\n"
                "        for (let j = 0; j < n-i-1; j++) {\n"
                "            if (arr[j] > arr[j+1]) {\n"
                "                // Swap elements\n"
                "                [arr[j], arr[j+1]] = [arr[j+1], arr[j]];\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "    return arr;\n"
                "}"
            );
        }
    }
    else {
        response = "I understand you're asking about \"" + query + "\". How can I assist you with this? " +
                  "I can help with code generation, explanations, debugging, or providing examples.";
    }
    
    // Update the chat history with the generated response
    aiState_.chatHistory.back().second = response;
    aiState_.isThinking = false;
    aiState_.suggestions = suggestions;
}

void EnhancedEditorWindow::applyAssistantSuggestion(int suggestionIndex) {
    if (suggestionIndex < 0 || suggestionIndex >= aiState_.suggestions.size()) {
        return;
    }
    
    // Get the suggestion code
    std::string code = aiState_.suggestions[suggestionIndex];
    
    // Insert the code at the current cursor position
    splitIntoLines(code);
    
    // Store suggestions in a temporary variable
    std::vector<std::string> suggestionLines;
    std::swap(suggestionLines, lines_);
    
    // Insert the suggestions at the current cursor position
    std::vector<std::string> oldLines = lines_;
    lines_.clear();
    
    // Add lines before cursor
    for (int i = 0; i < cursorLine_; i++) {
        lines_.push_back(oldLines[i]);
    }
    
    // Add the beginning of the current line
    std::string currentLine = oldLines[cursorLine_];
    std::string beforeCursor = cursorColumn_ <= currentLine.length() ? 
                              currentLine.substr(0, cursorColumn_) : currentLine;
    
    // Add first suggestion line to the current line
    if (!suggestionLines.empty()) {
        lines_.push_back(beforeCursor + suggestionLines[0]);
        
        // Add the rest of the suggestion lines
        for (size_t i = 1; i < suggestionLines.size(); i++) {
            lines_.push_back(suggestionLines[i]);
        }
    } else {
        lines_.push_back(beforeCursor);
    }
    
    // Add the rest of the current line
    if (cursorColumn_ < currentLine.length()) {
        std::string afterCursor = currentLine.substr(cursorColumn_);
        lines_.back() += afterCursor;
    }
    
    // Add remaining lines
    for (size_t i = cursorLine_ + 1; i < oldLines.size(); i++) {
        lines_.push_back(oldLines[i]);
    }
    
    // Update cursor position
    cursorLine_ += suggestionLines.size() - 1;
    if (suggestionLines.empty()) {
        // No change
    } else if (suggestionLines.size() == 1) {
        cursorColumn_ += suggestionLines[0].length();
    } else {
        cursorColumn_ = static_cast<int>(suggestionLines.back().length());
    }
    
    // Mark as modified
    setModified(true);
    
    // Clear suggestions after applying
    aiState_.suggestions.clear();
}

} // namespace ai_editor 