#include "EditorDemoWindow.h"
#include "core/Document.h"
#include "tabs/TabState.h"
#include "syntax/SyntaxHighlighter.h"
#include "search/SearchManager.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <iostream>

namespace ai_editor {

// Helper function to get file extension in lowercase
static std::string GetFileExtension(const std::string& filepath) {
    size_t dotPos = filepath.find_last_of(".");
    if (dotPos != std::string::npos) {
        std::string ext = filepath.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        return ext;
    }
    return "";
}

// Helper function to get filename from path
static std::string GetFilenameFromPath(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return filepath.substr(lastSlash + 1);
    }
    return filepath;
}

// Implementation of static method to check if a character is a word character
bool EditorDemoWindow::isWordCharacter(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

// Helper function to check if a line is foldable (contains an opening brace)
bool EditorDemoWindow::isFoldable(int line) const {
    if (line < 0 || line >= static_cast<int>(lines_.size())) {
        return false;
    }
    return lines_[line].find('{') != std::string::npos;
}

// Check if a line is currently folded
bool EditorDemoWindow::isFolded(int line) const {
    auto it = foldedLines_.find(line);
    return it != foldedLines_.end() && it->second;
}

// Get the next visible line after a potentially folded section
int EditorDemoWindow::getNextVisibleLine(int line) const {
    if (isFoldable(line) && isFolded(line)) {
        int depth = 1;
        while (++line < static_cast<int>(lines_.size())) {
            if (lines_[line].find('{') != std::string::npos) {
                depth++;
            } else if (lines_[line].find('}') != std::string::npos) {
                if (--depth <= 0) {
                    return line + 1;
                }
            }
        }
    }
    return line + 1;
}

// Record an operation in the undo stack
void EditorDemoWindow::recordOperation(const TextOperation& operation) {
    undoStack_.push_back(operation);
    // Clear redo stack when a new operation is recorded
    redoStack_.clear();
    
    // Limit undo stack size
    if (undoStack_.size() > 100) {
        undoStack_.pop_front();
    }
}

// Save editor settings
void EditorDemoWindow::saveSettings() const {
    try {
        std::ofstream out("editor_settings.ini");
        if (out.is_open()) {
            // Save window position and size
            out << "[Window]\n";
            out << "PosX=" << windowPos_.x << "\n";
            out << "PosY=" << windowPos_.y << "\n";
            out << "Width=" << windowSize_.x << "\n";
            out << "Height=" << windowSize_.y << "\n";
            out << "Maximized=" << (isMaximized_ ? "1" : "0") << "\n";
            
            // Save editor settings
            out << "\n[Editor]\n";
            out << "ShowLineNumbers=" << (settings_.showLineNumbers ? "1" : "0") << "\n";
            out << "EnableSyntaxHighlighting=" << (settings_.enableSyntaxHighlighting ? "1" : "0") << "\n";
            out << "EnableAutoIndent=" << (settings_.enableAutoIndent ? "1" : "0") << "\n";
            out << "EnableWordWrap=" << (settings_.enableWordWrap ? "1" : "0") << "\n";
            out << "ShowFoldingMarkers=" << (settings_.showFoldingMarkers ? "1" : "0") << "\n";
            out << "EnableAutoComplete=" << (settings_.enableAutoComplete ? "1" : "0") << "\n";
            out << "TabSize=" << settings_.tabSize << "\n";
            out << "FontSize=" << settings_.fontSize << "\n";
            out << "FontName=" << settings_.fontName << "\n";
            
            // Save recent files
            out << "\n[RecentFiles]\n";
            for (size_t i = 0; i < recentFiles_.size() && i < 10; i++) {
                out << "File" << i << "=" << recentFiles_[i] << "\n";
            }
            
            out.close();
            std::cout << "Settings saved to editor_settings.ini" << std::endl;
        } else {
            std::cerr << "Failed to save settings: Could not open file for writing" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving settings: " << e.what() << std::endl;
    }
}

// Helper function to trim whitespace from the end of a string
static inline std::string& rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

/**
 * @brief Constructs a new EditorDemoWindow instance.
 * 
 * Initializes the editor with default settings, creates an empty document,
 * and sets up syntax highlighting rules.
 */
EditorDemoWindow::EditorDemoWindow()
    : cursorLine_(0)
    , cursorColumn_(0)
    , hasSelection_(false)
    , selectionStartLine_(0)
    , selectionStartCol_(0)
    , selectionEndLine_(0)
    , selectionEndCol_(0)
    , activeTabIndex_(-1)
    , isModified_(false)
    , isOpen_(true)
    , showSearchPanel_(false)
    , searchCaseSensitive_(false)
    , searchWholeWord_(false)
    , showSettingsDialog_(false)
    , fontSize_(16.0f)
    , showWhitespace_(false)
    , wordWrap_(true)
    , showLineNumbers_(true)
    , backgroundColor_(0.15f, 0.15f, 0.15f, 1.0f)
    , textColor_(0.9f, 0.9f, 0.9f, 1.0f)
    , cursorColor_(1.0f, 1.0f, 1.0f, 1.0f)
    , selectionColor_(0.26f, 0.59f, 0.98f, 0.4f)
    , viewportStartLine_(0)
    , viewportHeight_(30)
{
    // Initialize core components
    tabManager_ = std::make_unique<TabManager>();
    syntaxHighlighter_ = std::make_unique<SyntaxHighlighter>();
    searchManager_ = std::make_unique<SearchManager>();
    
    // Initialize search and replace buffers
    searchBuffer_[0] = '\0';
    replaceBuffer_[0] = '\0';
    statusBuffer_[0] = '\0';
    newTabName_[0] = '\0';
    
    // Initialize syntax highlighting rules
    try {
        initializeLanguageDefinitions();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to initialize language definitions: " << e.what() << std::endl;
    }
    
    // Add a default tab
    addNewTab("Untitled");
}

/**
 * @brief Destroys the EditorDemoWindow instance.
 * 
 * Saves editor settings and cleans up resources.
 */
EditorDemoWindow::~EditorDemoWindow() {
    try {
        saveSettings();
    } catch (const std::exception& e) {
        std::cerr << "Error in ~EditorDemoWindow: " << e.what() << std::endl;
    }
}

/**
 * @brief Initializes the editor window.
 * 
 * Loads settings and performs any additional initialization needed.
 * 
 * @return true if initialization was successful, false otherwise.
 */
void EditorDemoWindow::Update() {
    if (!isOpen_) return;

    // Set up the main window
    ImGui::SetNextWindowSize(windowSize_, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(windowPos_, ImGuiCond_FirstUseEver);
    
    // Main editor window
    if (ImGui::Begin(windowTitle_.c_str(), &isOpen_, 
                    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse)) {
        
        RenderMenuBar();
        RenderTabBar();
        
        // Editor area
        ImGui::BeginChild("EditorArea", ImVec2(0, 0), true, 
                         ImGuiWindowFlags_HorizontalScrollbar);
        
        if (tabManager_->getTabCount() > 0 && activeTabIndex_ >= 0) {
            auto activeTab = tabManager_->getTab(activeTabIndex_);
            if (activeTab) {
                RenderEditor(activeTab);
            } else {
                ImGui::Text("No active tab. Use File > New or File > Open to get started.");
            }
        } else {
            ImGui::Text("No tabs open. Use File > New or File > Open to get started.");
        }
        
        ImGui::EndChild();
        
        RenderStatusBar();
    }
    ImGui::End();
    
    // Render any modal dialogs
    if (showSearchPanel_) {
        RenderSearchPanel();
    }
}
            auto& tab = tabs_[activeTabIndex_];
            currentLanguage_ = tab.language;
        }
        
        std::cout << "EditorDemoWindow initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize EditorDemoWindow: " << e.what() << std::endl;
        return false;
    }
}

void EditorDemoWindow::handleExit() {
    // Check for unsaved changes
    bool hasUnsavedChanges = false;
    
    if (tabManager_) {
        for (int i = 0; i < tabManager_->getTabCount(); i++) {
            auto tab = tabManager_->getTab(i);
            if (tab && tab->isModified()) {
                hasUnsavedChanges = true;
                break;
            }
        }
    }
    
    if (hasUnsavedChanges) {
        // TODO: Show save changes dialog
        // For now, just show a message
        std::cout << "You have unsaved changes. Save before exiting?" << std::endl;
    }
    
    // Close the application
    isOpen_ = false;
}

void EditorDemoWindow::handleNewFile() {
    // Add a new empty tab
    addNewTab("");
    
    // Update the window title to indicate a new untitled document
    if (tabManager_ && activeTabIndex_ >= 0 && activeTabIndex_ < tabManager_->getTabCount()) {
        updateFromActiveTab();
    }
}

void EditorDemoWindow::handleOpenFile() {
    // Show the open file dialog
    std::string filePath = showOpenFileDialog();
    if (!filePath.empty()) {
        // Check if the file is already open in another tab
        if (tabManager_) {
            for (int i = 0; i < tabManager_->getTabCount(); i++) {
                auto tab = tabManager_->getTab(i);
                if (tab && tab->getFilePath() == filePath) {
                    // Switch to the existing tab
                    switchToTab(i);
                    return;
                }
            }
        }
        
        // Load the file in a new tab
        loadFile(filePath);
    }
}

void EditorDemoWindow::handleSaveFile(bool saveAs) {
    if (!tabManager_ || activeTabIndex_ < 0 || activeTabIndex_ >= tabManager_->getTabCount()) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) {
        return;
    }
    
    std::string filePath = tab->getFilePath();
    
    // If we need to show the save dialog or if the file doesn't have a path yet
    if (saveAs || filePath.empty() || filePath == "Untitled") {
        std::string defaultPath = filePath;
        if (defaultPath.empty() || defaultPath == "Untitled") {
            defaultPath = "untitled.txt";
        }
        
        std::string newPath = showSaveFileDialog();
        if (newPath.empty()) {
            return; // User cancelled
        }
        filePath = newPath;
    }
    
    // Save the file
    if (saveFile(filePath)) {
        // The file was saved successfully, no need to update the tab here
        // as the TabState should handle updating its own state
        updateFromActiveTab();
    } else {
        // Show error message
        std::cerr << "Failed to save file: " << filePath << std::endl;
        // TODO: Show error dialog to the user
    }
}

