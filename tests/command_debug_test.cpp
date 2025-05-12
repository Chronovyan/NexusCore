#include "gtest/gtest.h"
#include "TestEditor.h"
#include "../src/EditorCommands.h"
#include "TestUtilities.h"
#include <memory>
#include <string>
#include <iostream>

// Debug test fixture for isolating issues
class DebugCommandTest : public test_utils::EditorCommandTestBase {
protected:
    void SetUp() override {
        // Call base class setup
        test_utils::EditorCommandTestBase::SetUp();
        
        // Disable syntax highlighting to avoid thread synchronization issues
        editor.enableSyntaxHighlighting(false);
        std::cout << "DEBUG: Syntax highlighting disabled for test" << std::endl;
    }
    
    void TearDown() override {
        // Call base class teardown
        test_utils::EditorCommandTestBase::TearDown();
    }
    
    // Helper to print buffer content for debugging
    void logBufferContent(const std::string& label) {
        std::cout << "DEBUG: " << label << " Buffer content:" << std::endl;
        for (size_t i = 0; i < editor.getBuffer().lineCount(); ++i) {
            std::cout << "DEBUG:   Line " << i << ": '" 
                      << editor.getBuffer().getLine(i) << "'" << std::endl;
        }
        std::cout << "DEBUG:   Cursor at: (" << editor.getCursorLine() 
                  << ", " << editor.getCursorCol() << ")" << std::endl;
    }
};

// Very simple test case for compound command
TEST_F(DebugCommandTest, SimpleCompoundTest) {
    // Setup buffer with initial content
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("Test line");
    editor.setCursor(0, 0);
    
    std::cout << "STARTING SIMPLE COMPOUND TEST" << std::endl;
    logBufferContent("Initial");

    // Create a compound command with a single operation
    auto compoundCmd = std::make_unique<CompoundCommand>();
    compoundCmd->addCommand(std::make_unique<InsertTextCommand>("PREFIX "));

    // Execute the compound command
    std::cout << "DEBUG: Executing compound command" << std::endl;
    compoundCmd->execute(editor);
    logBufferContent("After execution");

    // Verify state after execution
    ASSERT_EQ(1, editor.getBuffer().lineCount());
    ASSERT_EQ("PREFIX Test line", editor.getBuffer().getLine(0));
    ASSERT_EQ(0, editor.getCursorLine());
    ASSERT_EQ(7, editor.getCursorCol());

    // Undo the compound command
    std::cout << "DEBUG: Undoing compound command" << std::endl;
    compoundCmd->undo(editor);
    logBufferContent("After undo");

    // Verify undo restored the original state
    ASSERT_EQ(1, editor.getBuffer().lineCount());
    ASSERT_EQ("Test line", editor.getBuffer().getLine(0));
    ASSERT_EQ(0, editor.getCursorLine());
    ASSERT_EQ(0, editor.getCursorCol());
    
    std::cout << "SIMPLE COMPOUND TEST COMPLETED" << std::endl;
}

// Test compound command with deletion operation
TEST_F(DebugCommandTest, SimpleDeleteTest) {
    // Setup buffer with initial content
    editor.getBuffer().clear(false);
    editor.getBuffer().addLine("First line");
    editor.getBuffer().addLine("Second line");
    editor.getBuffer().addLine("Third line");
    editor.setCursor(1, 0);
    
    std::cout << "\nSTARTING SIMPLE DELETE TEST" << std::endl;
    logBufferContent("Initial");

    // Create a compound command with insert and delete operations
    auto compoundCmd = std::make_unique<CompoundCommand>();
    compoundCmd->addCommand(std::make_unique<InsertTextCommand>("Modified: "));
    compoundCmd->addCommand(std::make_unique<DeleteLineCommand>(2)); // Delete "Third line"

    // Execute the compound command
    std::cout << "DEBUG: Executing compound command" << std::endl;
    compoundCmd->execute(editor);
    logBufferContent("After execution");

    // Verify state after execution
    ASSERT_EQ(2, editor.getBuffer().lineCount());
    ASSERT_EQ("First line", editor.getBuffer().getLine(0));
    ASSERT_EQ("Modified: Second line", editor.getBuffer().getLine(1));
    ASSERT_EQ(1, editor.getCursorLine());
    ASSERT_EQ(10, editor.getCursorCol());

    // Undo the compound command
    std::cout << "DEBUG: Undoing compound command" << std::endl;
    compoundCmd->undo(editor);
    logBufferContent("After undo");

    // Verify undo restored the original state
    ASSERT_EQ(3, editor.getBuffer().lineCount());
    ASSERT_EQ("First line", editor.getBuffer().getLine(0));
    ASSERT_EQ("Second line", editor.getBuffer().getLine(1));
    ASSERT_EQ("Third line", editor.getBuffer().getLine(2));
    ASSERT_EQ(1, editor.getCursorLine());
    ASSERT_EQ(0, editor.getCursorCol());
    
    std::cout << "SIMPLE DELETE TEST COMPLETED" << std::endl;
}

