#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <algorithm>
#include "../interfaces/plugins/IUIExtensionRegistry.hpp"
#include "../AppDebugLog.h"

/**
 * @brief Implementation of the IUIExtensionRegistry interface.
 * 
 * This class manages UI extensions like menu items, toolbar buttons, etc.
 */
class UIExtensionRegistry : public IUIExtensionRegistry {
public:
    /**
     * @brief Constructor.
     */
    UIExtensionRegistry() {
        LOG_INFO("UIExtensionRegistry initialized");
        
        // Initialize with standard menus
        createMenu("file", "File");
        createMenu("edit", "Edit");
        createMenu("view", "View");
        createMenu("tools", "Tools");
        createMenu("plugins", "Plugins");
        createMenu("help", "Help");
        
        // Initialize with standard toolbars
        createToolbar("main", "Main");
        createToolbar("edit", "Edit");
    }

    /**
     * @brief Destructor.
     */
    ~UIExtensionRegistry() {
        LOG_INFO("UIExtensionRegistry destroyed");
    }

    /**
     * @brief Add a menu item to a menu.
     *
     * @param item The menu item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    bool addMenuItem(const MenuItem& item, int position = -1) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if the item already exists
        if (menuItems_.find(item.id) != menuItems_.end()) {
            LOG_WARNING("Menu item with ID '" + item.id + "' already exists");
            return false;
        }
        
        // Check if the parent menu exists (if specified)
        if (!item.parentMenuId.empty() && menus_.find(item.parentMenuId) == menus_.end()) {
            LOG_WARNING("Parent menu with ID '" + item.parentMenuId + "' not found");
            return false;
        }
        
        // Store the menu item
        menuItems_[item.id] = item;
        
        // Add the item to the parent menu's items list
        auto& menuItems = menuItemsByMenu_[item.parentMenuId];
        
        if (position >= 0 && position < static_cast<int>(menuItems.size())) {
            menuItems.insert(menuItems.begin() + position, item.id);
        } else {
            menuItems.push_back(item.id);
        }
        
        LOG_INFO("Added menu item: " + item.id + " (" + item.label + ")");
        return true;
    }

    /**
     * @brief Remove a menu item.
     *
     * @param itemId The ID of the menu item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    bool removeMenuItem(const std::string& itemId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = menuItems_.find(itemId);
        if (it == menuItems_.end()) {
            LOG_WARNING("Menu item with ID '" + itemId + "' not found for removal");
            return false;
        }
        
        // Get the parent menu ID
        std::string parentMenuId = it->second.parentMenuId;
        
        // Remove the item from the parent menu's items list
        auto& menuItems = menuItemsByMenu_[parentMenuId];
        auto itemIt = std::find(menuItems.begin(), menuItems.end(), itemId);
        if (itemIt != menuItems.end()) {
            menuItems.erase(itemIt);
        }
        
        // Remove the item itself
        menuItems_.erase(it);
        
        LOG_INFO("Removed menu item: " + itemId);
        return true;
    }

    /**
     * @brief Add a toolbar item to a toolbar.
     *
     * @param item The toolbar item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    bool addToolbarItem(const ToolbarItem& item, int position = -1) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if the item already exists
        if (toolbarItems_.find(item.id) != toolbarItems_.end()) {
            LOG_WARNING("Toolbar item with ID '" + item.id + "' already exists");
            return false;
        }
        
        // Check if the toolbar exists
        if (toolbars_.find(item.toolbarId) == toolbars_.end()) {
            LOG_WARNING("Toolbar with ID '" + item.toolbarId + "' not found");
            return false;
        }
        
        // Store the toolbar item
        toolbarItems_[item.id] = item;
        
        // Add the item to the toolbar's items list
        auto& toolbarItems = toolbarItemsByToolbar_[item.toolbarId];
        
        if (position >= 0 && position < static_cast<int>(toolbarItems.size())) {
            toolbarItems.insert(toolbarItems.begin() + position, item.id);
        } else {
            toolbarItems.push_back(item.id);
        }
        
        LOG_INFO("Added toolbar item: " + item.id + " (" + item.label + ")");
        return true;
    }

    /**
     * @brief Remove a toolbar item.
     *
     * @param itemId The ID of the toolbar item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    bool removeToolbarItem(const std::string& itemId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = toolbarItems_.find(itemId);
        if (it == toolbarItems_.end()) {
            LOG_WARNING("Toolbar item with ID '" + itemId + "' not found for removal");
            return false;
        }
        
        // Get the toolbar ID
        std::string toolbarId = it->second.toolbarId;
        
        // Remove the item from the toolbar's items list
        auto& toolbarItems = toolbarItemsByToolbar_[toolbarId];
        auto itemIt = std::find(toolbarItems.begin(), toolbarItems.end(), itemId);
        if (itemIt != toolbarItems.end()) {
            toolbarItems.erase(itemIt);
        }
        
        // Remove the item itself
        toolbarItems_.erase(it);
        
        LOG_INFO("Removed toolbar item: " + itemId);
        return true;
    }

    /**
     * @brief Add a context menu item.
     *
     * @param item The context menu item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    bool addContextMenuItem(const ContextMenuItem& item, int position = -1) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if the item already exists
        if (contextMenuItems_.find(item.id) != contextMenuItems_.end()) {
            LOG_WARNING("Context menu item with ID '" + item.id + "' already exists");
            return false;
        }
        
        // Store the context menu item
        contextMenuItems_[item.id] = item;
        
        // Add the item to the context's items list
        auto& contextItems = contextMenuItemsByContext_[item.contextId];
        
        if (position >= 0 && position < static_cast<int>(contextItems.size())) {
            contextItems.insert(contextItems.begin() + position, item.id);
        } else {
            contextItems.push_back(item.id);
        }
        
        LOG_INFO("Added context menu item: " + item.id + " (" + item.label + ")");
        return true;
    }

    /**
     * @brief Remove a context menu item.
     *
     * @param itemId The ID of the context menu item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    bool removeContextMenuItem(const std::string& itemId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = contextMenuItems_.find(itemId);
        if (it == contextMenuItems_.end()) {
            LOG_WARNING("Context menu item with ID '" + itemId + "' not found for removal");
            return false;
        }
        
        // Get the context ID
        std::string contextId = it->second.contextId;
        
        // Remove the item from the context's items list
        auto& contextItems = contextMenuItemsByContext_[contextId];
        auto itemIt = std::find(contextItems.begin(), contextItems.end(), itemId);
        if (itemIt != contextItems.end()) {
            contextItems.erase(itemIt);
        }
        
        // Remove the item itself
        contextMenuItems_.erase(it);
        
        LOG_INFO("Removed context menu item: " + itemId);
        return true;
    }

    /**
     * @brief Create a new menu.
     *
     * @param menuId A unique identifier for the menu.
     * @param label The display label for the menu.
     * @param parentMenuId The ID of the parent menu (empty for top-level menus).
     * @return true if the menu was created successfully, false otherwise.
     */
    bool createMenu(const std::string& menuId, const std::string& label,
                  const std::string& parentMenuId = "") override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if the menu already exists
        if (menus_.find(menuId) != menus_.end()) {
            LOG_WARNING("Menu with ID '" + menuId + "' already exists");
            return false;
        }
        
