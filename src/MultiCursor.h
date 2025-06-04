#pragma once

#include "interfaces/IMultiCursor.hpp"
#include "interfaces/ITextBuffer.hpp"
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <regex>

/**
 * @class MultiCursor
 * @brief Implementation of the IMultiCursor interface
 * 
 * This class manages multiple cursors and selections in the editor.
 */
class MultiCursor : public IMultiCursor {
public:
    /**
     * @brief Constructor
     */
    MultiCursor();
    
    /**
     * @brief Destructor
     */
    virtual ~MultiCursor() = default;
    
    // IMultiCursor implementation
    size_t getCursorCount() const override;
    CursorPosition getPrimaryCursorPosition() const override;
    void setPrimaryCursorPosition(const CursorPosition& position) override;
    std::vector<CursorPosition> getAllCursorPositions() const override;
    bool addCursor(const CursorPosition& position) override;
    bool removeCursor(const CursorPosition& position) override;
    void removeAllSecondaryCursors() override;
    bool hasSelection(size_t cursorIndex = 0) const override;
    TextSelection getSelection(size_t cursorIndex = 0) const override;
    std::vector<TextSelection> getAllSelections() const override;
    void startSelection(size_t cursorIndex = 0) override;
    void updateSelection(size_t cursorIndex = 0) override;
    void clearSelection(size_t cursorIndex = 0) override;
    void clearAllSelections() override;
    void setSelectionRange(const CursorPosition& start, const CursorPosition& end, size_t cursorIndex = 0) override;
    void moveCursors(const std::string& direction, const ITextBuffer& buffer) override;
    void forEachCursor(std::function<void(const CursorPosition&)> operation) const override;
    void forEachCursorAndSelection(std::function<void(const CursorPosition&, const TextSelection&)> operation) const override;
    size_t mergeOverlappingSelections() override;
    size_t addCursorsAtAllOccurrences(const std::string& pattern, const ITextBuffer& buffer, bool caseSensitive = true) override;
    size_t addCursorsAtColumn(size_t startLine, size_t endLine, size_t column, const ITextBuffer& buffer) override;

private:
    struct CursorData {
        CursorPosition position;
        bool hasSelection;
        TextSelection selection;
        
        CursorData() 
            : position({0, 0}), hasSelection(false), selection({{0, 0}, {0, 0}}) {}

        CursorData(const CursorPosition& pos) 
            : position(pos), hasSelection(false), selection({pos, pos}) {}
        
        CursorData(const CursorPosition& pos, const TextSelection& sel) 
            : position(pos), hasSelection(true), selection(sel) {}
        
        bool operator==(const CursorData& other) const {
            return position == other.position;
        }
    };
    
    // Vector of cursors, with the primary cursor at index 0
    std::vector<CursorData> cursors_;
    
    // Helper methods
    bool isValidCursorIndex(size_t index) const;
    void validateCursorPositions(const ITextBuffer& buffer);
    void sortCursors();
    bool isCursorPositionValid(const CursorPosition& position, const ITextBuffer& buffer) const;
    CursorPosition clampPosition(const CursorPosition& position, const ITextBuffer& buffer) const;
    std::vector<CursorPosition> findAllOccurrences(const std::string& pattern, const ITextBuffer& buffer, bool caseSensitive) const;
}; 