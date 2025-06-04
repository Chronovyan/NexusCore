#include "Editor.h"
#include "EditorCommands.h"
#include <sstream>
#include "AppDebugLog.h"

// Implementation of diff and merge methods

std::vector<std::string> Editor::getCurrentTextAsLines() const {
    std::vector<std::string> lines;
    
    // Get the number of lines in the buffer
    size_t lineCount = textBuffer_->lineCount();
    
    // Reserve space to avoid reallocations
    lines.reserve(lineCount);
    
    // Copy each line from the buffer
    for (size_t i = 0; i < lineCount; ++i) {
        lines.push_back(textBuffer_->getLine(i));
    }
    
    return lines;
}

bool Editor::loadTextFromFile(const std::string& filename, std::vector<std::string>& lines) {
    lines.clear();
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file: " + filename);
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading file: " + std::string(e.what()));
        return false;
    }
}

bool Editor::showDiff(const std::vector<std::string>& text1, const std::vector<std::string>& text2) {
    if (!diffEngine_) {
        LOG_ERROR("Diff engine not available");
        return false;
    }
    
    try {
        // Compute differences
        auto changes = diffEngine_->computeLineDiff(text1, text2);
        
        // Clear the current buffer
        while (textBuffer_->lineCount() > 0) {
            textBuffer_->deleteLine(0);
        }
        
        // Format the diff
        std::string formattedDiff = diffEngine_->formatUnifiedDiff(changes, text1, text2);
        
        // Split the formatted diff into lines
        std::istringstream iss(formattedDiff);
        std::string line;
        while (std::getline(iss, line)) {
            textBuffer_->addLine(line);
        }
        
        // Reset cursor and scroll position
        setCursor(0, 0);
        
        // Mark the buffer as modified
        setModified(true);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error creating diff: " + std::string(e.what()));
        return false;
    }
}

bool Editor::diffWithCurrent(const std::vector<std::string>& otherText) {
    // Get the current text as lines
    auto currentText = getCurrentTextAsLines();
    
    // Show the diff
    return showDiff(currentText, otherText);
}

bool Editor::diffWithFile(const std::string& filename) {
    // Get the current text as lines
    auto currentText = getCurrentTextAsLines();
    
    // Load the other file
    std::vector<std::string> otherText;
    if (!loadTextFromFile(filename, otherText)) {
        return false;
    }
    
    // Show the diff
    return showDiff(currentText, otherText);
}

bool Editor::mergeTexts(
    const std::vector<std::string>& base,
    const std::vector<std::string>& ours,
    const std::vector<std::string>& theirs) {
    if (!mergeEngine_) {
        LOG_ERROR("Merge engine not available");
        return false;
    }
    
    try {
        // Perform the merge
        currentMergeResult_ = mergeEngine_->merge(base, ours, theirs);
        
        // Clear the current buffer
        while (textBuffer_->lineCount() > 0) {
            textBuffer_->deleteLine(0);
        }
        
        // Add the merged lines to the buffer
        for (const auto& line : currentMergeResult_.mergedLines) {
            textBuffer_->addLine(line);
        }
        
        // Reset cursor and scroll position
        setCursor(0, 0);
        
        // Mark the buffer as modified
        setModified(true);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error performing merge: " + std::string(e.what()));
        return false;
    }
}

bool Editor::mergeWithFile(const std::string& theirFile, const std::string& baseFile) {
    // Get our current text
    auto ours = getCurrentTextAsLines();
    
    // Load the base file
    std::vector<std::string> base;
    if (!loadTextFromFile(baseFile, base)) {
        return false;
    }
    
    // Load their file
    std::vector<std::string> theirs;
    if (!loadTextFromFile(theirFile, theirs)) {
        return false;
    }
    
    // Perform the merge
    return mergeTexts(base, ours, theirs);
}

bool Editor::applyDiffChanges(
    const std::vector<DiffChange>& changes,
    const std::vector<std::string>& sourceText) {
    try {
        // Start a transaction to group all changes
        commandManager_->beginTransaction();
        
        // Apply each change in reverse order to avoid position shifting
        for (auto it = changes.rbegin(); it != changes.rend(); ++it) {
            const auto& change = *it;
            
            // Skip equal changes
            if (change.isEqual()) {
                continue;
            }
            
            if (change.isInsert()) {
                // Insert text
                std::string insertText;
                for (size_t i = 0; i < change.lineCount2; ++i) {
                    insertText += sourceText[change.startLine2 + i];
                    if (i < change.lineCount2 - 1) {
                        insertText += "\n";
                    }
                }
                
                // Set cursor position
                setCursor(change.startLine1, 0);
                
                // Insert the text
                typeText(insertText);
            } else if (change.isDelete()) {
                // Delete lines
                for (size_t i = 0; i < change.lineCount1; ++i) {
                    textBuffer_->deleteLine(change.startLine1);
                }
            } else if (change.isReplace()) {
                // Delete the old lines
                for (size_t i = 0; i < change.lineCount1; ++i) {
                    textBuffer_->deleteLine(change.startLine1);
                }
                
                // Insert the new lines
                std::string insertText;
                for (size_t i = 0; i < change.lineCount2; ++i) {
                    if (change.startLine2 + i < sourceText.size()) {
                        insertText += sourceText[change.startLine2 + i];
                        if (i < change.lineCount2 - 1) {
                            insertText += "\n";
                        }
                    }
                }
                
                // Set cursor position
                setCursor(change.startLine1, 0);
                
                // Insert the text
                typeText(insertText);
            }
        }
        
        // End the transaction
        commandManager_->endTransaction();
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error applying diff changes: " + std::string(e.what()));
        
        // Abort the transaction
        commandManager_->cancelTransaction();
        
        return false;
    }
}

bool Editor::resolveConflict(
    size_t conflictIndex,
    MergeConflictResolution resolution,
    const std::vector<std::string>& customResolution) {
    if (!mergeEngine_) {
        LOG_ERROR("Merge engine not available");
        return false;
    }
    
    try {
        // Resolve the conflict
        bool success = mergeEngine_->resolveConflict(
            currentMergeResult_,
            conflictIndex,
            resolution,
            customResolution);
        
        if (!success) {
            LOG_ERROR("Failed to resolve conflict");
            return false;
        }
        
        // Apply all resolutions
        success = mergeEngine_->applyResolutions(currentMergeResult_);
        
        if (!success) {
            LOG_ERROR("Failed to apply conflict resolutions");
            return false;
        }
        
        // Clear the current buffer
        while (textBuffer_->lineCount() > 0) {
            textBuffer_->deleteLine(0);
        }
        
        // Add the merged lines to the buffer
        for (const auto& line : currentMergeResult_.mergedLines) {
            textBuffer_->addLine(line);
        }
        
        // Reset cursor and scroll position
        setCursor(0, 0);
        
        // Mark the buffer as modified
        setModified(true);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error resolving conflict: " + std::string(e.what()));
        return false;
    }
} 