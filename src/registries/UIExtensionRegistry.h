#ifndef UI_EXTENSION_REGISTRY_H
#define UI_EXTENSION_REGISTRY_H

#include "../interfaces/plugins/IUIExtensionRegistry.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <map>

/**
 * @brief Implementation of the IUIExtensionRegistry interface.
 * 
 * This class manages the registration and organization of UI elements
 * that can be added by plugins, such as menu items, toolbar items,
 * and context menu items.
 */
class UIExtensionRegistry : public IUIExtensionRegistry {
public:
    /**
     * @brief Constructor
     */
    UIExtensionRegistry();
    
    /**
     * @brief Destructor
     */
    ~UIExtensionRegistry() override = default;
    
    /**
     * @brief Add a menu item to a menu.
     * 
     * @param item The menu item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    bool addMenuItem(const MenuItem& item, int position = -1) override;
    
    /**
     * @brief Remove a menu item.
     * 
     * @param itemId The ID of the menu item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    bool removeMenuItem(const std::string& itemId) override;
    
    /**
     * @brief Add a toolbar item to a toolbar.
     * 
     * @param item The toolbar item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    bool addToolbarItem(const ToolbarItem& item, int position = -1) override;
    
    /**
     * @brief Remove a toolbar item.
     * 
     * @param itemId The ID of the toolbar item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    bool removeToolbarItem(const std::string& itemId) override;
    
    /**
     * @brief Add a context menu item.
     * 
     * @param item The context menu item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    bool addContextMenuItem(const ContextMenuItem& item, int position = -1) override;
    
    /**
     * @brief Remove a context menu item.
     * 
     * @param itemId The ID of the context menu item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    bool removeContextMenuItem(const std::string& itemId) override;
    
    /**
     * @brief Create a new menu.
     * 
     * @param menuId A unique identifier for the menu.
     * @param label The display label for the menu.
     * @param parentMenuId The ID of the parent menu (empty for top-level menus).
     * @return true if the menu was created successfully, false otherwise.
     */
    bool createMenu(const std::string& menuId, const std::string& label,
                  const std::string& parentMenuId = "") override;
    
    /**
     * @brief Create a new toolbar.
     * 
     * @param toolbarId A unique identifier for the toolbar.
     * @param label The display label for the toolbar.
     * @return true if the toolbar was created successfully, false otherwise.
     */
    bool createToolbar(const std::string& toolbarId, const std::string& label) override;
    
    /**
     * @brief Get all registered menu IDs.
     * 
     * @return A vector of all registered menu IDs.
     */
    std::vector<std::string> getAllMenuIds() const override;
    
    /**
     * @brief Get all registered toolbar IDs.
     * 
     * @return A vector of all registered toolbar IDs.
     */
    std::vector<std::string> getAllToolbarIds() const override;
    
    /**
     * @brief Get all menu items for a specific menu.
     * 
     * @param menuId The ID of the menu to get items for.
     * @return A vector of menu items in the correct order.
     */
    std::vector<MenuItem> getMenuItems(const std::string& menuId) const;
    
    /**
     * @brief Get all toolbar items for a specific toolbar.
     * 
     * @param toolbarId The ID of the toolbar to get items for.
     * @return A vector of toolbar items in the correct order.
     */
    std::vector<ToolbarItem> getToolbarItems(const std::string& toolbarId) const;
    
    /**
     * @brief Get all context menu items for a specific context.
     * 
     * @param contextId The ID of the context to get items for.
     * @return A vector of context menu items in the correct order.
     */
    std::vector<ContextMenuItem> getContextMenuItems(const std::string& contextId) const;
    
private:
    // Menu-related data structures
    struct MenuInfo {
        std::string id;
        std::string label;
        std::string parentId;
    };
    
    std::unordered_map<std::string, MenuInfo> menus_;
    std::unordered_map<std::string, MenuItem> menuItems_;
    std::map<std::string, std::vector<std::string>> menuItemOrder_; // Maps menuId -> ordered list of item IDs
    
    // Toolbar-related data structures
    struct ToolbarInfo {
        std::string id;
        std::string label;
    };
    
    std::unordered_map<std::string, ToolbarInfo> toolbars_;
    std::unordered_map<std::string, ToolbarItem> toolbarItems_;
    std::map<std::string, std::vector<std::string>> toolbarItemOrder_; // Maps toolbarId -> ordered list of item IDs
    
    // Context menu-related data structures
    std::unordered_map<std::string, ContextMenuItem> contextMenuItems_;
    std::map<std::string, std::vector<std::string>> contextMenuItemOrder_; // Maps contextId -> ordered list of item IDs
    
    /**
     * @brief Check if a menu exists.
     * 
     * @param menuId The ID of the menu to check.
     * @return true if the menu exists, false otherwise.
     */
    bool menuExists(const std::string& menuId) const;
    
    /**
     * @brief Check if a toolbar exists.
     * 
     * @param toolbarId The ID of the toolbar to check.
     * @return true if the toolbar exists, false otherwise.
     */
    bool toolbarExists(const std::string& toolbarId) const;
    
    /**
     * @brief Insert an item ID at the specified position in an ordered list.
     * 
     * @param list The ordered list of item IDs.
     * @param itemId The ID of the item to insert.
     * @param position The position where to insert the item (-1 means append at the end).
     */
    void insertAtPosition(std::vector<std::string>& list, const std::string& itemId, int position);
};

#endif // UI_EXTENSION_REGISTRY_H 