#include "DiffCommand.h"
#include "../AppDebugLog.h"

DiffCommand::DiffCommand(std::shared_ptr<IEditor> editor, const std::string& targetFile)
    : editor_(editor), targetFile_(targetFile) {
}

void DiffCommand::execute() {
    LOG_DEBUG("Executing DiffCommand with target file: %s", targetFile_.c_str());
    
    if (!editor_) {
        LOG_ERROR("DiffCommand: Editor pointer is null");
        return;
    }
    
    // Use the editor's diffWithFile method to compare the current text with the target file
    bool success = editor_->diffWithFile(targetFile_);
    
    if (!success) {
        LOG_ERROR("DiffCommand: Failed to diff with file %s", targetFile_.c_str());
        return;
    }
    
    LOG_DEBUG("DiffCommand executed successfully");
}

std::string DiffCommand::getDisplayName() const {
    return "Compare with File";
} 