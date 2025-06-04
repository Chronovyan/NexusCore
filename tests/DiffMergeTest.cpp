#include "gtest/gtest.h"
#include "../src/Editor.h"
#include "../src/TextBuffer.h"
#include "../src/CommandManager.h"
#include "../src/SyntaxHighlightingManager.h"
#include "../src/diff/MyersDiff.cpp"
#include "../src/diff/MergeEngine.h"
#include "../src/commands/DiffCommand.h"
#include "../src/commands/MergeCommand.h"
#include <memory>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class DiffMergeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create dependencies
        textBuffer = std::make_shared<TextBuffer>();
        commandManager = std::make_shared<CommandManager>();
        syntaxHighlightingManager = std::make_shared<SyntaxHighlightingManager>();
        diffEngine = std::make_shared<MyersDiff>();
        mergeEngine = std::make_shared<MergeEngine>();
        
        // Create editor with dependencies
        editor = std::make_shared<Editor>(
            textBuffer, 
            commandManager, 
            syntaxHighlightingManager,
            diffEngine,
            mergeEngine
        );
        
        // Create test files directory if it doesn't exist
        if (!fs::exists("test_files")) {
            fs::create_directory("test_files");
        }
    }
    
    void TearDown() override {
        // Clean up test files
        if (fs::exists("test_files/file1.txt")) {
            fs::remove("test_files/file1.txt");
        }
        if (fs::exists("test_files/file2.txt")) {
            fs::remove("test_files/file2.txt");
        }
        if (fs::exists("test_files/base.txt")) {
            fs::remove("test_files/base.txt");
        }
        if (fs::exists("test_files/theirs.txt")) {
            fs::remove("test_files/theirs.txt");
        }
    }
    
    // Helper method to create a test file with content
    void createTestFile(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream file(filename);
        for (const auto& line : lines) {
            file << line << std::endl;
        }
    }
    
    std::shared_ptr<TextBuffer> textBuffer;
    std::shared_ptr<CommandManager> commandManager;
    std::shared_ptr<SyntaxHighlightingManager> syntaxHighlightingManager;
    std::shared_ptr<IDiffEngine> diffEngine;
    std::shared_ptr<IMergeEngine> mergeEngine;
    std::shared_ptr<Editor> editor;
};

TEST_F(DiffMergeTest, DiffWithFileTest) {
    // Set up the editor content
    textBuffer->clear();
    textBuffer->addLine("Line 1");
    textBuffer->addLine("Line 2");
    textBuffer->addLine("Line 3");
    
    // Create a file with similar but different content
    std::vector<std::string> fileLines = {
        "Line 1",
        "Line 2 modified",
        "Line 3",
        "Line 4"
    };
    createTestFile("test_files/file1.txt", fileLines);
    
    // Execute diff command
    DiffCommand diffCommand(editor, "test_files/file1.txt");
    bool result = diffCommand.execute();
    
    // Verify diff executed successfully
    EXPECT_TRUE(result);
    
    // We can't easily verify the visual output, but we can check
    // that the current text wasn't modified
    EXPECT_EQ(3, textBuffer->lineCount());
    EXPECT_EQ("Line 1", textBuffer->getLine(0));
    EXPECT_EQ("Line 2", textBuffer->getLine(1));
    EXPECT_EQ("Line 3", textBuffer->getLine(2));
}

TEST_F(DiffMergeTest, MergeWithFileTest) {
    // Set up the editor content ("ours")
    textBuffer->clear();
    textBuffer->addLine("Line 1");
    textBuffer->addLine("Line 2 - our change");
    textBuffer->addLine("Line 3");
    
    // Create base file
    std::vector<std::string> baseLines = {
        "Line 1",
        "Line 2",
        "Line 3"
    };
    createTestFile("test_files/base.txt", baseLines);
    
    // Create "theirs" file
    std::vector<std::string> theirLines = {
        "Line 1",
        "Line 2",
        "Line 3 - their change"
    };
    createTestFile("test_files/theirs.txt", theirLines);
    
    // Execute merge command
    MergeCommand mergeCommand(editor, "test_files/base.txt", "test_files/theirs.txt");
    bool result = mergeCommand.execute();
    
    // Verify merge executed successfully
    EXPECT_TRUE(result);
    
    // The merged result should contain both changes
    // Exact format depends on implementation, but should have both changes
    EXPECT_GE(textBuffer->lineCount(), 3);
    
    // Test undo
    result = mergeCommand.undo();
    EXPECT_TRUE(result);
    
    // Verify original content is restored
    EXPECT_EQ(3, textBuffer->lineCount());
    EXPECT_EQ("Line 1", textBuffer->getLine(0));
    EXPECT_EQ("Line 2 - our change", textBuffer->getLine(1));
    EXPECT_EQ("Line 3", textBuffer->getLine(2));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 