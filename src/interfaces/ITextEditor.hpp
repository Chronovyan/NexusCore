#pragma once

#include <string>
#include <functional>
#include <memory>

namespace ai_editor {

/**
 * @enum TextChangeType
 * @brief Represents the type of text change
 */
enum class TextChangeType {
    Insert,     // Insert text
    Delete      // Delete text
};

/**
 * @struct TextChange
 * @brief Information about a text change
 */
struct TextChange {
    TextChangeType type;     // Type of change
    int position;            // Position in the document
    std::string text;        // Text to insert (for Insert)
    int length;              // Length to delete (for Delete)
};

/**
 * @typedef TextChangeCallback
 * @brief Callback for text changes
 */
using TextChangeCallback = std::function<void(const TextChange&)>;

/**
 * @typedef CursorChangeCallback
 * @brief Callback for cursor position changes
 */
using CursorChangeCallback = std::function<void(int line, int column)>;

/**
 * @typedef SelectionChangeCallback
 * @brief Callback for selection changes
 */
using SelectionChangeCallback = std::function<void(int startLine, int startColumn, int endLine, int endColumn)>;

/**
 * @class ITextEditor
 * @brief Interface for text editors
 * 
 * This interface defines the methods that any text editor implementation
 * must provide. It includes methods for getting and setting content,
 * applying changes, and registering callbacks for various events.
 */
class ITextEditor {
public:
    virtual ~ITextEditor() = default;
    
    /**
     * @brief Get the current content of the editor
     * 
     * @return std::string The editor content
     */
    virtual std::string getContent() const = 0;
    
    /**
     * @brief Set the content of the editor
     * 
     * @param content The new content
     */
    virtual void setContent(const std::string& content) = 0;
    
    /**
     * @brief Apply a change to the editor
     * 
     * @param change The change to apply
     */
    virtual void applyChange(const TextChange& change) = 0;
    
    /**
     * @brief Register a callback for text changes
     * 
     * @param callback The callback function
     * @return int A unique identifier for this callback registration
     */
    virtual int registerTextChangeCallback(const TextChangeCallback& callback) = 0;
    
    /**
     * @brief Register a callback for cursor position changes
     * 
     * @param callback The callback function
     * @return int A unique identifier for this callback registration
     */
    virtual int registerCursorChangeCallback(const CursorChangeCallback& callback) = 0;
    
    /**
     * @brief Register a callback for selection changes
     * 
     * @param callback The callback function
     * @return int A unique identifier for this callback registration
     */
    virtual int registerSelectionChangeCallback(const SelectionChangeCallback& callback) = 0;
    
    /**
     * @brief Unregister a callback
     * 
     * @param callbackId The ID of the callback to unregister
     */
    virtual void unregisterCallback(int callbackId) = 0;
};

} // namespace ai_editor 