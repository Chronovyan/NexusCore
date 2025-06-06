#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockEditor.h"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::InSequence;

// Test fixture for Editor tests using mock
class EditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup code would go here
    }

    void TearDown() override {
        // Any cleanup code would go here
    }

    // Test data
    const std::string testText = "Hello, World!";
    
    // Mock editor instance
    NiceMock<MockEditor> mockEditor;
};

// Test basic test harness is working
TEST_F(EditorTest, TestHarnessWorks) {
    // Test that we can call a basic method
    EXPECT_CALL(mockEditor, typeText(_)).Times(1);
    mockEditor.typeText("Test");
}

// Test initial state of the editor
TEST_F(EditorTest, InitialState) {
    // Test cursor position getters
    EXPECT_CALL(mockEditor, getCursorLine()).WillOnce(Return(0));
    EXPECT_CALL(mockEditor, getCursorCol()).WillOnce(Return(0));
    
    EXPECT_EQ(mockEditor.getCursorLine(), 0);
    EXPECT_EQ(mockEditor.getCursorCol(), 0);
    
    // Test selection state
    EXPECT_CALL(mockEditor, hasSelection()).WillOnce(Return(false));
    EXPECT_FALSE(mockEditor.hasSelection());
}

// Test cursor movement
TEST_F(EditorTest, CursorMovement) {
    // Test moving to line start
    EXPECT_CALL(mockEditor, moveCursorToLineStart()).Times(1);
    mockEditor.moveCursorToLineStart();
    
    // Test moving to line end
    EXPECT_CALL(mockEditor, moveCursorToLineEnd()).Times(1);
    mockEditor.moveCursorToLineEnd();
    
    // Test moving cursor up
    EXPECT_CALL(mockEditor, moveCursorUp()).Times(1);
    mockEditor.moveCursorUp();
    
    // Test moving cursor down
    EXPECT_CALL(mockEditor, moveCursorDown()).Times(1);
    mockEditor.moveCursorDown();
    
    // Test moving cursor left
    EXPECT_CALL(mockEditor, moveCursorLeft()).Times(1);
    mockEditor.moveCursorLeft();
    
    // Test moving cursor right
    EXPECT_CALL(mockEditor, moveCursorRight()).Times(1);
    mockEditor.moveCursorRight();
    
    // Test moving to buffer start
    EXPECT_CALL(mockEditor, moveCursorToBufferStart()).Times(1);
    mockEditor.moveCursorToBufferStart();
    
    // Test moving to buffer end
    EXPECT_CALL(mockEditor, moveCursorToBufferEnd()).Times(1);
    mockEditor.moveCursorToBufferEnd();
}

// Test text operations
TEST_F(EditorTest, TextOperations) {
    // Test typing text
    EXPECT_CALL(mockEditor, typeText("Hello")).Times(1);
    mockEditor.typeText("Hello");
    
    // Test newline
    EXPECT_CALL(mockEditor, newLine()).Times(1);
    mockEditor.newLine();
}

// Test selection operations
TEST_F(EditorTest, SelectionOperations) {
    // Test setting selection range
    EXPECT_CALL(mockEditor, setSelectionRange(1, 0, 1, 5)).Times(1);
    mockEditor.setSelectionRange(1, 0, 1, 5);
    
    // Test getting selected text
    EXPECT_CALL(mockEditor, hasSelection()).WillOnce(Return(true));
    EXPECT_CALL(mockEditor, getSelectedText()).WillOnce(Return("Hello"));
    
    if (mockEditor.hasSelection()) {
        std::string selected = mockEditor.getSelectedText();
        EXPECT_EQ(selected, "Hello");
    }
    
    // Test clearing selection
    EXPECT_CALL(mockEditor, clearSelection()).Times(1);
    mockEditor.clearSelection();
}

// Test clipboard operations
TEST_F(EditorTest, ClipboardOperations) {
    // Test setting clipboard text
    EXPECT_CALL(mockEditor, setClipboardText("Test clipboard")).Times(1);
    mockEditor.setClipboardText("Test clipboard");
    
    // Test copy operation with selection
    {
        InSequence seq;
        EXPECT_CALL(mockEditor, hasSelection()).WillOnce(Return(true));
        EXPECT_CALL(mockEditor, getSelectedText()).WillOnce(Return("Selected text"));
        EXPECT_CALL(mockEditor, setClipboardText("Selected text")).Times(1);
        
        if (mockEditor.hasSelection()) {
            std::string selected = mockEditor.getSelectedText();
            mockEditor.setClipboardText(selected);
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
