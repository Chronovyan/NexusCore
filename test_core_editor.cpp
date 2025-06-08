#include <iostream>
#include <memory>
#include "TextBuffer.h"
#include "Editor.h"
#include "CommandManager.h"
#include "SyntaxHighlightingManager.h"

void runCoreEditorTests() {
    std::cout << "=== Starting Core Editor Tests ===\n";
    
    // Test 1: Basic TextBuffer operations
    {
        std::cout << "Test 1: TextBuffer basic operations... ";
        auto buffer = std::make_shared<TextBuffer>();
        
        // Insert text
        buffer->insert(0, 0, "Hello, World!");
        
        // Verify content
        if (buffer->getLine(0) != "Hello, World!") {
            std::cout << "FAILED: Text insertion failed\n";
            return;
        }
        
        // Delete text
        buffer->remove(0, 0, 7); // Remove "Hello, "
        if (buffer->getLine(0) != "World!") {
            std::cout << "FAILED: Text deletion failed\n";
            return;
        }
        
        std::cout << "PASSED\n";
    }
    
    // Test 2: Editor basic operations
    {
        std::cout << "Test 2: Editor basic operations... ";
        auto buffer = std::make_shared<TextBuffer>();
        auto cmdManager = std::make_shared<CommandManager>();
        auto hlManager = std::make_shared<SyntaxHighlightingManager>();
        
        Editor editor(buffer, cmdManager, hlManager);
        
        // Insert text
        editor.insertText("Test");
        if (buffer->getLine(0) != "Test") {
            std::cout << "FAILED: Editor text insertion failed\n";
            return;
        }
        
        // Undo
        editor.undo();
        if (!buffer->isEmpty()) {
            std::cout << "FAILED: Undo operation failed\n";
            return;
        }
        
        // Redo
        editor.redo();
        if (buffer->getLine(0) != "Test") {
            std::cout << "FAILED: Redo operation failed\n";
            return;
        }
        
        std::cout << "PASSED\n";
    }
    
    std::cout << "=== All Core Editor Tests PASSED ===\n";
}

int main() {
    try {
        runCoreEditorTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
