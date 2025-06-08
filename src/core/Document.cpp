#include "Document.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace ai_editor {

Document::Document() {
    // Start with a single empty line
    lines_.push_back("");
    
    // Set up undo/redo callbacks
    undoManager_.setOperationCallback(
        [this](const TextOperation& op, bool isRedo) {
            onUndoRedo(op, isRedo);
        });
}

Document::~Document() = default;

bool Document::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // Read file line by line
    lines_.clear();
    std::string line;
    while (std::getline(file, line)) {
        // Handle different line endings
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines_.push_back(line);
    }
    
    // If file is empty, ensure we have at least one line
    if (lines_.empty()) {
        lines_.push_back("");
    }
    
    filePath_ = filepath;
    isModified_ = false;
    undoManager_.clear();
    
    notifyDocumentChanged();
    return true;
}

bool Document::saveToFile(const std::string& filepath) {
    std::string savePath = filepath.empty() ? filePath_ : filepath;
    if (savePath.empty()) {
        return false; // No file path specified
    }
    
    std::ofstream file(savePath);
    if (!file.is_open()) {
        return false;
    }
    
    // Write all lines with appropriate line endings
    for (size_t i = 0; i < lines_.size(); ++i) {
        file << lines_[i];
        if (i < lines_.size() - 1) {
            file << '\n'; // Use native line endings
        }
    }
    
    filePath_ = savePath;
    isModified_ = false;
    
    return true;
}

void Document::newDocument() {
    lines_.clear();
    lines_.push_back("");
    filePath_.clear();
    isModified_ = false;
    undoManager_.clear();
    
    notifyDocumentChanged();
}

void Document::clear() {
    lines_.clear();
    lines_.push_back("");
    isModified_ = true;
    undoManager_.clear();
    
    notifyDocumentChanged();
}

bool Document::insertText(int line, int column, const std::string& text) {
    if (line < 0 || line >= static_cast<int>(lines_.size()) || column < 0) {
        return false;
    }
    
    // Handle multi-line insert
    size_t pos = 0;
    size_t nextPos;
    int currentLine = line;
    int currentCol = column;
    
    undoManager_.beginCompoundOperation();
    
    do {
        nextPos = text.find('\n', pos);
        std::string part = text.substr(pos, nextPos - pos);
        
        if (currentLine >= static_cast<int>(lines_.size())) {
            lines_.push_back("");
        }
        
        if (nextPos == std::string::npos) {
            // Last part, just insert text
            if (currentCol > static_cast<int>(lines_[currentLine].length())) {
                // Pad with spaces if needed
                lines_[currentLine].append(currentCol - lines_[currentLine].length(), ' ');
            }
            lines_[currentLine].insert(currentCol, part);
            
            // Record the insertion
            TextOperation op = TextOperation::createInsertion(currentLine, currentCol, part);
            undoManager_.recordOperation(op);
            
            notifyLineChanged(currentLine);
        } else {
            // Split the line
            std::string firstPart = lines_[currentLine].substr(0, currentCol);
            std::string secondPart = lines_[currentLine].substr(currentCol);
            
            // Record the split operation
            TextOperation op = TextOperation::createReplacement(
                currentLine, currentCol, 
                secondPart, 
                part + '\n' + secondPart);
            undoManager_.recordOperation(op);
            
            // Update the document
            lines_[currentLine] = firstPart + part;
            lines_.insert(lines_.begin() + currentLine + 1, secondPart);
            
            notifyLinesInserted(currentLine + 1, 1);
            
            currentLine++;
            currentCol = 0;
            pos = nextPos + 1;
        }
    } while (nextPos != std::string::npos);
    
    undoManager_.endCompoundOperation();
    isModified_ = true;
    
    return true;
}

