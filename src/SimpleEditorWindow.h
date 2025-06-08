#pragma once

#include <string>
#include <vector>
#include <imgui.h>

namespace ai_editor {

class SimpleEditorWindow {
public:
    SimpleEditorWindow();
    ~SimpleEditorWindow();

    // Initialize the editor window
    bool initialize();
    
    // Render the editor window
    void render(bool* p_open = nullptr);
    
    // File operations
    bool loadFile(const std::string& filename);
    bool saveFile(const std::string& filename);
    
    // Set content directly
    void setText(const std::string& text);
    std::string getText() const;

private:
    // Editor content
    std::vector<std::string> lines_;
    std::string filename_;
    bool isModified_ = false;
    
    // Editor state
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
    bool showLineNumbers_ = true;
    
    // UI helpers
    void renderMenuBar();
    void renderEditor();
    void renderStatusBar();
    
    // Helper methods
    void splitIntoLines(const std::string& text);
    std::string joinLines() const;
};

} // namespace ai_editor 