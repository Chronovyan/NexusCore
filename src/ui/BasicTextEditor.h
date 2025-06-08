#pragma once

#include "../TextBuffer.h"
#include <string>
#include <vector>
#include <memory>
#include <imgui.h>
#include <imgui_markdown.h>

namespace ai_editor {

class BasicTextEditor {
public:
    BasicTextEditor();
    ~BasicTextEditor();

    // Initialize the editor
    bool initialize();
    
    // Render the editor window
    void render(bool* p_open = nullptr);
    
    // File operations
    bool loadFile(const std::string& filename);
    bool saveFile(const std::string& filename);
    
    // Content management
    void setText(const std::string& text);
    std::string getText() const;
    
private:
    // Text buffer
    std::unique_ptr<TextBuffer> textBuffer_;
    
    // Editor state
    std::string filename_;
    bool isModified_ = false;
    bool showLineNumbers_ = true;
    bool wordWrap_ = true;
    
    // Cursor position
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
    
    // UI state
    ImVec4 textColor_ = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    ImVec4 backgroundColor_ = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    float fontSize_ = 16.0f;
    
    // Private methods
    void renderMenuBar();
    void renderEditor();
    void renderStatusBar();
    void handleKeyboardShortcuts();
    
    // File dialog
    void showFileDialog(bool* p_open);
    
    // Markdown renderer
    static void linkCallback(ImGui::MarkdownLinkCallbackData data);
    static ImGui::MarkdownImageData imageCallback(ImGui::MarkdownLinkCallbackData data);
};

} // namespace ai_editor
