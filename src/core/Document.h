#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "../undo/UndoManager.h"

namespace ai_editor {

class DocumentObserver;

/**
 * @class Document
 * @brief Represents a text document with undo/redo support
 */
class Document {
public:
    using LineType = std::string;
    using LineIterator = std::vector<LineType>::iterator;
    using ConstLineIterator = std::vector<LineType>::const_iterator;
    
    /**
     * @brief Constructor
     */
    Document();
    
    /**
     * @brief Destructor
     */
    ~Document();
    
    // Non-copyable
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    
    // Move operations
    Document(Document&&) = default;
    Document& operator=(Document&&) = default;
    
    // Document operations
    
    /**
     * @brief Load content from a file
     * @param filepath Path to the file to load
     * @return True if the file was loaded successfully
     */
    bool loadFromFile(const std::string& filepath);
    
    /**
     * @brief Save content to a file
     * @param filepath Path to save to (empty to use current path)
     * @return True if the file was saved successfully
     */
    bool saveToFile(const std::string& filepath = "");
    
    /**
     * @brief Create a new empty document
     */
    void newDocument();
    
    /**
     * @brief Clear the document
     */
    void clear();
    
    // Text editing operations
    
    /**
     * @brief Insert text at the specified position
     * @param line Line number (0-based)
     * @param column Column number (0-based)
     * @param text Text to insert
     * @return True if the text was inserted successfully
     */
    bool insertText(int line, int column, const std::string& text);
    
    /**
     * @brief Delete text from the specified range
     * @param startLine Start line (0-based)
     * @param startColumn Start column (0-based)
     * @param endLine End line (inclusive)
     * @param endColumn End column (exclusive)
     * @return The deleted text
     */
    std::string deleteText(int startLine, int startColumn, int endLine, int endColumn);
    
    /**
     * @brief Replace text in the specified range
     * @param startLine Start line (0-based)
     * @param startColumn Start column (0-based)
     * @param endLine End line (inclusive)
     * @param endColumn End column (exclusive)
     * @param newText New text to insert
     * @return The replaced text
     */
    std::string replaceText(int startLine, int startColumn, 
                           int endLine, int endColumn, 
                           const std::string& newText);
    
    // Getters
    
    /**
     * @brief Get the number of lines in the document
     */
    size_t getLineCount() const { return lines_.size(); }
    
    /**
     * @brief Get a line by index
     * @param line Line number (0-based)
     * @return The line as a string
     */
    const std::string& getLine(size_t line) const;
    
    /**
     * @brief Get the entire document as a single string
     * @param lineEnding Line ending to use (default: "\n")
     * @return The document content as a string
     */
    std::string getText(const std::string& lineEnding = "\n") const;
    
    /**
     * @brief Get the file path of the document
     */
    const std::string& getFilePath() const { return filePath_; }
    
    /**
     * @brief Check if the document has been modified since the last save
     */
    bool isModified() const { return isModified_; }
    
    /**
     * @brief Get the undo manager for this document
     */
    UndoManager& getUndoManager() { return undoManager_; }
    
    /**
     * @brief Get the undo manager for this document (const version)
     */
    const UndoManager& getUndoManager() const { return undoManager_; }
    
    // Iterators
    ConstLineIterator begin() const { return lines_.begin(); }
    ConstLineIterator end() const { return lines_.end(); }
    LineIterator begin() { return lines_.begin(); }
    LineIterator end() { return lines_.end(); }
    
    // Observer management
    void addObserver(DocumentObserver* observer);
    void removeObserver(DocumentObserver* observer);
    
private:
    std::vector<LineType> lines_;
    std::string filePath_;
    bool isModified_ = false;
    UndoManager undoManager_;
    std::vector<DocumentObserver*> observers_;
    
    // Helper methods
    void notifyDocumentChanged();
    void notifyLineChanged(int line);
    void notifyLinesInserted(int startLine, int count);
    void notifyLinesRemoved(int startLine, int count);
    
    // Undo/redo callbacks
    void onUndoRedo(const TextOperation& op, bool isRedo);
};

/**
 * @class DocumentObserver
 * @brief Interface for classes that need to observe document changes
 */
class DocumentObserver {
public:
    virtual ~DocumentObserver() = default;
    
    /**
     * @brief Called when the entire document has changed
     */
    virtual void onDocumentChanged(Document* doc) = 0;
    
    /**
     * @brief Called when a specific line has changed
     * @param line The line number that changed
     */
    virtual void onLineChanged(Document* doc, int line) = 0;
    
    /**
     * @brief Called when lines are inserted
     * @param startLine The first line that was inserted
     * @param count The number of lines inserted
     */
    virtual void onLinesInserted(Document* doc, int startLine, int count) = 0;
    
    /**
     * @brief Called when lines are removed
     * @param startLine The first line that was removed
     * @param count The number of lines removed
     */
    virtual void onLinesRemoved(Document* doc, int startLine, int count) = 0;
};

} // namespace ai_editor
