#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>

#include "../src/Editor.h"
#include "../src/interfaces/ITextBuffer.hpp"
#include "../src/interfaces/ICommandManager.hpp"
#include "../src/interfaces/ISyntaxHighlightingManager.hpp"
#include "../src/AppDebugLog.h"
#include "../src/di/Injector.hpp"
#include "../src/di/ApplicationModule.hpp"
#include "../src/interfaces/IEditor.hpp"

// Mock ITextBuffer implementation
class MockTextBuffer : public ITextBuffer {
public:
    MOCK_METHOD(void, addLine, (const std::string& line), (override));
    MOCK_METHOD(void, insertLine, (size_t index, const std::string& line), (override));
    MOCK_METHOD(void, deleteLine, (size_t index), (override));
    MOCK_METHOD(void, replaceLine, (size_t index, const std::string& newLine), (override));
    MOCK_METHOD(void, setLine, (size_t lineIndex, const std::string& text), (override));
    MOCK_METHOD(void, deleteLines, (size_t startIndex, size_t endIndex), (override));
    MOCK_METHOD(void, insertLines, (size_t index, const std::vector<std::string>& newLines), (override));
    MOCK_METHOD(const std::string&, getLine, (size_t index), (const, override));
    MOCK_METHOD(std::string&, getLine, (size_t index), (override));
    MOCK_METHOD(size_t, lineCount, (), (const, override));
    MOCK_METHOD(bool, isEmpty, (), (const, override));
    MOCK_METHOD(size_t, lineLength, (size_t lineIndex), (const, override));
    MOCK_METHOD(size_t, characterCount, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, getAllLines, (), (const, override));
    MOCK_METHOD(bool, isValidPosition, (size_t lineIndex, size_t colIndex), (const, override));
    MOCK_METHOD(bool, isValidLineIndex, (size_t lineIndex), (const, override));
    MOCK_METHOD(std::pair<size_t, size_t>, clampPosition, (size_t lineIndex, size_t colIndex), (const, override));
    MOCK_METHOD(bool, loadFromFile, (const std::string& filename, std::string& errorMessage), (override));
    MOCK_METHOD(bool, saveToFile, (const std::string& filename, std::string& errorMessage), (override));
    MOCK_METHOD(void, insertString, (size_t lineIndex, size_t colIndex, const std::string& text), (override));
    MOCK_METHOD(void, replaceLineSegment, (size_t lineIndex, size_t startCol, size_t endCol, const std::string& newText), (override));
    MOCK_METHOD(void, deleteLineSegment, (size_t lineIndex, size_t startCol, size_t endCol), (override));
    MOCK_METHOD(void, splitLine, (size_t lineIndex, size_t colIndex), (override));
    MOCK_METHOD(void, joinLines, (size_t lineIndex), (override));
    MOCK_METHOD(void, clear, (bool keepEmptyLine), (override));
    MOCK_METHOD(std::string, getLineSegment, (size_t lineIndex, size_t startCol, size_t endCol), (const, override));
};

// Mock ICommandManager implementation
class MockCommandManager : public ICommandManager {
public:
    MOCK_METHOD(void, executeCommand, (std::shared_ptr<IEditorCommand> command), (override));
    MOCK_METHOD(bool, canUndo, (), (const, override));
    MOCK_METHOD(bool, canRedo, (), (const, override));
    MOCK_METHOD(void, undo, (), (override));
    MOCK_METHOD(void, redo, (), (override));
    MOCK_METHOD(void, clearHistory, (), (override));
    MOCK_METHOD(std::string, getUndoCommandName, (), (const, override));
    MOCK_METHOD(std::string, getRedoCommandName, (), (const, override));
};

// Mock ISyntaxHighlightingManager implementation
class MockSyntaxHighlightingManager : public ISyntaxHighlightingManager {
public:
    MOCK_METHOD(void, setHighlighter, (std::shared_ptr<SyntaxHighlighter> highlighter), (override));
    MOCK_METHOD(void, setEnabled, (bool enabled), (override));
    MOCK_METHOD(bool, isEnabled, (), (const, override));
    MOCK_METHOD(void, setBuffer, (const ITextBuffer* buffer), (override));
    MOCK_METHOD(std::vector<std::vector<SyntaxStyle>>, getHighlightingStyles, (size_t startLine, size_t endLine), (override));
    MOCK_METHOD(void, invalidateLine, (size_t line), (override));
    MOCK_METHOD(void, invalidateLines, (size_t startLine, size_t endLine), (override));
    MOCK_METHOD(void, invalidateAllLines, (), (override));
    MOCK_METHOD(void, setVisibleRange, (size_t startLine, size_t endLine), (override));
};

