#pragma once

#include "../interfaces/plugins/ICommand.hpp"
#include "../interfaces/IEditor.hpp"
#include <string>
#include <memory>

/**
 * @brief Command that enables merging the current document with files.
 * 
 * This command implements the merge functionality, allowing users to merge
 * the current text in the editor with the contents of specified files.
 */
class MergeCommand : public ICommand {
public:
    /**
     * @brief Constructs a MergeCommand with the specified editor and files.
     * 
     * @param editor Pointer to the editor instance.
     * @param baseFile Path to the base file (common ancestor).
     * @param theirFile Path to their file (the file to merge with).
     */
    MergeCommand(std::shared_ptr<IEditor> editor, const std::string& baseFile, const std::string& theirFile);
    
    /**
     * @brief Executes the merge operation.
     * 
     * Merges the current text in the editor with the contents of the specified files
     * and displays the result, highlighting conflicts if any.
     */
    void execute() override;
    
    /**
     * @brief Returns the display name of this command.
     * 
     * @return The human-readable name of the command.
     */
    std::string getDisplayName() const override;
    
private:
    std::shared_ptr<IEditor> editor_;
    std::string baseFile_;
    std::string theirFile_;
}; 