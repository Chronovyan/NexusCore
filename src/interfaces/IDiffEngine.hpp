#pragma once

#include <string>
#include <vector>
#include <memory>

/**
 * @struct DiffChange
 * @brief Represents a single change between two texts
 */
struct DiffChange {
    enum class ChangeType {
        EQUAL,      // Text sections are identical
        INSERT,     // Text was inserted
        DELETE,     // Text was deleted
        REPLACE     // Text was replaced
    };
    
    ChangeType type;
    size_t startLine1;      // Start line in text1 (0-based)
    size_t lineCount1;      // Number of lines in text1
    size_t startLine2;      // Start line in text2 (0-based)
    size_t lineCount2;      // Number of lines in text2
    
    // For character-level diffs within a line
    size_t startChar1;      // Start character in text1 line (0-based)
    size_t charCount1;      // Number of characters in text1
    size_t startChar2;      // Start character in text2 line (0-based)
    size_t charCount2;      // Number of characters in text2
    
    bool isLineLevel;       // True if this is a line-level change, false for character-level
    
    // Helper methods
    bool isEqual() const { return type == ChangeType::EQUAL; }
    bool isInsert() const { return type == ChangeType::INSERT; }
    bool isDelete() const { return type == ChangeType::DELETE; }
    bool isReplace() const { return type == ChangeType::REPLACE; }
};

/**
 * @interface IDiffEngine
 * @brief Interface for diff engine components
 * 
 * This interface defines the contract for diff engine implementations,
 * providing methods to compute differences between texts.
 */
class IDiffEngine {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IDiffEngine() = default;
    
    /**
     * @brief Compute line-level differences between two texts
     * 
     * @param text1 First text (lines)
     * @param text2 Second text (lines)
     * @return Vector of line-level diff changes
     */
    virtual std::vector<DiffChange> computeLineDiff(
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2) = 0;
    
    /**
     * @brief Compute character-level differences between two texts
     * 
     * This method computes both line-level and character-level differences.
     * 
     * @param text1 First text (lines)
     * @param text2 Second text (lines)
     * @param charLevelForEqualLines Whether to compute character-level diffs for equal lines
     * @return Vector of line and character level diff changes
     */
    virtual std::vector<DiffChange> computeCharacterDiff(
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2,
        bool charLevelForEqualLines = false) = 0;
    
    /**
     * @brief Compute differences between two strings
     * 
     * @param str1 First string
     * @param str2 Second string
     * @return Vector of character-level diff changes
     */
    virtual std::vector<DiffChange> computeStringDiff(
        const std::string& str1,
        const std::string& str2) = 0;
    
    /**
     * @brief Format changes as a unified diff
     * 
     * @param changes Vector of diff changes
     * @param text1 First text (lines)
     * @param text2 Second text (lines)
     * @param contextLines Number of context lines to include
     * @return Formatted unified diff as a string
     */
    virtual std::string formatUnifiedDiff(
        const std::vector<DiffChange>& changes,
        const std::vector<std::string>& text1,
        const std::vector<std::string>& text2,
        size_t contextLines = 3) = 0;
};

// Type alias for smart pointer to IDiffEngine
using IDiffEnginePtr = std::shared_ptr<IDiffEngine>; 