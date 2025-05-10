#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "EditorTestable.h"
#include "../src/SyntaxHighlighter.h"

// Test enabling/disabling syntax highlighting
void testEnableSyntaxHighlighting() {
    TestEditor editor;
    
    // Default should be enabled in our test
    assert(editor.isSyntaxHighlightingEnabled());
    
    // Test disabling
    editor.enableSyntaxHighlighting(false);
    assert(!editor.isSyntaxHighlightingEnabled());
    
    // Test re-enabling
    editor.enableSyntaxHighlighting(true);
    assert(editor.isSyntaxHighlightingEnabled());
    
    std::cout << "Syntax highlighting enable/disable test passed!" << std::endl;
}

// Test setting filename and highlighter detection
void testFilenameAndHighlighterDetection() {
    TestEditor editor;
    
    // Initially, no filename and no highlighter
    assert(editor.getFilename().empty());
    assert(editor.getCurrentHighlighter() == nullptr);
    
    // Set C++ filename
    editor.setFilename("test.cpp");
    assert(editor.getFilename() == "test.cpp");
    assert(editor.getCurrentHighlighter() != nullptr);
    
    // Verify it's a C++ highlighter
    SyntaxHighlighter* highlighter = editor.getCurrentHighlighter();
    assert(highlighter != nullptr);
    assert(highlighter->getLanguageName() == "C++");
    
    // Test with an unsupported extension
    editor.setFilename("test.unknown");
    assert(editor.getFilename() == "test.unknown");
    assert(editor.getCurrentHighlighter() == nullptr);
    
    std::cout << "Filename and highlighter detection test passed!" << std::endl;
}

// Test the highlighting of C++ code
void testCppSyntaxHighlighting() {
    TestEditor editor;
    editor.setFilename("test.cpp");
    
    // Add some C++ code to highlight
    editor.addLine("#include <iostream>");
    editor.addLine("");
    editor.addLine("int main() {");
    editor.addLine("    // This is a comment");
    editor.addLine("    int x = 42;");
    editor.addLine("    std::string text = \"Hello, world!\";");
    editor.addLine("    if (x > 0) {");
    editor.addLine("        std::cout << text << std::endl;");
    editor.addLine("    }");
    editor.addLine("    return 0;");
    editor.addLine("}");
    
    // Verify we have a highlighter
    assert(editor.getCurrentHighlighter() != nullptr);
    
    // Get the highlighting styles
    auto styles = editor.getHighlightingStyles();
    assert(styles.size() == editor.getBuffer().lineCount());
    
    // Check specific syntax elements on specific lines
    
    // Line 0 in test logic (actual line 1 in buffer): "#include <iostream>" - should have preprocessor highlight
    if (styles.size() > 1) { // Check for index 1
        const auto& line1Styles = styles[1]; // Use index 1
        bool foundPreprocessor = false;

        // Debug: Print contents of line1Styles
        // std::cout << "Debug: line1Styles (styles[1]) content:" << std::endl;
        // if (line1Styles.empty()) {
        //     std::cout << "  line1Styles is empty." << std::endl;
        // }
        for (const auto& style : line1Styles) {
            // std::cout << "  Start: " << style.startCol 
            //           << ", End: " << style.endCol 
            //           << ", Color: " << static_cast<int>(style.color) // Cast enum to int for printing
            //           << std::endl;
            if (style.color == SyntaxColor::Preprocessor) {
                foundPreprocessor = true;
                break; // Restore break
            }
        }
        assert(foundPreprocessor);
    } else {
        assert(false && "styles array does not contain enough elements for line 1 (index 1)");
    }
    
    // Line 2 in test logic (actual line 3 in buffer): "int main() {" - should have type and function highlights
    if (styles.size() > 3) { // Check for index 3
        const auto& line3Styles = styles[3]; // Use index 3
        bool foundType = false;
        bool foundFunction = false;
        for (const auto& style : line3Styles) {
            if (style.color == SyntaxColor::Type) {
                foundType = true;
            }
            if (style.color == SyntaxColor::Function) {
                foundFunction = true;
            }
        }
        assert(foundType);
        assert(foundFunction);
    } else {
        assert(false && "styles array does not contain enough elements for line 3 (index 3)");
    }
    
    // Line 3 in test logic (actual line 4 in buffer): "// This is a comment" - should have comment highlight
    if (styles.size() > 4) { // Check for index 4
        const auto& line4Styles = styles[4]; // Use index 4
        bool foundComment = false;
        for (const auto& style : line4Styles) {
            if (style.color == SyntaxColor::Comment) {
                foundComment = true;
                break;
            }
        }
        assert(foundComment);
    } else {
        assert(false && "styles array does not contain enough elements for line 4 (index 4)");
    }
    
    // Line 4 in test logic (actual line 5 in buffer): "int x = 42;" - should have type and number highlights
    if (styles.size() > 5) { // Check for index 5
        const auto& line5Styles = styles[5]; // Use index 5
        bool foundNumber = false;
        bool foundType = false;
        for (const auto& style : line5Styles) {
            if (style.color == SyntaxColor::Type) {
                foundType = true;
            }
            if (style.color == SyntaxColor::Number) {
                foundNumber = true;
            }
        }
        assert(foundType);
        assert(foundNumber);
    } else {
        assert(false && "styles array does not contain enough elements for line 5 (index 5)");
    }
    
    // Line 5 in test logic (actual line 6 in buffer): 'std::string text = "Hello, world!";' - should have string highlight
    if (styles.size() > 6) { // Check for index 6
        const auto& line6Styles = styles[6]; // Use index 6
        bool foundString = false;
        for (const auto& style : line6Styles) {
            if (style.color == SyntaxColor::String) {
                foundString = true;
                break;
            }
        }
        assert(foundString);
    } else {
        assert(false && "styles array does not contain enough elements for line 6 (index 6)");
    }
    
    // Line 6 in test logic (actual line 7 in buffer): "if (x > 0) {" - should have keyword highlight
    if (styles.size() > 7) { // Check for index 7
        const auto& line7Styles = styles[7]; // Use index 7
        bool foundKeyword = false;
        for (const auto& style : line7Styles) {
            if (style.color == SyntaxColor::Keyword) {
                foundKeyword = true;
                break;
            }
        }
        assert(foundKeyword);
    } else {
        assert(false && "styles array does not contain enough elements for line 7 (index 7)");
    }
    
    std::cout << "C++ syntax highlighting test passed!" << std::endl;
}

