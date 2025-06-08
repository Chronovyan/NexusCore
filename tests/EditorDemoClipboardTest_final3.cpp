#define IMGUI_DEFINE_MATH_OPERATORS
#include <gtest/gtest.h>
#include "../src/EditorDemoWindow.h"
#include <imgui.h>

// Define the test class in the same namespace as EditorDemoWindow
namespace ai_editor {

class EditorDemoWindowTest : public ::testing::Test {
protected:
    using TabState = EditorDemoWindow::TabState;
    
    void SetUp() override {
        // Initialize the editor
        editor_.EditorDemoWindow::initialize();
        
        // Add a new tab with some test content
        editor_.EditorDemoWindow::addNewTab("Test Tab");
        
        // Get the active tab through the friend class
        auto& tabs = editor_.tabs_;
        if (!tabs.empty()) {
            auto& tab = tabs[0];
            tab.lines = {"The quick brown fox", "jumps over the lazy dog", "Testing 123"};
            tab.cursorLine = 0;
            tab.cursorColumn = 0;
            tab.hasSelection = false;
            tab.selectionStartLine = 0;
            tab.selectionStartCol = 0;
            tab.selectionEndLine = 0;
            tab.selectionEndCol = 0;
        }
    }

    // Helper method to set a selection range
    void setSelection(int startLine, int startCol, int endLine, int endCol) {
        auto& tabs = editor_.tabs_;
        if (tabs.empty()) return;
        
        auto& tab = tabs[0];
        tab.hasSelection = true;
        tab.selectionStartLine = startLine;
        tab.selectionStartCol = startCol;
        tab.selectionEndLine = endLine;
        tab.selectionEndCol = endCol;
        tab.cursorLine = endLine;
        tab.cursorColumn = endCol;
    }

    // Helper method to get the active tab content
    std::string getActiveTabContent() {
        auto& tabs = editor_.tabs_;
        if (tabs.empty()) return "";
        
        auto& tab = tabs[0];
        std::string content;
        for (size_t i = 0; i < tab.lines.size(); ++i) {
            if (i > 0) content += "\n";
            content += tab.lines[i];
        }
        return content;
    }
    
    // Helper to get cursor position
    std::pair<int, int> getCursorPosition() {
        auto& tabs = editor_.tabs_;
        if (tabs.empty()) return {0, 0};
        
        auto& tab = tabs[0];
        return {tab.cursorLine, tab.cursorColumn};
    }
    
    // Helper to check if there's a selection
    bool hasSelection() {
        auto& tabs = editor_.tabs_;
        if (tabs.empty()) return false;
        
        return tabs[0].hasSelection;
    }

    // Helper method to get the current status message
    std::string getStatusMessage() const {
        return std::string(editor_.statusBuffer_);
    }
    
    // Helper method to get the active tab
    TabState* getActiveTab() {
        auto& tabs = editor_.tabs_;
        if (tabs.empty()) return nullptr;
        return &tabs[0];
    }

