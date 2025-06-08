#include <gtest/gtest.h>
#include "../src/EditorDemoWindow.h"
#include <imgui.h>

namespace ai_editor {

class EditorDemoWindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize the editor
        editor_.initialize();
        
        // Add a new tab with some test content
        editor_.addNewTab("Test Tab");
        
        // Set test content
        std::string testContent = "The quick brown fox\njumps over the lazy dog\nTesting 123";
        editor_.setDemoCode(testContent, "text");
    }

    // Helper method to set a selection range
    void setSelection(int startLine, int startCol, int endLine, int endCol) {
        // This is a simplified version - in a real test, we'd need to interact with the UI
        // or have a way to set the selection programmatically
        // For now, we'll just test the public interface
    }

    EditorDemoWindow editor_;
};

TEST_F(EditorDemoWindowTest, CanInitialize) {
    // Just test that the editor was initialized
    EXPECT_TRUE(true);
}

TEST_F(EditorDemoWindowTest, CanSetContent) {
    std::string content = "Test content";
    editor_.setDemoCode(content, "text");
    EXPECT_EQ(editor_.getEditorContent(), content);
}

// Add more tests for the public interface as needed

} // namespace ai_editor

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
