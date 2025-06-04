#include "diff/DiffMergeFactory.h"
#include "AppDebugLog.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

/**
 * @brief Read a file and return its contents as a vector of lines
 * 
 * @param filename The name of the file to read
 * @return A vector of strings, each representing a line in the file
 */
std::vector<std::string> readFile(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    
    if (!file) {
        LOG_ERROR("Could not open file: " + filename);
        return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

/**
 * @brief Write a vector of lines to a file
 * 
 * @param filename The name of the file to write
 * @param lines The lines to write
 * @return True if the file was written successfully
 */
bool writeFile(const std::string& filename, const std::vector<std::string>& lines) {
    std::ofstream file(filename);
    
    if (!file) {
        LOG_ERROR("Could not open file for writing: " + filename);
        return false;
    }
    
    for (const auto& line : lines) {
        file << line << '\n';
    }
    
    return true;
}

/**
 * @brief Run a diff example
 * 
 * @param file1 The first file
 * @param file2 The second file
 * @param outputFile The file to write the diff to
 */
void runDiffExample(const std::string& file1, const std::string& file2, const std::string& outputFile) {
    LOG_INFO("Running diff example with files: " + file1 + " and " + file2);
    
    // Read files
    auto lines1 = readFile(file1);
    auto lines2 = readFile(file2);
    
    // Create diff engine
    auto diffEngine = DiffMergeFactory::createDiffEngine();
    
    // Compute diff
    auto changes = diffEngine->computeLineDiff(lines1, lines2);
    
    // Print changes
    LOG_INFO("Found " + std::to_string(changes.size()) + " changes");
    
    for (const auto& change : changes) {
        std::string changeType;
        
        switch (change.type) {
            case DiffChange::ChangeType::EQUAL:
                changeType = "EQUAL";
                break;
            case DiffChange::ChangeType::INSERT:
                changeType = "INSERT";
                break;
            case DiffChange::ChangeType::DELETE:
                changeType = "DELETE";
                break;
            case DiffChange::ChangeType::REPLACE:
                changeType = "REPLACE";
                break;
        }
        
        LOG_INFO("Change: " + changeType + 
                 " at line " + std::to_string(change.startLine1) + 
                 " in file 1 and line " + std::to_string(change.startLine2) + 
                 " in file 2");
    }
    
    // Format as unified diff
    std::string diff = diffEngine->formatUnifiedDiff(changes, lines1, lines2);
    
    // Write diff to file
    std::ofstream diffFile(outputFile);
    
    if (!diffFile) {
        LOG_ERROR("Could not open diff file for writing: " + outputFile);
        return;
    }
    
    diffFile << diff;
    
    LOG_INFO("Diff written to " + outputFile);
}

/**
 * @brief Run a merge example
 * 
 * @param baseFile The base file (common ancestor)
 * @param ourFile Our version of the file
 * @param theirFile Their version of the file
 * @param mergedFile The file to write the merged result to
 */
void runMergeExample(
    const std::string& baseFile,
    const std::string& ourFile,
    const std::string& theirFile,
    const std::string& mergedFile) {
    
    LOG_INFO("Running merge example with files: " + 
             baseFile + " (base), " + 
             ourFile + " (ours), and " + 
             theirFile + " (theirs)");
    
    // Read files
    auto baseLines = readFile(baseFile);
    auto ourLines = readFile(ourFile);
    auto theirLines = readFile(theirFile);
    
    // Create merge engine
    auto mergeEngine = DiffMergeFactory::createMergeEngine();
    
    // Perform merge
    auto mergeResult = mergeEngine->merge(baseLines, ourLines, theirLines);
    
    // Print conflicts
    LOG_INFO("Merge result has " + 
             std::to_string(mergeResult.conflicts.size()) + 
             " conflicts");
    
    // Write merged result to file
    writeFile(mergedFile, mergeResult.mergedLines);
    
    LOG_INFO("Merged result written to " + mergedFile);
    
    // If there are conflicts, let's resolve one as an example
    if (!mergeResult.conflicts.empty()) {
        LOG_INFO("Resolving first conflict by taking our version");
        
        // Resolve the first conflict by taking our version
        mergeEngine->resolveConflict(
            mergeResult, 
            0, 
            MergeConflictResolution::TAKE_OURS);
        
        // Apply resolutions
        mergeEngine->applyResolutions(mergeResult);
        
        // Write resolved result to a new file
        std::string resolvedFile = mergedFile + ".resolved";
        writeFile(resolvedFile, mergeResult.mergedLines);
        
        LOG_INFO("Resolved result written to " + resolvedFile);
    }
}

/**
 * @brief Main function to run the examples
 * 
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    // Check if we have enough arguments for diff example
    if (argc >= 4) {
        runDiffExample(argv[1], argv[2], argv[3]);
    } else {
        LOG_INFO("Usage for diff: " + std::string(argv[0]) + 
                 " <file1> <file2> <diff_output>");
    }
    
    // Check if we have enough arguments for merge example
    if (argc >= 7) {
        runMergeExample(argv[4], argv[5], argv[6], argv[7]);
    } else {
        LOG_INFO("Usage for merge: " + std::string(argv[0]) + 
                 " <file1> <file2> <diff_output> <base_file> <our_file> <their_file> <merged_output>");
    }
    
    return 0;
} 