#pragma once

#include <string>
#include <vector>
#include <memory>

namespace ai_editor {

/**
 * @struct RemoteCursor
 * @brief Information about a remote user's cursor
 */
struct RemoteCursor {
    std::string userId;      // User ID
    std::string username;    // Display name
    int line;                // Cursor line
    int column;              // Cursor column
    std::string color;       // Cursor color
};

/**
 * @struct RemoteSelection
 * @brief Information about a remote user's selection
 */
struct RemoteSelection {
    std::string userId;      // User ID
    std::string username;    // Display name
    int startLine;           // Selection start line
    int startColumn;         // Selection start column
    int endLine;             // Selection end line
    int endColumn;           // Selection end column
    std::string color;       // Selection color
};

/**
 * @interface IUIManager
 * @brief Interface for managing UI elements
 */
class IUIManager {
public:
    virtual ~IUIManager() = default;
    
    /**
     * @brief Update remote cursors in the UI
     * 
     * @param cursors Information about remote cursors
     */
    virtual void updateRemoteCursors(const std::vector<RemoteCursor>& cursors) = 0;
    
    /**
     * @brief Update remote selections in the UI
     * 
     * @param selections Information about remote selections
     */
    virtual void updateRemoteSelections(const std::vector<RemoteSelection>& selections) = 0;
};

} // namespace ai_editor 