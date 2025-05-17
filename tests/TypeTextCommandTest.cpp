#include "gtest/gtest.h"
#include "EditorCommands.h"
#include "Editor.h"

class TypeTextCommandTest : public ::testing::Test {
protected:
    Editor editor;
    
    // Set up the fixture before each test
    void SetUp() override {
        // Make sure we have an empty line to work with
        // This will call validateAndClampCursor internally
        if (editor.getBuffer().isEmpty()) {
            editor.getBuffer().addLine("");
        }
        
        // Set cursor to 0,0 for consistency
        editor.setCursor(0, 0);
    }
};

// Test that TypeTextCommand correctly inserts a character
TEST_F(TypeTextCommandTest, Execute) {
    // Setup
    Editor editor;
    TextBuffer& buffer = editor.getBuffer();
    buffer.clear(true);
    buffer.replaceLine(0, "Hello World");
    editor.setCursor(0, 5);

    // Create command - InsertTextCommand with a single character
    std::string text = "A";
    InsertTextCommand command(text, 0, 5);

    // Execute the command
    command.execute(editor);

    // Verify character was inserted
    EXPECT_EQ("HelloA World", buffer.getLine(0));
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(6, editor.getCursorCol());
}

// Test that TypeTextCommand correctly undoes character insertion
TEST_F(TypeTextCommandTest, Undo) {
    // Setup
    Editor editor;
    TextBuffer& buffer = editor.getBuffer();
    buffer.clear(true);
    buffer.replaceLine(0, "Hello World");
    editor.setCursor(0, 5);

    // Create command - InsertTextCommand with a single character
    std::string text = "A";
    InsertTextCommand command(text, 0, 5);

    // Execute and then undo the command
    command.execute(editor);
    command.undo(editor);

    // Verify the character insertion was undone
    EXPECT_EQ("Hello World", buffer.getLine(0));
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(5, editor.getCursorCol());
}

// Test that processCharacterInput correctly creates and executes a TypeTextCommand
TEST_F(TypeTextCommandTest, ProcessCharacterInput) {
    // Process a character input
    editor.processCharacterInput('B');
    
    // Verify the text was inserted
    EXPECT_EQ("B", editor.getBuffer().getLine(0));
    
    // Verify cursor was moved after the inserted character
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(1, editor.getCursorCol());
    
    // Verify that undo works through the command manager
    EXPECT_TRUE(editor.canUndo());
    editor.undo();
    
    // Verify the text was removed
    EXPECT_EQ("", editor.getBuffer().getLine(0));
    
    // Verify cursor was moved back to original position
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(0, editor.getCursorCol());
}

// Test that typeChar method uses our new processCharacterInput method
TEST_F(TypeTextCommandTest, TypeChar) {
    // Test regular character
    editor.typeChar('C');
    
    // Verify the text was inserted
    EXPECT_EQ("C", editor.getBuffer().getLine(0));
    
    // Verify cursor was moved after the inserted character
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(1, editor.getCursorCol());
    
    // Verify that undo works
    EXPECT_TRUE(editor.canUndo());
    editor.undo();
    
    // Verify the text was removed
    EXPECT_EQ("", editor.getBuffer().getLine(0));
    
    // Verify cursor was moved back to original position
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(0, editor.getCursorCol());
    
    // Test newline character
    editor.typeChar('X'); // Insert a character first
    editor.typeChar('\n'); // Then insert newline
    
    // Verify there are now two lines
    EXPECT_EQ(2, editor.getBuffer().lineCount());
    EXPECT_EQ("X", editor.getBuffer().getLine(0));
    EXPECT_EQ("", editor.getBuffer().getLine(1));
    
    // Verify cursor is at the beginning of the new line
    EXPECT_EQ(1, editor.getCursorLine());
    EXPECT_EQ(0, editor.getCursorCol());
}

TEST_F(TypeTextCommandTest, ExecuteInsertCharacter) {
    // Setup
    Editor editor;
    TextBuffer& buffer = editor.getBuffer();
    buffer.clear(true);
    buffer.replaceLine(0, "Hello World");
    editor.setCursor(0, 5);

    // Create command - InsertTextCommand with a single character
    std::string text = "A";
    InsertTextCommand command(text, 0, 5);

    // Execute the command
    command.execute(editor);

    // Verify character was inserted
    EXPECT_EQ("HelloA World", buffer.getLine(0));
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(6, editor.getCursorCol());
}

TEST_F(TypeTextCommandTest, UndoInsertCharacter) {
    // Setup
    Editor editor;
    TextBuffer& buffer = editor.getBuffer();
    buffer.clear(true);
    buffer.replaceLine(0, "Hello World");
    editor.setCursor(0, 5);

    // Create command - InsertTextCommand with a single character
    std::string text = "A";
    InsertTextCommand command(text, 0, 5);

    // Execute and then undo the command
    command.execute(editor);
    command.undo(editor);

    // Verify the character insertion was undone
    EXPECT_EQ("Hello World", buffer.getLine(0));
    EXPECT_EQ(0, editor.getCursorLine());
    EXPECT_EQ(5, editor.getCursorCol());
} 