void EditorDemoWindow::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        // File menu
        if (ImGui::BeginMenu("File")) {
            // New file
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                handleNewFile();
            }
            
            // Open file
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                handleOpenFile();
            }
            
            // Recent files submenu
            if (ImGui::BeginMenu("Open Recent")) {
                // TODO: Implement recent files list
                if (ImGui::MenuItem("No recent files")) {}
                ImGui::EndMenu();
            }
            
            ImGui::Separator();
            
            // Save options
            if (ImGui::MenuItem("Save", "Ctrl+S", false, activeTabIndex_ >= 0)) {
                handleSaveFile(false);
            }
            
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, activeTabIndex_ >= 0)) {
                handleSaveFile(true);
            }
            
            // Save All option
            if (ImGui::MenuItem("Save All", "Ctrl+Alt+S", false, activeTabIndex_ >= 0)) {
                if (tabManager_) {
                    bool anySaveFailed = false;
                    int activeTab = activeTabIndex_;
                    
                    // Save all modified tabs
                    for (int i = 0; i < tabManager_->getTabCount(); i++) {
                        auto tab = tabManager_->getTab(i);
                        if (tab && tab->isModified()) {
                            std::string path = tab->getFilePath();
                            if (!path.empty() && path != "Untitled") {
                                if (!saveFile(path)) {
                                    anySaveFailed = true;
                                    // Switch to the tab that failed to save
                                    switchToTab(i);
                                    break;
                                }
                            } else {
                                // For untitled files, switch to the tab and show Save As dialog
                                switchToTab(i);
                                if (!saveFileAs()) {
                                    anySaveFailed = true;
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Restore the original active tab if no errors occurred
                    if (!anySaveFailed && activeTab >= 0 && activeTab < tabManager_->getTabCount()) {
                        switchToTab(activeTab);
                    }
                }
            }
            
            ImGui::Separator();
            
            // Close current tab
            if (ImGui::MenuItem("Close Tab", "Ctrl+W", false, activeTabIndex_ >= 0)) {
                if (activeTabIndex_ >= 0) {
                    closeTab(activeTabIndex_);
                }
            }
            
            // Close all tabs
            if (ImGui::MenuItem("Close All Tabs", "Ctrl+Shift+W", false, 
                              tabManager_ && tabManager_->getTabCount() > 0)) {
                closeAllTabs();
            }
            
            ImGui::Separator();
            
            // Exit application
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                handleExit();
            }
            
            ImGui::EndMenu();
        }
        
        // Confirmation dialog for closing a single tab with unsaved changes
        if (showCloseTabDialog_ && tabToClose_ >= 0) {
            ImGui::OpenPopup("Save Changes##CloseTab");
            showCloseTabDialog_ = false;
        }
        
        // Confirmation dialog for closing all tabs with unsaved changes
        if (showCloseAllTabsDialog_) {
            ImGui::OpenPopup("Save Changes##CloseAllTabs");
            showCloseAllTabsDialog_ = false;
        }
        
        // Center the confirmation dialogs
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        // Single tab close confirmation dialog
        if (ImGui::BeginPopupModal("Save Changes##CloseTab", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. Save changes before closing this tab?");
            ImGui::Separator();
            
            if (ImGui::Button("Save", ImVec2(120, 0))) {
                // Save the current tab
                auto tab = tabManager_->getTab(tabToClose_);
                if (tab) {
                    std::string path = tab->getFilePath();
                    if (!path.empty() && path != "Untitled") {
                        saveFile(path);
                    } else {
                        // For untitled files, prompt for save location
                        switchToTab(tabToClose_);
                        if (!saveFileAs()) {
                            // If user cancels save as, stop the operation
                            ImGui::CloseCurrentPopup();
                            tabToClose_ = -1;
                            ImGui::EndPopup();
                            return;
                        }
                    }
                }
                // Close the tab after saving
                closeTabInternal(tabToClose_);
                tabToClose_ = -1;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Don't Save", ImVec2(120, 0))) {
                // Close the tab without saving
                closeTabInternal(tabToClose_);
                tabToClose_ = -1;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                tabToClose_ = -1;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        // All tabs close confirmation dialog
        if (ImGui::BeginPopupModal("Save Changes##CloseAllTabs", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. Save changes before closing all tabs?");
            ImGui::Separator();
            
            if (ImGui::Button("Save All", ImVec2(120, 0))) {
                // Save all modified tabs
                for (int i = 0; i < tabManager_->getTabCount(); i++) {
                    auto tab = tabManager_->getTab(i);
                    if (tab && tab->isModified()) {
                        std::string path = tab->getFilePath();
                        if (!path.empty() && path != "Untitled") {
                            saveFile(path);
                        } else {
                            // For untitled files, prompt for save location
                            switchToTab(i);
                            if (!saveFileAs()) {
                                // If user cancels save as, stop the operation
                                ImGui::CloseCurrentPopup();
                                break;
                            }
                        }
                    }
                }
                // Close all tabs after saving
                closeAllTabsInternal();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Don't Save", ImVec2(120, 0))) {
                // Close all tabs without saving
                closeAllTabsInternal();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            auto tab = tabManager_->GetActiveTab();
            bool hasSelection = tab && tab->HasSelection();
            
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, tab && tab->CanUndo())) 
                tab->Undo();
                
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, tab && tab->CanRedo())) 
                tab->Redo();
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Cut", "Ctrl+X", false, hasSelection)) 
                tab->Cut();
                
            if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection)) 
                tab->Copy();
                
            if (ImGui::MenuItem("Paste", "Ctrl+V", false, ImGui::GetClipboardText() != nullptr)) 
                tab->Paste();
                
            if (ImGui::MenuItem("Delete", "Del", false, hasSelection)) 
                tab->DeleteSelection();
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Find", "Ctrl+F")) {
                showSearchPanel_ = true;
                // TODO: Focus search input
            }
            
            if (ImGui::MenuItem("Replace", "Ctrl+H")) {
                showSearchPanel_ = true;
                // TODO: Set replace mode
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Word Wrap", nullptr, &wordWrap_);
            ImGui::MenuItem("Show Line Numbers", nullptr, &showLineNumbers_);
            ImGui::MenuItem("Show Whitespace", nullptr, &showWhitespace_);
            
            if (ImGui::BeginMenu("Font Size")) {
                if (ImGui::MenuItem("Increase", "Ctrl++")) { 
                    fontSize_ = std::min(fontSize_ + 1.0f, 48.0f); 
                }
                if (ImGui::MenuItem("Decrease", "Ctrl+-")) { 
                    fontSize_ = std::max(fontSize_ - 1.0f, 8.0f); 
                }
                if (ImGui::MenuItem("Reset", "Ctrl+0")) { 
                    fontSize_ = 16.0f; 
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}
    if (p_open && !*p_open) {
        // Handle window close request
        isOpen_ = false;
        return;
    }

    // Set up the main editor window
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("AI First Editor", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    // Render menu bar
    renderMenuBar(p_open);
    
    // Render tab bar
    if (tabs_.empty()) {
        // Add a default tab if none exists
        addNewTab("Untitled");
    }
    
    renderTabBar();
    
    // Render editor content for current tab
    if (!tabs_.empty() && activeTabIndex_ >= 0 && activeTabIndex_ < static_cast<int>(tabs_.size())) {
        auto& tab = tabs_[activeTabIndex_];
        
        // Update current language
        currentLanguage_ = tab.language;
        
        // Render editor content
        ImGui::BeginChild("EditorContent");
        
        // Display line numbers if enabled
        if (settings_.showLineNumbers) {
            ImGui::BeginChild("LineNumbers", ImVec2(40, 0), false);
            for (int i = 0; i < static_cast<int>(tab.lines.size()); i++) {
                ImGui::Text("%4d ", i + 1);
            }
            ImGui::EndChild();
            ImGui::SameLine();
        }
        
        // Render text content with syntax highlighting
        ImGui::BeginChild("TextContent");
        for (int i = 0; i < static_cast<int>(tab.lines.size()); i++) {
            if (settings_.enableSyntaxHighlighting && !tab.language.empty()) {
                renderLineWithSyntaxHighlighting(tab.lines[i], i);
            } else {
                ImGui::TextUnformatted(tab.lines[i].c_str());
            }
        }
        ImGui::EndChild();
        
        // Handle mouse input for text selection
        if (ImGui::IsWindowHovered()) {
            handleMouseInput();
        }
        
        // Handle keyboard input
        if (ImGui::IsWindowFocused()) {
            handleKeyboardShortcuts();
        }
        
        ImGui::EndChild();
        
        // Render status bar
        renderStatusBar();
    }
    
    // Render search panel if visible
    if (showSearchPanel_) {
        renderSearchPanel();
    }
    
    // Render settings dialog if visible
    if (showSettingsDialog_) {
        renderSettingsDialog();

void EditorDemoWindow::renderMenuBar(bool* p_open) {
    // ... (rest of the code remains the same)
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo())) {
                undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo())) {
                redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                cutSelection();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                copySelection();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                pasteAtCursor();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Find", "Ctrl+F")) {
                showSearchPanel_ = true;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Line Numbers", NULL, &settings_.showLineNumbers);
            ImGui::MenuItem("Syntax Highlighting", NULL, &settings_.enableSyntaxHighlighting);
            ImGui::MenuItem("Word Wrap", NULL, &settings_.enableWordWrap);
            ImGui::MenuItem("Code Folding", NULL, &settings_.showFoldingMarkers);
            ImGui::MenuItem("Settings", NULL, &showSettingsDialog_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Language")) {
            if (ImGui::MenuItem("Plain Text", NULL, currentLanguage_ == "text")) {
                currentLanguage_ = "text";
            }
            if (ImGui::MenuItem("C++", NULL, currentLanguage_ == "cpp")) {
                currentLanguage_ = "cpp";
            }
            if (ImGui::MenuItem("Python", NULL, currentLanguage_ == "python")) {
                currentLanguage_ = "python";
            }
            if (ImGui::MenuItem("JavaScript", NULL, currentLanguage_ == "javascript")) {
                currentLanguage_ = "javascript";
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void EditorDemoWindow::RenderEditor(const std::shared_ptr<TabState>& tab) {
    if (!tab) return;
    
    // Get window size for editor
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    // Calculate character size for line numbers
    ImVec2 charSize = ImGui::CalcTextSize("W");
    float lineNumberWidth = ImGui::CalcTextSize("9999").x + 16.0f;
    
    // Get document content
    const auto& content = tab->getContent();
    std::istringstream contentStream(content);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(contentStream, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    
    // Draw line numbers
    if (settings_.showLineNumbers) {
        ImGui::BeginChild("LineNumbers", ImVec2(lineNumberWidth, 0), false);
        {
            int lineCount = static_cast<int>(lines.size());
            for (int i = 0; i < lineCount; ++i) {
                // Skip rendering if line is folded
                bool isFolded = isFoldedLine(i);
                if (isFolded) continue;
                
                // Highlight current line
                if (i == cursorLine_ && activeTabIndex_ == tabManager_->getCurrentTabIndex()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                }
                
                ImGui::Text("%4d", i + 1);
                
                if (i == tab->GetCursorLine()) {
                    ImGui::PopStyleColor();
                }
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }
    
    // Draw text editor
    ImGui::BeginChild("TextEditor", ImVec2(0, 0), true, 
                     ImGuiWindowFlags_HorizontalScrollbar | 
                     (settings_.enableWordWrap ? 0 : ImGuiWindowFlags_AlwaysHorizontalScrollbar));
    
    // Set up keyboard focus and scrolling
    bool isFocused = ImGui::IsWindowFocused() && (activeTabIndex_ == tabManager_->getCurrentTabIndex());
    
    // Handle keyboard input
    if (isFocused) {
        // Handle keyboard shortcuts and text input
        handleKeyboardInput();
    }
    
    // Calculate visible lines
    int firstVisibleLine = static_cast<int>(ImGui::GetScrollY() / charSize.y);
    int lastVisibleLine = firstVisibleLine + static_cast<int>((size.y / charSize.y) + 1);
    
    // Clamp to valid range
    int lineCount = static_cast<int>(lines.size());
    firstVisibleLine = std::max(0, firstVisibleLine);
    lastVisibleLine = std::min(lineCount - 1, lastVisibleLine);
    
    // Render visible lines
    for (int i = firstVisibleLine; i <= lastVisibleLine; ++i) {
        if (i < 0 || i >= lineCount) continue;
        
        const auto& line = lines[i];
        
        // Skip rendering if line is folded
        if (isFoldedLine(i)) continue;
        
        // Draw the line
        ImGui::TextUnformatted("");
        
        float startX = ImGui::GetCursorPosX();
        float startY = ImGui::GetCursorPosY();
        
        // Apply syntax highlighting if enabled
        if (settings_.enableSyntaxHighlighting && !tab->getLanguage().empty()) {
            // TODO: Implement syntax highlighting
            ImGui::TextUnformatted(line.c_str());
        } else {
            ImGui::TextUnformatted(line.c_str());
        }
        
        // Handle cursor and selection
        if (cursorLine_ == i && isFocused) {
            // Draw cursor
            int cursorCol = std::min(cursorColumn_, static_cast<int>(line.length()));
            std::string lineBeforeCursor = line.substr(0, cursorCol);
            float cursorX = startX + ImGui::CalcTextSize(lineBeforeCursor.c_str()).x;
            
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            cursorPos.x = startX + ImGui::CalcTextSize(lineBeforeCursor.c_str()).x;
            cursorPos.y = startY;
            
            // Blinking cursor effect
            static auto lastBlink = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastBlink).count();
                
            if (elapsed > 500) {  // Blink every 500ms
                lastBlink = now;
                cursorVisible_ = !cursorVisible_;
            }
            
            if (cursorVisible_) {
                ImGui::GetWindowDrawList()->AddLine(
                    cursorPos,
                    ImVec2(cursorPos.x, cursorPos.y + charSize.y),
                    IM_COL32(255, 255, 255, 255)
                );
            }
        }
        
        // Draw selection if any
        if (hasSelection_ && i >= selectionStartLine_ && i <= selectionEndLine_) {
            int selStartCol = (i == selectionStartLine_) ? selectionStartColumn_ : 0;
            int selEndCol = (i == selectionEndLine_) ? selectionEndColumn_ : line.length();
            
            // Ensure selection is within bounds
            selStartCol = std::min(selStartCol, static_cast<int>(line.length()));
            selEndCol = std::min(selEndCol, static_cast<int>(line.length()));
            
            if (selStartCol < selEndCol) {
                std::string beforeSelection = line.substr(0, selStartCol);
                std::string selectionText = line.substr(selStartCol, selEndCol - selStartCol);
                
                // Calculate selection dimensions
                float selStartX = startX + ImGui::CalcTextSize(beforeSelection.c_str()).x;
                float selEndX = selStartX + ImGui::CalcTextSize(selectionText.c_str()).x;
                
                // Draw selection background
                ImVec2 selMin(selStartX, startY);
                ImVec2 selMax(selEndX, startY + charSize.y);
                
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImGui::GetCursorScreenPos() + ImVec2(selStartX - startX, 0),
                    ImGui::GetCursorScreenPos() + ImVec2(selEndX - startX, charSize.y),
                    IM_COL32(51, 103, 214, 102)  // Semi-transparent blue
                );
            }
        }
    }
    
    ImGui::EndChild();  // End TextEditor
    
    // Update scroll position if needed
    if (cursorLine_ < firstVisibleLine || cursorLine_ > lastVisibleLine) {
        ImGui::SetScrollY(cursorLine_ * charSize.y);
    }
    
    ImGui::EndChild();  // End EditorArea
}

void EditorDemoWindow::handleKeyboardInput() {
    ImGuiIO& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;
    
    // Handle character input
    for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
        char c = io.InputQueueCharacters[i];
        if (c != 0 && (c == '\n' || (c >= ' ' && c <= '~'))) {
            // Insert character at cursor position
            insertCharacter(c);
        }
    }
    
    // Handle special keys
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (ctrl) {
            // Move to previous word
            moveToPreviousWord();
        } else {
            // Move left
            moveCursorLeft(shift);
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (ctrl) {
            // Move to next word
            moveToNextWord();
        } else {
            // Move right
            moveCursorRight(shift);
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        moveCursorUp(shift);
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        moveCursorDown(shift);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        moveToLineStart(shift);
    } else if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        moveToLineEnd(shift);
    } else if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        pageUp(shift);
    } else if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
        pageDown(shift);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        deleteForward();
    } else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        deleteBackward();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        // Select all
        selectAll();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        // Undo
        undo();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
        // Redo
        redo();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
        // Copy
        copySelection();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X)) {
        // Cut
        cutSelection();
    } else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
        // Paste
        pasteFromClipboard();
    } else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        // Insert newline
        insertNewLine();
    } else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        // Insert tab or handle tab completion
        if (ctrl) {
            // Handle tab completion
            completeWord();
        } else {
            // Insert tab
            insertCharacter('\t');
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        // Clear selection
        clearSelection();
    }
    
    // Handle mouse selection
    if (ImGui::IsMouseDown(0)) {
        handleMouseSelection();
    }
}
                
                ImVec2 selStartPos = ImGui::GetCursorScreenPos();
                selStartPos.x = startX + selStartX;
                selStartPos.y = startY;
                
                ImVec2 selEndPos = selStartPos;
                selEndPos.x = startX + selEndX;
                selEndPos.y += charSize.y;
                
                ImGui::GetWindowDrawList()->AddRectFilled(
                    selStartPos,
                    selEndPos,
                    IM_COL32(51, 153, 255, 102)  // Semi-transparent blue
                );
            }
        }
        
        ImGui::NewLine();
    }
    
    // Handle keyboard input
    if (isFocused) {
        HandleKeyboardInput();
    }
    
    // Auto-scroll to cursor if needed
    if (autoScrollToCursor_) {
        float cursorY = tab->GetCursorLine() * charSize.y;
        float scrollY = ImGui::GetScrollY();
        float scrollMaxY = ImGui::GetScrollMaxY();
        
        if (cursorY < scrollY) {
            ImGui::SetScrollY(cursorY);
        } else if (cursorY > scrollY + size.y - charSize.y * 2) {
            ImGui::SetScrollY(cursorY - size.y + charSize.y * 2);
        }
        
        autoScrollToCursor_ = false;
    }
    
    ImGui::EndChild();
}

