#include "SimpleEditorWindow.h"
#include <imgui.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace ai_editor {

SimpleEditorWindow::SimpleEditorWindow() {
    // Initialize with an empty document
    lines_.push_back("");
}

SimpleEditorWindow::~SimpleEditorWindow() {
    // Nothing to clean up currently
}

bool SimpleEditorWindow::initialize() {
    std::cout << "SimpleEditorWindow initialized successfully" << std::endl;
    return true;
}

void SimpleEditorWindow::render(bool* p_open) {
    if (p_open && !*p_open) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Text Editor", p_open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);
    
    renderMenuBar();
    renderEditor();
    renderStatusBar();
    
    ImGui::End();
}

void SimpleEditorWindow::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                // TODO: Implement file dialog
                // For now, just load a hardcoded file
                loadFile("example.txt");
            }
            
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (!filename_.empty()) {
                    saveFile(filename_);
                } else {
                    // TODO: Implement save as dialog
                    saveFile("example.txt");
                }
            }
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                // TODO: Implement save as dialog
                saveFile("example.txt");
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                // User wants to quit
                if (p_open) *p_open = false;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            // TODO: Implement these actions
            ImGui::MenuItem("Cut", "Ctrl+X");
            ImGui::MenuItem("Copy", "Ctrl+C");
            ImGui::MenuItem("Paste", "Ctrl+V");
            ImGui::Separator();
            ImGui::MenuItem("Find", "Ctrl+F");
            ImGui::MenuItem("Replace", "Ctrl+H");
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Line Numbers", NULL, &showLineNumbers_);
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void SimpleEditorWindow::renderEditor() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    float lineHeight = ImGui::GetTextLineHeight();
    float cellWidth = ImGui::GetTextLineHeight() * 0.7f;
    float lineNumberWidth = 50.0f;
    
    // Create a child window for the editor area
    if (ImGui::BeginChild("EditorContent", ImVec2(0, -lineHeight * 1.5f), true)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        
        // Draw each line
        for (int i = 0; i < lines_.size(); i++) {
            // Draw line number if enabled
            if (showLineNumbers_) {
                ImGui::Text("%4d", i + 1);
                ImGui::SameLine();
                ImGui::SameLine(lineNumberWidth);
            }
            
            // Draw line content
            ImGui::Text("%s", lines_[i].c_str());
        }
        
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    
    ImGui::PopStyleVar();
}

void SimpleEditorWindow::renderStatusBar() {
    ImGui::Separator();
    ImGui::Text("Line: %d, Col: %d | %s", 
                cursorLine_ + 1, 
                cursorColumn_ + 1,
                filename_.empty() ? "Untitled" : filename_.c_str());
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    if (isModified_) {
        ImGui::Text("Modified");
    }
}

bool SimpleEditorWindow::loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    setText(buffer.str());
    filename_ = filename;
    isModified_ = false;
    
    std::cout << "Loaded file: " << filename << std::endl;
    return true;
}

bool SimpleEditorWindow::saveFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    std::string content = joinLines();
    file << content;
    file.close();
    
    filename_ = filename;
    isModified_ = false;
    
    std::cout << "Saved file: " << filename << std::endl;
    return true;
}

void SimpleEditorWindow::setText(const std::string& text) {
    splitIntoLines(text);
    cursorLine_ = 0;
    cursorColumn_ = 0;
    isModified_ = false;
}

std::string SimpleEditorWindow::getText() const {
    return joinLines();
}

void SimpleEditorWindow::splitIntoLines(const std::string& text) {
    lines_.clear();
    
    std::stringstream ss(text);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines_.push_back(line);
    }
    
    // Ensure there's at least one line
    if (lines_.empty()) {
        lines_.push_back("");
    }
}

std::string SimpleEditorWindow::joinLines() const {
    std::stringstream ss;
    
    for (size_t i = 0; i < lines_.size(); i++) {
        ss << lines_[i];
        // Add newline for all lines except the last one
        if (i < lines_.size() - 1) {
            ss << "\n";
        }
    }
    
    return ss.str();
}

} // namespace ai_editor 