// Direct test of CompoundCommand without TestEditor
TEST(DirectCompoundCommandTest, BasicTest) {
    // Create a minimal buffer for testing
    TextBuffer buffer;
    buffer.addLine("Test line");
    
    // Create a mock editor object that just tracks state
    struct MockEditor {
        TextBuffer& buffer;
        size_t cursorLine = 0;
        size_t cursorCol = 0;
        
        MockEditor(TextBuffer& buf) : buffer(buf) {}
        
        TextBuffer& getBuffer() { return buffer; }
        size_t getCursorLine() const { return cursorLine; }
        size_t getCursorCol() const { return cursorCol; }
        void setCursor(size_t line, size_t col) { 
            cursorLine = line; 
            cursorCol = col; 
        }
    };
    
    MockEditor editor(buffer);
    
    std::cout << "STARTING DIRECT COMPOUND TEST" << std::endl;
    std::cout << "Initial buffer: '" << buffer.getLine(0) << "'" << std::endl;
    std::cout << "Initial cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Create a compound command with a single operation
    auto cmd = std::make_unique<InsertTextCommand>("PREFIX ");
    cmd->execute(editor);
    
    std::cout << "After insert: '" << buffer.getLine(0) << "'" << std::endl;
    std::cout << "Cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Verify state
    ASSERT_EQ("PREFIX Test line", buffer.getLine(0));
    ASSERT_EQ(0, editor.getCursorLine());
    ASSERT_EQ(7, editor.getCursorCol());
    
    // Undo
    cmd->undo(editor);
    
    std::cout << "After undo: '" << buffer.getLine(0) << "'" << std::endl;
    std::cout << "Cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Verify undo
    ASSERT_EQ("Test line", buffer.getLine(0));
    ASSERT_EQ(0, editor.getCursorLine());
    ASSERT_EQ(0, editor.getCursorCol());
    
    std::cout << "DIRECT TEST COMPLETE" << std::endl;
}

// Now try with a compound operation
TEST(DirectCompoundCommandTest, CompoundTest) {
    // Create a minimal buffer for testing
    TextBuffer buffer;
    buffer.addLine("First line");
    buffer.addLine("Second line");
    buffer.addLine("Third line");
    
    // Create a mock editor object that just tracks state
    struct MockEditor {
        TextBuffer& buffer;
        size_t cursorLine = 0;
        size_t cursorCol = 0;
        
        MockEditor(TextBuffer& buf) : buffer(buf) {}
        
        TextBuffer& getBuffer() { return buffer; }
        size_t getCursorLine() const { return cursorLine; }
        size_t getCursorCol() const { return cursorCol; }
        void setCursor(size_t line, size_t col) { 
            cursorLine = line; 
            cursorCol = col; 
        }
    };
    
    MockEditor editor(buffer);
    editor.setCursor(1, 0); // Position at start of "Second line"
    
    std::cout << "\nSTARTING DIRECT COMPOUND TEST WITH MULTIPLE OPERATIONS" << std::endl;
    std::cout << "Initial buffer:" << std::endl;
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::cout << "  Line " << i << ": '" << buffer.getLine(i) << "'" << std::endl;
    }
    std::cout << "Initial cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Create a compound command
    auto compoundCmd = std::make_unique<CompoundCommand>();
    
    // Add commands to the compound command
    std::cout << "Adding InsertTextCommand(\"Modified: \")" << std::endl;
    compoundCmd->addCommand(std::make_unique<InsertTextCommand>("Modified: "));
    
    std::cout << "Adding DeleteLineCommand(2)" << std::endl;
    compoundCmd->addCommand(std::make_unique<DeleteLineCommand>(2)); // Delete "Third line"
    
    // Execute the compound command
    std::cout << "Executing compound command" << std::endl;
    compoundCmd->execute(editor);
    
    // Print the result
    std::cout << "After execution:" << std::endl;
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::cout << "  Line " << i << ": '" << buffer.getLine(i) << "'" << std::endl;
    }
    std::cout << "Cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Verify state
    ASSERT_EQ(2, buffer.lineCount());
    ASSERT_EQ("First line", buffer.getLine(0));
    ASSERT_EQ("Modified: Second line", buffer.getLine(1));
    ASSERT_EQ(1, editor.getCursorLine());
    ASSERT_EQ(10, editor.getCursorCol());
    
    // Undo the compound command
    std::cout << "Undoing compound command" << std::endl;
    compoundCmd->undo(editor);
    
    // Print the result after undo
    std::cout << "After undo:" << std::endl;
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::cout << "  Line " << i << ": '" << buffer.getLine(i) << "'" << std::endl;
    }
    std::cout << "Cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Verify undo restored the original state
    ASSERT_EQ(3, buffer.lineCount());
    ASSERT_EQ("First line", buffer.getLine(0));
    ASSERT_EQ("Second line", buffer.getLine(1));
    ASSERT_EQ("Third line", buffer.getLine(2));
    ASSERT_EQ(1, editor.getCursorLine());
    ASSERT_EQ(0, editor.getCursorCol());
    
    std::cout << "DIRECT COMPOUND TEST COMPLETE" << std::endl;
}

