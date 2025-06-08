#include "TabState.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace ai_editor {

TabState::TabState(const std::string& filepath) {
    static int nextId = 1;
    id_ = "tab_" + std::to_string(nextId++);
    
    if (!filepath.empty()) {
        loadFile(filepath);
    }
}

std::string TabState::getDisplayName() const {
    if (!document_.getFilePath().empty()) {
        return std::filesystem::path(document_.getFilePath()).filename().string();
    }
    return "untitled";
}

void TabState::setCursorPosition(int line, int column) {
    if (line >= 0 && column >= 0) {
        cursorLine_ = line;
        cursorColumn_ = column;
    }
}

bool TabState::loadFile(const std::string& filepath) {
    if (document_.loadFromFile(filepath)) {
        // Try to detect language from file extension
        std::string ext = std::filesystem::path(filepath).extension().string();
        if (!ext.empty()) {
            ext = ext.substr(1); // Remove the dot
            // Simple language detection based on file extension
            if (ext == "cpp" || ext == "h" || ext == "hpp" || ext == "cxx" || ext == "cc") {
                language_ = "cpp";
            } else if (ext == "py") {
                language_ = "python";
            } else if (ext == "js" || ext == "ts" || ext == "jsx" || ext == "tsx") {
                language_ = "javascript";
            } else if (ext == "java") {
                language_ = "java";
            } else if (ext == "cs") {
                language_ = "csharp";
            } else if (ext == "go") {
                language_ = "go";
            } else if (ext == "rs") {
                language_ = "rust";
            } else if (ext == "rb") {
                language_ = "ruby";
            } else if (ext == "php") {
                language_ = "php";
            } else if (ext == "swift") {
                language_ = "swift";
            } else if (ext == "kt" || ext == "kts") {
                language_ = "kotlin";
            } else if (ext == "sh") {
                language_ = "shell";
            } else if (ext == "json") {
                language_ = "json";
            } else if (ext == "xml" || ext == "html" || ext == "xhtml") {
                language_ = "xml";
            } else if (ext == "css") {
                language_ = "css";
            } else if (ext == "md" || ext == "markdown") {
                language_ = "markdown";
            } else if (ext == "yaml" || ext == "yml") {
                language_ = "yaml";
            } else if (ext == "toml") {
                language_ = "toml";
            } else if (ext == "ini" || ext == "cfg" || ext == "conf") {
                language_ = "ini";
            } else if (ext == "sql") {
                language_ = "sql";
            }
        }
        return true;
    }
    return false;
}

bool TabState::saveFile(const std::string& filepath) {
    return document_.saveToFile(filepath);
}

bool TabState::saveAsFile(const std::string& filepath) {
    return document_.saveToFile(filepath);
}

void TabState::setContent(const std::string& content, const std::string& language) {
    document_.clear();
    document_.insertText(0, 0, content);
    if (!language.empty()) {
        language_ = language;
    }
}

std::string TabState::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    
    return ss.str();
}

// TabManager implementation

TabManager::TabPtr TabManager::addTab(const std::string& filepath) {
    auto tab = std::make_shared<TabState>(filepath);
    tabs_.push_back(tab);
    currentTabIndex_ = static_cast<int>(tabs_.size()) - 1;
    updateTabStates();
    return tab;
}

bool TabManager::closeTab(size_t index) {
    if (index >= tabs_.size()) {
        return false;
    }
    
    tabs_.erase(tabs_.begin() + index);
    
    if (tabs_.empty()) {
        currentTabIndex_ = -1;
    } else if (currentTabIndex_ >= static_cast<int>(tabs_.size())) {
        currentTabIndex_ = static_cast<int>(tabs_.size()) - 1;
    } else if (static_cast<size_t>(currentTabIndex_) == index && currentTabIndex_ > 0) {
        currentTabIndex_--;
    }
    
    updateTabStates();
    return true;
}

bool TabManager::closeCurrentTab() {
    if (currentTabIndex_ < 0 || currentTabIndex_ >= static_cast<int>(tabs_.size())) {
        return false;
    }
    return closeTab(static_cast<size_t>(currentTabIndex_));
}

TabManager::TabPtr TabManager::getCurrentTab() const {
    if (currentTabIndex_ < 0 || currentTabIndex_ >= static_cast<int>(tabs_.size())) {
        return nullptr;
    }
    return tabs_[currentTabIndex_];
}

bool TabManager::setCurrentTab(size_t index) {
    if (index >= tabs_.size()) {
        return false;
    }
    
    if (currentTabIndex_ >= 0 && currentTabIndex_ < static_cast<int>(tabs_.size())) {
        tabs_[currentTabIndex_]->setActive(false);
    }
    
    currentTabIndex_ = static_cast<int>(index);
    tabs_[currentTabIndex_]->setActive(true);
    
    return true;
}

TabManager::TabPtr TabManager::getTab(size_t index) const {
    if (index >= tabs_.size()) {
        return nullptr;
    }
    return tabs_[index];
}

bool TabManager::hasUnsavedChanges() const {
    for (const auto& tab : tabs_) {
        if (tab->isModified()) {
            return true;
        }
    }
    return false;
}

TabManager::TabPtr TabManager::findTabByPath(const std::string& filepath) const {
    if (filepath.empty()) {
        return nullptr;
    }
    
    std::filesystem::path pathToFind = std::filesystem::absolute(filepath).lexically_normal();
    
    for (const auto& tab : tabs_) {
        if (!tab->getFilePath().empty()) {
            std::filesystem::path tabPath = std::filesystem::absolute(tab->getFilePath()).lexically_normal();
            if (tabPath == pathToFind) {
                return tab;
            }
        }
    }
    
    return nullptr;
}

TabManager::TabPtr TabManager::findTabById(const std::string& id) const {
    if (id.empty()) {
        return nullptr;
    }
    
    for (const auto& tab : tabs_) {
        if (tab->getId() == id) {
            return tab;
        }
    }
    
    return nullptr;
}

int TabManager::getNextTabIndex() const {
    if (tabs_.empty()) {
        return -1;
    }
    
    if (currentTabIndex_ < 0) {
        return 0;
    }
    
    return (currentTabIndex_ + 1) % static_cast<int>(tabs_.size());
}

int TabManager::getPreviousTabIndex() const {
    if (tabs_.empty()) {
        return -1;
    }
    
    if (currentTabIndex_ <= 0) {
        return static_cast<int>(tabs_.size()) - 1;
    }
    
    return currentTabIndex_ - 1;
}

void TabManager::closeAllTabs() {
    tabs_.clear();
    currentTabIndex_ = -1;
}

void TabManager::closeOtherTabs() {
    if (currentTabIndex_ < 0 || currentTabIndex_ >= static_cast<int>(tabs_.size())) {
        return;
    }
    
    auto currentTab = tabs_[currentTabIndex_];
    tabs_.clear();
    tabs_.push_back(currentTab);
    currentTabIndex_ = 0;
    updateTabStates();
}

void TabManager::closeTabsToRight() {
    if (currentTabIndex_ < 0 || currentTabIndex_ >= static_cast<int>(tabs_.size()) - 1) {
        return;
    }
    
    tabs_.erase(tabs_.begin() + currentTabIndex_ + 1, tabs_.end());
    updateTabStates();
}

void TabManager::updateTabStates() {
    for (size_t i = 0; i < tabs_.size(); ++i) {
        tabs_[i]->setActive(i == static_cast<size_t>(currentTabIndex_));
    }
}

} // namespace ai_editor