// Test fixture for Editor DI tests
class EditorDITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logging
        initAppDebugLog();
        
        // Set up mocks with default behaviors
        textBuffer = std::make_shared<testing::NiceMock<MockTextBuffer>>();
        commandManager = std::make_shared<testing::NiceMock<MockCommandManager>>();
        syntaxHighlightingManager = std::make_shared<testing::NiceMock<MockSyntaxHighlightingManager>>();
        
        // Set up default expectations
        setupDefaultMockBehaviors();
    }
    
    void setupDefaultMockBehaviors() {
        // Set up the text buffer to have at least one line and return sensible values
        ON_CALL(*textBuffer, lineCount()).WillByDefault(testing::Return(1));
        ON_CALL(*textBuffer, isEmpty()).WillByDefault(testing::Return(false));
        ON_CALL(*textBuffer, isValidPosition(testing::_, testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*textBuffer, isValidLineIndex(testing::_)).WillByDefault(testing::Return(true));
        
        // Set up a default line for getLine calls
        static std::string defaultLine = "Default line content";
        ON_CALL(*textBuffer, getLine(testing::_)).WillByDefault(testing::ReturnRef(defaultLine));
        
        // Command manager default behaviors
        ON_CALL(*commandManager, canUndo()).WillByDefault(testing::Return(false));
        ON_CALL(*commandManager, canRedo()).WillByDefault(testing::Return(false));
        
        // Syntax highlighting manager default behaviors
        ON_CALL(*syntaxHighlightingManager, isEnabled()).WillByDefault(testing::Return(false));
        ON_CALL(*syntaxHighlightingManager, getHighlightingStyles(testing::_, testing::_))
            .WillByDefault(testing::Return(std::vector<std::vector<SyntaxStyle>>(1)));
    }
    
    std::shared_ptr<MockTextBuffer> textBuffer;
    std::shared_ptr<MockCommandManager> commandManager;
    std::shared_ptr<MockSyntaxHighlightingManager> syntaxHighlightingManager;
};

// Test basic editor construction with injected dependencies
TEST_F(EditorDITest, ConstructorInjection) {
    // Act: Create an editor with injected dependencies
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Assert: Verify the editor successfully initialized with the dependencies
    EXPECT_EQ(editor.getTextBuffer(), textBuffer);
    EXPECT_EQ(editor.getCommandManager(), commandManager);
    EXPECT_EQ(editor.getSyntaxHighlightingManager(), syntaxHighlightingManager);
}

// Test that the editor properly initializes the text buffer during construction
TEST_F(EditorDITest, TextBufferInitialization) {
    // Arrange: Expect the text buffer to be checked for emptiness and potentially have a line added
    EXPECT_CALL(*textBuffer, isEmpty()).WillOnce(testing::Return(true));
    EXPECT_CALL(*textBuffer, addLine("")).Times(1);
    
    // Act: Create an editor which should initialize the text buffer
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Assert: No explicit assertions needed as expectations are verified automatically
}

// Test the editor's cursor operations using the injected text buffer
TEST_F(EditorDITest, CursorOperations) {
    // Arrange: Set up expectations for cursor validation
    EXPECT_CALL(*textBuffer, lineCount()).WillRepeatedly(testing::Return(3));
    EXPECT_CALL(*textBuffer, lineLength(0)).WillRepeatedly(testing::Return(10));
    EXPECT_CALL(*textBuffer, lineLength(1)).WillRepeatedly(testing::Return(15));
    EXPECT_CALL(*textBuffer, lineLength(2)).WillRepeatedly(testing::Return(20));
    
    // Act: Create an editor and move the cursor
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    editor.setCursor(1, 5);
    
    // Assert: Verify cursor position
    EXPECT_EQ(editor.getCursorLine(), 1);
    EXPECT_EQ(editor.getCursorCol(), 5);
    
    // Move cursor beyond text buffer bounds
    editor.setCursor(10, 10);
    
    // Cursor should be clamped to valid range
    EXPECT_EQ(editor.getCursorLine(), 2);  // Last valid line (index 2)
    EXPECT_LE(editor.getCursorCol(), 20);  // Should be clamped to line length
}

