#ifndef EDITOR_DIFF_MERGE_COMMANDS_H
#define EDITOR_DIFF_MERGE_COMMANDS_H

#include "interfaces/ICommand.hpp"
#include "interfaces/IEditor.hpp"
#include "commands/DiffCommand.h"
#include "commands/MergeCommand.h"
#include <memory>
#include <string>

/**
 * @brief Factory functions for creating diff and merge related commands.
 * 
 * This namespace provides factory functions to create various commands
 * related to text diff and merge operations in the editor.
 */
namespace EditorDiffMergeCommands {

/**
 * @brief Creates a command to diff the current text with a file.
 * 
 * @param editor Pointer to the editor instance.
 * @param targetFile Path to the file to compare against.
 * @return A command that performs the diff operation.
 */
inline std::shared_ptr<ICommand> createDiffWithFileCommand(
    std::shared_ptr<IEditor> editor,
    const std::string& targetFile) {
    return std::make_shared<DiffCommand>(editor, targetFile);
}

/**
 * @brief Creates a command to merge the current text with files.
 * 
 * @param editor Pointer to the editor instance.
 * @param baseFile Path to the base file (common ancestor).
 * @param theirFile Path to their file (the file to merge with).
 * @return A command that performs the merge operation.
 */
inline std::shared_ptr<ICommand> createMergeWithFilesCommand(
    std::shared_ptr<IEditor> editor,
    const std::string& baseFile,
    const std::string& theirFile) {
    return std::make_shared<MergeCommand>(editor, baseFile, theirFile);
}

} // namespace EditorDiffMergeCommands

#endif // EDITOR_DIFF_MERGE_COMMANDS_H 