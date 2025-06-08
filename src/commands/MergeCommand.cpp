#include "MergeCommand.h"
#include "../AppDebugLog.h"

MergeCommand::MergeCommand(std::shared_ptr<IEditor> editor, const std::string& baseFile, const std::string& theirFile)
    : editor_(editor), baseFile_(baseFile), theirFile_(theirFile) {
}

void MergeCommand::execute() {
    LOG_DEBUG("Executing MergeCommand with base file: %s, their file: %s", 
              baseFile_.c_str(), theirFile_.c_str());
    
    if (!editor_) {
        LOG_ERROR("MergeCommand: Editor pointer is null");
        return;
    }
    
    try {
        // For a real implementation, we'd need to have public methods in the IEditor interface
        // to support these operations. For now, we'll just call mergeWithFile which should
        // handle saving the original state internally if needed.
        
        // Use the editor's mergeWithFile method to merge the current text with the specified files
        bool success = editor_->mergeWithFile(baseFile_, theirFile_);
        
        if (!success) {
            LOG_ERROR("MergeCommand: Failed to merge with files base: %s, their: %s", 
                      baseFile_.c_str(), theirFile_.c_str());
            return;
        }
        
        LOG_DEBUG("MergeCommand executed successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("MergeCommand exception: %s", e.what());
    }
}

std::string MergeCommand::getDisplayName() const {
    return "Merge with Files";
} 