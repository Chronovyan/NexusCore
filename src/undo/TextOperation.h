#pragma once

#include <string>
#include <vector>

namespace ai_editor {

/**
 * @class TextOperation
 * @brief Represents a single text editing operation for undo/redo functionality
 */
class TextOperation {
public:
    enum class Type {
        INSERT,  // Text was inserted
        DELETE,  // Text was deleted
        REPLACE // Text was replaced (combination of delete and insert)
    };
    
    /**
     * @brief Constructor for INSERT operation
     */
    static TextOperation createInsertion(int line, int column, const std::string& text);
    
    /**
     * @brief Constructor for DELETE operation
     */
    static TextOperation createDeletion(int line, int column, const std::string& text, 
                                       int endLine = -1, int endColumn = -1);
    
    /**
     * @brief Constructor for REPLACE operation
     */
    static TextOperation createReplacement(int line, int column, 
                                          const std::string& oldText, 
                                          const std::string& newText,
                                          int endLine = -1, int endColumn = -1);
    
    // Getters
    Type getType() const { return type_; }
    int getLine() const { return line_; }
    int getColumn() const { return column_; }
    int getEndLine() const { return endLine_; }
    int getEndColumn() const { return endColumn_; }
    const std::string& getText() const { return text_; }
    const std::string& getOldText() const { return oldText_; }
    
    /**
     * @brief Check if this is a multi-line operation
     */
    bool isMultiLine() const { return endLine_ != -1 && endLine_ != line_; }
    
    /**
     * @brief Get a description of the operation for debugging
     */
    std::string getDescription() const;
    
private:
    TextOperation() = default;
    
    Type type_;
    int line_ = 0;
    int column_ = 0;
    int endLine_ = -1;
    int endColumn_ = -1;
    std::string text_;      // Text that was inserted or new text in case of replace
    std::string oldText_;   // Only used for REPLACE operations
};

} // namespace ai_editor
