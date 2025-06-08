#include <gtest/gtest.h>
#include "../src/EditorDemoWindow.h"
#include "TestFramework.h"
#include <string>
#include <vector>

using namespace ai_editor;

class EditorDemoWindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        editor_.initialize();
        // Setup a basic document with some text
        TabState tab;
        tab.lines = {"The quick brown fox", "jumps over the lazy dog", "Testing 123"};
        tab.cursorLine = 0;
        tab.cursorColumn = 0;
        editor_.tabs_.push_back(tab);
        editor_.activeTabIndex_ = 0;
    }

    EditorDemoWindow editor_;
};

TEST_F(EditorDemoWindowTest, SingleLineCopy) {
    // Setup selection from column 4 to 9 ("quick")
    auto& tab = editor_.tabs_[0];
    tab.cursorColumn = 4;
    tab.hasSelection = true;
    tab.selectionStartLine = 0;
    tab.selectionStartCol = 4;
    tab.selectionEndLine = 0;
    tab.selectionEndCol = 9;

    editor_.copySelection();
    
    // Verify status message
    EXPECT_NE(std::string(editor_.statusBuffer_).find("Copied 5 characters"), std::string::npos);
    
    // In a real test, we would verify clipboard content here
    // For now, we'll just verify the selection state remains
    EXPECT_TRUE(tab.hasSelection);
}

TEST_F(EditorDemoWindowTest, MultiLineCopy) {
    // Setup selection from (0,4) to (1,8) ("quick brown fox\njumps o")
    auto& tab = editor_.tabs_[0];
    tab.hasSelection = true;
    tab.selectionStartLine = 0;
    tab.selectionStartCol = 4;
    tab.selectionEndLine = 1;
    tab.selectionEndCol = 8;

    editor_.copySelection();
    
    // Verify status message (length of "quick brown fox\njumps o" is 23)
    EXPECT_NE(std::string(editor_.statusBuffer_).find("Copied 23 characters"), std::string::npos);
    
    // Verify selection state remains
    EXPECT_TRUE(tab.hasSelection);
}

TEST_F(EditorDemoWindowTest, SingleLineCut) {
    // Setup selection from column 4 to 9 ("quick")
    auto& tab = editor_.tabs_[0];
    tab.cursorColumn = 4;
    tab.hasSelection = true;
    tab.selectionStartLine = 0;
    tab.selectionStartCol = 4;
    tab.selectionEndLine = 0;
    tab.selectionEndCol = 9;

    editor_.cutSelection();
    
    // Verify status message
    EXPECT_NE(std::string(editor_.statusBuffer_).find("Copied 5 characters"), std::string::npos);
    
    // Verify text was removed
    EXPECT_EQ(tab.lines[0], "The  brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab.cursorLine, 0);
    EXPECT_EQ(tab.cursorColumn, 4);
    
    // Verify selection was cleared
    EXPECT_FALSE(tab.hasSelection);
}

TEST_F(EditorDemoWindowTest, MultiLineCut) {
    // Setup selection from (0,4) to (1,8) ("quick brown fox\njumps o")
    auto& tab = editor_.tabs_[0];
    tab.hasSelection = true;
    tab.selectionStartLine = 0;
    tab.selectionStartCol = 4;
    tab.selectionEndLine = 1;
    tab.selectionEndCol = 8;

    editor_.cutSelection();
    
    // Verify status message
    EXPECT_NE(std::string(editor_.statusBuffer_).find("Copied 23 characters"), std::string::npos);
    
    // Verify text was removed and lines were merged
    EXPECT_EQ(tab.lines[0], "The ver the lazy dog");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab.cursorLine, 0);
    EXPECT_EQ(tab.cursorColumn, 4);
    
    // Verify selection was cleared
    EXPECT_FALSE(tab.hasSelection);
}

TEST_F(EditorDemoWindowTest, PasteSingleLine) {
    // Setup clipboard content
    std::string testText = "test";
    ImGui::SetClipboardText(testText.c_str());
    
    auto& tab = editor_.tabs_[0];
    tab.cursorLine = 0;
    tab.cursorColumn = 4; // After "The "
    
    editor_.pasteAtCursor();
    
    // Verify text was inserted
    EXPECT_EQ(tab.lines[0], "The testquick brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab.cursorColumn, 8); // After "test"
    
    // Verify status message
    EXPECT_NE(std::string(editor_.statusBuffer_).find("Pasted 4 characters"), std::string::npos);
}

TEST_F(EditorDemoWindowTest, PasteMultiLine) {
    // Setup clipboard content with newlines
    std::string testText = "test\nmulti\nline";
    ImGui::SetClipboardText(testText.c_str());
    
    auto& tab = editor_.tabs_[0];
    tab.cursorLine = 0;
    tab.cursorColumn = 4; // After "The "
    
    editor_.pasteAtCursor();
    
    // Verify text was inserted and split into lines
    ASSERT_EQ(tab.lines.size(), 5); // Was 3, added 2 new lines
    EXPECT_EQ(tab.lines[0], "The test");
    EXPECT_EQ(tab.lines[1], "multi");
    EXPECT_EQ(tab.lines[2], "linequick brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab.cursorLine, 2);
    EXPECT_EQ(tab.cursorColumn, 4); // After "line"
    
    // Verify status message
    EXPECT_NE(std::string(editor_.statusBuffer_).find("Pasted 14 characters"), std::string::npos);
}

TEST_F(EditorDemoWindowTest, PasteWithSelection) {
    // Setup selection
    auto& tab = editor_.tabs_[0];
    tab.hasSelection = true;
    tab.selectionStartLine = 0;
    tab.selectionStartCol = 4; // "quick"
    tab.selectionEndLine = 0;
    tab.selectionEndCol = 9;
    
    // Setup clipboard content
    std::string testText = "test";
    ImGui::SetClipboardText(testText.c_str());
    
    editor_.pasteAtCursor();
    
    // Verify selection was replaced
    EXPECT_EQ(tab.lines[0], "The test brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab.cursorColumn, 8); // After "test"
    
    // Verify selection was cleared
    EXPECT_FALSE(tab.hasSelection);
}

TEST_F(EditorDemoWindowTest, CopyNoSelection) {
    // No selection set
    auto& tab = editor_.tabs_[0];
    tab.hasSelection = false;
    
    // Clear status buffer
    editor_.statusBuffer_[0] = '\0';
    
    editor_.copySelection();
    
    // Verify no status message was set
    EXPECT_EQ(editor_.statusBuffer_[0], '\0');
}

TEST_F(EditorDemoWindowTest, PasteEmptyClipboard) {
    // Setup empty clipboard
    ImGui::SetClipboardText(""""");
    
    auto& tab = editor_.tabs_[0];
    const std::string originalLine = tab.lines[0];
    
    // Clear status buffer
    editor_.statusBuffer_[0] = '\0';
    
    editor_.pasteAtCursor();
    
    // Verify no changes were made
    EXPECT_EQ(tab.lines[0], originalLine);
    
    // Verify no status message was set
    EXPECT_EQ(editor_.statusBuffer_[0], '\0');
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
