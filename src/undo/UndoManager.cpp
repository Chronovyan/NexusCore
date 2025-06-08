#include "UndoManager.h"
#include <algorithm>

namespace ai_editor {

UndoManager::UndoManager(size_t maxUndoSteps)
    : maxUndoSteps_(maxUndoSteps) {
}

void UndoManager::recordOperation(const TextOperation& operation) {
    if (inCompoundOperation_) {
        currentCompound_.push_back(operation);
    } else {
        OperationList singleOpList;
        singleOpList.push_back(operation);
        pushOperationList(std::move(singleOpList));
    }
    
    // Clear redo stack when a new operation is recorded
    redoStack_.clear();
}

bool UndoManager::undo() {
    if (inCompoundOperation_) {
        endCompoundOperation();
    }
    
    if (undoStack_.empty()) {
        return false;
    }
    
    // Move the operations from undo to redo stack (in reverse order)
    OperationList operations = std::move(undoStack_.back());
    undoStack_.pop_back();
    
    // Apply the inverse of each operation in reverse order
    for (auto it = operations.rbegin(); it != operations.rend(); ++it) {
        const auto& op = *it;
        TextOperation inverseOp;
        
        // Create inverse operation
        switch (op.getType()) {
            case TextOperation::Type::INSERT:
                inverseOp = TextOperation::createDeletion(
                    op.getLine(), op.getColumn(), op.getText());
                break;
                
            case TextOperation::Type::DELETE:
                inverseOp = TextOperation::createInsertion(
                    op.getLine(), op.getColumn(), op.getText());
                break;
                
            case TextOperation::Type::REPLACE:
                inverseOp = TextOperation::createReplacement(
                    op.getLine(), op.getColumn(), 
                    op.getText(), // New text becomes old text in inverse
                    op.getOldText(), // Old text becomes new text in inverse
                    op.getEndLine(), op.getEndColumn());
                break;
        }
        
        // Apply the inverse operation
        if (operationCallback_) {
            operationCallback_(inverseOp, false);
        }
    }
    
    // Add to redo stack
    redoStack_.push_back(std::move(operations));
    
    return true;
}

bool UndoManager::redo() {
    if (redoStack_.empty()) {
        return false;
    }
    
    // Move the operations from redo to undo stack
    OperationList operations = std::move(redoStack_.back());
    redoStack_.pop_back();
    
    // Re-apply each operation
    for (const auto& op : operations) {
        if (operationCallback_) {
            operationCallback_(op, true);
        }
    }
    
    // Add back to undo stack
    pushOperationList(std::move(operations));
    
    return true;
}

void UndoManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    currentCompound_.clear();
    inCompoundOperation_ = false;
}

std::string UndoManager::getUndoDescription() const {
    if (undoStack_.empty()) {
        return "";
    }
    
    const auto& operations = undoStack_.back();
    if (operations.empty()) {
        return "";
    }
    
    return operations.front().getDescription();
}

std::string UndoManager::getRedoDescription() const {
    if (redoStack_.empty()) {
        return "";
    }
    
    const auto& operations = redoStack_.back();
    if (operations.empty()) {
        return "";
    }
    
    return operations.front().getDescription();
}

void UndoManager::beginCompoundOperation() {
    if (inCompoundOperation_) {
        // Nested compound operations are not supported
        return;
    }
    
    inCompoundOperation_ = true;
    currentCompound_.clear();
}

void UndoManager::endCompoundOperation() {
    if (!inCompoundOperation_ || currentCompound_.empty()) {
        inCompoundOperation_ = false;
        return;
    }
    
    // Push the compound operation to the undo stack
    OperationList compoundCopy = std::move(currentCompound_);
    inCompoundOperation_ = false;
    
    pushOperationList(std::move(compoundCopy));
}

void UndoManager::pushOperationList(OperationList&& operations) {
    if (operations.empty()) {
        return;
    }
    
    // Add to undo stack
    undoStack_.push_back(std::move(operations));
    
    // Enforce maximum undo steps
    if (maxUndoSteps_ > 0 && undoStack_.size() > maxUndoSteps_) {
        undoStack_.pop_front();
    }
    
    // Clear redo stack when a new operation is recorded
    redoStack_.clear();
}

} // namespace ai_editor
