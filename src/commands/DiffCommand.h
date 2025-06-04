#pragma once

#include "../interfaces/plugins/ICommand.hpp"
#include "../interfaces/IEditor.hpp"
#include <string>
#include <memory>

/**
 * @brief Command that enables comparing the current document with a file.
 * 
 * This command implements the diff functionality, allowing users to compare
 * the current text in the editor with the contents of a specified file.
 */
class DiffCommand : public ICommand {
public:
    /**
     * @brief Constructs a DiffCommand with the specified editor and target file.
     * 
     * @param editor Pointer to the editor instance.
     * @param targetFile Path to the file to compare against.
     */
    DiffCommand(std::shared_ptr<IEditor> editor, const std::string& targetFile);
    
    /**
     * @brief Executes the diff operation.
     * 
     * Compares the current text in the editor with the contents of the target file
     * and displays the differences.
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
    std::string targetFile_;
}; 