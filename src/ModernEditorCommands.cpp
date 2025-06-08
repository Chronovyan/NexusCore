#include "EditorCommands.h"
#include "AppDebugLog.h"
#include <fstream>
#include <sstream>

// Implementation for DeleteLineCommand - Commented out due to duplicate definition in EditorCommands.cpp
/*
void DeleteLineCommand::execute(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        execute();
    } else {
        // Store the line before deleting
        deletedLine_ = editor.getBuffer().getLine(lineIndex_);
        
        // Delete the line
        editor.deleteLine(lineIndex_);
        wasDeleted_ = true;
    }
}

void DeleteLineCommand::undo(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        undo();
    } else if (wasDeleted_) {
        // Re-insert the deleted line
        editor.insertLine(lineIndex_, deletedLine_);
        wasDeleted_ = false;
    }
}
*/

// Implementation for ReplaceLineCommand - Commented out due to duplicate definition in EditorCommands.cpp
/*
void ReplaceLineCommand::execute(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        execute();
    } else {
        // Store the original text before replacing
        originalText_ = editor.getBuffer().getLine(lineIndex_);
        
        // Replace the line
        editor.replaceLine(lineIndex_, newText_);
        wasExecuted_ = true;
    }
}

void ReplaceLineCommand::undo(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        undo();
    } else if (wasExecuted_) {
        // Restore the original text
        editor.replaceLine(lineIndex_, originalText_);
        wasExecuted_ = false;
    }
}
*/

// Implementation for InsertLineCommand - Commented out due to duplicate definition in EditorCommands.cpp
/*
void InsertLineCommand::execute(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        execute();
    } else {
        // Insert the line
        editor.insertLine(lineIndex_, text_);
        wasExecuted_ = true;
    }
}

void InsertLineCommand::undo(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        undo();
    } else if (wasExecuted_) {
        // Delete the inserted line
        editor.deleteLine(lineIndex_);
        wasExecuted_ = false;
    }
}
*/

// Implementation for LoadFileCommand
void LoadFileCommand::execute(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        execute();
    } else {
        // Store the original buffer state
        ITextBuffer& buffer = editor.getBuffer();
        originalBufferContent_ = std::vector<std::string>();
        for (size_t i = 0; i < buffer.lineCount(); ++i) {
            originalBufferContent_.push_back(buffer.getLine(i));
        }
        
        // Directly load the file contents into buffer
        bool success = false;
        
        // Clear the buffer first
        while (buffer.lineCount() > 0) {
            buffer.deleteLine(0);
        }
        
        try {
            std::ifstream file(filePath_);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    buffer.addLine(line);
                }
                
                // Don't set cursor, filename, or modified state here
                // Those will be set by Editor::loadFile after this command completes
                
                success = true;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error loading file: " + std::string(e.what()));
            success = false;
        }
        
        wasExecuted_ = success;
    }
}

void LoadFileCommand::undo(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        undo();
    } else if (wasExecuted_) {
        // Restore the original buffer state
        ITextBuffer& buffer = editor.getBuffer();
        
        // Clear existing buffer
        while (buffer.lineCount() > 0) {
            buffer.deleteLine(0);
        }
        
        // Restore saved content
        for (const auto& line : originalBufferContent_) {
            buffer.addLine(line);
        }
        
        wasExecuted_ = false;
    }
}

// Implementation for SaveFileCommand
void SaveFileCommand::execute(Editor& editor) {
    if (textBuffer_) {
        // Use the direct interface if available
        execute();
    } else {
        // FIXED: Directly save the file instead of calling back to editor methods
        // to avoid infinite recursion
        bool success = false;
        
        try {
            std::ofstream file(filePath_.empty() ? editor.getFilename() : filePath_);
            if (file.is_open()) {
                ITextBuffer& buffer = editor.getBuffer();
                
                // Write each line to the file
                for (size_t i = 0; i < buffer.lineCount(); ++i) {
                    file << buffer.getLine(i);
                    
                    // Add newline if it's not the last line or the line doesn't end with newline
                    if (i < buffer.lineCount() - 1 || 
                        (buffer.getLine(i).length() > 0 && 
                         buffer.getLine(i).back() != '\n')) {
                        file << std::endl;
                    }
                }
                
                success = true;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error saving file: " + std::string(e.what()));
            success = false;
        }
        
        wasExecuted_ = success;
    }
}

void SaveFileCommand::undo(Editor& /*editor*/) {
    // Saving a file doesn't change the buffer state, so undo is a no-op
} 