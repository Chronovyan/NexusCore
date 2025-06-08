#pragma once

#include "TextOperation.h"
#include <deque>
#include <memory>
#include <functional>
#include <vector>

namespace ai_editor {

class Document;

/**
 * @class UndoManager
 * @brief Manages undo and redo operations for the text editor
 */
class UndoManager {
public:
    using OperationCallback = std::function<void(const TextOperation&, bool /*isRedo*/)>;
    
    /**
     * @brief Constructor
     * @param document The document this undo manager is associated with
     * @param maxUndoSteps Maximum number of undo steps to store (0 for unlimited)
     */
    explicit UndoManager(size_t maxUndoSteps = 1000);
    
    /**
     * @brief Record a new operation
     * @param operation The operation to record
     */
    void recordOperation(const TextOperation& operation);
    
    /**
     * @brief Undo the last operation
     * @return True if an operation was undone, false if there are no operations to undo
     */
    bool undo();
    
    /**
     * @brief Redo the last undone operation
     * @return True if an operation was redone, false if there are no operations to redo
     */
    bool redo();
    
    /**
     * @brief Clear the undo and redo stacks
     */
    void clear();
    
    /**
     * @brief Check if there are operations that can be undone
     */
    bool canUndo() const { return !undoStack_.empty(); }
    
    /**
     * @brief Check if there are operations that can be redone
     */
    bool canRedo() const { return !redoStack_.empty(); }
    
    /**
     * @brief Get the description of the next undo operation
     */
    std::string getUndoDescription() const;
    
    /**
     * @brief Get the description of the next redo operation
     */
    std::string getRedoDescription() const;
    
    /**
     * @brief Set a callback to be called when an operation is undone or redone
     */
    void setOperationCallback(OperationCallback callback) {
        operationCallback_ = std::move(callback);
    }
    
    /**
     * @brief Begin a compound operation (group of operations that should be undone/redone together)
     */
    void beginCompoundOperation();
    
    /**
     * @brief End a compound operation
     */
    void endCompoundOperation();
    
    /**
     * @brief Check if we're currently recording a compound operation
     */
    bool isInCompoundOperation() const { return inCompoundOperation_; }
    
    /**
     * @brief Set the maximum number of undo steps to store
     */
    void setMaxUndoSteps(size_t maxSteps) { maxUndoSteps_ = maxSteps; }
    
    /**
     * @brief Get the current number of undo steps stored
     */
    size_t getUndoCount() const { return undoStack_.size(); }
    
    /**
     * @brief Get the current number of redo steps stored
     */
    size_t getRedoCount() const { return redoStack_.size(); }
    
private:
    using OperationList = std::vector<TextOperation>;
    
    std::deque<OperationList> undoStack_;
    std::deque<OperationList> redoStack_;
    OperationCallback operationCallback_;
    size_t maxUndoSteps_;
    bool inCompoundOperation_ = false;
    OperationList currentCompound_;
    
    void pushOperationList(OperationList&& operations);
};

} // namespace ai_editor
