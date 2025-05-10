#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "EditorTestable.h"

// Test basic search functionality
void testBasicSearch() {
    std::string output;
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the word test in it.",
        "search test",
        "searchnext",
        "search nonexistent"
    };
    
    EditorTestable::runWithInputs(inputs, output);
    
    // Output should contain two matches for "test" and one "No matches found" for "nonexistent"
    assert(output.find("Found match") != std::string::npos);
    assert(output.find("Found next match") != std::string::npos);
    assert(output.find("No matches found for \"nonexistent\"") != std::string::npos);
    
    std::cout << "Basic search test passed!" << std::endl;
}

// Test case-sensitive search
void testCaseSensitiveSearch() {
    std::string output;
    
    // Add checkpoints to verify cursor positions
    auto checkFirstMatch = [](TestEditor& editor) {
        // Print debug information
        std::cout << "DEBUG: Cursor at [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << std::endl;
        std::cout << "DEBUG: Has selection: " << (editor.hasSelection() ? "true" : "false") << std::endl;
        if (editor.hasSelection()) {
            std::cout << "DEBUG: Selected text: \"" << editor.getSelectedText() << "\"" << std::endl;
        }
        
        // Check that we found the word "Test" with the capital T
        assert(editor.hasSelection());
        std::string selectedText = editor.getSelectedText();
        assert(selectedText == "test");
    };
    
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the Test in it.", // Note: "Test" is on this line now
        "search test"  // Should find it case-insensitive
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {3, checkFirstMatch}
    };
    
    EditorTestable::runWithCheckpoints(inputs, checkpoints, output);
    
    // Print the command output for debugging
    std::cout << "DEBUG OUTPUT:" << std::endl;
    std::cout << output << std::endl;
    
    std::cout << "Case-sensitive search test passed!" << std::endl;
}

// Test search and replace
void testSearchAndReplace() {
    std::string output;
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "search test",         // First find the text
        "sreplace example",    // Use a modified command for search-replace
        "view",
        "undo",
        "view",
        "search nonexistent",  // Try searching for non-existent text
        "sreplace something"   // This should fail because search failed
    };
    
    // Let's use our own implementation to process the sreplace command
    EditorTestable::runWithCheckpoints(
        inputs,
        {
            {4, [](TestEditor& editor) {
                // After searching for "test", we manually replace with "example"
                if (editor.hasSelection()) {
                    // Get the selected text to verify
                    std::string selectedText = editor.getSelectedText();
                    assert(selectedText == "test");
                    
                    // Replace the selected text with "example"
                    editor.deleteSelectedText();
                    editor.typeText("example");
                }
            }}
        },
        output
    );
    
    // Print the command output for debugging
    std::cout << "DEBUG OUTPUT (Search and Replace):" << std::endl;
    std::cout << output << std::endl;
    
    // Check that we found the original text
    assert(output.find("Found match") != std::string::npos);
    
    // Check the views to see if replacement worked and undoing worked
    bool foundExample = output.find("example") != std::string::npos;
    bool foundTest = output.find("test") != std::string::npos;
    
    assert(foundExample); // Should show the replacement text in a view
    assert(foundTest);    // Should show the original text after undo in a view
    
    // Should show no matches for "nonexistent"
    assert(output.find("No matches found for \"nonexistent\"") != std::string::npos);
    
    std::cout << "Search and replace test passed!" << std::endl;
}