void EditorDemoWindow::RenderTabBar() {
    // Get tabs from the tab manager
    const auto& tabs = tabManager_->getTabs();
    
    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
        // Keep a copy of tabs to close since we can't modify the container while iterating
        std::vector<int> tabsToClose;
        
        for (size_t i = 0; i < tabs.size(); ++i) {
            const auto& tab = tabs[i];
            bool isActive = (activeTabIndex_ == static_cast<int>(i));
            bool isOpen = true;
            
            // Get display name and add * if modified
            std::string tabLabel = tab->getDisplayName();
            if (tab->isModified()) {
                tabLabel += " *";
            }
            
            // Tab item with close button
            ImGuiTabItemFlags flags = isActive ? ImGuiTabItemFlags_SetSelected : 0;
            if (ImGui::BeginTabItem(tabLabel.c_str(), &isOpen, flags)) {
                if (!isActive) {
                    switchToTab(static_cast<int>(i));
                }
                ImGui::EndTabItem();
            }
            
            // Handle middle-click to close tab
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(2)) {
                tabsToClose.push_back(static_cast<int>(i));
            }
            
            // Handle right-click context menu
            if (ImGui::BeginPopupContextItem(tab->getId().c_str())) {
                if (ImGui::MenuItem("Close")) tabsToClose.push_back(static_cast<int>(i));
                if (ImGui::MenuItem("Close Others")) {
                    // Close all tabs except this one
                    for (int j = static_cast<int>(tabs.size()) - 1; j >= 0; --j) {
                        if (j != static_cast<int>(i)) {
                            closeTab(j);
                        }
                    }
                }
                if (ImGui::MenuItem("Close All")) {
                    // Close all tabs
                    for (int j = static_cast<int>(tabs.size()) - 1; j >= 0; --j) {
                        closeTab(j);
                    }
                }
                ImGui::EndPopup();
            }
            
            if (!isOpen) {
                tabsToClose.push_back(static_cast<int>(i));
            }
        }
        
        // Process tab close requests
        for (int i = static_cast<int>(tabsToClose.size()) - 1; i >= 0; --i) {
            int tabIndex = tabsToClose[i];
            closeTab(tabIndex);
        }
        
        ImGui::EndTabBar();
    }
    
    // Add a new tab button
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        AddNewTab();
    }
}
    if (ImGui::BeginTabBar("##EditorTabs", ImGuiTabBarFlags_Reorderable)) {
        for (int i = 0; i < tabs_.size(); i++) {
            // Generate a unique ID for each tab
            std::string tabName = tabs_[i].filename;
            if (tabName.empty()) {
                tabName = "Untitled " + std::to_string(i + 1);
            } else {
                // Extract just the filename from the path
                size_t lastSlash = tabName.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    tabName = tabName.substr(lastSlash + 1);
                }
            }
            
            // Add a '*' if the tab has unsaved changes
            if (tabs_[i].isModified) {
                tabName += "*";
            }
            
            // Calculate a unique ID for each tab
            std::string tabId = "##Tab" + std::to_string(i);
            
            bool tabOpen = true;
            ImGuiTabItemFlags tabFlags = (i == currentTab_) ? ImGuiTabItemFlags_SetSelected : 0;
            
            if (ImGui::BeginTabItem((tabName + tabId).c_str(), &tabOpen, tabFlags)) {
                // If this tab is now selected and it wasn't before, switch to it
                if (i != currentTab_) {
                    switchToTab(i);
                }
                ImGui::EndTabItem();
            }
            
            // If the tab was closed, handle it
            if (!tabOpen) {
                closeTab(i);
                // Don't continue processing tabs since we've modified the array
                break;
            }
        }
        
        // Add a "+" button for new tabs
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
            addNewTab();
        }
        
        ImGui::EndTabBar();
    }
}

void EditorDemoWindow::renderStatusBar() {
    auto tab = tabManager_->GetActiveTab();
    if (!tab) return;
    
    auto document = tab->GetDocument();
    if (!document) return;
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    
    // Status bar height
    float statusBarHeight = ImGui::GetFrameHeight();
    
    // Begin status bar
    if (ImGui::BeginChild("##StatusBar", ImVec2(0, statusBarHeight), false, 
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        
        // Left side: Cursor position and file info
        ImGui::Text("Ln %d, Col %d", 
                   tab->GetCursorLine() + 1, 
                   tab->GetCursorColumn() + 1);
        
        // Add a separator
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Document status (modified/read-only)
        if (document->IsDirty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Modified");
        } else {
            ImGui::Text("Saved");
        }
        
        // Add a separator
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // File encoding
        ImGui::Text("%s", document->GetEncoding().c_str());
        
        // Add a separator
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        
        // Line endings
        ImGui::Text("%s", document->GetLineEndings().c_str());
        
        // Right side: File path and other info
        float rightAlign = ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - 10.0f;
        
        // File path (truncated if too long)
        std::string filePath = document->GetFilePath();
        if (!filePath.empty()) {
            std::string displayPath = filePath;
            float pathWidth = ImGui::CalcTextSize(displayPath.c_str()).x;
            
            // Truncate path if too long
            const float maxPathWidth = ImGui::GetWindowWidth() * 0.3f; // Max 30% of window width
            if (pathWidth > maxPathWidth) {
                // Find position to start ellipsis
                size_t charsToShow = static_cast<size_t>((maxPathWidth / pathWidth) * displayPath.length() * 0.8f);
                if (charsToShow < displayPath.length()) {
                    displayPath = "..." + displayPath.substr(displayPath.length() - charsToShow);
                }
            }
            
            ImGui::SameLine(rightAlign - pathWidth);
            ImGui::TextDisabled("%s", displayPath.c_str());
            
            // Tooltip with full path
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", filePath.c_str());
                ImGui::EndTooltip();
            }
        }
    }
    ImGui::EndChild();
    
    // Handle status bar click for context menu
    if (ImGui::IsItemClicked(1)) { // Right-click
        ImGui::OpenPopup("StatusBarContextMenu");
    }
    
    // Status bar context menu
    if (ImGui::BeginPopup("StatusBarContextMenu")) {
        if (ImGui::MenuItem("Copy Full Path")) {
            auto tab = tabManager_->GetActiveTab();
            if (tab && !tab->GetDocument()->GetFilePath().empty()) {
                // TODO: Implement clipboard functionality
                // SetClipboardText(tab->GetDocument()->GetFilePath().c_str());
            }
        }
        if (ImGui::MenuItem("Open Containing Folder")) {
            auto tab = tabManager_->GetActiveTab();
            if (tab && !tab->GetDocument()->GetFilePath().empty()) {
                // TODO: Implement opening containing folder
                // OpenContainingFolder(tab->GetDocument()->GetFilePath());
            }
        }
        ImGui::Separator();
        
        // Line endings submenu
        if (ImGui::BeginMenu("Line Endings")) {
            bool isLF = tab->GetDocument()->GetLineEndings() == "LF";
            bool isCRLF = tab->GetDocument()->GetLineEndings() == "CRLF";
            bool isCR = tab->GetDocument()->GetLineEndings() == "CR";
            
            if (ImGui::MenuItem("LF (Unix)", nullptr, &isLF)) {
                tab->GetDocument()->SetLineEndings("LF");
            }
            if (ImGui::MenuItem("CRLF (Windows)", nullptr, &isCRLF)) {
                tab->GetDocument()->SetLineEndings("CRLF");
            }
            if (ImGui::MenuItem("CR (Classic Mac)", nullptr, &isCR)) {
                tab->GetDocument()->SetLineEndings("CR");
            }
            
            ImGui::EndMenu();
        }
        
        // Encoding submenu
        if (ImGui::BeginMenu("Encoding")) {
            static const char* encodings[] = {
                "UTF-8",
                "UTF-8 with BOM",
                "UTF-16 LE",
                "UTF-16 BE",
                "UTF-32 LE",
                "UTF-32 BE",
                "Windows-1252",
                "ISO-8859-1"
            };
            
            std::string currentEncoding = tab->GetDocument()->GetEncoding();
            
            for (const char* encoding : encodings) {
                bool isSelected = (currentEncoding == encoding);
                if (ImGui::MenuItem(encoding, nullptr, &isSelected)) {
                    tab->GetDocument()->SetEncoding(encoding);
                }
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void EditorDemoWindow::renderSearchPanel() {
    if (ImGui::Begin("Search & Replace", &showSearchPanel_, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Set focus to the Find field when the panel is first shown
        if (isSearchFocused_) {
            ImGui::SetKeyboardFocusHere();
            isSearchFocused_ = false;
        }
        
        // Find input
        if (ImGui::InputText("##Find", searchBuffer_, sizeof(searchBuffer_), 
                            ImGuiInputTextFlags_EnterReturnsTrue)) {
            findNext();
        }
        
        // Replace input
        bool replaceRequested = ImGui::InputText("##Replace", replaceBuffer_, sizeof(replaceBuffer_), 
                                               ImGuiInputTextFlags_EnterReturnsTrue);
        
        // Options
        ImGui::Checkbox("Case sensitive", &caseSensitiveSearch_);
        ImGui::SameLine();
        ImGui::Checkbox("Whole word", &wholeWordSearch_);
        
        // Buttons
        if (ImGui::Button("Find Next") || replaceRequested) {
            findNext();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Replace")) {
            replace(replaceBuffer_);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Replace All")) {
            int count = replaceAll(searchBuffer_, replaceBuffer_, caseSensitiveSearch_, wholeWordSearch_);
            std::cout << "Replaced " << count << " occurrences" << std::endl;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            showSearchPanel_ = false;
        }
        
        // Display search status
        if (!searchBuffer_[0]) {
            ImGui::Text("Enter text to search");
        } else if (searchState_.hasMatch) {
            ImGui::Text("Match at line %d", searchState_.matchLine + 1);
        } else {
            ImGui::Text("No matches found");
        }
    }
    ImGui::End();
}

void EditorDemoWindow::HandleKeyboardInput() {
    auto tab = tabManager_->GetActiveTab();
    if (!tab) return;
    
    auto document = tab->GetDocument();
    if (!document) return;
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Handle keyboard shortcuts
    if (io.KeyCtrl) {
        // File operations
        if (ImGui::IsKeyPressed(ImGuiKey_N)) {
            NewFile();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            OpenFile();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (io.KeyShift) {
                SaveCurrentFileAs();
            } else {
                SaveCurrentFile();
            }
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_W)) {
            CloseCurrentTab();
            return;
        }
        
        // Edit operations
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            tab->Undo();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            tab->Redo();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
            Find();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_H)) {
            showSearchPanel_ = true;
            // TODO: Set replace mode
            return;
        }
        
        // Text selection
        if (ImGui::IsKeyPressed(ImGuiKey_A)) {
            // Select all
            tab->SetSelection(0, 0, document->GetLineCount() - 1, 
                            document->GetLine(document->GetLineCount() - 1).length());
            return;
        }
        
        // Copy/Cut/Paste
        if (ImGui::IsKeyPressed(ImGuiKey_C)) {
            tab->Copy();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_X)) {
            tab->Cut();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V)) {
            tab->Paste();
            return;
        }
    }
    
    // Handle text input
    if (io.InputQueueCharacters.Size > 0) {
        for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
            char c = io.InputQueueCharacters[i];
            if (c != 0) {
                // Handle special characters
                if (c == '\r' || c == '\n') {
                    // Handle enter key
                    document->InsertText(tab->GetCursorLine(), tab->GetCursorColumn(), "\n");
                    tab->SetCursorPosition(tab->GetCursorLine() + 1, 0);
                } else if (c == '\t') {
                    // Handle tab key - insert spaces
                    std::string spaces(tabSize_, ' ');
                    document->InsertText(tab->GetCursorLine(), tab->GetCursorColumn(), spaces);
                    tab->SetCursorPosition(tab->GetCursorLine(), tab->GetCursorColumn() + tabSize_);
                } else {
                    // Insert regular character
                    std::string text(1, c);
                    document->InsertText(tab->GetCursorLine(), tab->GetCursorColumn(), text);
                    tab->MoveCursorRight();
                }
            }
        }
    }
    
    // Handle navigation keys
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (io.KeyShift) {
            // Extend selection left
            if (tab->HasSelection()) {
                tab->MoveCursorLeft();
            } else {
                tab->StartSelection();
                tab->MoveCursorLeft();
            }
        } else {
            // Just move cursor left
            if (tab->HasSelection()) {
                tab->CollapseSelection();
            } else {
                tab->MoveCursorLeft();
            }
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (io.KeyShift) {
            // Extend selection right
            if (tab->HasSelection()) {
                tab->MoveCursorRight();
            } else {
                tab->StartSelection();
                tab->MoveCursorRight();
            }
        } else {
            // Just move cursor right
            if (tab->HasSelection()) {
                tab->CollapseSelection();
            } else {
                tab->MoveCursorRight();
            }
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (io.KeyShift) {
            // Extend selection up
            if (!tab->HasSelection()) {
                tab->StartSelection();
            }
            tab->MoveCursorUp();
        } else {
            // Just move cursor up
            if (tab->HasSelection()) {
                tab->CollapseSelection();
            }
            tab->MoveCursorUp();
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (io.KeyShift) {
            // Extend selection down
            if (!tab->HasSelection()) {
                tab->StartSelection();
            }
            tab->MoveCursorDown();
        } else {
            // Just move cursor down
            if (tab->HasSelection()) {
                tab->CollapseSelection();
            }
            tab->MoveCursorDown();
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        if (io.KeyCtrl) {
            // Move to start of document
            tab->SetCursorPosition(0, 0);
        } else {
            // Move to start of line
            tab->MoveCursorToStartOfLine();
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        if (io.KeyCtrl) {
            // Move to end of document
            int lastLine = document->GetLineCount() - 1;
            tab->SetCursorPosition(lastLine, document->GetLine(lastLine).length());
        } else {
            // Move to end of line
            tab->MoveCursorToEndOfLine();
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        // Move page up (about 20 lines)
        int newLine = std::max(0, tab->GetCursorLine() - 20);
        tab->SetCursorPosition(newLine, tab->GetCursorColumn());
    } else if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
        // Move page down (about 20 lines)
        int newLine = std::min(document->GetLineCount() - 1, tab->GetCursorLine() + 20);
        tab->SetCursorPosition(newLine, tab->GetCursorColumn());
    } else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (tab->HasSelection()) {
            tab->DeleteSelection();
        } else if (tab->GetCursorColumn() > 0) {
            // Delete character before cursor
            document->DeleteText(tab->GetCursorLine(), tab->GetCursorColumn() - 1, 1);
            tab->MoveCursorLeft();
        } else if (tab->GetCursorLine() > 0) {
            // Merge with previous line
            int prevLineLength = document->GetLine(tab->GetCursorLine() - 1).length();
            std::string currentLine = document->GetLine(tab->GetCursorLine());
            document->InsertText(tab->GetCursorLine() - 1, prevLineLength, currentLine);
            document->DeleteLine(tab->GetCursorLine());
            tab->SetCursorPosition(tab->GetCursorLine() - 1, prevLineLength);
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (tab->HasSelection()) {
            tab->DeleteSelection();
        } else if (tab->GetCursorColumn() < document->GetLine(tab->GetCursorLine()).length()) {
            // Delete character after cursor
            document->DeleteText(tab->GetCursorLine(), tab->GetCursorColumn(), 1);
        } else if (tab->GetCursorLine() < document->GetLineCount() - 1) {
            // Merge with next line
            std::string nextLine = document->GetLine(tab->GetCursorLine() + 1);
            int currentLineLength = document->GetLine(tab->GetCursorLine()).length();
            document->InsertText(tab->GetCursorLine(), currentLineLength, nextLine);
            document->DeleteLine(tab->GetCursorLine() + 1);
        }
    }
}

void EditorDemoWindow::insertText(const std::string& text) {
    // Insert text at the current cursor position
    lines_[cursorLine_].insert(cursorColumn_, text);
    cursorColumn_ += text.length();
}

void EditorDemoWindow::deleteText(int startLine, int startCol, int endLine, int endCol) {
    // Delete text from start to end position
    if (startLine == endLine) {
        lines_[startLine].erase(startCol, endCol - startCol);
    } else {
        // Delete the text
        lines_[startLine].erase(startCol);
        lines_[startLine] += lines_[endLine].substr(endCol);
        
        // Erase the lines in between
        if (endLine > startLine + 1) {
            lines_.erase(lines_.begin() + startLine + 1, lines_.begin() + endLine + 1);
        }
    }
}

void EditorDemoWindow::loadFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            lines_.clear();
            std::string line;
            while (std::getline(file, line)) {
                lines_.push_back(line);
            }
            file.close();
            currentFile_ = filename;
            std::cout << "Loaded file: " << filename << std::endl;
        } else {
            std::cerr << "Failed to load file: Could not open file for reading" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading file: " << e.what() << std::endl;
    }
}

void EditorDemoWindow::saveFile(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (const auto& line : lines_) {
                file << line << "\n";
            }
            file.close();
            currentFile_ = filename;
            std::cout << "Saved file: " << filename << std::endl;
        } else {
            std::cerr << "Failed to save file: Could not open file for writing" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving file: " << e.what() << std::endl;
    }
}

