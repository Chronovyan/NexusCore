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
    
    // Line 0: "#include <iostream>" - should have preprocessor highlight
    const auto& line0Styles = styles[0];
    bool foundPreprocessor = false;
    for (const auto& style : line0Styles) {
        if (style.color == SyntaxColor::Preprocessor) {
            foundPreprocessor = true;
            break;
        }
    }
    assert(foundPreprocessor);
    
    // Line 2: "int main() {" - should have type and function highlights
    const auto& line2Styles = styles[2];
    bool foundType = false;
    bool foundFunction = false;
    for (const auto& style : line2Styles) {
        if (style.color == SyntaxColor::Type) {
            foundType = true;
        }
        if (style.color == SyntaxColor::Function) {
            foundFunction = true;
        }
    }
    assert(foundType);
    assert(foundFunction);
    
    // Line 3: "// This is a comment" - should have comment highlight
    const auto& line3Styles = styles[3];
    bool foundComment = false;
    for (const auto& style : line3Styles) {
        if (style.color == SyntaxColor::Comment) {
            foundComment = true;
            break;
        }
    }
    assert(foundComment);
    
    // Line 4: "int x = 42;" - should have type and number highlights
    const auto& line4Styles = styles[4];
    bool foundNumber = false;
    foundType = false;
    for (const auto& style : line4Styles) {
        if (style.color == SyntaxColor::Type) {
            foundType = true;
        }
        if (style.color == SyntaxColor::Number) {
            foundNumber = true;
        }
    }
    assert(foundType);
    assert(foundNumber);
    
    // Line 5: 'std::string text = "Hello, world!";' - should have string highlight
    const auto& line5Styles = styles[5];
    bool foundString = false;
    for (const auto& style : line5Styles) {
        if (style.color == SyntaxColor::String) {
            foundString = true;
            break;
        }
    }
    assert(foundString);
    
    // Line 6: "if (x > 0) {" - should have keyword highlight
    const auto& line6Styles = styles[6];
    bool foundKeyword = false;
    for (const auto& style : line6Styles) {
        if (style.color == SyntaxColor::Keyword) {
            foundKeyword = true;
            break;
        }
    }
    assert(foundKeyword);
    
    std::cout << "C++ syntax highlighting test passed!" << std::endl;
}

// Test that editing a line invalidates the highlighting cache
void testHighlightingCacheInvalidation() {
    TestEditor editor;
    editor.setFilename("test.cpp");
    
    // Add some C++ code
    editor.addLine("int x = 42;");
    
    // Get initial highlighting
    auto initialStyles = editor.getHighlightingStyles();
    
    // Now modify the line
    editor.replaceLine(0, "double y = 3.14;");
    
    // Get updated highlighting 
    auto updatedStyles = editor.getHighlightingStyles();
    
    // The highlighting should be different
    assert(initialStyles[0].size() != 0);
    assert(updatedStyles[0].size() != 0);
    
    // Check that the styles are different (this is a bit simplistic)
    bool stylesChanged = false;
    if (initialStyles[0].size() != updatedStyles[0].size()) {
        stylesChanged = true;
    } else {
        for (size_t i = 0; i < initialStyles[0].size(); ++i) {
            if (initialStyles[0][i].startCol != updatedStyles[0][i].startCol ||
                initialStyles[0][i].endCol != updatedStyles[0][i].endCol ||
                initialStyles[0][i].color != updatedStyles[0][i].color) {
                stylesChanged = true;
                break;
            }
        }
    }
    
    assert(stylesChanged);
    
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