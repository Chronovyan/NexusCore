#include "UIExtensionRegistry.h"
#include "../AppDebugLog.h"
#include <algorithm>
#include <iostream>

UIExtensionRegistry::UIExtensionRegistry() 
    : menus_(),
      menuItems_(),
      menuItemOrder_(),
      toolbars_(),
      toolbarItems_(),
      toolbarItemOrder_(),
      contextMenuItems_(),
      contextMenuItemOrder_() {
    LOG_DEBUG("UIExtensionRegistry initialized");
}

bool UIExtensionRegistry::addMenuItem(const MenuItem& item, int position) {
    // Check if the item ID already exists
    if (menuItems_.find(item.id) != menuItems_.end()) {
        LOG_ERROR("UIExtensionRegistry: Menu item ID '" + item.id + "' already exists");
        return false;
    }
    
    // For non-separators, validate parent menu and command ID
    if (!item.isSeparator) {
        // Check if the parent menu exists
        if (!item.parentMenuId.empty() && !menuExists(item.parentMenuId)) {
            LOG_ERROR("UIExtensionRegistry: Parent menu ID '" + item.parentMenuId + 
                     "' does not exist for menu item '" + item.id + "'");
            return false;
        }
        
        // For non-submenu items, validate command ID
        if (item.commandId.empty() && !item.isSeparator) {
            // Item without a command ID should have child items (submenu)
            bool hasChildItems = false;
            for (const auto& existingItem : menuItems_) {
                if (existingItem.second.parentMenuId == item.id) {
                    hasChildItems = true;
                    break;
                }
            }
            
            if (!hasChildItems) {
                LOG_ERROR("UIExtensionRegistry: Menu item '" + item.id + 
                         "' has no command ID and is not a submenu or separator");
                return false;
            }
        }
    }
    
    // Store the menu item
    menuItems_[item.id] = item;
    
    // Add to the order list for the parent menu
    std::string parentId = item.parentMenuId.empty() ? "main" : item.parentMenuId;
    insertAtPosition(menuItemOrder_[parentId], item.id, position);
    
    LOG_DEBUG("UIExtensionRegistry: Added menu item '" + item.id + 
             "' to parent '" + parentId + "'");
    return true;
}

bool UIExtensionRegistry::removeMenuItem(const std::string& itemId) {
    // Check if the item exists
    auto itemIt = menuItems_.find(itemId);
    if (itemIt == menuItems_.end()) {
        LOG_ERROR("UIExtensionRegistry: Menu item ID '" + itemId + "' not found for removal");
        return false;
    }
    
    // Remove children first (recursive removal)
    std::vector<std::string> childrenToRemove;
    for (const auto& item : menuItems_) {
        if (item.second.parentMenuId == itemId) {
            childrenToRemove.push_back(item.first);
        }
    }
    
    for (const auto& childId : childrenToRemove) {
        removeMenuItem(childId);
    }
    
    // Get the parent menu ID
    std::string parentId = itemIt->second.parentMenuId.empty() ? "main" : itemIt->second.parentMenuId;
    
    // Remove from order list
    auto& orderList = menuItemOrder_[parentId];
    orderList.erase(std::remove(orderList.begin(), orderList.end(), itemId), orderList.end());
    
    // Remove the item itself
    menuItems_.erase(itemId);
    
    LOG_DEBUG("UIExtensionRegistry: Removed menu item '" + itemId + "'");
    return true;
}

bool UIExtensionRegistry::addToolbarItem(const ToolbarItem& item, int position) {
    // Check if the item ID already exists
    if (toolbarItems_.find(item.id) != toolbarItems_.end()) {
        LOG_ERROR("UIExtensionRegistry: Toolbar item ID '" + item.id + "' already exists");
        return false;
    }
    
    // Check if the toolbar exists
    if (!toolbarExists(item.toolbarId)) {
        LOG_ERROR("UIExtensionRegistry: Toolbar ID '" + item.toolbarId + 
                 "' does not exist for toolbar item '" + item.id + "'");
        return false;
    }
    
    // For non-separators, validate command ID
    if (!item.commandId.empty() || item.isSeparator) {
        // Store the toolbar item
        toolbarItems_[item.id] = item;
        
        // Add to the order list
        insertAtPosition(toolbarItemOrder_[item.toolbarId], item.id, position);
        
        LOG_DEBUG("UIExtensionRegistry: Added toolbar item '" + item.id + 
                 "' to toolbar '" + item.toolbarId + "'");
        return true;
    } else {
        LOG_ERROR("UIExtensionRegistry: Toolbar item '" + item.id + 
                 "' has no command ID and is not a separator");
        return false;
    }
}