// Test the problematic case with insert, delete line, and delete char
TEST(DirectCompoundCommandTest, InsertDeleteTest) {
    // Create a minimal buffer for testing
    TextBuffer buffer;
    buffer.addLine("First line");
    buffer.addLine("Second line");
    buffer.addLine("Third line");
    
    // Create a mock editor object that just tracks state
    struct MockEditor {
        TextBuffer& buffer;
        size_t cursorLine = 0;
        size_t cursorCol = 0;
        
        MockEditor(TextBuffer& buf) : buffer(buf) {}
        
        TextBuffer& getBuffer() { return buffer; }
        size_t getCursorLine() const { return cursorLine; }
        size_t getCursorCol() const { return cursorCol; }
        void setCursor(size_t line, size_t col) { 
            cursorLine = line; 
            cursorCol = col; 
        }
    };
    
    MockEditor editor(buffer);
    editor.setCursor(1, 0); // Position at start of "Second line"
    
    std::cout << "\nSTARTING PROBLEMATIC CASE TEST" << std::endl;
    std::cout << "Initial buffer:" << std::endl;
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::cout << "  Line " << i << ": '" << buffer.getLine(i) << "'" << std::endl;
    }
    std::cout << "Initial cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Create a compound command
    auto compoundCmd = std::make_unique<CompoundCommand>();
    
    // Add commands to the compound command
    std::cout << "Adding InsertTextCommand(\"Modified: \")" << std::endl;
    compoundCmd->addCommand(std::make_unique<InsertTextCommand>("Modified: "));
    
    std::cout << "Adding DeleteLineCommand(2)" << std::endl;
    compoundCmd->addCommand(std::make_unique<DeleteLineCommand>(2)); // Delete "Third line"
    
    std::cout << "Adding DeleteCharCommand(false)" << std::endl;
    compoundCmd->addCommand(std::make_unique<DeleteCharCommand>(false)); // Delete char after cursor
    
    // Execute the compound command
    std::cout << "Executing compound command" << std::endl;
    compoundCmd->execute(editor);
    
    // Print the result
    std::cout << "After execution:" << std::endl;
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::cout << "  Line " << i << ": '" << buffer.getLine(i) << "'" << std::endl;
    }
    std::cout << "Cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Verify state - This reflects what the test SHOULD expect
    ASSERT_EQ(2, buffer.lineCount());
    ASSERT_EQ("First line", buffer.getLine(0));
    std::string line1 = buffer.getLine(1);
    std::cout << "Actual second line: '" << line1 << "'" << std::endl;
    ASSERT_EQ("Modified: Scond line", line1); // 'e' in 'Second' deleted
    ASSERT_EQ(1, editor.getCursorLine());
    ASSERT_EQ(10, editor.getCursorCol()); // Should be after "Modified: "
    
    // Undo the compound command
    std::cout << "Undoing compound command" << std::endl;
    compoundCmd->undo(editor);
    
    // Print the result after undo
    std::cout << "After undo:" << std::endl;
    for (size_t i = 0; i < buffer.lineCount(); ++i) {
        std::cout << "  Line " << i << ": '" << buffer.getLine(i) << "'" << std::endl;
    }
    std::cout << "Cursor: (" << editor.getCursorLine() << ", " << editor.getCursorCol() << ")" << std::endl;
    
    // Verify undo restored the original state
    ASSERT_EQ(3, buffer.lineCount());
    ASSERT_EQ("First line", buffer.getLine(0));
    ASSERT_EQ("Second line", buffer.getLine(1));
    ASSERT_EQ("Third line", buffer.getLine(2));
    ASSERT_EQ(1, editor.getCursorLine());
    ASSERT_EQ(0, editor.getCursorCol());
    
    std::cout << "PROBLEMATIC CASE TEST COMPLETE" << std::endl;
} 