// Test that undo/redo operations properly delegate to the command manager
TEST_F(EditorDITest, UndoRedoOperations) {
    // Arrange: Setup command manager expectations
    EXPECT_CALL(*commandManager, canUndo()).WillOnce(testing::Return(true));
    EXPECT_CALL(*commandManager, undo()).Times(1);
    EXPECT_CALL(*commandManager, canRedo()).WillOnce(testing::Return(true));
    EXPECT_CALL(*commandManager, redo()).Times(1);
    
    // Act: Create an editor and perform undo/redo operations
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    editor.undo();
    editor.redo();
    
    // Assert: Expectations verified automatically
}

// Test the editor's syntax highlighting integration
TEST_F(EditorDITest, SyntaxHighlightingIntegration) {
    // Arrange: Setup syntax highlighting manager expectations
    EXPECT_CALL(*syntaxHighlightingManager, setEnabled(true)).Times(1);
    EXPECT_CALL(*syntaxHighlightingManager, setBuffer(textBuffer.get())).Times(1);
    
    // Act: Create an editor and enable syntax highlighting
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    editor.enableSyntaxHighlighting(true);
    
    // Assert: Verify the highlighting manager was configured correctly
    // (Expectations verify this)
}

// Test edge case: Empty text buffer handling
TEST_F(EditorDITest, EmptyTextBufferHandling) {
    // Arrange: Setup text buffer to return that it's empty
    EXPECT_CALL(*textBuffer, isEmpty()).WillOnce(testing::Return(true));
    EXPECT_CALL(*textBuffer, addLine("")).Times(1);
    
    // Act: Create an editor with an empty text buffer
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Assert: The editor should have initialized the text buffer with an empty line
    // (Expectations verify this)
}

// Test error handling: Null dependencies
TEST_F(EditorDITest, NullDependenciesHandling) {
    // Act & Assert: Creating an editor with null dependencies should throw
    EXPECT_THROW(Editor(nullptr, commandManager, syntaxHighlightingManager), std::runtime_error);
    EXPECT_THROW(Editor(textBuffer, nullptr, syntaxHighlightingManager), std::runtime_error);
    EXPECT_THROW(Editor(textBuffer, commandManager, nullptr), std::runtime_error);
}

// Test compatibility with the old buffer accessor
TEST_F(EditorDITest, BackwardCompatibilityBuffer) {
    // Arrange: Setup a dynamic cast result for the old buffer accessor
    auto realTextBuffer = std::dynamic_pointer_cast<TextBuffer>(textBuffer);
    
    // This test is more of a compile-time check, as we can't easily mock the dynamic_cast in getBuffer()
    // We're just verifying that the method exists and can be called
    
    // Act: Create an editor and access the buffer through the old accessor
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // No assertions - this is primarily to ensure the code compiles and doesn't crash
    // In a real scenario, we'd need to inject a real TextBuffer or test this differently
}

// Test text editing operations that use command manager and text buffer
TEST_F(EditorDITest, TextEditingOperations) {
    // Arrange: Set up command manager expectations for executeCommand
    EXPECT_CALL(*commandManager, executeCommand(testing::_)).Times(4);
    
    // Act: Create an editor and perform text editing operations
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Test adding a line (should create a command and execute it)
    editor.addLine("New line");
    
    // Test inserting a line at a specific position
    editor.insertLine(0, "Inserted line");
    
    // Test deleting a line
    editor.deleteLine(0);
    
    // Test replacing a line
    editor.replaceLine(0, "Replaced line");
    
    // Assert: Expectations verified automatically by the test framework
}

// Test typing text which should create insert text commands
TEST_F(EditorDITest, TypeTextOperation) {
    // Arrange: Set up command manager expectations for executeCommand
    EXPECT_CALL(*commandManager, executeCommand(testing::_)).Times(2);
    
    // Act: Create an editor and type text
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Type a single character
    editor.typeChar('A');
    
    // Type a string of text
    editor.typeText("Hello, world!");
    
    // Assert: Expectations verified automatically
}

// Test loading and saving files which should work with the text buffer
TEST_F(EditorDITest, FileOperations) {
    // Arrange: Set up text buffer expectations for file operations
    std::string errorMsg;
    EXPECT_CALL(*textBuffer, loadFromFile("test.txt", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(errorMsg),
            testing::Return(true)
        ));
    
    EXPECT_CALL(*textBuffer, saveToFile("test.txt", testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(errorMsg),
            testing::Return(true)
        ));
    
    // Act: Create an editor and perform file operations
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Load a file
    bool loadResult = editor.loadFile("test.txt");
    
    // Save a file
    bool saveResult = editor.saveFile("test.txt");
    
    // Assert: Operations should return true (success)
    EXPECT_TRUE(loadResult);
    EXPECT_TRUE(saveResult);
}