bool UIExtensionRegistry::removeToolbarItem(const std::string& itemId) {
    // Check if the item exists
    auto itemIt = toolbarItems_.find(itemId);
    if (itemIt == toolbarItems_.end()) {
        LOG_ERROR("UIExtensionRegistry: Toolbar item ID '" + itemId + "' not found for removal");
        return false;
    }
    
    // Get the toolbar ID
    std::string toolbarId = itemIt->second.toolbarId;
    
    // Remove from order list
    auto& orderList = toolbarItemOrder_[toolbarId];
    orderList.erase(std::remove(orderList.begin(), orderList.end(), itemId), orderList.end());
    
    // Remove the item itself
    toolbarItems_.erase(itemId);
    
    LOG_DEBUG("UIExtensionRegistry: Removed toolbar item '" + itemId + "'");
    return true;
}

bool UIExtensionRegistry::addContextMenuItem(const ContextMenuItem& item, int position) {
    // Check if the item ID already exists
    if (contextMenuItems_.find(item.id) != contextMenuItems_.end()) {
        LOG_ERROR("UIExtensionRegistry: Context menu item ID '" + item.id + "' already exists");
        return false;
    }
    
    // For non-separators, validate command ID
    if (!item.commandId.empty() || item.isSeparator) {
        // Store the context menu item
        contextMenuItems_[item.id] = item;
        
        // Add to the order list
        insertAtPosition(contextMenuItemOrder_[item.contextId], item.id, position);
        
        LOG_DEBUG("UIExtensionRegistry: Added context menu item '" + item.id + 
                 "' to context '" + item.contextId + "'");
        return true;
    } else {
        LOG_ERROR("UIExtensionRegistry: Context menu item '" + item.id + 
                 "' has no command ID and is not a separator");
        return false;
    }
}

bool UIExtensionRegistry::removeContextMenuItem(const std::string& itemId) {
    // Check if the item exists
    auto itemIt = contextMenuItems_.find(itemId);
    if (itemIt == contextMenuItems_.end()) {
        LOG_ERROR("UIExtensionRegistry: Context menu item ID '" + itemId + "' not found for removal");
        return false;
    }
    
    // Get the context ID
    std::string contextId = itemIt->second.contextId;
    
    // Remove from order list
    auto& orderList = contextMenuItemOrder_[contextId];
    orderList.erase(std::remove(orderList.begin(), orderList.end(), itemId), orderList.end());
    
    // Remove the item itself
    contextMenuItems_.erase(itemId);
    
    LOG_DEBUG("UIExtensionRegistry: Removed context menu item '" + itemId + "'");
    return true;
}

bool UIExtensionRegistry::createMenu(const std::string& menuId, const std::string& label, 
                                 const std::string& parentMenuId) {
    // Check if the menu ID already exists
    if (menuExists(menuId)) {
        LOG_ERROR("UIExtensionRegistry: Menu ID '" + menuId + "' already exists");
        return false;
    }
    
    // Check if the parent menu exists
    if (!parentMenuId.empty() && !menuExists(parentMenuId)) {
        LOG_ERROR("UIExtensionRegistry: Parent menu ID '" + parentMenuId + 
                 "' does not exist for menu '" + menuId + "'");
        return false;
    }
    
    // Create the menu
    MenuInfo menuInfo;
    menuInfo.id = menuId;
    menuInfo.label = label;
    menuInfo.parentId = parentMenuId;
    
    menus_[menuId] = menuInfo;
    
    // If this is a submenu, create a menu item for it
    if (!parentMenuId.empty()) {
        MenuItem item;
        item.id = menuId + "_menuitem";
        item.label = label;
        item.parentMenuId = parentMenuId;
        item.commandId = ""; // Empty command ID indicates it's a submenu
        item.enabled = true;
        item.visible = true;
        item.isSeparator = false;
        
        // Add the menu item
        addMenuItem(item);
    }
    
    LOG_DEBUG("UIExtensionRegistry: Created menu '" + menuId + 
             (parentMenuId.empty() ? "' as top-level menu" : "' as submenu of '" + parentMenuId + "'"));
    return true;
}

