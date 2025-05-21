#ifndef IUI_EXTENSION_REGISTRY_HPP
#define IUI_EXTENSION_REGISTRY_HPP

#include <string>
#include <memory>
#include <vector>
#include <functional>

/**
 * @brief Represents a menu item that can be added to menus.
 */
struct MenuItem {
    std::string id;              ///< Unique identifier for the menu item
    std::string label;           ///< Display label for the menu item
    std::string parentMenuId;    ///< ID of the parent menu (empty for top-level items)
    std::string commandId;       ///< ID of the command to execute when the item is clicked (empty for submenus)
    std::string iconPath;        ///< Path to the icon image (optional)
    bool enabled = true;         ///< Whether the menu item is enabled
    bool visible = true;         ///< Whether the menu item is visible
    bool isSeparator = false;    ///< Whether this item is a separator
    std::string tooltip;         ///< Tooltip text (optional)
    std::string shortcutKey;     ///< Keyboard shortcut (e.g., "Ctrl+S")
};

/**
 * @brief Represents a toolbar item that can be added to toolbars.
 */
struct ToolbarItem {
    std::string id;              ///< Unique identifier for the toolbar item
    std::string toolbarId;       ///< ID of the toolbar to add the item to
    std::string label;           ///< Display label for the toolbar item
    std::string commandId;       ///< ID of the command to execute when the item is clicked
    std::string iconPath;        ///< Path to the icon image
    bool enabled = true;         ///< Whether the toolbar item is enabled
    bool visible = true;         ///< Whether the toolbar item is visible
    bool isSeparator = false;    ///< Whether this item is a separator
    std::string tooltip;         ///< Tooltip text
};

/**
 * @brief Represents a context menu item that appears in right-click menus.
 */
struct ContextMenuItem {
    std::string id;              ///< Unique identifier for the context menu item
    std::string contextId;       ///< ID of the context to add the item to (e.g., "editor", "fileExplorer")
    std::string label;           ///< Display label for the context menu item
    std::string commandId;       ///< ID of the command to execute when the item is clicked
    std::string iconPath;        ///< Path to the icon image (optional)
    bool enabled = true;         ///< Whether the context menu item is enabled
    bool visible = true;         ///< Whether the context menu item is visible
    bool isSeparator = false;    ///< Whether this item is a separator
    
    /**
     * @brief Function to determine when this context menu item should be shown.
     * 
     * This callback allows context menu items to be shown only in specific contexts,
     * such as when right-clicking on certain file types or when text is selected.
     */
    std::function<bool(void*)> visibilityCallback = nullptr;
};

/**
 * @brief Interface for registering UI extensions like menu items, toolbar buttons, etc.
 * 
 * Plugins can use this interface to extend the editor's user interface with
 * custom menu items, toolbar buttons, context menu items, etc.
 */
class IUIExtensionRegistry {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IUIExtensionRegistry() = default;
    
    /**
     * @brief Add a menu item to a menu.
     * 
     * @param item The menu item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    virtual bool addMenuItem(const MenuItem& item, int position = -1) = 0;
    
    /**
     * @brief Remove a menu item.
     * 
     * @param itemId The ID of the menu item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    virtual bool removeMenuItem(const std::string& itemId) = 0;
    
    /**
     * @brief Add a toolbar item to a toolbar.
     * 
     * @param item The toolbar item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    virtual bool addToolbarItem(const ToolbarItem& item, int position = -1) = 0;
    
    /**
     * @brief Remove a toolbar item.
     * 
     * @param itemId The ID of the toolbar item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    virtual bool removeToolbarItem(const std::string& itemId) = 0;
    
    /**
     * @brief Add a context menu item.
     * 
     * @param item The context menu item to add.
     * @param position The position where to insert the item (-1 means append at the end).
     * @return true if the item was added successfully, false otherwise.
     */
    virtual bool addContextMenuItem(const ContextMenuItem& item, int position = -1) = 0;
    
    /**
     * @brief Remove a context menu item.
     * 
     * @param itemId The ID of the context menu item to remove.
     * @return true if the item was found and removed, false otherwise.
     */
    virtual bool removeContextMenuItem(const std::string& itemId) = 0;
    
    /**
     * @brief Create a new menu.
     * 
     * @param menuId A unique identifier for the menu.
     * @param label The display label for the menu.
     * @param parentMenuId The ID of the parent menu (empty for top-level menus).
     * @return true if the menu was created successfully, false otherwise.
     */
    virtual bool createMenu(const std::string& menuId, const std::string& label,
                          const std::string& parentMenuId = "") = 0;
    
    /**
     * @brief Create a new toolbar.
     * 
     * @param toolbarId A unique identifier for the toolbar.
     * @param label The display label for the toolbar.
     * @return true if the toolbar was created successfully, false otherwise.
     */
    virtual bool createToolbar(const std::string& toolbarId, const std::string& label) = 0;
    
    /**
     * @brief Get all registered menu IDs.
     * 
     * @return A vector of all registered menu IDs.
     */
    virtual std::vector<std::string> getAllMenuIds() const = 0;
    
    /**
     * @brief Get all registered toolbar IDs.
     * 
     * @return A vector of all registered toolbar IDs.
     */
    virtual std::vector<std::string> getAllToolbarIds() const = 0;
};

#endif // IUI_EXTENSION_REGISTRY_HPP 