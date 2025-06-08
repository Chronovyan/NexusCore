#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include "TransactionCommandManager.h"
#include "Editor.h"
#include "EditorCommands.h"

class TransactionCommandManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        editor_ = std::make_unique<Editor>();
        commandManager_ = std::make_unique<TransactionCommandManager>();
        
        // Setup initial text in the buffer
        editor_->insertText("Line 1\nLine 2\nLine 3\nLine 4\n");
    }

    std::unique_ptr<Editor> editor_;
    std::unique_ptr<TransactionCommandManager> commandManager_;
    
    // Helper method to get buffer content as string
    std::string getBufferContent() {
        std::stringstream ss;
        editor_->printBuffer(ss);
        return ss.str();
    }
};

// Test that basic command execution works
TEST_F(TransactionCommandManagerTest, BasicCommandExecution) {
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(0, 0, "Start: "),
        *editor_
    );
    
    EXPECT_EQ("Start: Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
    
    commandManager_->undo(*editor_);
    EXPECT_EQ("Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
    
    commandManager_->redo(*editor_);
    EXPECT_EQ("Start: Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
}

// Test a simple transaction with multiple commands
TEST_F(TransactionCommandManagerTest, SimpleTransaction) {
    // Start a transaction
    EXPECT_TRUE(commandManager_->beginTransaction("Test Transaction"));
    EXPECT_TRUE(commandManager_->isInTransaction());
    
    // Execute multiple commands within the transaction
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(0, 0, "A: "),
        *editor_
    );
    
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(1, 0, "B: "),
        *editor_
    );
    
    // End the transaction
    EXPECT_TRUE(commandManager_->endTransaction());
    EXPECT_FALSE(commandManager_->isInTransaction());
    
    // Verify the changes
    EXPECT_EQ("A: Line 1\nB: Line 2\nLine 3\nLine 4\n", getBufferContent());
    
    // Undo the entire transaction at once
    commandManager_->undo(*editor_);
    EXPECT_EQ("Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
    
    // Redo the entire transaction
    commandManager_->redo(*editor_);
    EXPECT_EQ("A: Line 1\nB: Line 2\nLine 3\nLine 4\n", getBufferContent());
}

// Test nested transactions
TEST_F(TransactionCommandManagerTest, NestedTransactions) {
    // Start outer transaction
    EXPECT_TRUE(commandManager_->beginTransaction("Outer"));
    EXPECT_EQ(1, commandManager_->getTransactionDepth());
    
    // First command in outer transaction
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(0, 0, "Outer1: "),
        *editor_
    );
    
    // Start inner transaction
    EXPECT_TRUE(commandManager_->beginTransaction("Inner"));
    EXPECT_EQ(2, commandManager_->getTransactionDepth());
    
    // Commands in inner transaction
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(1, 0, "Inner1: "),
        *editor_
    );
    
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(2, 0, "Inner2: "),
        *editor_
    );
    
    // End inner transaction
    EXPECT_TRUE(commandManager_->endTransaction());
    EXPECT_EQ(1, commandManager_->getTransactionDepth());
    
    // Another command in outer transaction
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(3, 0, "Outer2: "),
        *editor_
    );
    
    // End outer transaction
    EXPECT_TRUE(commandManager_->endTransaction());
    EXPECT_EQ(0, commandManager_->getTransactionDepth());
    
    // Verify all changes
    EXPECT_EQ("Outer1: Line 1\nInner1: Line 2\nInner2: Line 3\nOuter2: Line 4\n", getBufferContent());
    
    // Undo the entire transaction hierarchy at once
    commandManager_->undo(*editor_);
    EXPECT_EQ("Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
    
    // Redo the entire transaction hierarchy
    commandManager_->redo(*editor_);
    EXPECT_EQ("Outer1: Line 1\nInner1: Line 2\nInner2: Line 3\nOuter2: Line 4\n", getBufferContent());
}

// Test canceling a transaction
TEST_F(TransactionCommandManagerTest, CancelTransaction) {
    // Start a transaction
    EXPECT_TRUE(commandManager_->beginTransaction());
    
    // Execute some commands
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(0, 0, "Should be canceled: "),
        *editor_
    );
    
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(1, 0, "Also canceled: "),
        *editor_
    );
    
    // Buffer should show changes temporarily
    EXPECT_EQ("Should be canceled: Line 1\nAlso canceled: Line 2\nLine 3\nLine 4\n", getBufferContent());
    
    // Cancel the transaction
    EXPECT_TRUE(commandManager_->cancelTransaction());
    
    // Changes should still be in the buffer, but not in the undo stack
    EXPECT_EQ("Should be canceled: Line 1\nAlso canceled: Line 2\nLine 3\nLine 4\n", getBufferContent());
    
    // Verify there's nothing to undo
    EXPECT_FALSE(commandManager_->canUndo());
}

// Test empty transactions
TEST_F(TransactionCommandManagerTest, EmptyTransaction) {
    // Record initial undo stack size
    size_t initialUndoSize = commandManager_->undoStackSize();
    
    // Start and end an empty transaction
    EXPECT_TRUE(commandManager_->beginTransaction("Empty"));
    EXPECT_TRUE(commandManager_->endTransaction());
    
    // Undo stack size should not change
    EXPECT_EQ(initialUndoSize, commandManager_->undoStackSize());
}

// Test mixing transactions and regular commands
TEST_F(TransactionCommandManagerTest, MixedCommands) {
    // Regular command
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(0, 0, "Regular1: "),
        *editor_
    );
    
    // Transaction
    EXPECT_TRUE(commandManager_->beginTransaction("Transaction"));
    
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(1, 0, "Transaction1: "),
        *editor_
    );
    
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(2, 0, "Transaction2: "),
        *editor_
    );
    
    EXPECT_TRUE(commandManager_->endTransaction());
    
    // Another regular command
    commandManager_->executeCommand(
        std::make_unique<InsertTextCommand>(3, 0, "Regular2: "),
        *editor_
    );
    
    // Verify all changes
    EXPECT_EQ("Regular1: Line 1\nTransaction1: Line 2\nTransaction2: Line 3\nRegular2: Line 4\n", getBufferContent());
    
    // Undo the last regular command
    commandManager_->undo(*editor_);
    EXPECT_EQ("Regular1: Line 1\nTransaction1: Line 2\nTransaction2: Line 3\nLine 4\n", getBufferContent());
    
    // Undo the transaction (both commands at once)
    commandManager_->undo(*editor_);
    EXPECT_EQ("Regular1: Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
    
    // Undo the first regular command
    commandManager_->undo(*editor_);
    EXPECT_EQ("Line 1\nLine 2\nLine 3\nLine 4\n", getBufferContent());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 