void EditorDemoWindow::setLanguageFromFilename(const std::string& filename) {
    std::string extension = "";
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = filename.substr(dotPos + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "cpp" || extension == "h" || extension == "hpp" || extension == "c" || extension == "hxx" || extension == "cxx" || extension == "cc") {
            currentLanguage_ = "cpp";
        } else if (extension == "py") {
            currentLanguage_ = "python";
        } else if (extension == "js" || extension == "ts" || extension == "jsx" || extension == "tsx") {
            currentLanguage_ = "javascript";
        } else if (extension == "html" || extension == "htm") {
            currentLanguage_ = "html";
        } else if (extension == "css") {
            currentLanguage_ = "css";
        } else if (extension == "json") {
            currentLanguage_ = "json";
        } else if (extension == "xml" || extension == "svg") {
            currentLanguage_ = "xml";
        } else if (extension == "md" || extension == "markdown") {
            currentLanguage_ = "markdown";
        } else if (extension == "java") {
            currentLanguage_ = "java";
        } else if (extension == "cs") {
            currentLanguage_ = "csharp";
        } else if (extension == "php") {
            currentLanguage_ = "php";
        } else if (extension == "rb") {
            currentLanguage_ = "ruby";
        } else if (extension == "go") {
            currentLanguage_ = "go";
        } else if (extension == "rs") {
            currentLanguage_ = "rust";
        } else if (extension == "swift") {
            currentLanguage_ = "swift";
        } else if (extension == "kt" || extension == "kts") {
            currentLanguage_ = "kotlin";
        } else if (extension == "sh" || extension == "bash") {
            currentLanguage_ = "shell";
        } else if (extension == "sql") {
            currentLanguage_ = "sql";
        } else {
            currentLanguage_ = "text";
        }
    } else {
        currentLanguage_ = "text";
    }
}

/**
 * @brief Checks if there are operations that can be undone.
 * 
 * @return true if there are operations in the undo stack, false otherwise.
 */
bool EditorDemoWindow::canUndo() const {
    if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size())) {
        return false;
    }
    const auto& tab = tabs_[activeTabIndex_];
    return !tab.undoStack.empty();
}

/**
 * @brief Checks if there are operations that can be redone.
 * 
 * @return true if there are operations in the redo stack, false otherwise.
 */
bool EditorDemoWindow::canRedo() const {
    if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size())) {
        return false;
    }
    const auto& tab = tabs_[activeTabIndex_];
    return !tab.redoStack.empty();
}

/**
 * @brief Undoes the last operation in the current tab.
 */
void EditorDemoWindow::undo() {
    try {
        if (!canUndo()) {
            std::cout << "Nothing to undo" << std::endl;
            return;
        }
        
        auto& tab = tabs_[activeTabIndex_];
        if (tab.undoStack.empty()) {
            return;
        }
        
        // Get the last operation from the undo stack
        TextOperation op = tab.undoStack.back();
        tab.undoStack.pop_back();
        
        // Apply the inverse operation and push to redo stack
        TextOperation inverseOp;
        inverseOp.type = (op.type == TextOperation::Type::INSERT) ? 
                        TextOperation::Type::DELETE : 
                        TextOperation::Type::INSERT;
        
        inverseOp.line = op.line;
        inverseOp.column = op.column;
        inverseOp.text = op.replacedText;
        inverseOp.replacedText = op.text;
        inverseOp.endLine = op.endLine;
        inverseOp.endColumn = op.endColumn;
        
        // Apply the inverse operation
        applyOperation(inverseOp, true);
        
        // Push the original operation to redo stack
        tab.redoStack.push_back(op);
        
        // Mark tab as modified if needed
        tab.isModified = true;
        
        std::cout << "Undo: " << (op.type == TextOperation::Type::INSERT ? "Insert" : "Delete") 
                  << " at line " << op.line << ", col " << op.column << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to undo: " << e.what() << std::endl;
    }
}

/**
 * @brief Redoes the last undone operation in the current tab.
 */
void EditorDemoWindow::redo() {
    try {
        if (!canRedo()) {
            std::cout << "Nothing to redo" << std::endl;
            return;
        }
        
        auto& tab = tabs_[activeTabIndex_];
        if (tab.redoStack.empty()) {
            return;
        }
        
        // Get the last operation from the redo stack
        TextOperation op = tab.redoStack.back();
        tab.redoStack.pop_back();
        
        // Apply the operation
        applyOperation(op, false);
        
        // Push the operation back to undo stack
        tab.undoStack.push_back(op);
        
        // Mark tab as modified if needed
        tab.isModified = true;
        
        std::cout << "Redo: " << (op.type == TextOperation::Type::INSERT ? "Insert" : "Delete") 
                  << " at line " << op.line << ", col " << op.column << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to redo: " << e.what() << std::endl;
    }
}

/**
 * @brief Creates a new empty file in a new tab.
 * 
 * @return true if the new file was created successfully, false otherwise.
 */
bool EditorDemoWindow::newFile() {
    try {
        // Add a new tab with an empty document
        int tabIndex = addNewTab("");
        if (tabIndex < 0) {
            std::cerr << "Failed to create new file: Could not add new tab" << std::endl;
            return false;
        }
        
        // Clear any existing content
        auto& tab = tabs_[tabIndex];
        tab.lines.clear();
        tab.lines.push_back("");
        tab.language = "text";
        tab.isModified = false;
        
        // Reset cursor and selection
        cursorLine_ = 0;
        cursorColumn_ = 0;
        hasSelection_ = false;
        
        std::cout << "Created new empty file" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create new file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Opens a file in a new tab.
 * 
 * @param filename The path to the file to open. If empty, shows a file dialog.
 * @return true if the file was opened successfully, false otherwise.
 */
bool EditorDemoWindow::openFile(const std::string& filename) {
    try {
        std::string filePath = filename;
        
        // Show file dialog if no filename provided
        if (filePath.empty()) {
            filePath = showOpenFileDialog();
            if (filePath.empty()) {
                // User cancelled the dialog
                return false;
            }
        }
        
        // Check if file exists
        if (!std::filesystem::exists(filePath)) {
            std::cerr << "File does not exist: " << filePath << std::endl;
            return false;
        }
        
        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return false;
        }
        
        // Read file content into lines
        std::vector<std::string> fileLines;
        std::string line;
        while (std::getline(file, line)) {
            fileLines.push_back(line);
        }
        file.close();
        
        // Add new tab with file content
        int tabIndex = addNewTab(filePath);
        if (tabIndex < 0) {
            std::cerr << "Failed to open file: Could not add new tab" << std::endl;
            return false;
        }
        
        // Update tab with file content
        auto& tab = tabs_[tabIndex];
        tab.lines = std::move(fileLines);
        tab.filename = filePath;
        tab.displayName = std::filesystem::path(filePath).filename().string();
        tab.isModified = false;
        
        // Detect language from filename
        setLanguageFromFilename(filePath);
        tab.language = currentLanguage_;
        
        // Reset cursor and selection
        cursorLine_ = 0;
        cursorColumn_ = 0;
        hasSelection_ = false;
        
        std::cout << "Opened file: " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to open file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Saves the current file.
 * 
 * If the file has a filename, it saves to that file. Otherwise, it prompts
 * the user for a filename using saveFileAs().
 * 
 * @return true if the file was saved successfully, false otherwise.
 */
bool EditorDemoWindow::saveCurrentFile() {
    try {
        if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size())) {
            std::cerr << "No active tab to save" << std::endl;
            return false;
        }
        
        auto& tab = tabs_[activeTabIndex_];
        
        // If file doesn't have a name, show save as dialog
        if (tab.filename.empty()) {
            return saveFileAs();
        }
        
        // Save to existing file
        std::ofstream file(tab.filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << tab.filename << std::endl;
            return false;
        }
        
        // Write content to file
        for (size_t i = 0; i < tab.lines.size(); ++i) {
            file << tab.lines[i];
            if (i < tab.lines.size() - 1) {
                file << "\n";
            }
        }
        
        file.close();
        tab.isModified = false;
        
        // Update window title to remove modified indicator
        windowTitle_ = tab.filename + " - AI First Editor";
        
        std::cout << "File saved: " << tab.filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Shows a save file dialog and saves the current file with a new name.
 * 
 * @return true if the file was saved successfully, false otherwise.
 */
bool EditorDemoWindow::saveFileAs() {
    try {
        if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size())) {
            std::cerr << "No active tab to save" << std::endl;
            return false;
        }
        
        // Show save file dialog
        std::string filePath = showSaveFileDialog();
        if (filePath.empty()) {
            // User cancelled the dialog
            return false;
        }
        
        // Ensure the file has an extension
        if (std::filesystem::path(filePath).extension().empty()) {
            filePath += ".txt";
        }
        
        auto& tab = tabs_[activeTabIndex_];
        
        // Save to the new file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }
        
        // Write content to file
        for (size_t i = 0; i < tab.lines.size(); ++i) {
            file << tab.lines[i];
            if (i < tab.lines.size() - 1) {
                file << "\n";
            }
        }
        
        file.close();
        
        // Update tab info
        tab.filename = filePath;
        tab.displayName = std::filesystem::path(filePath).filename().string();
        tab.isModified = false;
        
        // Update window title
        windowTitle_ = tab.filename + " - AI First Editor";
        
        // Update language based on new filename
        setLanguageFromFilename(filePath);
        tab.language = currentLanguage_;
        
        std::cout << "File saved as: " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save file as: " << e.what() << std::endl;
        return false;
    }
}

void EditorDemoWindow::addNewTab(const std::string& filename) {
    try {
        // Create a new tab using the tab manager
        auto newTab = tabManager_->addTab(filename);
        
        if (!filename.empty()) {
            // Set language based on file extension
            setLanguageFromFilename(filename);
            newTab->setLanguage(currentLanguage_);
            
            // Load file content
            std::ifstream file(filename);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                  std::istreambuf_iterator<char>());
                file.close();
                newTab->setContent(content, currentLanguage_);
            } else {
                // If file can't be opened, still create a new tab with empty content
                newTab->setContent("", currentLanguage_);
            }
        } else {
            // Create empty untitled tab
            newTab->setContent("", "text");
            setLanguageFromFilename("");
            currentLanguage_ = "text";
        }
        
        // Activate the new tab
        activeTabIndex_ = tabManager_->getTabCount() - 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to add new tab: " << e.what() << std::endl;
    }
}

