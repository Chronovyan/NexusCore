#include "BasicTextEditor.h"
#include "../EditorError.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace ai_editor {

BasicTextEditor::BasicTextEditor() 
    : textBuffer_(std::make_unique<TextBuffer>()) {
}

BasicTextEditor::~BasicTextEditor() {
}

bool BasicTextEditor::initialize() {
    // Initialize with an empty line
    textBuffer_->insertText(0, 0, "");
    return true;
}

void BasicTextEditor::render(bool* p_open) {
    if (p_open && !*p_open) {
        return;
    }

    // Set up the editor window
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("AI Text Editor", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    // Render menu bar
    renderMenuBar();
    
    // Handle keyboard shortcuts
    handleKeyboardShortcuts();
    
    // Render the text editor
    renderEditor();
    
    // Render status bar
    renderStatusBar();
    
    ImGui::End();
}

bool BasicTextEditor::loadFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        textBuffer_->clear();
        textBuffer_->insertText(0, 0, buffer.str());
        
        filename_ = filename;
        isModified_ = false;
        cursorLine_ = 0;
        cursorColumn_ = 0;
        
        return true;
    } catch (const std::exception& e) {
        EditorError::logError("Failed to load file: " + std::string(e.what()));
        return false;
    }
}

bool BasicTextEditor::saveFile(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        std::string content = textBuffer_->getText();
        file << content;
        
        filename_ = filename;
        isModified_ = false;
        
        return true;
    } catch (const std::exception& e) {
        EditorError::logError("Failed to save file: " + std::string(e.what()));
        return false;
    }
}

void BasicTextEditor::setText(const std::string& text) {
    textBuffer_->clear();
    textBuffer_->insertText(0, 0, text);
    isModified_ = true;
}

std::string BasicTextEditor::getText() const {
    return textBuffer_->getText();
}

void BasicTextEditor::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                textBuffer_->clear();
                textBuffer_->insertText(0, 0, "");
                filename_.clear();
                isModified_ = false;
            }
            
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                // Will be handled by file dialog
            }
            
            if (ImGui::MenuItem("Save", "Ctrl+S", false, !filename_.empty())) {
                saveFile(filename_);
            }
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                // Will be handled by file dialog
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit")) {
                if (p_open) *p_open = false;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, textBuffer_->canUndo())) {
                textBuffer_->undo();
            }
            
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, textBuffer_->canRedo())) {
                textBuffer_->redo();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                // TODO: Implement cut
            }
            
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                // TODO: Implement copy
            }
            
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                // TODO: Implement paste
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Line Numbers", nullptr, &showLineNumbers_);
            ImGui::MenuItem("Word Wrap", nullptr, &wordWrap_);
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void BasicTextEditor::renderEditor() {
    // Style for the editor
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    // Get the available region for the editor
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y -= ImGui::GetTextLineHeightWithSpacing() + 4; // Make room for status bar
    
    // Begin the child window for the editor
    ImGui::BeginChild("Editor", size, false, 
                     ImGuiWindowFlags_HorizontalScrollbar | 
                     (wordWrap_ ? 0 : ImGuiWindowFlags_AlwaysHorizontalScrollbar));
    
    // Get the text content
    std::string content = textBuffer_->getText();
    
    // Calculate line count
    size_t lineCount = textBuffer_->getLineCount();
    
    // Display line numbers if enabled
    if (showLineNumbers_) {
        float lineNumberWidth = ImGui::CalcTextSize("9999").x + 10;
        
        ImGui::BeginChild("LineNumbers", ImVec2(lineNumberWidth, 0), false);
        
        for (size_t i = 0; i < lineCount; i++) {
            ImGui::Text("%4zu", i + 1);
        }
        
        ImGui::EndChild();
        ImGui::SameLine();
    }
    
    // Display the text content
    ImGui::BeginGroup();
    
    // Convert the text to a format suitable for ImGui
    // TODO: Add syntax highlighting and proper text rendering
    ImGui::TextUnformatted(content.c_str());
    
    ImGui::EndGroup();
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void BasicTextEditor::renderStatusBar() {
    ImGui::Separator();
    
    // Display cursor position
    ImGui::Text("Ln %d, Col %d", cursorLine_ + 1, cursorColumn_ + 1);
    
    // Display file name if available
    if (!filename_.empty()) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Text("%s%s", std::filesystem::path(filename_).filename().string().c_str(), 
                   isModified_ ? " *" : "");
    }
}

void BasicTextEditor::handleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Handle keyboard shortcuts
    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;
    
    // Ctrl+S - Save
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (shift || filename_.empty()) {
            // Show save as dialog
        } else if (!filename_.empty()) {
            saveFile(filename_);
        }
    }
    
    // Ctrl+O - Open
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        // Show open dialog
    }
    
    // Ctrl+N - New
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
        textBuffer_->clear();
        textBuffer_->insertText(0, 0, "");
        filename_.clear();
        isModified_ = false;
    }
    
    // Ctrl+Z - Undo
    if (ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Z) && textBuffer_->canUndo()) {
        textBuffer_->undo();
    }
    
    // Ctrl+Y or Ctrl+Shift+Z - Redo
    if ((ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Y)) || 
        (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_Z))) {
        if (textBuffer_->canRedo()) {
            textBuffer_->redo();
        }
    }
}

// Markdown callbacks
void BasicTextEditor::linkCallback(ImGui::MarkdownLinkCallbackData data) {
    std::string url(data.link, data.linkLength);
    // TODO: Handle link clicks
}

ImGui::MarkdownImageData BasicTextEditor::imageCallback(ImGui::MarkdownLinkCallbackData data) {
    // TODO: Handle image rendering
    return ImGui::MarkdownImageData{false, false, ImVec2(0, 0)};
}

} // namespace ai_editor
