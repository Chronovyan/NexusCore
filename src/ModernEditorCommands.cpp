#include "EditorCommands.h"
#include "AppDebugLog.h"

// Implementation for DeleteLineCommand
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

// Implementation for ReplaceLineCommand
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

// Implementation for InsertLineCommand
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
        
        // Load the file - use openFile() from IEditor interface
        bool success = editor.openFile(filePath_);
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
            editor.deleteLine(0);
        }
        
        // Restore saved content
        for (const auto& line : originalBufferContent_) {
            editor.addLine(line);
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
        // Save the file using the appropriate interface method
        bool success;
        if (filePath_.empty()) {
            success = editor.saveFile(); // Use current filename
        } else {
            success = editor.saveFileAs(filePath_); // Use specified filename
        }
        wasExecuted_ = success;
    }
}

void SaveFileCommand::undo(Editor& editor) {
    // Saving a file doesn't change the buffer state, so undo is a no-op
} 