std::string Document::deleteText(int startLine, int startColumn, int endLine, int endColumn) {
    if (startLine < 0 || startLine >= static_cast<int>(lines_.size()) ||
        endLine < 0 || endLine >= static_cast<int>(lines_.size()) ||
        startColumn < 0 || endColumn < 0) {
        return "";
    }
    
    // Ensure start is before end
    if (endLine < startLine || (endLine == startLine && endColumn < startColumn)) {
        std::swap(startLine, endLine);
        std::swap(startColumn, endColumn);
    }
    
    std::string deletedText;
    
    undoManager_.beginCompoundOperation();
    
    if (startLine == endLine) {
        // Single line deletion
        std::string& line = lines_[startLine];
        if (startColumn < static_cast<int>(line.length())) {
            deletedText = line.substr(startColumn, endColumn - startColumn);
            line.erase(startColumn, endColumn - startColumn);
            
            TextOperation op = TextOperation::createDeletion(
                startLine, startColumn, deletedText);
            undoManager_.recordOperation(op);
            
            notifyLineChanged(startLine);
        }
    } else {
        // Multi-line deletion
        std::string& firstLine = lines_[startLine];
        std::string& lastLine = lines_[endLine];
        
        // Get the deleted text with newlines
        std::ostringstream oss;
        oss << firstLine.substr(startColumn) << '\n';
        
        for (int i = startLine + 1; i < endLine; ++i) {
            oss << lines_[i] << '\n';
        }
        
        if (endColumn > 0) {
            oss << lastLine.substr(0, endColumn);
        }
        
        deletedText = oss.str();
        
        // Record the deletion
        TextOperation op = TextOperation::createDeletion(
            startLine, startColumn, 
            deletedText,
            endLine, endColumn);
        undoManager_.recordOperation(op);
        
        // Update the document
        std::string newFirstLine = firstLine.substr(0, startColumn) + lastLine.substr(endColumn);
        lines_[startLine] = newFirstLine;
        
        // Remove the lines in between
        lines_.erase(lines_.begin() + startLine + 1, lines_.begin() + endLine + 1);
        
        notifyLinesRemoved(startLine + 1, endLine - startLine);
        notifyLineChanged(startLine);
    }
    
    undoManager_.endCompoundOperation();
    isModified_ = true;
    
    return deletedText;
}

std::string Document::replaceText(int startLine, int startColumn, 
                                int endLine, int endColumn,
                                const std::string& newText) {
    std::string oldText = getText().substr(
        getTextPosition(startLine, startColumn),
        getTextPosition(endLine, endColumn) - getTextPosition(startLine, startColumn)
    );
    
    if (oldText.empty() && newText.empty()) {
        return "";
    }
    
    undoManager_.beginCompoundOperation();
    
    // Delete the old text
    std::string deletedText = deleteText(startLine, startColumn, endLine, endColumn);
    
    // Insert the new text
    if (!newText.empty()) {
        insertText(startLine, startColumn, newText);
    }
    
    undoManager_.endCompoundOperation();
    isModified_ = true;
    
    return deletedText;
}

const std::string& Document::getLine(size_t line) const {
    static const std::string emptyLine;
    if (line >= lines_.size()) {
        return emptyLine;
    }
    return lines_[line];
}

std::string Document::getText(const std::string& lineEnding) const {
    std::ostringstream oss;
    for (size_t i = 0; i < lines_.size(); ++i) {
        oss << lines_[i];
        if (i < lines_.size() - 1) {
            oss << lineEnding;
        }
    }
    return oss.str();
}

void Document::addObserver(DocumentObserver* observer) {
    if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void Document::removeObserver(DocumentObserver* observer) {
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

void Document::notifyDocumentChanged() {
    for (auto observer : observers_) {
        if (observer) {
            observer->onDocumentChanged(this);
        }
    }
}

void Document::notifyLineChanged(int line) {
    for (auto observer : observers_) {
        if (observer) {
            observer->onLineChanged(this, line);
        }
    }
}

void Document::notifyLinesInserted(int startLine, int count) {
    for (auto observer : observers_) {
        if (observer) {
            observer->onLinesInserted(this, startLine, count);
        }
    }
}

void Document::notifyLinesRemoved(int startLine, int count) {
    for (auto observer : observers_) {
        if (observer) {
            observer->onLinesRemoved(this, startLine, count);
        }
    }
}

void Document::onUndoRedo(const TextOperation& op, bool isRedo) {
    // Replay the operation
    switch (op.getType()) {
        case TextOperation::Type::INSERT:
            insertText(op.getLine(), op.getColumn(), op.getText());
            break;
            
        case TextOperation::Type::DELETE:
            deleteText(op.getLine(), op.getColumn(), 
                     op.getEndLine(), op.getEndColumn());
            break;
            
        case TextOperation::Type::REPLACE:
            replaceText(op.getLine(), op.getColumn(),
                       op.getEndLine(), op.getEndColumn(),
                       op.getText());
            break;
    }
}

} // namespace ai_editor