        // Check if the parent menu exists (if specified)
        if (!parentMenuId.empty() && menus_.find(parentMenuId) == menus_.end()) {
            LOG_WARNING("Parent menu with ID '" + parentMenuId + "' not found");
            return false;
        }
        
        // Create the menu
        MenuInfo menu;
        menu.id = menuId;
        menu.label = label;
        menu.parentMenuId = parentMenuId;
        
        menus_[menuId] = menu;
        
        // Add the menu to the parent menu's items list (if specified)
        if (!parentMenuId.empty()) {
            menuItemsByMenu_[parentMenuId].push_back(menuId);
        }
        
        LOG_INFO("Created menu: " + menuId + " (" + label + ")");
        return true;
    }
    
    /**
     * @brief Create a new toolbar.
     *
     * @param toolbarId A unique identifier for the toolbar.
     * @param label The display label for the toolbar.
     * @return true if the toolbar was created successfully, false otherwise.
     */
    bool createToolbar(const std::string& toolbarId, const std::string& label) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if the toolbar already exists
        if (toolbars_.find(toolbarId) != toolbars_.end()) {
            LOG_WARNING("Toolbar with ID '" + toolbarId + "' already exists");
            return false;
        }
        
        // Create the toolbar
        ToolbarInfo toolbar;
        toolbar.id = toolbarId;
        toolbar.label = label;
        
        toolbars_[toolbarId] = toolbar;
        
        LOG_INFO("Created toolbar: " + toolbarId + " (" + label + ")");
        return true;
    }

    /**
     * @brief Get all registered menu IDs.
     *
     * @return A vector of all registered menu IDs.
     */
    std::vector<std::string> getAllMenuIds() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> ids;
        ids.reserve(menus_.size());
        
        for (const auto& pair : menus_) {
            ids.push_back(pair.first);
        }
        
        return ids;
    }

    /**
     * @brief Get all registered toolbar IDs.
     *
     * @return A vector of all registered toolbar IDs.
     */
    std::vector<std::string> getAllToolbarIds() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> ids;
        ids.reserve(toolbars_.size());
        
        for (const auto& pair : toolbars_) {
            ids.push_back(pair.first);
        }
        
        return ids;
    }

private:
    /**
     * @brief Information about a menu.
     */
    struct MenuInfo {
        std::string id;
        std::string label;
        std::string parentMenuId;
    };
    
    /**
     * @brief Information about a toolbar.
     */
    struct ToolbarInfo {
        std::string id;
        std::string label;
    };
    
    std::unordered_map<std::string, MenuInfo> menus_;
    std::unordered_map<std::string, MenuItem> menuItems_;
    std::unordered_map<std::string, std::vector<std::string>> menuItemsByMenu_;
    
    std::unordered_map<std::string, ToolbarInfo> toolbars_;
    std::unordered_map<std::string, ToolbarItem> toolbarItems_;
    std::unordered_map<std::string, std::vector<std::string>> toolbarItemsByToolbar_;
    
    std::unordered_map<std::string, ContextMenuItem> contextMenuItems_;
    std::unordered_map<std::string, std::vector<std::string>> contextMenuItemsByContext_;
    
    mutable std::mutex mutex_;  // Protects all data structures
}; 