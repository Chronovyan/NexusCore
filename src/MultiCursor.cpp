#include "MultiCursor.h"
#include <algorithm>
#include <stdexcept>
#include <set>
#include <regex>

MultiCursor::MultiCursor() {
    // Initialize with a single cursor at position (0, 0)
    cursors_.emplace_back(CursorPosition{0, 0});
}

size_t MultiCursor::getCursorCount() const {
    return cursors_.size();
}

CursorPosition MultiCursor::getPrimaryCursorPosition() const {
    // The primary cursor is always at index 0
    return cursors_[0].position;
}

void MultiCursor::setPrimaryCursorPosition(const CursorPosition& position) {
    cursors_[0].position = position;
}

std::vector<CursorPosition> MultiCursor::getAllCursorPositions() const {
    std::vector<CursorPosition> positions;
    positions.reserve(cursors_.size());
    
    for (const auto& cursor : cursors_) {
        positions.push_back(cursor.position);
    }
    
    return positions;
}

bool MultiCursor::addCursor(const CursorPosition& position) {
    // Check if a cursor already exists at this position
    for (const auto& cursor : cursors_) {
        if (cursor.position == position) {
            return false; // Cursor already exists at this position
        }
    }
    
    // Add the new cursor
    cursors_.emplace_back(position);
    
    return true;
}

bool MultiCursor::removeCursor(const CursorPosition& position) {
    // We must always have at least one cursor (the primary one)
    if (cursors_.size() <= 1) {
        return false;
    }
    
    // Find the cursor at the specified position
    auto it = std::find_if(cursors_.begin() + 1, cursors_.end(),
                          [&position](const CursorData& cursor) {
                              return cursor.position == position;
                          });
    
    // If found, remove it
    if (it != cursors_.end()) {
        cursors_.erase(it);
        return true;
    }
    
    return false; // No cursor found at the specified position
}

void MultiCursor::removeAllSecondaryCursors() {
    // Keep only the primary cursor
    if (cursors_.size() > 1) {
        cursors_.resize(1);
    }
}

bool MultiCursor::hasSelection(size_t cursorIndex) const {
    if (!isValidCursorIndex(cursorIndex)) {
        return false;
    }
    
    return cursors_[cursorIndex].hasSelection;
}

TextSelection MultiCursor::getSelection(size_t cursorIndex) const {
    if (!isValidCursorIndex(cursorIndex) || !cursors_[cursorIndex].hasSelection) {
        // Return an empty selection at the cursor position
        const CursorPosition& pos = cursorIndex < cursors_.size() ? 
                                    cursors_[cursorIndex].position : 
                                    cursors_[0].position;
        return {pos, pos};
    }
    
    return cursors_[cursorIndex].selection;
}

std::vector<TextSelection> MultiCursor::getAllSelections() const {
    std::vector<TextSelection> selections;
    
    for (const auto& cursor : cursors_) {
        if (cursor.hasSelection) {
            selections.push_back(cursor.selection);
        }
    }
    
    return selections;
}

void MultiCursor::startSelection(size_t cursorIndex) {
    if (!isValidCursorIndex(cursorIndex)) {
        return;
    }
    
    auto& cursor = cursors_[cursorIndex];
    cursor.hasSelection = true;
    cursor.selection.start = cursor.position;
    cursor.selection.end = cursor.position;
}

void MultiCursor::updateSelection(size_t cursorIndex) {
    if (!isValidCursorIndex(cursorIndex) || !cursors_[cursorIndex].hasSelection) {
        return;
    }
    
    auto& cursor = cursors_[cursorIndex];
    cursor.selection.end = cursor.position;
}

void MultiCursor::clearSelection(size_t cursorIndex) {
    if (!isValidCursorIndex(cursorIndex)) {
        return;
    }
    
    auto& cursor = cursors_[cursorIndex];
    cursor.hasSelection = false;
}

void MultiCursor::clearAllSelections() {
    for (auto& cursor : cursors_) {
        cursor.hasSelection = false;
    }
}

void MultiCursor::setSelectionRange(const CursorPosition& start, const CursorPosition& end, size_t cursorIndex) {
    if (!isValidCursorIndex(cursorIndex)) {
        return;
    }
    
    auto& cursor = cursors_[cursorIndex];
    cursor.hasSelection = true;
    cursor.selection.start = start;
    cursor.selection.end = end;
}