void EditorDemoWindow::closeTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    auto tab = tabManager_->getTab(tabIndex);
    if (tab->isModified()) {
        // TODO: Add save prompt here
        std::cout << "Tab has unsaved changes. Save before closing?" << std::endl;
    }
    
    tabManager_->closeTab(tabIndex);
    
    // Update active tab index
    if (tabManager_->getTabCount() == 0) {
        // No tabs left
        activeTabIndex_ = -1;
        currentLanguage_ = "text";
        currentFilePath_ = "";
        isModified_ = false;
    } else if (activeTabIndex_ >= static_cast<int>(tabManager_->getTabCount())) {
        // If the closed tab was the last one, select the new last tab
        activeTabIndex_ = static_cast<int>(tabManager_->getTabCount()) - 1;
        updateFromActiveTab();
    } else if (tabIndex <= activeTabIndex_) {
        // If the closed tab was before the active one, update the active index
        activeTabIndex_ = std::max(0, activeTabIndex_ - 1);
        updateFromActiveTab();
    }
}

void EditorDemoWindow::switchToTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    activeTabIndex_ = tabIndex;
    tabManager_->setCurrentTab(tabIndex);
    updateFromActiveTab();
}

void EditorDemoWindow::updateFromActiveTab() {
    if (activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (tab) {
        currentLanguage_ = tab->getLanguage();
        currentFilePath_ = tab->getFilePath();
        isModified_ = tab->isModified();
        
        // Update window title
        std::string title = tab->getDisplayName();
        if (tab->isModified()) {
            title += " *";
        }
        title += " - AI First Editor";
        setWindowTitle(title);
    }
}

void EditorDemoWindow::setLanguageFromFilename(const std::string& filename) {
    std::string extension = "";
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = filename.substr(dotPos + 1);
    }
    
    // Convert to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
    
    // Set language based on extension
    if (extension == "cpp" || extension == "h" || extension == "hpp" || extension == "c" || extension == "cc") {
        currentLanguage_ = "cpp";
    } else if (extension == "py") {
        currentLanguage_ = "python";
    } else if (extension == "js") {
        currentLanguage_ = "javascript";
    } else if (extension == "html" || extension == "htm") {
        currentLanguage_ = "html";
    } else if (extension == "css") {
        currentLanguage_ = "css";
    } else if (extension == "json") {
        currentLanguage_ = "json";
    } else if (extension == "xml") {
        currentLanguage_ = "xml";
    } else if (extension == "md" || extension == "markdown") {
        currentLanguage_ = "markdown";
    } else {
        currentLanguage_ = "text";
    }
}

void EditorDemoWindow::initializeLanguageDefinitions() {
    // C++ language definition
    LanguageDefinition cpp;
    cpp.name = "cpp";
    cpp.extensions = {"cpp", "h", "hpp", "c", "cc"};
    cpp.lineCommentStart = "//";
    cpp.blockComment = {"/*", "*/"};
    
    // C++ keywords with colors
    const ImVec4 keywordColor = ImVec4(0.3f, 0.5f, 1.0f, 1.0f);
    const ImVec4 typeColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    const ImVec4 preprocessorColor = ImVec4(0.8f, 0.2f, 0.8f, 1.0f);
    const ImVec4 commentColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    const ImVec4 stringColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
    const ImVec4 numberColor = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    
    // C++ keywords
    std::vector<std::string> cppKeywords = {
        "auto", "break", "case", "catch", "class", "const", "continue", "default", "delete", "do",
        "else", "enum", "explicit", "export", "extern", "for", "friend", "goto", "if", "inline",
        "mutable", "namespace", "new", "operator", "private", "protected", "public", "register",
        "return", "sizeof", "static", "struct", "switch", "template", "this", "throw", "try",
        "typedef", "typename", "union", "using", "virtual", "volatile", "while"
    };
    
    for (const auto& keyword : cppKeywords) {
        cpp.keywords[keyword] = keywordColor;
    }
    
    // C++ types
    std::vector<std::string> cppTypes = {
        "bool", "char", "double", "float", "int", "long", "short", "signed", "unsigned", "void",
        "size_t", "wchar_t", "nullptr", "true", "false"
    };
    
    for (const auto& type : cppTypes) {
        cpp.keywords[type] = typeColor;
    }
    
    // C++ preprocessor directives
    cpp.preprocessors = {"#include", "#define", "#if", "#ifdef", "#ifndef", "#else", "#elif", "#endif", "#pragma", "#error"};
    
    // C++ syntax rules
    cpp.rules.push_back({"//[^\n]*", commentColor, false}); // Line comments
    cpp.rules.push_back({"/\\*[^*]*\\*+(?:[^*/][^*]*\\*+)*/", commentColor, true}); // Block comments
    cpp.rules.push_back({"\"(?:\\\\.|[^\\\\\"])*\"", stringColor, false}); // Double-quoted strings
    cpp.rules.push_back({"'(?:\\\\.|[^\\\\'])*'", stringColor, false}); // Single-quoted strings
    cpp.rules.push_back({"[+-]?\\d+[.\\d]*f?", numberColor, false}); // Numbers
    
    // Add more languages as needed...
    
    // Store language definitions
    languageDefinitions_["cpp"] = cpp;
}

void EditorDemoWindow::renderLineWithSyntaxHighlighting(const std::string& line, int lineNumber) {
    // If the language doesn't have special highlighting, just display the text
    if (currentLanguage_ == "text" || !settings_.enableSyntaxHighlighting) {
        ImGui::TextUnformatted(line.c_str());
        return;
    }
    
    // Get the language definition
    const LanguageDefinition* langDef = nullptr;
    auto it = languageDefinitions_.find(currentLanguage_);
    if (it != languageDefinitions_.end()) {
        langDef = &it->second;
    }
    
    // If no language definition found, just display the text
    if (!langDef) {
        ImGui::TextUnformatted(line.c_str());
        return;
    }
    
    // Default color
    ImVec4 defaultColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Start position for rendering
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    float lineStartX = startPos.x;
    float lineStartY = startPos.y;
    
    // Tokenize the line based on syntax
    struct Token {
        std::string text;
        ImVec4 color;
    };
    
    std::vector<Token> tokens;
    
    // Simple tokenization based on word boundaries
    size_t pos = 0;
    while (pos < line.length()) {
        // Skip whitespace
        if (std::isspace(line[pos])) {
            size_t start = pos;
            while (pos < line.length() && std::isspace(line[pos])) {
                pos++;
            }
            tokens.push_back({line.substr(start, pos - start), defaultColor});
            continue;
        }
        
        // Check for comments
        if (pos + 1 < line.length() && line[pos] == '/' && line[pos + 1] == '/') {
            // Line comment
            tokens.push_back({line.substr(pos), ImVec4(0.5f, 0.5f, 0.5f, 1.0f)});
            break;
        }
        
        // Check for preprocessor directives
        if (line[pos] == '#' && pos == 0) {
            // Preprocessor directive
            tokens.push_back({line, ImVec4(0.8f, 0.2f, 0.8f, 1.0f)});
            break;
        }
        
        // Check for string literals
        if (line[pos] == '"') {
            size_t start = pos;
            pos++;
            bool escaped = false;
            while (pos < line.length()) {
                if (escaped) {
                    escaped = false;
                } else if (line[pos] == '\\') {
                    escaped = true;
                } else if (line[pos] == '"') {
                    pos++;
                    break;
                }
                pos++;
            }
            tokens.push_back({line.substr(start, pos - start), ImVec4(1.0f, 0.5f, 0.0f, 1.0f)});
            continue;
        }
        
        // Check for character literals
        if (line[pos] == '\'') {
            size_t start = pos;
            pos++;
            bool escaped = false;
            while (pos < line.length()) {
                if (escaped) {
                    escaped = false;
                } else if (line[pos] == '\\') {
                    escaped = true;
                } else if (line[pos] == '\'') {
                    pos++;
                    break;
                }
                pos++;
            }
            tokens.push_back({line.substr(start, pos - start), ImVec4(1.0f, 0.5f, 0.0f, 1.0f)});
            continue;
        }
        
        // Check for numbers
        if (std::isdigit(line[pos]) || (line[pos] == '.' && pos + 1 < line.length() && std::isdigit(line[pos + 1]))) {
            size_t start = pos;
            while (pos < line.length() && (std::isdigit(line[pos]) || line[pos] == '.' || line[pos] == 'f')) {
                pos++;
            }
            tokens.push_back({line.substr(start, pos - start), ImVec4(1.0f, 0.4f, 0.4f, 1.0f)});
            continue;
        }
        
        // Check for identifiers or keywords
        if (std::isalpha(line[pos]) || line[pos] == '_') {
            size_t start = pos;
            while (pos < line.length() && (std::isalnum(line[pos]) || line[pos] == '_')) {
                pos++;
            }
            
            std::string word = line.substr(start, pos - start);
            
            // Check if it's a keyword
            auto keywordIt = langDef->keywords.find(word);
            if (keywordIt != langDef->keywords.end()) {
                tokens.push_back({word, keywordIt->second});
            } else {
                tokens.push_back({word, defaultColor});
            }
            
            continue;
        }
        
        // Other characters
        tokens.push_back({line.substr(pos, 1), defaultColor});
        pos++;
    }
    
    // Render tokens
    float xOffset = 0.0f;
    for (const auto& token : tokens) {
        ImVec2 tokenPos = ImVec2(lineStartX + xOffset, lineStartY);
        ImGui::SetCursorScreenPos(tokenPos);
        ImGui::TextColored(token.color, "%s", token.text.c_str());
        xOffset += ImGui::CalcTextSize(token.text.c_str()).x;
    }
    
    // Reset cursor position
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetTextLineHeight());
}

void EditorDemoWindow::setDemoCode(const std::string& code, const std::string& language) {
    // Not implemented
}

bool EditorDemoWindow::search(const std::string& searchText, bool caseSensitive, bool wholeWord) {
    if (searchText.empty()) {
        return false;
    }
    
    std::string searchIn;
    std::string searchFor = searchText;
    
    if (!caseSensitive) {
        searchFor = searchText;
        std::transform(searchFor.begin(), searchFor.end(), searchFor.begin(), ::tolower);
    }
    
    // Search from current cursor position to end
    for (size_t i = 0; i < lines_.size(); ++i) {
        std::string line = lines_[i];
        if (!caseSensitive) {
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        }
        
        size_t pos = line.find(searchFor);
        if (pos != std::string::npos) {
            // Found a match
            if (wholeWord) {
                // Check if it's a whole word
                if ((pos == 0 || !isWordChar(line[pos-1])) && 
                    (pos + searchFor.length() >= line.length() || !isWordChar(line[pos + searchFor.length()]))) {
                    return true;
                }
            } else {
                return true;
            }
        }
    }
    
    return false;
}

bool EditorDemoWindow::findNext() {
    if (tabs_.empty() || currentTab_ < 0 || currentTab_ >= (int)tabs_.size()) {
        return false;
    }
    
    auto& tab = tabs_[currentTab_];
    if (searchBuffer_.empty() || tab.lines.empty()) {
        return false;
    }
    
    // Start searching from the current cursor position
    int startLine = tab.cursorLine;
    int startCol = tab.cursorColumn;
    
    // Convert search text to lowercase if case-insensitive
    std::string searchFor = caseSensitive_ ? searchBuffer_ : searchBuffer_;
    if (!caseSensitive_) {
        std::transform(searchFor.begin(), searchFor.end(), searchFor.begin(), ::tolower);
    }
    
    // Search from current position to end of file
    for (int i = startLine; i < (int)tab.lines.size(); i++) {
        std::string line = caseSensitive_ ? tab.lines[i] : tab.lines[i];
        if (!caseSensitive_) {
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        }
        
        size_t startPos = (i == startLine) ? startCol : 0;
        size_t pos = line.find(searchFor, startPos);
        
        if (pos != std::string::npos) {
            // Check if it's a whole word match if needed
            if (wholeWord_) {
                bool isWordStart = (pos == 0) || !isWordChar(tab.lines[i][pos - 1]);
                bool isWordEnd = (pos + searchFor.length() >= line.length()) || 
                                !isWordChar(tab.lines[i][pos + searchFor.length()]);
                if (!isWordStart || !isWordEnd) {
                    // Not a whole word match, continue searching
                    continue;
                }
            }
            
            // Found a match, update cursor and selection
            tab.cursorLine = i;
            tab.cursorColumn = (int)pos;
            tab.hasSelection = true;
            tab.selectionStartLine = i;
            tab.selectionStartCol = (int)pos;
            tab.selectionEndLine = i;
            tab.selectionEndCol = (int)(pos + searchFor.length());
            
            // Scroll to make the selection visible
            // (In a real implementation, we'd handle scrolling here)
            
            return true;
        }
    }
    
    // If we get here, we didn't find a match
    return false;
}

