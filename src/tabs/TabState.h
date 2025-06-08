#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../core/Document.h"

namespace ai_editor {

/**
 * @class TabState
 * @brief Represents the state of an editor tab
 */
class TabState {
public:
    /**
     * @brief Constructor
     * @param filepath Path to the file to open in this tab (empty for new file)
     */
    explicit TabState(const std::string& filepath = "");
    
    /**
     * @brief Get the display name for this tab
     * @return The display name (filename or "untitled")
     */
    std::string getDisplayName() const;
    
    /**
     * @brief Get the file path for this tab
     * @return The file path (empty if not saved)
     */
    const std::string& getFilePath() const { return document_.getFilePath(); }
    
    /**
     * @brief Get the document associated with this tab
     */
    Document& getDocument() { return document_; }
    
    /**
     * @brief Get the document associated with this tab (const version)
     */
    const Document& getDocument() const { return document_; }
    
    /**
     * @brief Check if this tab has unsaved changes
     */
    bool isModified() const { return document_.isModified(); }
    
    /**
     * @brief Get the cursor line position
     */
    int getCursorLine() const { return cursorLine_; }
    
    /**
     * @brief Get the cursor column position
     */
    int getCursorColumn() const { return cursorColumn_; }
    
    /**
     * @brief Set the cursor position
     */
    void setCursorPosition(int line, int column);
    
    /**
     * @brief Get the scroll position
     */
    std::pair<float, float> getScrollPosition() const { return {scrollX_, scrollY_}; }
    
    /**
     * @brief Set the scroll position
     */
    void setScrollPosition(float x, float y) { scrollX_ = x; scrollY_ = y; }
    
    /**
     * @brief Get the language for syntax highlighting
     */
    const std::string& getLanguage() const { return language_; }
    
    /**
     * @brief Set the language for syntax highlighting
     */
    void setLanguage(const std::string& language) { language_ = language; }
    
    /**
     * @brief Get the unique ID for this tab
     */
    const std::string& getId() const { return id_; }
    
    /**
     * @brief Check if this tab is active
     */
    bool isActive() const { return isActive_; }
    
    /**
     * @brief Set whether this tab is active
     */
    void setActive(bool active) { isActive_ = active; }
    
    // File operations
    
    /**
     * @brief Load a file into this tab
     * @param filepath Path to the file to load
     * @return True if the file was loaded successfully
     */
    bool loadFile(const std::string& filepath);
    
    /**
     * @brief Save the tab's content to a file
     * @param filepath Path to save to (empty to use current path)
     * @return True if the file was saved successfully
     */
    bool saveFile(const std::string& filepath = "");
    
    /**
     * @brief Save the tab's content to a new file
     * @param filepath Path to save to
     * @return True if the file was saved successfully
     */
    bool saveAsFile(const std::string& filepath);
    
    /**
     * @brief Get the content of the tab as a string
     */
    std::string getContent() const { return document_.getText(); }
    
    /**
     * @brief Set the content of the tab
     * @param content The new content
     * @param language The language of the content (optional)
     */
    void setContent(const std::string& content, const std::string& language = "");
    
private:
    Document document_;
    std::string language_;
    int cursorLine_ = 0;
    int cursorColumn_ = 0;
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    std::string id_;
    bool isActive_ = false;
    
    // Generate a unique ID for this tab
    static std::string generateId();
};

/**
 * @class TabManager
 * @brief Manages a collection of editor tabs
 */
class TabManager {
public:
    using TabPtr = std::shared_ptr<TabState>;
    using TabList = std::vector<TabPtr>;
    
    /**
     * @brief Constructor
     */
    TabManager() = default;
    
    /**
     * @brief Add a new tab
     * @param filepath Path to the file to open in the new tab (empty for new file)
     * @return The new tab
     */
    TabPtr addTab(const std::string& filepath = "");
    
    /**
     * @brief Close a tab
     * @param index Index of the tab to close
     * @return True if the tab was closed, false if the index is invalid
     */
    bool closeTab(size_t index);
    
    /**
     * @brief Close the current tab
     * @return True if a tab was closed, false if there are no tabs
     */
    bool closeCurrentTab();
    
    /**
     * @brief Get the current tab
     * @return The current tab, or nullptr if there are no tabs
     */
    TabPtr getCurrentTab() const;
    
    /**
     * @brief Get the index of the current tab
     * @return The index of the current tab, or -1 if there are no tabs
     */
    int getCurrentTabIndex() const { return currentTabIndex_; }
    
    /**
     * @brief Set the current tab by index
     * @param index Index of the tab to make current
     * @return True if the current tab was changed, false if the index is invalid
     */
    bool setCurrentTab(size_t index);
    
    /**
     * @brief Get the number of tabs
     */
    size_t getTabCount() const { return tabs_.size(); }
    
    /**
     * @brief Get a tab by index
     * @param index Index of the tab to get
     * @return The tab at the specified index, or nullptr if the index is invalid
     */
    TabPtr getTab(size_t index) const;
    
    /**
     * @brief Get all tabs
     */
    const TabList& getTabs() const { return tabs_; }
    
    /**
     * @brief Check if there are any unsaved changes in any tab
     */
    bool hasUnsavedChanges() const;
    
    /**
     * @brief Find a tab by file path
     * @param filepath The file path to search for
     * @return The tab with the specified file path, or nullptr if not found
     */
    TabPtr findTabByPath(const std::string& filepath) const;
    
    /**
     * @brief Find a tab by its ID
     * @param id The ID of the tab to find
     * @return The tab with the specified ID, or nullptr if not found
     */
    TabPtr findTabById(const std::string& id) const;
    
    /**
     * @brief Get the next tab index (for cycling through tabs)
     */
    int getNextTabIndex() const;
    
    /**
     * @brief Get the previous tab index (for cycling through tabs)
     */
    int getPreviousTabIndex() const;
    
    /**
     * @brief Close all tabs
     */
    void closeAllTabs();
    
    /**
     * @brief Close all tabs except the current one
     */
    void closeOtherTabs();
    
    /**
     * @brief Close all tabs to the right of the current tab
     */
    void closeTabsToRight();
    
private:
    TabList tabs_;
    int currentTabIndex_ = -1;
    
    void updateTabStates();
};

} // namespace ai_editor
