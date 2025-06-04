#include <iostream>
#include <memory>
#include "TransactionCommandManager.h"
#include "EditorCommands.h"
#include "Editor.h"

/**
 * This example demonstrates how to use the TransactionCommandManager
 * to group multiple commands into a single undoable/redoable transaction.
 */
int main() {
    // Create an editor and command manager
    Editor editor;
    auto commandManager = std::make_shared<TransactionCommandManager>();

    // Add some initial text to the editor
    editor.insertText("Hello, World!\nThis is a test.\nLet's try transaction grouping.\n");

    std::cout << "Initial text:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";

    // Simple command execution without transactions
    std::cout << "Executing a single command...\n";
    commandManager->executeCommand(
        std::make_unique<InsertTextCommand>(3, 0, "New line at position 3.\n"),
        editor
    );
    
    std::cout << "After single command:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";
    
    // Begin a transaction for a group of related operations
    std::cout << "Beginning a transaction for multiple operations...\n";
    commandManager->beginTransaction("Format document");
    
    // Execute multiple commands as part of the transaction
    commandManager->executeCommand(
        std::make_unique<DeleteSelectionCommand>(0, 0, 0, 7), // Delete "Hello, "
        editor
    );
    
    commandManager->executeCommand(
        std::make_unique<ReplaceSelectionCommand>(0, 0, 0, 5, "Greetings"), // Replace "World" with "Greetings"
        editor
    );
    
    commandManager->executeCommand(
        std::make_unique<InsertTextCommand>(1, 0, "// "), // Add comment to line 2
        editor
    );
    
    commandManager->executeCommand(
        std::make_unique<InsertTextCommand>(2, 0, "// "), // Add comment to line 3
        editor
    );
    
    // End the transaction, grouping all commands into a single undoable unit
    commandManager->endTransaction();
    
    std::cout << "After transaction:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";
    
    // Demonstrate undoing the transaction (all commands at once)
    std::cout << "Undoing the transaction...\n";
    commandManager->undo(editor);
    
    std::cout << "After undo:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";
    
    // Demonstrate redoing the transaction
    std::cout << "Redoing the transaction...\n";
    commandManager->redo(editor);
    
    std::cout << "After redo:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";
    
    // Demonstrate nested transactions
    std::cout << "Demonstrating nested transactions...\n";
    
    // Outer transaction
    commandManager->beginTransaction("Document restructuring");
    
    // Add a title
    commandManager->executeCommand(
        std::make_unique<InsertTextCommand>(0, 0, "# Document Title\n\n"),
        editor
    );
    
    // Inner transaction for formatting multiple paragraphs
    commandManager->beginTransaction("Format paragraphs");
    
    // Add formatting to multiple paragraphs
    for (int i = 3; i < 7; ++i) {
        if (i < editor.getBufferLineCount()) {
            commandManager->executeCommand(
                std::make_unique<InsertTextCommand>(i, 0, "> "), // Add blockquote formatting
                editor
            );
        }
    }
    
    // End the inner transaction
    commandManager->endTransaction();
    
    // Add a footer to the document
    commandManager->executeCommand(
        std::make_unique<InsertTextCommand>(editor.getBufferLineCount(), 0, "\n--- End of Document ---\n"),
        editor
    );
    
    // End the outer transaction
    commandManager->endTransaction();
    
    std::cout << "After nested transactions:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";
    
    // Undo everything at once
    std::cout << "Undoing all transactions...\n";
    while (commandManager->canUndo()) {
        commandManager->undo(editor);
    }
    
    std::cout << "After undoing all:\n";
    editor.printBuffer(std::cout);
    std::cout << "\n";
    
    return 0;
} 