bool EditorDemoWindow::replace(const std::string& replaceText) {
    // In a real implementation, this would replace the current selection or next match
    // For now, just return true to indicate success
    return true;
}

int EditorDemoWindow::replaceAll(const std::string& searchText, const std::string& replaceText,
                               bool caseSensitive, bool wholeWord) {
    if (searchText.empty()) {
        return 0;
    }
    
    int replaceCount = 0;
    std::string searchFor = searchText;
    
    if (!caseSensitive) {
        searchFor = searchText;
        std::transform(searchFor.begin(), searchFor.end(), searchFor.begin(), ::tolower);
    }
    
    // Replace all occurrences in all lines
    for (auto& line : lines_) {
        std::string lineLower = line;
        if (!caseSensitive) {
            std::transform(lineLower.begin(), lineLower.end(), lineLower.begin(), ::tolower);
        }
        
        size_t pos = 0;
        while ((pos = lineLower.find(searchFor, pos)) != std::string::npos) {
            bool isWholeWord = true;
            
            if (wholeWord) {
                // Check if it's a whole word
                isWholeWord = (pos == 0 || !isWordChar(lineLower[pos-1])) && 
                             (pos + searchFor.length() >= lineLower.length() || 
                              !isWordChar(lineLower[pos + searchFor.length()]));
            }
            
            if (isWholeWord) {
                // Replace the text in the original line
                line.replace(pos, searchFor.length(), replaceText);
                
                // Update the lowercase version for the next search
                if (!caseSensitive) {
                    lineLower = line;
                    std::transform(lineLower.begin(), lineLower.end(), lineLower.begin(), ::tolower);
                }
                
                pos += replaceText.length();
                replaceCount++;
            } else {
                pos++;
            }
        }
    }
    
    return replaceCount;
}

std::string EditorDemoWindow::getEditorContent() const {
    return "";
}

void EditorDemoWindow::handleMouseSelection() {
    // Get the current mouse position and convert it to text coordinates
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
    
    // Calculate the line and column based on mouse position
    // This is a simplified version - you'll need to adjust based on your rendering code
    float lineHeight = ImGui::GetTextLineHeight();
    int line = viewportStartLine_ + static_cast<int>((mousePos.y - cursorScreenPos.y) / lineHeight);
    
    // Clamp the line number to valid range
    line = std::max(0, std::min(static_cast<int>(lines_.size()) - 1, line));
    
    // Calculate the column based on mouse X position
    // This is a simplified version - you'll need to account for line numbers, folding markers, etc.
    float charWidth = ImGui::CalcTextSize("M").x;
    int column = static_cast<int>((mousePos.x - cursorScreenPos.x) / charWidth);
    
    // Clamp the column to valid range
    column = std::max(0, std::min(static_cast<int>(lines_[line].length()), column));
    
    // Update the cursor position
    cursorLine_ = line;
    cursorColumn_ = column;
    
    // Update the selection
    if (ImGui::IsMouseDown(0)) {
        if (!hasSelection_) {
            // Start a new selection
            hasSelection_ = true;
            selectionStartLine_ = line;
            selectionStartCol_ = column;
        }
        selectionEndLine_ = line;
        selectionEndCol_ = column;
    }
}

bool EditorDemoWindow::isFoldedLine(int line) const {
    auto it = foldedLines_.find(line);
    return it != foldedLines_.end() && it->second;
}

void EditorDemoWindow::updateSelection(bool shift) {
    if (!shift) {
        hasSelection_ = false;
        return;
    }
    
    if (!hasSelection_) {
        hasSelection_ = true;
        selectionStartLine_ = cursorLine_;
        selectionStartCol_ = cursorColumn_;
    }
    
    selectionEndLine_ = cursorLine_;
    selectionEndCol_ = cursorColumn_;
}

void EditorDemoWindow::ensureCursorVisible() {
    // Make sure the cursor is within the viewport
    if (cursorLine_ < viewportStartLine_) {
        viewportStartLine_ = cursorLine_;
    } else if (cursorLine_ >= viewportStartLine_ + viewportHeight_) {
        viewportStartLine_ = cursorLine_ - viewportHeight_ + 1;
    }
    
    // Clamp the viewport to valid range
    viewportStartLine_ = std::max(0, std::min(static_cast<int>(lines_.size()) - viewportHeight_, viewportStartLine_));
}

void EditorDemoWindow::moveCursor(int line, int column, bool select) {
    // Clamp the line and column to valid ranges
    cursorLine_ = std::max(0, std::min(static_cast<int>(lines_.size()) - 1, line));
    cursorColumn_ = std::max(0, std::min(static_cast<int>(lines_[cursorLine_].length()), column));
    
    // Update selection if needed
    if (select) {
        if (!hasSelection_) {
            hasSelection_ = true;
            selectionStartLine_ = cursorLine_;
            selectionStartCol_ = cursorColumn_;
        }
        selectionEndLine_ = cursorLine_;
        selectionEndCol_ = cursorColumn_;
    } else {
        hasSelection_ = false;
    }
    
    ensureCursorVisible();
}

void EditorDemoWindow::insertTextAtCursor(const std::string& text) {
    if (text.empty()) return;
    
    // Get the current tab
    if (activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) return;
    
    // Get the current content
    std::string content = tab->getContent();
    
    // Find the insertion point
    size_t pos = 0;
    int currentLine = 0;
    size_t lineStart = 0;
    
    // Find the current line
    while (currentLine < cursorLine_ && pos < content.length()) {
        if (content[pos] == '\n') {
            currentLine++;
            lineStart = pos + 1;
        }
        pos++;
    }
    
    // Find the column position
    size_t lineEnd = content.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
        lineEnd = content.length();
    }
    
    size_t insertPos = lineStart + std::min(static_cast<size_t>(cursorColumn_), lineEnd - lineStart);
    
    // Insert the text
    content.insert(insertPos, text);
    
    // Update the tab content
    tab->setContent(content, tab->getLanguage());
    
    // Update cursor position
    cursorColumn_ += text.length();
    
    // Update modification state
    isModified_ = true;
    updateFromActiveTab();
}

void EditorDemoWindow::deleteSelection() {
    if (!hasSelection_) return;
    
    // Get the current tab
    if (activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) return;
    
    // Get the current content
    std::string content = tab->getContent();
    
    // Find the start and end positions of the selection
    size_t startPos = 0, endPos = 0;
    int currentLine = 0;
    size_t pos = 0;
    
    // Find the start of the selection
    while (currentLine < selectionStartLine_ && pos < content.length()) {
        if (content[pos] == '\n') {
            currentLine++;
        }
        pos++;
    }
    startPos = pos + selectionStartCol_;
    
    // Find the end of the selection
    currentLine = selectionStartLine_;
    pos = startPos;
    while (currentLine < selectionEndLine_ && pos < content.length()) {
        if (content[pos] == '\n') {
            currentLine++;
        }
        pos++;
    }
    endPos = pos + selectionEndCol_;
    
    // Erase the selected text
    content.erase(startPos, endPos - startPos);
    
    // Update the tab content
    tab->setContent(content, tab->getLanguage());
    
    // Update cursor position
    cursorLine_ = selectionStartLine_;
    cursorColumn_ = selectionStartCol_;
    
    // Clear the selection
    hasSelection_ = false;
    
    // Update modification state
    isModified_ = true;
    updateFromActiveTab();
}

void EditorDemoWindow::copyToClipboard(const std::string& text) {
    ImGui::SetClipboardText(text.c_str());
}

std::string EditorDemoWindow::getSelectedText() const {
    if (!hasSelection_) return "";
    
    std::string result;
    int startLine = std::min(selectionStartLine_, selectionEndLine_);
    int endLine = std::max(selectionStartLine_, selectionEndLine_);
    int startCol = (selectionStartLine_ < selectionEndLine_) ? selectionStartCol_ : selectionEndCol_;
    int endCol = (selectionStartLine_ < selectionEndLine_) ? selectionEndCol_ : selectionStartCol_;
    
    for (int i = startLine; i <= endLine; i++) {
        if (i < 0 || i >= static_cast<int>(lines_.size())) continue;
        
        const std::string& line = lines_[i];
        int lineStart = (i == startLine) ? startCol : 0;
        int lineEnd = (i == endLine) ? endCol : line.length();
        
        if (lineStart < line.length()) {
            result += line.substr(lineStart, lineEnd - lineStart);
        }
        
        if (i < endLine) {
            result += "\n";
        }
    }
    
    return result;
}

void EditorDemoWindow::setLanguageFromFilename(const std::string& filename) {
    // Extract the file extension
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        currentLanguage_ = "text";
        return;
    }
    
    std::string ext = filename.substr(dotPos + 1);
    
    // Map file extensions to languages
    if (ext == "cpp" || ext == "hpp" || ext == "h" || ext == "cxx" || ext == "hxx") {
        currentLanguage_ = "cpp";
    } else if (ext == "py") {
        currentLanguage_ = "python";
    } else if (ext == "js") {
        currentLanguage_ = "javascript";
    } else if (ext == "java") {
        currentLanguage_ = "java";
    } else if (ext == "cs") {
        currentLanguage_ = "csharp";
    } else if (ext == "go") {
        currentLanguage_ = "go";
    } else if (ext == "rs") {
        currentLanguage_ = "rust";
    } else if (ext == "ts") {
        currentLanguage_ = "typescript";
    } else if (ext == "php") {
        currentLanguage_ = "php";
    } else if (ext == "rb") {
        currentLanguage_ = "ruby";
    } else if (ext == "swift") {
        currentLanguage_ = "swift";
    } else if (ext == "kt" || ext == "kts") {
        currentLanguage_ = "kotlin";
    } else if (ext == "sh") {
        currentLanguage_ = "shell";
    } else if (ext == "md") {
        currentLanguage_ = "markdown";
    } else if (ext == "json") {
        currentLanguage_ = "json";
    } else if (ext == "xml" || ext == "html" || ext == "xhtml") {
        currentLanguage_ = "xml";
    } else if (ext == "css") {
        currentLanguage_ = "css";
    } else if (ext == "sql") {
        currentLanguage_ = "sql";
    } else {
        currentLanguage_ = "text";
    }
}

// Tab management methods
void EditorDemoWindow::addNewTab(const std::string& filename) {
    if (!tabManager_) {
        tabManager_ = std::make_unique<TabManager>();
    }
    
    // Add a new tab with the given filename and get the new tab
    auto newTab = tabManager_->addTab(filename);
    if (!newTab) {
        std::cerr << "Failed to create new tab" << std::endl;
        return;
    }
    
    activeTabIndex_ = tabManager_->getTabCount() - 1;
    
    // If the file exists, set the language based on the filename
    if (!filename.empty() && std::filesystem::exists(filename)) {
        setLanguageFromFilename(filename);
    }
    
    // Update the editor state
    updateFromActiveTab();
}

void EditorDemoWindow::closeTab(int tabIndex) {
    if (!tabManager_ || tabIndex < 0 || tabIndex >= tabManager_->getTabCount()) {
        return;
    }
    
    // Check if the tab has unsaved changes
    auto tab = tabManager_->getTab(tabIndex);
    if (tab && tab->isModified()) {
        // Show confirmation dialog for this tab
        tabToClose_ = tabIndex;
        showCloseTabDialog_ = true;
        return;
    }
    
    // Close the tab directly if there are no unsaved changes
    closeTabInternal(tabIndex);
}

void EditorDemoWindow::closeTabInternal(int tabIndex) {
    if (!tabManager_ || tabIndex < 0 || tabIndex >= tabManager_->getTabCount()) {
        return;
    }
    
    // Store the current tab count
    int tabCount = tabManager_->getTabCount();
    
    // Remove the tab
    tabManager_->closeTab(tabIndex);
    
    // Update the active tab index
    if (activeTabIndex_ == tabIndex) {
        // If we're closing the active tab, select the previous tab if possible
        if (tabIndex > 0) {
            activeTabIndex_ = tabIndex - 1;
        } else if (tabCount > 1) {
            // If we're closing the first tab and there are others, select the next tab
            activeTabIndex_ = 0;
        } else {
            // No tabs left
            activeTabIndex_ = -1;
        }
    } else if (activeTabIndex_ > tabIndex) {
        // If we're closing a tab before the active tab, adjust the active tab index
        activeTabIndex_--;
    }
    
    // If no tabs are left, create a new empty tab
    if (tabManager_->getTabCount() == 0) {
        addNewTab("");
    } else {
        updateFromActiveTab();
    }
    
    // Notify any listeners that the document has changed
    if (onContentChanged_) {
        onContentChanged_();
    }
}

void EditorDemoWindow::switchToTab(int tabIndex) {
    if (tabManager_ && tabIndex >= 0 && tabIndex < tabManager_->getTabCount()) {
        activeTabIndex_ = tabIndex;
        updateFromActiveTab();
    }
}

void EditorDemoWindow::closeAllTabs() {
    if (!tabManager_) return;
    
    // Check if there are any unsaved changes
    bool hasUnsavedChanges = false;
    for (int i = 0; i < tabManager_->getTabCount(); i++) {
        auto tab = tabManager_->getTab(i);
        if (tab && tab->isModified()) {
            hasUnsavedChanges = true;
            break;
        }
    }
    
    if (hasUnsavedChanges) {
        // Show confirmation dialog
        showCloseAllTabsDialog_ = true;
    } else {
        // No unsaved changes, close all tabs directly
        closeAllTabsInternal();
    }
}