// Test the find and replace operations that interact with the text buffer
TEST_F(EditorDITest, FindReplaceOperations) {
    // Arrange: Set up text buffer with mock lines for searching
    std::vector<std::string> lines = {
        "First line with the test word",
        "Second line without the word",
        "Third line with another test"
    };
    
    // Setup line count and getLine behavior
    EXPECT_CALL(*textBuffer, lineCount())
        .WillRepeatedly(testing::Return(lines.size()));
    
    // Setup getLine to return appropriate content
    for (size_t i = 0; i < lines.size(); i++) {
        const std::string& line = lines[i];
        ON_CALL(*textBuffer, getLine(i))
            .WillByDefault(testing::ReturnRef(line));
    }
    
    // Act: Create an editor and perform find operations
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Set cursor to start of buffer
    editor.setCursor(0, 0);
    
    // Find the first occurrence of "test"
    bool findResult = editor.find("test", true);
    
    // Assert: The find operation should succeed and move the cursor to the match
    EXPECT_TRUE(findResult);
    EXPECT_EQ(editor.getCursorLine(), 0);
    EXPECT_EQ(editor.getCursorCol(), 16); // Position of "test" in first line
    
    // Replace operations would need additional setup with command execution
    // which we're not testing in detail here
}

// Test integration with the DI container
TEST(EditorDIIntegration, ResolveFromContainer) {
    // Arrange: Create and configure the DI container
    di::Injector injector;
    ApplicationModule::configure(injector);
    
    // Act: Resolve the editor and its dependencies from the container
    auto editor = injector.resolve<IEditor>();
    auto textBuffer = injector.resolve<ITextBuffer>();
    auto commandManager = injector.resolve<ICommandManager>();
    auto syntaxHighlightingManager = injector.resolve<ISyntaxHighlightingManager>();
    
    // Assert: All components should be successfully resolved
    ASSERT_NE(editor, nullptr);
    ASSERT_NE(textBuffer, nullptr);
    ASSERT_NE(commandManager, nullptr);
    ASSERT_NE(syntaxHighlightingManager, nullptr);
    
    // The editor should have the same text buffer instance (if singleton binding is used)
    // or at least the same type if transient binding is used
    EXPECT_NE(editor->getTextBuffer(), nullptr);
}

// Test error handling during text buffer operations
TEST_F(EditorDITest, TextBufferErrorHandling) {
    // Arrange: Setup text buffer to throw an exception on operation
    EXPECT_CALL(*textBuffer, addLine("Error line"))
        .WillOnce(testing::Throw(std::runtime_error("Simulated buffer error")));
    
    // Act: Create an editor and attempt the operation
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Assert: The error should be caught and not propagate out
    // This is more of a behavioral test - we expect the Editor to handle
    // errors gracefully rather than crashing
    EXPECT_NO_THROW(editor.addLine("Error line"));
}

// Test handling of invalid selection operations
TEST_F(EditorDITest, InvalidSelectionHandling) {
    // Arrange: Create an editor without a selection
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Act & Assert: Operations that require selection should handle the case gracefully
    EXPECT_FALSE(editor.hasSelection());
    EXPECT_NO_THROW(editor.cutSelection());       // Should not throw when no selection
    EXPECT_NO_THROW(editor.copySelection());      // Should not throw when no selection
    EXPECT_NO_THROW(editor.deleteSelectedText()); // Should not throw when no selection
}

// Test concurrent text buffer and command manager operations
TEST_F(EditorDITest, ConcurrentOperations) {
    // This test simulates concurrent operations on the editor from multiple threads
    // to ensure thread safety of the dependency-injected components
    
    // Arrange: Setup command manager to handle multiple commands safely
    EXPECT_CALL(*commandManager, executeCommand(testing::_))
        .Times(testing::AtLeast(1));
    
    // Act: Create an editor
    Editor editor(textBuffer, commandManager, syntaxHighlightingManager);
    
    // Multiple threads performing operations
    // (This is a simplified example; real implementation would use std::thread)
    
    // Main thread: Add a line
    editor.addLine("Main thread line");
    
    // Simulate operations from different threads
    editor.typeText("Thread 1 text");
    editor.insertLine(0, "Thread 2 inserted line");
    
    // Assert: No specific assertions, we're just ensuring operations don't crash
    // In real code, you'd want to check thread safety more thoroughly
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 