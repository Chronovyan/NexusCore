#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "ITextBuffer.hpp"

/**
 * @struct CursorPosition
 * @brief Represents a cursor position with line and column coordinates
 */
struct CursorPosition {
    size_t line;
    size_t column;
    
    bool operator==(const CursorPosition& other) const {
        return line == other.line && column == other.column;
    }
    
    bool operator!=(const CursorPosition& other) const {
        return !(*this == other);
    }
    
    bool operator<(const CursorPosition& other) const {
        return (line < other.line) || (line == other.line && column < other.column);
    }
    
    bool operator<=(const CursorPosition& other) const {
        return (*this < other) || (*this == other);
    }
    
    bool operator>(const CursorPosition& other) const {
        return !(*this <= other);
    }
    
    bool operator>=(const CursorPosition& other) const {
        return !(*this < other);
    }
};

/**
 * @struct TextSelection
 * @brief Represents a text selection with start and end positions
 */
struct TextSelection {
    CursorPosition start;
    CursorPosition end;
    
    bool isEmpty() const {
        return start == end;
    }
    
    // Returns true if this selection contains the given position
    bool contains(const CursorPosition& position) const {
        return (start <= position && position <= end) ||
               (end <= position && position <= start);
    }
    
    // Returns true if this selection overlaps with another selection
    bool overlaps(const TextSelection& other) const {
        return contains(other.start) || contains(other.end) ||
               other.contains(start) || other.contains(end);
    }
    
    // Normalizes the selection so that start <= end
    void normalize() {
        if (end < start) {
            std::swap(start, end);
        }
    }
};

/**
 * @interface IMultiCursor
 * @brief Interface for managing multiple cursors and selections
 * 
 * This interface defines the contract for the multiple cursor component,
 * providing methods for creating, managing, and manipulating multiple
 * cursors and selections in the editor.
 */
class IMultiCursor {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IMultiCursor() = default;
    
    /**
     * @brief Get the number of active cursors
     * 
     * @return The number of cursors
     */
    virtual size_t getCursorCount() const = 0;
    
    /**
     * @brief Get the position of the primary cursor
     * 
     * @return The position of the primary cursor
     */
    virtual CursorPosition getPrimaryCursorPosition() const = 0;
    
    /**
     * @brief Set the position of the primary cursor
     * 
     * @param position The new position for the primary cursor
     */
    virtual void setPrimaryCursorPosition(const CursorPosition& position) = 0;
    
    /**
     * @brief Get the positions of all cursors
     * 
     * @return Vector of cursor positions, with the primary cursor first
     */
    virtual std::vector<CursorPosition> getAllCursorPositions() const = 0;
    
    /**
     * @brief Add a new cursor at the specified position
     * 
     * @param position The position for the new cursor
     * @return True if the cursor was added, false if it couldn't be added
     */
    virtual bool addCursor(const CursorPosition& position) = 0;
    
    /**
     * @brief Remove the cursor at the specified position
     * 
     * @param position The position of the cursor to remove
     * @return True if a cursor was removed, false otherwise
     */
    virtual bool removeCursor(const CursorPosition& position) = 0;
    
    /**
     * @brief Remove all cursors except the primary one
     */
    virtual void removeAllSecondaryCursors() = 0;
    
    /**
     * @brief Check if there's a selection at the specified cursor
     * 
     * @param cursorIndex Index of the cursor to check (0 is primary)
     * @return True if the cursor has a selection, false otherwise
     */
    virtual bool hasSelection(size_t cursorIndex = 0) const = 0;
    
    /**
     * @brief Get the selection for the specified cursor
     * 
     * @param cursorIndex Index of the cursor (0 is primary)
     * @return The selection for the cursor
     */
    virtual TextSelection getSelection(size_t cursorIndex = 0) const = 0;
    
    /**
     * @brief Get all active selections
     * 
     * @return Vector of all selections
     */
    virtual std::vector<TextSelection> getAllSelections() const = 0;
    
    /**
     * @brief Start a selection at the specified cursor
     * 
     * @param cursorIndex Index of the cursor (0 is primary)
     */
    virtual void startSelection(size_t cursorIndex = 0) = 0;
    
    /**
     * @brief Update the selection at the specified cursor to its current position
     * 
     * @param cursorIndex Index of the cursor (0 is primary)
     */
    virtual void updateSelection(size_t cursorIndex = 0) = 0;
    
    /**
     * @brief Clear the selection at the specified cursor
     * 
     * @param cursorIndex Index of the cursor (0 is primary)
     */
    virtual void clearSelection(size_t cursorIndex = 0) = 0;
    
    /**
     * @brief Clear all selections
     */
    virtual void clearAllSelections() = 0;
    
    /**
     * @brief Set the selection range for the specified cursor
     * 
     * @param start Start position of the selection
     * @param end End position of the selection
     * @param cursorIndex Index of the cursor (0 is primary)
     */
    virtual void setSelectionRange(const CursorPosition& start, const CursorPosition& end, size_t cursorIndex = 0) = 0;
    
    /**
     * @brief Move all cursors in the specified direction
     * 
     * @param direction Direction to move (e.g., "up", "down", "left", "right")
     * @param buffer Text buffer for boundary checking
     */
    virtual void moveCursors(const std::string& direction, const ITextBuffer& buffer) = 0;
    
    /**
     * @brief Apply a function to each cursor position
     * 
     * @param operation Function to apply to each cursor position
     */
    virtual void forEachCursor(std::function<void(const CursorPosition&)> operation) const = 0;
    
    /**
     * @brief Apply a function to each cursor position and selection
     * 
     * @param operation Function to apply to each cursor and its selection
     */
    virtual void forEachCursorAndSelection(std::function<void(const CursorPosition&, const TextSelection&)> operation) const = 0;
    
    /**
     * @brief Merge overlapping selections
     * 
     * @return Number of selections after merging
     */
    virtual size_t mergeOverlappingSelections() = 0;
    
    /**
     * @brief Add cursors at all occurrences of a pattern
     * 
     * @param pattern Pattern to search for
     * @param buffer Text buffer to search in
     * @param caseSensitive Whether to use case-sensitive search
     * @return Number of new cursors added
     */
    virtual size_t addCursorsAtAllOccurrences(const std::string& pattern, const ITextBuffer& buffer, bool caseSensitive = true) = 0;
    
    /**
     * @brief Add cursors at the same column position on multiple lines
     * 
     * @param startLine Start line for column selection
     * @param endLine End line for column selection
     * @param column Column position for all cursors
     * @param buffer Text buffer for boundary checking
     * @return Number of new cursors added
     */
    virtual size_t addCursorsAtColumn(size_t startLine, size_t endLine, size_t column, const ITextBuffer& buffer) = 0;
}; 