void MultiCursor::moveCursors(const std::string& direction, const ITextBuffer& buffer) {
    for (auto& cursor : cursors_) {
        if (direction == "up") {
            if (cursor.position.line > 0) {
                cursor.position.line--;
                // Adjust column if needed
                const std::string& line = buffer.getLine(cursor.position.line);
                cursor.position.column = std::min(cursor.position.column, line.length());
            }
        } else if (direction == "down") {
            if (cursor.position.line < buffer.lineCount() - 1) {
                cursor.position.line++;
                // Adjust column if needed
                const std::string& line = buffer.getLine(cursor.position.line);
                cursor.position.column = std::min(cursor.position.column, line.length());
            }
        } else if (direction == "left") {
            if (cursor.position.column > 0) {
                cursor.position.column--;
            } else if (cursor.position.line > 0) {
                cursor.position.line--;
                const std::string& line = buffer.getLine(cursor.position.line);
                cursor.position.column = line.length();
            }
        } else if (direction == "right") {
            const std::string& line = buffer.getLine(cursor.position.line);
            if (cursor.position.column < line.length()) {
                cursor.position.column++;
            } else if (cursor.position.line < buffer.lineCount() - 1) {
                cursor.position.line++;
                cursor.position.column = 0;
            }
        } else if (direction == "home") {
            cursor.position.column = 0;
        } else if (direction == "end") {
            const std::string& line = buffer.getLine(cursor.position.line);
            cursor.position.column = line.length();
        }
        
        // Update selection if active
        if (cursor.hasSelection) {
            cursor.selection.end = cursor.position;
        }
    }
    
    // Deduplicate cursors after movement
    sortCursors();
    auto last = std::unique(cursors_.begin(), cursors_.end());
    cursors_.erase(last, cursors_.end());
    
    // Ensure we always have at least one cursor
    if (cursors_.empty()) {
        cursors_.emplace_back(CursorPosition{0, 0});
    }
}

void MultiCursor::forEachCursor(std::function<void(const CursorPosition&)> operation) const {
    for (const auto& cursor : cursors_) {
        operation(cursor.position);
    }
}

void MultiCursor::forEachCursorAndSelection(std::function<void(const CursorPosition&, const TextSelection&)> operation) const {
    for (const auto& cursor : cursors_) {
        if (cursor.hasSelection) {
            operation(cursor.position, cursor.selection);
        } else {
            // Create an empty selection at the cursor position
            TextSelection emptySelection{cursor.position, cursor.position};
            operation(cursor.position, emptySelection);
        }
    }
}

size_t MultiCursor::mergeOverlappingSelections() {
    // Early exit if there are no or only one selection
    if (cursors_.size() <= 1) {
        return cursors_.size();
    }
    
    // First, collect all active selections
    std::vector<TextSelection> selections;
    for (const auto& cursor : cursors_) {
        if (cursor.hasSelection) {
            TextSelection sel = cursor.selection;
            sel.normalize();
            selections.push_back(sel);
        }
    }
    
    if (selections.empty()) {
        return cursors_.size();
    }
    
    // Sort selections by start position
    std::sort(selections.begin(), selections.end(), 
              [](const TextSelection& a, const TextSelection& b) {
                  return a.start < b.start;
              });
    
    // Merge overlapping selections
    std::vector<TextSelection> mergedSelections;
    mergedSelections.push_back(selections[0]);
    
    for (size_t i = 1; i < selections.size(); ++i) {
        TextSelection& current = mergedSelections.back();
        const TextSelection& next = selections[i];
        
        if (current.overlaps(next) || 
            (next.start.line == current.end.line && next.start.column == current.end.column)) {
            // Merge the selections
            current.end = (next.end > current.end) ? next.end : current.end;
        } else {
            // Add as a new selection
            mergedSelections.push_back(next);
        }
    }
    
    // Replace cursors with the merged selections
    cursors_.clear();
    
    // Recreate cursors from merged selections
    for (const auto& sel : mergedSelections) {
        CursorData newCursor(sel.end);
        newCursor.hasSelection = true;
        newCursor.selection = sel;
        cursors_.push_back(newCursor);
    }
    
    // Ensure we always have at least one cursor
    if (cursors_.empty()) {
        cursors_.emplace_back(CursorPosition{0, 0});
    }
    
    return cursors_.size();
}

size_t MultiCursor::addCursorsAtAllOccurrences(const std::string& pattern, const ITextBuffer& buffer, bool caseSensitive) {
    if (pattern.empty()) {
        return 0;
    }
    
    // Find all occurrences of the pattern
    std::vector<CursorPosition> positions = findAllOccurrences(pattern, buffer, caseSensitive);
    
    // Early exit if no occurrences found
    if (positions.empty()) {
        return 0;
    }
    
    // Set of existing cursor positions for quick lookup
    std::set<CursorPosition*> existingPositions;
    for (auto& cursor : cursors_) {
        existingPositions.insert(&cursor.position);
    }
    
    // Add new cursors for positions that don't already have a cursor
    size_t addedCount = 0;
    for (const auto& pos : positions) {
        bool alreadyExists = false;
        for (const auto& cursor : cursors_) {
            if (cursor.position == pos) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            cursors_.emplace_back(pos);
            addedCount++;
        }
    }
    
    // Sort cursors after adding new ones
    sortCursors();
    
    return addedCount;
}

