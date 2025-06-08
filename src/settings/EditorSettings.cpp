#include "EditorSettings.h"
#include <fstream>
#include <algorithm>
#include <imgui.h>

namespace ai_editor {

bool EditorSettings::loadFromFile(const std::string& filename) {
    // TODO: Implement loading from JSON or INI file
    // For now, just return true to indicate success
    return true;
}

bool EditorSettings::saveToFile(const std::string& filename) const {
    // TODO: Implement saving to JSON or INI file
    // For now, just return true to indicate success
    return true;
}

void EditorSettings::addRecentFile(const std::string& filepath) {
    // Remove the file if it already exists in the list
    auto it = std::find(recentFiles.begin(), recentFiles.end(), filepath);
    if (it != recentFiles.end()) {
        recentFiles.erase(it);
    }
    
    // Add to the beginning of the list
    recentFiles.insert(recentFiles.begin(), filepath);
    
    // Trim the list if it's too long
    if (recentFiles.size() > MAX_RECENT_FILES) {
        recentFiles.resize(MAX_RECENT_FILES);
    }
}

void EditorSettings::applyTheme() const {
    if (theme == "Dark") {
        loadDarkTheme();
    } else if (theme == "Light") {
        loadLightTheme();
    } else if (theme == "Classic") {
        loadClassicTheme();
    }
    
    // Apply any custom theme overrides if enabled
    if (useCustomTheme) {
        // TODO: Apply custom theme colors
    }
}

void EditorSettings::applyFont() const {
    // TODO: Implement font loading and application
    // This would involve using ImGui's font system
}

void EditorSettings::loadDarkTheme() const {
    ImGui::StyleColorsDark();
    
    // Customize the dark theme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    // Add more customizations as needed
}

void EditorSettings::loadLightTheme() const {
    ImGui::StyleColorsLight();
    
    // Customize the light theme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    // Add more customizations as needed
}

void EditorSettings::loadClassicTheme() const {
    ImGui::StyleColorsClassic();
    
    // Customize the classic theme if needed
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    // Add more customizations as needed
}

} // namespace ai_editor