void EditorDemoWindow::closeAllTabsInternal() {
    if (!tabManager_) return;
    
    // Close all tabs from last to first to avoid index shifting issues
    while (tabManager_->getTabCount() > 0) {
        tabManager_->closeTab(tabManager_->getTabCount() - 1);
    }
    
    // Reset the active tab index
    activeTabIndex_ = -1;
    
    // Clear any editor state
    lines_.clear();
    cursorLine_ = 0;
    cursorColumn_ = 0;
    hasSelection_ = false;
    viewportStartLine_ = 0;
    currentFilePath_.clear();
    currentLanguage_ = "text";
    isModified_ = false;
    
    // Clear the undo/redo stacks
    if (tabManager_->getCurrentTab()) {
        // Clear the undo/redo stacks in the active tab if needed
        // The TabState class doesn't have a ClearUndoRedoStacks method
        // If you need to clear undo/redo stacks, you'll need to implement that functionality
    }
    
    // Update the window title
    windowTitle_ = "AI-First Text Editor";
    
    // Notify any listeners that the document has changed
    if (onContentChanged_) {
        onContentChanged_();
    }
}

void EditorDemoWindow::updateFromActiveTab() {
    if (!tabManager_ || activeTabIndex_ < 0 || activeTabIndex_ >= tabManager_->getTabCount()) {
        // No active tab, clear the editor
        lines_.clear();
        cursorLine_ = 0;
        cursorColumn_ = 0;
        hasSelection_ = false;
        currentFilePath_ = "";
        currentLanguage_ = "text";
        isModified_ = false;
        return;
    }
    
    // Get the active tab
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) {
        return;
    }
    
    // Get the document from the tab
    const auto& document = tab->getDocument();
    
    // Update the editor state from the tab
    currentFilePath_ = tab->getFilePath();
    currentLanguage_ = tab->getLanguage();
    isModified_ = tab->isModified();
    
    // Update the lines from the document
    const std::string& content = document.getText();
    lines_.clear();
    
    size_t start = 0;
    size_t end = content.find('\n');
    
    while (end != std::string::npos) {
        lines_.push_back(content.substr(start, end - start));
        start = end + 1;
        end = content.find('\n', start);
    }
    
    // Add the last line
    if (start < content.length()) {
        lines_.push_back(content.substr(start));
    } else if (content.empty()) {
        lines_.push_back("");
    }
    
    // Update the window title
    std::string title = "AI-First Text Editor";
    if (!currentFilePath_.empty()) {
        size_t lastSlash = currentFilePath_.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? 
            currentFilePath_.substr(lastSlash + 1) : currentFilePath_;
        title = filename + " - " + title;
    }
    setWindowTitle(title);
    
    // Ensure the cursor is visible
    ensureCursorVisible();
}

// File operations
bool EditorDemoWindow::loadFile(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        // Show error message
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    
    try {
        // Create a new tab with the file content
        // The TabState constructor will handle loading the file
        addNewTab(filename);
        
        // Update the editor state
        updateFromActiveTab();
        
        // Add to recent files
        addToRecentFiles(filename);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error reading file " << filename << ": " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

bool EditorDemoWindow::saveFile(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    
    if (!tabManager_ || activeTabIndex_ < 0 || activeTabIndex_ >= tabManager_->getTabCount()) {
        return false;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) {
        return false;
    }
    
    try {
        // Save the file through the tab's document
        if (!tab->saveAsFile(filename)) {
            std::cerr << "Failed to save file: " << filename << std::endl;
            return false;
        }
        
        // Update the current file path and modified state
        currentFilePath_ = filename;
        isModified_ = false;
        
        // Update window title
        std::string title = "AI-First Text Editor";
        if (!currentFilePath_.empty()) {
            size_t lastSlash = currentFilePath_.find_last_of("/\\");
            std::string displayName = (lastSlash != std::string::npos) ? 
                currentFilePath_.substr(lastSlash + 1) : currentFilePath_;
            title = displayName + " - " + title;
        }
        setWindowTitle(title);
        
        // Add to recent files
        addToRecentFiles(filename);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving file " << filename << ": " << e.what() << std::endl;
        return false;
    }
}

std::string EditorDemoWindow::getEditorContent() const {
    if (!tabManager_ || activeTabIndex_ < 0 || activeTabIndex_ >= tabManager_->getTabCount()) {
        return "";
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    return tab ? tab->getContent() : "";
}

void EditorDemoWindow::setDemoCode(const std::string& code, const std::string& language) {
    if (!tabManager_ || activeTabIndex_ < 0 || activeTabIndex_ >= tabManager_->getTabCount()) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (tab) {
        tab->setContent(code, language);
        currentLanguage_ = language;
        updateFromActiveTab();
    }
}

// Search and replace
bool EditorDemoWindow::search(const std::string& searchText, bool caseSensitive, bool wholeWord) {
    if (searchText.empty()) {
        return false;
    }
    
    // Save the search state
    searchState_.searchText = searchText;
    searchState_.caseSensitive = caseSensitive;
    searchState_.wholeWord = wholeWord;
    searchState_.matches.clear();
    
    // Search through all lines
    for (int i = 0; i < static_cast<int>(lines_.size()); i++) {
        const std::string& line = lines_[i];
        size_t pos = 0;
        
        while ((pos = findNextMatchInLine(line, searchText, pos, caseSensitive, wholeWord)) != std::string::npos) {
            searchState_.matches.push_back({i, static_cast<int>(pos)});
            pos += searchText.length();
        }
    }
    
    if (!searchState_.matches.empty()) {
        // Select the first match
        auto [line, col] = searchState_.matches[0];
        moveCursor(line, col);
        searchState_.currentMatchLine = line;
        searchState_.currentMatchCol = col;
        return true;
    }
    
    return false;
}

bool EditorDemoWindow::findNext() {
    if (searchState_.searchText.empty() || searchState_.matches.empty()) {
        return false;
    }
    
    // Find the current match in the list
    int currentIndex = -1;
    for (size_t i = 0; i < searchState_.matches.size(); i++) {
        if (searchState_.matches[i].first == searchState_.currentMatchLine &&
            searchState_.matches[i].second == searchState_.currentMatchCol) {
            currentIndex = static_cast<int>(i);
            break;
        }
    }
    
    // Move to the next match (wrap around if needed)
    if (currentIndex >= 0) {
        int nextIndex = (currentIndex + 1) % searchState_.matches.size();
        auto [line, col] = searchState_.matches[nextIndex];
        moveCursor(line, col);
        searchState_.currentMatchLine = line;
        searchState_.currentMatchCol = col;
        return true;
    }
    
    return false;
}

bool EditorDemoWindow::replace(const std::string& replaceText) {
    if (searchState_.searchText.empty() || !hasSelection_) {
        return false;
    }
    
    // Get the selected text
    std::string selected = getSelectedText();
    
    // Check if the selected text matches the search text
    bool match = searchState_.caseSensitive ? 
        (selected == searchState_.searchText) : 
        (strcasecmp(selected.c_str(), searchState_.searchText.c_str()) == 0);
    
    if (match) {
        // Replace the text
        deleteSelection();
        insertTextAtCursor(replaceText);
        
        // Update the search matches
        search(searchState_.searchText, searchState_.caseSensitive, searchState_.wholeWord);
        return true;
    }
    
    return false;
}

int EditorDemoWindow::replaceAll(const std::string& searchText, const std::string& replaceText, 
                               bool caseSensitive, bool wholeWord) {
    if (searchText.empty()) {
        return 0;
    }
    
    int replaceCount = 0;
    
    // Search through all lines
    for (int i = 0; i < static_cast<int>(lines_.size()); i++) {
        std::string& line = lines_[i];
        size_t pos = 0;
        size_t offset = 0;
        
        while ((pos = findNextMatchInLine(line, searchText, offset, caseSensitive, wholeWord)) != std::string::npos) {
            // Replace the text
            line.replace(pos, searchText.length(), replaceText);
            offset = pos + replaceText.length();
            replaceCount++;
        }
    }
    
    if (replaceCount > 0) {
        // Update the tab content
        if (tabManager_ && activeTabIndex_ >= 0 && activeTabIndex_ < tabManager_->getTabCount()) {
            auto tab = tabManager_->getTab(activeTabIndex_);
            if (tab) {
                std::string content;
                for (size_t i = 0; i < lines_.size(); i++) {
                    if (i > 0) content += "\n";
                    content += lines_[i];
                }
                tab->setContent(content, currentLanguage_);
                isModified_ = true;
            }
        }
    }
    
    return replaceCount;
}

// Undo/Redo
bool EditorDemoWindow::canUndo() const {
    return !undoStack_.empty();
}

bool EditorDemoWindow::canRedo() const {
    return !redoStack_.empty();
}

void EditorDemoWindow::undo() {
    if (undoStack_.empty()) {
        return;
    }
    
    // Get the last operation
    TextOperation op = undoStack_.back();
    undoStack_.pop_back();
    
    // Apply the inverse operation
    applyOperation(op, true);
    
    // Push to redo stack
    redoStack_.push_back(op);
    
    // Update the editor state
    updateFromActiveTab();
}

void EditorDemoWindow::redo() {
    if (redoStack_.empty()) {
        return;
    }
    
    // Get the last redo operation
    TextOperation op = redoStack_.back();
    redoStack_.pop_back();
    
    // Apply the operation
    applyOperation(op, false);
    
    // Push to undo stack
    undoStack_.push_back(op);
    
    // Update the editor state
    updateFromActiveTab();
}

// Helper methods for text operations
void EditorDemoWindow::recordOperation(const TextOperation& op) {
    // Clear the redo stack when a new operation is recorded
    redoStack_.clear();
    
    // Add the operation to the undo stack
    undoStack_.push_back(op);
    
    // Limit the size of the undo stack (optional)
    const size_t maxUndoSteps = 100;
    if (undoStack_.size() > maxUndoSteps) {
        undoStack_.pop_front();
    }
}

void EditorDemoWindow::applyOperation(const TextOperation& op, bool isUndo) {
    if (!tabManager_ || activeTabIndex_ < 0 || activeTabIndex_ >= tabManager_->getTabCount()) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) {
        return;
    }
    
    // Get the current content
    std::string content = tab->getContent();
    
    // Apply the operation
    if (op.type == TextOperation::Type::Insert) {
        if (isUndo) {
            // Undo insert: remove the inserted text
            content.erase(op.startPos, op.text.length());
        } else {
            // Redo insert: insert the text
            content.insert(op.startPos, op.text);
        }
    } else if (op.type == TextOperation::Type::Delete) {
        if (isUndo) {
            // Undo delete: insert the deleted text
            content.insert(op.startPos, op.text);
        } else {
            // Redo delete: remove the text
            content.erase(op.startPos, op.text.length());
        }
    }
    
    // Update the tab content
    tab->setContent(content, currentLanguage_);
    
    // Update the cursor position
    if (isUndo) {
        cursorLine_ = op.cursorLineBefore;
        cursorColumn_ = op.cursorColBefore;
    } else {
        cursorLine_ = op.cursorLineAfter;
        cursorColumn_ = op.cursorColAfter;
    }
    
    // Update the modification state
    isModified_ = tab->isModified();
}

// File dialog helpers
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

std::string EditorDemoWindow::showOpenFileDialog() {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0"
                     "Text Files (*.txt)\0*.txt\0"
                     "C++ Files (*.cpp, *.h, *.hpp)\0*.cpp;*.h;*.hpp\0"
                     "Python Files (*.py)\0*.py\0"
                     "JavaScript Files (*.js)\0*.js\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "";
    ofn.lpstrTitle = "Open File";
    
    if (GetOpenFileNameA(&ofn)) {
        return fileName;
    }
    
    return "";
}

std::string EditorDemoWindow::showSaveFileDialog() {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0"
                     "Text Files (*.txt)\0*.txt\0"
                     "C++ Files (*.cpp, *.h, *.hpp)\0*.cpp;*.h;*.hpp\0"
                     "Python Files (*.py)\0*.py\0"
                     "JavaScript Files (*.js)\0*.js\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "txt";
    ofn.lpstrTitle = "Save File";
    
    if (GetSaveFileNameA(&ofn)) {
        return fileName;
    }
    
    return "";
}

void EditorDemoWindow::renderFileDialog(bool isOpen, bool& result) {
    // TODO: Implement file dialog rendering
    // For now, do nothing
    (void)isOpen;
    (void)result;
}

// Code folding
bool EditorDemoWindow::isFoldable(int line) const {
    // Simple heuristic: a line is foldable if it contains an opening brace
    // and the next line is indented
    if (line < 0 || line >= static_cast<int>(lines_.size()) - 1) {
        return false;
    }
    
    const std::string& lineText = lines_[line];
    return lineText.find('{') != std::string::npos && 
           !lines_[line + 1].empty() && 
           (lines_[line + 1][0] == ' ' || lines_[line + 1][0] == '\t');
}

void EditorDemoWindow::toggleFold(int line) {
    if (line < 0 || line >= static_cast<int>(lines_.size())) {
        return;
    }
    
    auto it = foldedLines_.find(line);
    if (it != foldedLines_.end()) {
        foldedLines_.erase(it);
    } else {
        foldedLines_[line] = true;
    }
}

bool EditorDemoWindow::isFolded(int line) const {
    return foldedLines_.find(line) != foldedLines_.end();
}

int EditorDemoWindow::getNextVisibleLine(int line) const {
    if (line < 0 || line >= static_cast<int>(lines_.size())) {
        return line;
    }
    
    // If this line is folded, skip to the matching closing brace
    if (isFolded(line)) {
        int depth = 1;
        for (int i = line + 1; i < static_cast<int>(lines_.size()); i++) {
            const std::string& currentLine = lines_[i];
            for (char c : currentLine) {
                if (c == '{') depth++;
                else if (c == '}') depth--;
                
                if (depth == 0) {
                    return i + 1;
                }
            }
        }
    }
    
    return line + 1;
}

void EditorDemoWindow::renderFoldingMarker(int line, bool isFoldable, bool isFolded) const {
    // TODO: Implement folding marker rendering
    // For now, do nothing
    (void)line;
    (void)isFoldable;
    (void)isFolded;
}

// Demo examples
void EditorDemoWindow::loadCppExample() {
    const char* exampleCode = 
        "#include <iostream>\n"
        "\n"
        "int main() {\n"
        "    std::cout << \"Hello, World!\\n\";\n"
        "    return 0;\n"
        "}\n";
    
    setDemoCode(exampleCode, "cpp");
}

void EditorDemoWindow::loadPythonExample() {
    const char* exampleCode =
        "def main():\n"
        "    print(\"Hello, World!\")\n"
        "\n"
        "if __name__ == \"__main__\":\n"
        "    main()\n";
    
    setDemoCode(exampleCode, "python");
}

void EditorDemoWindow::loadJavaScriptExample() {
    const char* exampleCode =
        "function main() {\n"
        "    console.log(\"Hello, World!\");\n"
        "}\n"
        "\n"
        "main();\n";
    
    setDemoCode(exampleCode, "javascript");
}

// Cursor movement methods
void EditorDemoWindow::moveCursorLeft(bool shift) {
    if (cursorColumn_ > 0) {
        cursorColumn_--;
    } else if (cursorLine_ > 0) {
        cursorLine_--;
        cursorColumn_ = static_cast<int>(lines_[cursorLine_].length());
    }
    updateSelection(shift);
}

void EditorDemoWindow::moveCursorRight(bool shift) {
    if (cursorColumn_ < static_cast<int>(lines_[cursorLine_].length())) {
        cursorColumn_++;
    } else if (cursorLine_ < static_cast<int>(lines_.size()) - 1) {
        cursorLine_++;
        cursorColumn_ = 0;
    }
    updateSelection(shift);
}

void EditorDemoWindow::moveCursorUp(bool shift) {
    if (cursorLine_ > 0) {
        cursorLine_--;
        cursorColumn_ = std::min(cursorColumn_, static_cast<int>(lines_[cursorLine_].length()));
        updateSelection(shift);
    }
}

void EditorDemoWindow::moveCursorDown(bool shift) {
    if (cursorLine_ < static_cast<int>(lines_.size()) - 1) {
        cursorLine_++;
        cursorColumn_ = std::min(cursorColumn_, static_cast<int>(lines_[cursorLine_].length()));
        updateSelection(shift);
    }
}

void EditorDemoWindow::moveToLineStart(bool shift) {
    cursorColumn_ = 0;
    updateSelection(shift);
}

void EditorDemoWindow::moveToLineEnd(bool shift) {
    cursorColumn_ = static_cast<int>(lines_[cursorLine_].length());
    updateSelection(shift);
}

void EditorDemoWindow::moveToPreviousWord(bool shift) {
    // TODO: Implement word navigation
    moveCursorLeft(shift);
}

void EditorDemoWindow::moveToNextWord(bool shift) {
    // TODO: Implement word navigation
    moveCursorRight(shift);
}

void EditorDemoWindow::pageUp(bool shift) {
    // TODO: Implement page up/down with proper viewport handling
    for (int i = 0; i < 10 && cursorLine_ > 0; i++) {
        moveCursorUp(shift);
    }
}

void EditorDemoWindow::pageDown(bool shift) {
    // TODO: Implement page up/down with proper viewport handling
    for (int i = 0; i < 10 && cursorLine_ < static_cast<int>(lines_.size()) - 1; i++) {
        moveCursorDown(shift);
    }
}

void EditorDemoWindow::insertCharacter(char c) {
    // Get the current tab
    if (activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) return;
    
    // Get the current content
    std::string content = tab->getContent();
    
    // Insert the character at the cursor position
    size_t pos = 0;
    int currentLine = 0;
    size_t lineStart = 0;
    
    // Find the current line
    while (currentLine < cursorLine_ && pos < content.length()) {
        if (content[pos] == '\n') {
            currentLine++;
            lineStart = pos + 1;
        }
        pos++;
    }
    
    // Find the column position
    size_t lineEnd = content.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
        lineEnd = content.length();
    }
    
    size_t insertPos = lineStart + std::min(static_cast<size_t>(cursorColumn_), lineEnd - lineStart);
    
    // Insert the character
    content.insert(insertPos, 1, c);
    
    // Update the tab content
    tab->setContent(content, tab->getLanguage());
    
    // Move cursor right
    cursorColumn_++;
    
    // Update modification state
    isModified_ = true;
    updateFromActiveTab();
}

void EditorDemoWindow::insertNewLine() {
    // Get the current tab
    if (activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabManager_->getTabCount())) {
        return;
    }
    
    auto tab = tabManager_->getTab(activeTabIndex_);
    if (!tab) return;
    
    // Get the current content
    std::string content = tab->getContent();
    
    // Insert newline at cursor position
    size_t pos = 0;
    int currentLine = 0;
    size_t lineStart = 0;
    
    // Find the current line
    while (currentLine < cursorLine_ && pos < content.length()) {
        if (content[pos] == '\n') {
            currentLine++;
            lineStart = pos + 1;
        }
        pos++;
    }
    
    // Find the column position
    size_t lineEnd = content.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
        lineEnd = content.length();
    }
    
    size_t insertPos = lineStart + std::min(static_cast<size_t>(cursorColumn_), lineEnd - lineStart);
    
    // Insert newline
    content.insert(insertPos, "\n");
    
    // Update the tab content
    tab->setContent(content, tab->getLanguage());
    
    // Move cursor to start of new line
    cursorLine_++;
    cursorColumn_ = 0;
    
    // Update modification state
    isModified_ = true;
    updateFromActiveTab();
}