    EditorDemoWindow editor_;
};

// Test declarations
TEST_F(EditorDemoWindowTest, SingleLineCopy) {
    // Setup selection from column 4 to 9 ("quick")
    setSelection(0, 4, 0, 9);
    
    // Call copySelection through the friend class
    editor_.copySelection();
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Copied"), std::string::npos);
    
    // Verify the selection state remains
    EXPECT_TRUE(hasSelection());
}

TEST_F(EditorDemoWindowTest, MultiLineCopy) {
    // Setup selection from (0,4) to (1,8) ("quick brown fox\njumps o")
    setSelection(0, 4, 1, 8);
    
    // Call copySelection through the friend class
    editor_.copySelection();
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Copied"), std::string::npos);
    
    // Verify selection state remains
    EXPECT_TRUE(hasSelection());
}

TEST_F(EditorDemoWindowTest, SingleLineCut) {
    // Setup selection from column 4 to 9 ("quick")
    setSelection(0, 4, 0, 9);
    
    // Call cutSelection through the friend class
    editor_.cutSelection();
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Copied"), std::string::npos);
    
    // Get the active tab
    auto* tab = getActiveTab();
    ASSERT_NE(tab, nullptr);
    
    // Verify text was removed
    EXPECT_EQ(tab->lines[0], "The  brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab->cursorLine, 0);
    EXPECT_EQ(tab->cursorColumn, 4);
    
    // Verify selection was cleared
    EXPECT_FALSE(tab->hasSelection);
}

TEST_F(EditorDemoWindowTest, MultiLineCut) {
    // Setup selection from (0,4) to (1,8) ("quick brown fox\njumps o")
    setSelection(0, 4, 1, 8);
    
    // Call cutSelection through the friend class
    editor_.cutSelection();
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Copied"), std::string::npos);
    
    // Get the active tab
    auto* tab = getActiveTab();
    ASSERT_NE(tab, nullptr);
    
    // Verify text was removed and lines were merged
    EXPECT_EQ(tab->lines[0], "The ver the lazy dog");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab->cursorLine, 0);
    EXPECT_EQ(tab->cursorColumn, 4);
    
    // Verify selection was cleared
    EXPECT_FALSE(tab->hasSelection);
}

TEST_F(EditorDemoWindowTest, PasteSingleLine) {
    // Setup clipboard content
    std::string testText = "test";
    ImGui::SetClipboardText(testText.c_str());
    
    auto* tab = getActiveTab();
    ASSERT_NE(tab, nullptr);
    
    tab->cursorLine = 0;
    tab->cursorColumn = 4; // After "The "
    
    // Call pasteAtCursor through the friend class
    editor_.pasteAtCursor();
    
    // Verify text was inserted
    EXPECT_EQ(tab->lines[0], "The testquick brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab->cursorColumn, 8); // After "test"
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Pasted"), std::string::npos);
}

TEST_F(EditorDemoWindowTest, PasteMultiLine) {
    // Setup clipboard content with newlines
    std::string testText = "test\nmulti\nline";
    ImGui::SetClipboardText(testText.c_str());
    
    auto* tab = getActiveTab();
    ASSERT_NE(tab, nullptr);
    
    tab->cursorLine = 0;
    tab->cursorColumn = 4; // After "The "
    
    // Call pasteAtCursor through the friend class
    editor_.pasteAtCursor();
    
    // Verify text was inserted and split into lines
    ASSERT_EQ(tab->lines.size(), 3); // Should be 3 lines total
    EXPECT_EQ(tab->lines[0], "The test");
    EXPECT_EQ(tab->lines[1], "multi");
    EXPECT_EQ(tab->lines[2], "linequick brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab->cursorLine, 2);
    EXPECT_EQ(tab->cursorColumn, 4); // After "line"
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Pasted"), std::string::npos);
}

TEST_F(EditorDemoWindowTest, PasteWithSelection) {
    // Setup selection
    setSelection(0, 4, 0, 9); // Select "quick"
    
    // Setup clipboard content
    std::string testText = "test";
    ImGui::SetClipboardText(testText.c_str());
    
    // Call pasteAtCursor through the friend class
    editor_.pasteAtCursor();
    
    // Get the active tab
    auto* tab = getActiveTab();
    ASSERT_NE(tab, nullptr);
    
    // Verify text was replaced
    EXPECT_EQ(tab->lines[0], "The test brown fox");
    
    // Verify cursor position was updated
    EXPECT_EQ(tab->cursorLine, 0);
    EXPECT_EQ(tab->cursorColumn, 8); // After "test"
    
    // Verify selection was cleared
    EXPECT_FALSE(tab->hasSelection);
    
    // Verify status message
    EXPECT_NE(getStatusMessage().find("Pasted"), std::string::npos);
}

TEST_F(EditorDemoWindowTest, CopyNoSelection) {
    // No selection set
    auto* tab = getActiveTab();
    ASSERT_NE(tab, nullptr);
    
    tab->hasSelection = false;
    
    // Clear status buffer
    editor_.statusBuffer_[0] = '\0';
    
    // Call copySelection through the friend class
    editor_.copySelection();
    
    // Verify no status message was set
    EXPECT_EQ(editor_.statusBuffer_[0], '\0');
}

TEST_F(EditorDemoWindowTest, PasteEmptyClipboard) {
    // Setup empty clipboard
    ImGui::SetClipboardText(""""");
    
    // Clear status buffer
    editor_.statusBuffer_[0] = '\0';
    
    // Call pasteAtCursor through the friend class
    editor_.pasteAtCursor();
    
    // Verify no status message was set
    EXPECT_EQ(editor_.statusBuffer_[0], '\0');
}

} // namespace ai_editor

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