size_t MultiCursor::addCursorsAtColumn(size_t startLine, size_t endLine, size_t column, const ITextBuffer& buffer) {
    // Validate inputs
    if (startLine > endLine || startLine >= buffer.lineCount()) {
        return 0;
    }
    
    // Clamp endLine to buffer size
    endLine = std::min(endLine, buffer.lineCount() - 1);
    
    // Add cursors at the specified column for each line
    size_t addedCount = 0;
    for (size_t line = startLine; line <= endLine; ++line) {
        const std::string& lineText = buffer.getLine(line);
        size_t col = std::min(column, lineText.length());
        
        CursorPosition pos{line, col};
        
        // Check if this position already has a cursor
        bool alreadyExists = false;
        for (const auto& cursor : cursors_) {
            if (cursor.position == pos) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            cursors_.emplace_back(pos);
            addedCount++;
        }
    }
    
    // Sort cursors after adding new ones
    sortCursors();
    
    return addedCount;
}

// Private helper methods

bool MultiCursor::isValidCursorIndex(size_t index) const {
    return index < cursors_.size();
}

void MultiCursor::validateCursorPositions(const ITextBuffer& buffer) {
    for (auto& cursor : cursors_) {
        cursor.position = clampPosition(cursor.position, buffer);
        
        if (cursor.hasSelection) {
            cursor.selection.start = clampPosition(cursor.selection.start, buffer);
            cursor.selection.end = clampPosition(cursor.selection.end, buffer);
        }
    }
}

void MultiCursor::sortCursors() {
    // Save the primary cursor
    CursorData primaryCursor = cursors_[0];
    
    // Sort all cursors by position
    std::sort(cursors_.begin(), cursors_.end(),
             [](const CursorData& a, const CursorData& b) {
                 return a.position < b.position;
             });
    
    // Find and move the primary cursor back to index 0
    auto it = std::find(cursors_.begin(), cursors_.end(), primaryCursor);
    if (it != cursors_.begin() && it != cursors_.end()) {
        // Rotate to put the primary cursor at the beginning
        std::rotate(cursors_.begin(), it, it + 1);
    }
}

bool MultiCursor::isCursorPositionValid(const CursorPosition& position, const ITextBuffer& buffer) const {
    if (buffer.isEmpty()) {
        return position.line == 0 && position.column == 0;
    }
    
    if (position.line >= buffer.lineCount()) {
        return false;
    }
    
    const std::string& line = buffer.getLine(position.line);
    return position.column <= line.length();
}

CursorPosition MultiCursor::clampPosition(const CursorPosition& position, const ITextBuffer& buffer) const {
    CursorPosition clamped = position;
    
    if (buffer.isEmpty()) {
        return {0, 0};
    }
    
    // Clamp line to valid range
    clamped.line = std::min(clamped.line, buffer.lineCount() - 1);
    
    // Clamp column to valid range for the line
    const std::string& line = buffer.getLine(clamped.line);
    clamped.column = std::min(clamped.column, line.length());
    
    return clamped;
}

std::vector<CursorPosition> MultiCursor::findAllOccurrences(const std::string& pattern, const ITextBuffer& buffer, bool caseSensitive) const {
    std::vector<CursorPosition> positions;
    
    // Early exit if pattern is empty or buffer is empty
    if (pattern.empty() || buffer.isEmpty()) {
        return positions;
    }
    
    // Create regex flags
    std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
    if (!caseSensitive) {
        flags |= std::regex_constants::icase;
    }
    
    // Create regex object
    std::regex patternRegex(pattern, flags);
    
    // Search for pattern in each line
    for (size_t lineIdx = 0; lineIdx < buffer.lineCount(); ++lineIdx) {
        const std::string& line = buffer.getLine(lineIdx);
        
        // Use regex to find all matches in the line
        std::sregex_iterator it(line.begin(), line.end(), patternRegex);
        std::sregex_iterator end;
        
        while (it != end) {
            const std::smatch& match = *it;
            positions.push_back({lineIdx, static_cast<size_t>(match.position())});
            ++it;
        }
    }
    
    return positions;
} 