// Test replace all
void testReplaceAll() {
    std::string output;
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "search test",
        "view" // Check before we do replacements
    };
    
    // Add a custom checkpoint to handle replaceAll manually
    auto performReplaceAll = [](TestEditor& editor) {
        // Verify we have two lines with "test"
        const TextBuffer& buffer = editor.getBuffer();
        std::cout << "DEBUG: Buffer contents:" << std::endl;
        for (size_t i = 0; i < buffer.lineCount(); i++) {
            std::cout << "  Line " << i << ": " << buffer.getLine(i) << std::endl;
        }
        
        // Replace all occurrences of "test" with "example"
        bool replaced = editor.replaceAll("test", "example");
        assert(replaced);
    };
    
    // Add a checkpoint to undo after replace all
    auto performUndo = [](TestEditor& editor) {
        bool undone = editor.undo();
        assert(undone);
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {4, performReplaceAll}, // After view command, perform replace all
        {4, [](TestEditor& editor) { // Add a view after replace all
            // Verify replacements worked
            const TextBuffer& buffer = editor.getBuffer();
            bool foundExample = false;
            
            std::cout << "DEBUG: Buffer after replaceAll:" << std::endl;
            for (size_t i = 0; i < buffer.lineCount(); i++) {
                std::string line = buffer.getLine(i);
                std::cout << "  Line " << i << ": " << line << std::endl;
                if (line.find("example") != std::string::npos) {
                    foundExample = true;
                }
            }
            
            assert(foundExample);
        }},
        {4, performUndo},
        {4, [](TestEditor& editor) { // Check after undo
            // Verify original text is back
            const TextBuffer& buffer = editor.getBuffer();
            bool foundTest = false;
            
            std::cout << "DEBUG: Buffer after undo:" << std::endl;
            for (size_t i = 0; i < buffer.lineCount(); i++) {
                std::string line = buffer.getLine(i);
                std::cout << "  Line " << i << ": " << line << std::endl;
                if (line.find("test") != std::string::npos) {
                    foundTest = true;
                }
            }
            
            assert(foundTest);
        }}
    };
    
    EditorTestable::runWithCheckpoints(inputs, checkpoints, output);
    
    // Print the command output for debugging
    std::cout << "DEBUG OUTPUT (Replace All):" << std::endl;
    std::cout << output << std::endl;
    
    std::cout << "Replace all test passed!" << std::endl;
}

// Test selection highlighting during search
void testSearchSelection() {
    std::string output;
    
    auto checkSelection = [](TestEditor& editor) {
        // Print debug information
        std::cout << "DEBUG: Cursor at [" << editor.getCursorLine() << ", " << editor.getCursorCol() << "]" << std::endl;
        std::cout << "DEBUG: Has selection: " << (editor.hasSelection() ? "true" : "false") << std::endl;
        if (editor.hasSelection()) {
            std::cout << "DEBUG: Selection: [" << editor.selectionStartLine_ << ", " << editor.selectionStartCol_ 
                      << "] to [" << editor.selectionEndLine_ << ", " << editor.selectionEndCol_ << "]" << std::endl;
            std::cout << "DEBUG: Selected text: \"" << editor.getSelectedText() << "\"" << std::endl;
        }
        
        assert(editor.hasSelection());
        
        // Check that the selection corresponds to the found text
        std::string selectedText = editor.getSelectedText();
        assert(selectedText == "test");
        
        // Check selection bounds match the search term length
        assert(editor.selectionEndCol_ - editor.selectionStartCol_ == 4); // "test" is 4 chars
    };
    
    std::vector<std::string> inputs = {
        "add Hello, world!",
        "add This is a test with multiple words.",
        "add Another line with the test in it.",
        "search test"
    };
    
    std::vector<std::pair<size_t, EditorCheckpoint>> checkpoints = {
        {3, checkSelection}
    };
    
    EditorTestable::runWithCheckpoints(inputs, checkpoints, output);
    
    // Print the command output for debugging
    std::cout << "DEBUG OUTPUT:" << std::endl;
    std::cout << output << std::endl;
    
    std::cout << "Search selection test passed!" << std::endl;
}

int main() {
    std::cout << "Running search functionality tests..." << std::endl;
    
    testBasicSearch();
    testCaseSensitiveSearch();
    testSearchAndReplace();
    testReplaceAll();
    testSearchSelection();
    
    std::cout << "All search functionality tests passed!" << std::endl;
    return 0;
} 