void EditorDemoWindow::deleteForward() {
    // TODO: Implement delete forward
    // For now, just move right if possible
    if (cursorColumn_ < static_cast<int>(lines_[cursorLine_].length())) {
        cursorColumn_++;
    } else if (cursorLine_ < static_cast<int>(lines_.size()) - 1) {
        cursorLine_++;
        cursorColumn_ = 0;
    }
}

void EditorDemoWindow::deleteBackward() {
    // TODO: Implement delete backward
    // For now, just move left if possible
    if (cursorColumn_ > 0) {
        cursorColumn_--;
    } else if (cursorLine_ > 0) {
        cursorLine_--;
        cursorColumn_ = static_cast<int>(lines_[cursorLine_].length());
    }
}

void EditorDemoWindow::selectAll() {
    if (lines_.empty()) return;
    
    hasSelection_ = true;
    selectionStartLine_ = 0;
    selectionStartCol_ = 0;
    selectionEndLine_ = static_cast<int>(lines_.size()) - 1;
    selectionEndCol_ = static_cast<int>(lines_.back().length());
    
    // Move cursor to end of selection
    cursorLine_ = selectionEndLine_;
    cursorColumn_ = selectionEndCol_;
}

void EditorDemoWindow::clearSelection() {
    hasSelection_ = false;
}

void EditorDemoWindow::cutSelection() {
    if (!hasSelection_) return;
    
    // TODO: Implement cut selection
    copySelection();
    // For now, just clear the selection
    clearSelection();
}

void EditorDemoWindow::copySelection() {
    if (!hasSelection_) return;
    
    // TODO: Implement copy selection
    // For now, just clear the clipboard
    ImGui::SetClipboardText("");
}

void EditorDemoWindow::pasteFromClipboard() {
    const char* clipboardText = ImGui::GetClipboardText();
    if (!clipboardText) return;
    
    // TODO: Implement paste from clipboard
    // For now, just insert the first character
    if (strlen(clipboardText) > 0) {
        insertCharacter(clipboardText[0]);
    }
}

void EditorDemoWindow::completeWord() {
    // TODO: Implement word completion
}

void EditorDemoWindow::updateSelection(bool shift) {
    if (!shift) {
        clearSelection();
        return;
    }
    
    if (!hasSelection_) {
        hasSelection_ = true;
        selectionStartLine_ = cursorLine_;
        selectionStartCol_ = cursorColumn_;
    }
    
    selectionEndLine_ = cursorLine_;
    selectionEndCol_ = cursorColumn_;
}

void EditorDemoWindow::cutSelection() {
    if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size()))
        return;

    auto& tab = tabs_[activeTabIndex_];
    
    // First copy the selection
    copySelection();
    
    // Then delete the selected text
    if (tab.hasSelection) {
        deleteSelection();
    }
}

void EditorDemoWindow::copySelection() {
    if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size()))
        return;

    auto& tab = tabs_[activeTabIndex_];
    
    if (!tab.hasSelection)
        return;
        
    std::string selectedText;
    
    if (tab.selectionStartLine == tab.selectionEndLine) {
        // Single line selection
        const auto& line = tab.lines[tab.selectionStartLine];
        size_t start = std::min(tab.selectionStartCol, tab.selectionEndCol);
        size_t end = std::max(tab.selectionStartCol, tab.selectionEndCol);
        selectedText = line.substr(start, end - start);
    } else {
        // Multi-line selection
        int startLine = std::min(tab.selectionStartLine, tab.selectionEndLine);
        int endLine = std::max(tab.selectionStartLine, tab.selectionEndLine);
        int startCol = (startLine == tab.selectionStartLine) ? tab.selectionStartCol : tab.selectionEndCol;
        int endCol = (endLine == tab.selectionEndLine) ? tab.selectionEndCol : tab.selectionStartCol;
        
        // First line
        const auto& firstLine = tab.lines[startLine];
        selectedText = firstLine.substr(startCol) + "\n";
        
        // Middle lines
        for (int i = startLine + 1; i < endLine; i++) {
            selectedText += tab.lines[i] + "\n";
        }
        
        // Last line (if different from first line)
        if (endLine > startLine) {
            const auto& lastLine = tab.lines[endLine];
            selectedText += lastLine.substr(0, endCol);
        }
    }
    
    // Set the clipboard text
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGui::SetClipboardText(selectedText.c_str());
    }
    
    // Update status
    snprintf(statusBuffer_, sizeof(statusBuffer_), "Copied %d characters to clipboard", (int)selectedText.length());
}

void EditorDemoWindow::pasteAtCursor() {
    if (tabs_.empty() || activeTabIndex_ < 0 || activeTabIndex_ >= static_cast<int>(tabs_.size()))
        return;

    auto& tab = tabs_[activeTabIndex_];
    
    // Delete any selected text first
    if (tab.hasSelection) {
        deleteSelection();
    }
    
    // Get clipboard text
    const char* clipboardText = "";
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        clipboardText = ImGui::GetClipboardText();
    }
    
    if (!clipboardText || !*clipboardText)
        return;
    
    // Split the clipboard text into lines
    std::vector<std::string> linesToInsert;
    std::string clipboardStr(clipboardText);
    size_t start = 0;
    size_t end = clipboardStr.find('\n');
    
    while (end != std::string::npos) {
        std::string line = clipboardStr.substr(start, end - start);
        // Remove carriage return if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        linesToInsert.push_back(line);
        start = end + 1;
        end = clipboardStr.find('\n', start);
    }
    
    // Add the last line if there's any remaining text
    if (start < clipboardStr.length()) {
        std::string line = clipboardStr.substr(start);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        linesToInsert.push_back(line);
    }
    
    if (linesToInsert.empty())
        return;
    
    // Record the operation for undo
    TextOperation op;
    op.type = TextOperation::Type::INSERT;
    op.line = tab.cursorLine;
    op.column = tab.cursorColumn;
    op.text = clipboardStr;
    
    // Handle multi-line paste
    if (linesToInsert.size() > 1) {
        op.endLine = tab.cursorLine + static_cast<int>(linesToInsert.size()) - 1;
        op.endColumn = static_cast<int>(linesToInsert.back().length());
    } else {
        op.endLine = tab.cursorLine;
        op.endColumn = tab.cursorColumn + static_cast<int>(linesToInsert[0].length());
    }
    
    recordOperation(op);
    
    // Insert the text
    if (linesToInsert.size() == 1) {
        // Single line insert
        std::string& line = tab.lines[tab.cursorLine];
        line.insert(tab.cursorColumn, linesToInsert[0]);
        tab.cursorColumn += static_cast<int>(linesToInsert[0].length());
    } else {
        // Multi-line insert
        std::string& firstLine = tab.lines[tab.cursorLine];
        std::string restOfFirstLine = firstLine.substr(tab.cursorColumn);
        
        // Modify the first line
        firstLine = firstLine.substr(0, tab.cursorColumn) + linesToInsert[0];
        
        // Insert middle lines
        for (size_t i = 1; i < linesToInsert.size() - 1; i++) {
            tab.lines.insert(tab.lines.begin() + tab.cursorLine + i, linesToInsert[i]);
        }
        
        // Handle last line
        std::string lastLine = linesToInsert.back() + restOfFirstLine;
        tab.lines.insert(tab.lines.begin() + tab.cursorLine + linesToInsert.size() - 1, lastLine);
        
        // Update cursor position
        tab.cursorLine += static_cast<int>(linesToInsert.size()) - 1;
        tab.cursorColumn = static_cast<int>(linesToInsert.back().length());
    }
    
    // Update selection
    tab.hasSelection = false;
    tab.isModified = true;
    
    // Update status
    snprintf(statusBuffer_, sizeof(statusBuffer_), "Pasted %d characters", (int)clipboardStr.length());
}

} // namespace ai_editor
} // namespace ai_editor 