bool UIExtensionRegistry::createToolbar(const std::string& toolbarId, const std::string& label) {
    // Check if the toolbar ID already exists
    if (toolbarExists(toolbarId)) {
        LOG_ERROR("UIExtensionRegistry: Toolbar ID '" + toolbarId + "' already exists");
        return false;
    }
    
    // Create the toolbar
    ToolbarInfo toolbarInfo;
    toolbarInfo.id = toolbarId;
    toolbarInfo.label = label;
    
    toolbars_[toolbarId] = toolbarInfo;
    
    LOG_DEBUG("UIExtensionRegistry: Created toolbar '" + toolbarId + "'");
    return true;
}

std::vector<std::string> UIExtensionRegistry::getAllMenuIds() const {
    std::vector<std::string> menuIds;
    menuIds.reserve(menus_.size());
    
    for (const auto& menu : menus_) {
        menuIds.push_back(menu.first);
    }
    
    // Sort the IDs for consistent ordering
    std::sort(menuIds.begin(), menuIds.end());
    
    return menuIds;
}

std::vector<std::string> UIExtensionRegistry::getAllToolbarIds() const {
    std::vector<std::string> toolbarIds;
    toolbarIds.reserve(toolbars_.size());
    
    for (const auto& toolbar : toolbars_) {
        toolbarIds.push_back(toolbar.first);
    }
    
    // Sort the IDs for consistent ordering
    std::sort(toolbarIds.begin(), toolbarIds.end());
    
    return toolbarIds;
}

std::vector<MenuItem> UIExtensionRegistry::getMenuItems(const std::string& menuId) const {
    std::vector<MenuItem> items;
    
    // Get the parent menu ID for the lookup
    std::string parentId = menuId.empty() ? "main" : menuId;
    
    // Find the order list for this menu
    auto orderIt = menuItemOrder_.find(parentId);
    if (orderIt != menuItemOrder_.end()) {
        // Add items in order
        for (const auto& itemId : orderIt->second) {
            auto itemIt = menuItems_.find(itemId);
            if (itemIt != menuItems_.end()) {
                items.push_back(itemIt->second);
            }
        }
    }
    
    return items;
}

std::vector<ToolbarItem> UIExtensionRegistry::getToolbarItems(const std::string& toolbarId) const {
    std::vector<ToolbarItem> items;
    
    // Find the order list for this toolbar
    auto orderIt = toolbarItemOrder_.find(toolbarId);
    if (orderIt != toolbarItemOrder_.end()) {
        // Add items in order
        for (const auto& itemId : orderIt->second) {
            auto itemIt = toolbarItems_.find(itemId);
            if (itemIt != toolbarItems_.end()) {
                items.push_back(itemIt->second);
            }
        }
    }
    
    return items;
}

std::vector<ContextMenuItem> UIExtensionRegistry::getContextMenuItems(const std::string& contextId) const {
    std::vector<ContextMenuItem> items;
    
    // Find the order list for this context
    auto orderIt = contextMenuItemOrder_.find(contextId);
    if (orderIt != contextMenuItemOrder_.end()) {
        // Add items in order
        for (const auto& itemId : orderIt->second) {
            auto itemIt = contextMenuItems_.find(itemId);
            if (itemIt != contextMenuItems_.end()) {
                items.push_back(itemIt->second);
            }
        }
    }
    
    return items;
}

bool UIExtensionRegistry::menuExists(const std::string& menuId) const {
    // Check if it's a built-in menu (like "main")
    if (menuId == "main") {
        return true;
    }
    
    // Check if it's a registered menu
    return menus_.find(menuId) != menus_.end();
}

bool UIExtensionRegistry::toolbarExists(const std::string& toolbarId) const {
    return toolbars_.find(toolbarId) != toolbars_.end();
}

void UIExtensionRegistry::insertAtPosition(std::vector<std::string>& list, 
                                       const std::string& itemId, 
                                       int position) {
    // If position is -1 or out of bounds, append to end
    if (position < 0 || position >= static_cast<int>(list.size())) {
        list.push_back(itemId);
    } else {
        // Insert at specified position
        list.insert(list.begin() + position, itemId);
    }
} 