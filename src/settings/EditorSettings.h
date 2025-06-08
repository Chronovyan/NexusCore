#pragma once

#include <string>
#include <imgui.h>

namespace ai_editor {

/**
 * @class EditorSettings
 * @brief Manages all editor settings and configurations
 */
class EditorSettings {
public:
    // General settings
    bool showLineNumbers = true;
    bool enableSyntaxHighlighting = true;
    bool enableAutoIndent = true;
    bool enableWordWrap = false;
    bool showFoldingMarkers = true;
    bool enableAutoComplete = true;
    
    // Colors
    ImVec4 currentLineBackgroundColor = ImVec4(0.3f, 0.3f, 0.3f, 0.3f);
    ImVec4 selectedTextBackgroundColor = ImVec4(0.2f, 0.4f, 0.8f, 0.5f);
    
    // Display
    float tabSize = 4.0f;
    float fontSize = 14.0f;
    std::string fontName = "Consolas";
    
    // Editor behavior
    bool autoSave = false;
    int autoSaveInterval = 300; // seconds
    bool showWhitespace = false;
    bool showLineEndings = false;
    
    // Window state
    bool windowMaximized = false;
    ImVec2 windowSize = ImVec2(1280, 720);
    ImVec2 windowPos = ImVec2(0, 0);
    
    // File handling
    std::string defaultFileExtension = "txt";
    std::string lastOpenedDirectory = "";
    std::string lastSavedDirectory = "";
    
    // Recent files
    static constexpr int MAX_RECENT_FILES = 10;
    std::vector<std::string> recentFiles;
    
    // Theme
    std::string theme = "Dark";
    bool useCustomTheme = false;
    
    /**
     * @brief Load settings from a file
     * @param filename Path to the settings file
     * @return True if settings were loaded successfully
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Save settings to a file
     * @param filename Path to the settings file
     * @return True if settings were saved successfully
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Add a file to the recent files list
     * @param filepath Path to the file to add
     */
    void addRecentFile(const std::string& filepath);
    
    /**
     * @brief Apply the current theme to ImGui
     */
    void applyTheme() const;
    
    /**
     * @brief Apply the current font settings to ImGui
     */
    void applyFont() const;
    
private:
    /**
     * @brief Load the default dark theme
     */
    void loadDarkTheme() const;
    
    /**
     * @brief Load the default light theme
     */
    void loadLightTheme() const;
    
    /**
     * @brief Load the default classic theme
     */
    void loadClassicTheme() const;
};

} // namespace ai_editor