// Test that editing a line invalidates the highlighting cache
void testHighlightingCacheInvalidation() {
    TestEditor editor;
    editor.setFilename("test.cpp");
    
    // Add a line that will be styled
    editor.addLine("int x = 42;"); // This is line 1 in buffer, line 0 is ""
    
    // Get initial highlighting for line 1
    auto initialHighlightVec = editor.getHighlightingStyles();
    assert(initialHighlightVec.size() > 1 && "Buffer should have at least 2 lines for this test");
    const auto& line1InitialStyles = initialHighlightVec[1];
    assert(!line1InitialStyles.empty() && "Line 'int x = 42;' should have styles initially");
    
    // Now modify line 1
    editor.replaceLine(1, "double y = 3.14;"); // Replace the content of line 1
    
    // Get updated highlighting for line 1
    auto updatedHighlightVec = editor.getHighlightingStyles();
    assert(updatedHighlightVec.size() > 1 && "Buffer should still have at least 2 lines");
    const auto& line1UpdatedStyles = updatedHighlightVec[1];
    assert(!line1UpdatedStyles.empty() && "Line 'double y = 3.14;' should have styles");
    
    // Check that the styles are different for line 1
    bool stylesChanged = false;
    if (line1InitialStyles.size() != line1UpdatedStyles.size()) {
        stylesChanged = true;
    } else {
        for (size_t i = 0; i < line1InitialStyles.size(); ++i) {
            if (line1InitialStyles[i].startCol != line1UpdatedStyles[i].startCol ||
                line1InitialStyles[i].endCol != line1UpdatedStyles[i].endCol ||
                line1InitialStyles[i].color != line1UpdatedStyles[i].color) {
                stylesChanged = true;
                break;
            }
        }
    }
    assert(stylesChanged && "Styles for the modified line should have changed");
    
    std::cout << "Syntax highlighting cache invalidation test passed!" << std::endl;
}

// Main function for running syntax highlighting tests
int main() {
    // Run the tests
    testEnableSyntaxHighlighting();
    testFilenameAndHighlighterDetection();
    testCppSyntaxHighlighting();
    testHighlightingCacheInvalidation();
    
    std::cout << "All syntax highlighting tests passed!" << std::endl;
    return 0;
} 