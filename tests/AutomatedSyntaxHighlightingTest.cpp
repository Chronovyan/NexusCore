#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include "../src/SyntaxHighlighter.h"
#include "../src/Editor.h"
#include "../src/SyntaxHighlightingManager.h"
#include "EditorTestable.h"

class AutomatedSyntaxHighlightingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }
    
    void TearDown() override {
        // Common cleanup for all tests
    }
};

/**
 * Test enabling and disabling syntax highlighting
 */
TEST_F(AutomatedSyntaxHighlightingTest, EnableDisableHighlighting) {
    TestEditor editor;
    
    // Check default state
    ASSERT_FALSE(editor.isSyntaxHighlightingEnabled()) << "Syntax highlighting should be disabled by default";
    
    // Enable highlighting
    editor.enableSyntaxHighlighting(true);
    ASSERT_TRUE(editor.isSyntaxHighlightingEnabled()) << "Syntax highlighting should be enabled after calling enableSyntaxHighlighting(true)";
    
    // Disable highlighting
    editor.enableSyntaxHighlighting(false);
    ASSERT_FALSE(editor.isSyntaxHighlightingEnabled()) << "Syntax highlighting should be disabled after calling enableSyntaxHighlighting(false)";
}

/**
 * Test filename detection and highlighter selection
 */
TEST_F(AutomatedSyntaxHighlightingTest, FilenameAndHighlighterDetection) {
    TestEditor editor;
    
    // Enable syntax highlighting
    editor.enableSyntaxHighlighting(true);
    
    // Get the default filename (might be "untitled.txt" or similar)
    std::string defaultFilename = editor.getFilename();
    ASSERT_FALSE(defaultFilename.empty()) << "Default filename should not be empty";
    
    // Set C++ filename
    editor.setFilename("test.cpp");
    ASSERT_EQ(editor.getFilename(), "test.cpp") << "Filename should be set correctly";
    std::shared_ptr<SyntaxHighlighter> highlighter = editor.getCurrentHighlighter();
    ASSERT_NE(highlighter, nullptr) << "Highlighter should be set for .cpp extension";
    
    // Set text filename (typically no highlighter)
    editor.setFilename("test.txt");
    std::shared_ptr<SyntaxHighlighter> textHighlighter = editor.getCurrentHighlighter();
    
    // Set C++ filename again to ensure highlighter is reset
    editor.setFilename("test.cpp");
    highlighter = editor.getCurrentHighlighter();
    ASSERT_NE(highlighter, nullptr) << "Highlighter should be set for .cpp extension";
    
    // With highlighting disabled, we should get no highlighter regardless of extension
    editor.enableSyntaxHighlighting(false);
    editor.setFilename("test.cpp");
    ASSERT_EQ(editor.getCurrentHighlighter(), nullptr) << "Highlighter should be null when highlighting is disabled";
}

/**
 * Test C++ syntax highlighting
 */
TEST_F(AutomatedSyntaxHighlightingTest, CppSyntaxHighlighting) {
    TestEditor editor;
    
    // Setup
    editor.enableSyntaxHighlighting(true);
    editor.setFilename("test.cpp");
    
    // Add C++ code
    editor.addLine("#include <iostream>");
    editor.addLine("// This is a comment");
    editor.addLine("int main() {");
    editor.addLine("    std::string test = \"Hello World\";");
    editor.addLine("    return 0;");
    editor.addLine("}");
    
    // Get the number of lines in the buffer
    size_t lineCount = editor.getBuffer().lineCount();
    
    // Verify highlighting is applied
    auto styles = editor.getHighlightingStyles();
    ASSERT_EQ(styles.size(), lineCount) << "There should be styles for all " << lineCount << " lines";
    
    // Verify we have some non-empty style entries
    bool hasStyles = false;
    for (const auto& lineStyles : styles) {
        if (!lineStyles.empty()) {
            hasStyles = true;
            break;
        }
    }
    ASSERT_TRUE(hasStyles) << "At least one line should have syntax styles";
}

/**
 * Test highlighting cache invalidation
 */
TEST_F(AutomatedSyntaxHighlightingTest, HighlightingCacheInvalidation) {
    TestEditor editor;
    
    // Setup
    editor.enableSyntaxHighlighting(true);
    editor.setFilename("test.cpp");
    
    // Add a line and get styles
    editor.addLine("int x = 10;");
    auto initialStyles = editor.getHighlightingStyles();
    
    // Edit the line to invalidate cache
    editor.setCursor(0, 0);
    editor.typeText("//");
    auto updatedStyles = editor.getHighlightingStyles();
    
    // We can't directly compare the vectors of SyntaxStyle objects, so check for differences in other ways
    bool stylesChanged = false;
    
    // Check if both have styles
    if (initialStyles.size() > 0 && updatedStyles.size() > 0) {
        // Check if the number of style segments changed (most likely scenario)
        if (initialStyles[0].size() != updatedStyles[0].size()) {
            stylesChanged = true;
        } else if (!initialStyles[0].empty() && !updatedStyles[0].empty()) {
            // Check if the first style segment's color changed
            stylesChanged = (initialStyles[0][0].color != updatedStyles[0][0].color);
        }
    }
    
    ASSERT_TRUE(stylesChanged) << "Styles should change after editing";
}

/**
 * Test different file types
 */
TEST_F(AutomatedSyntaxHighlightingTest, DifferentFileTypes) {
    TestEditor editor;
    editor.enableSyntaxHighlighting(true);
    
    // Test supported extensions
    std::vector<std::string> supportedExtensions = {".cpp", ".h", ".hpp"};
    for (const auto& ext : supportedExtensions) {
        editor.setFilename("test" + ext);
        ASSERT_NE(editor.getCurrentHighlighter(), nullptr) 
            << ext << " should have a highlighter";
    }
    
    // Test unsupported extensions
    std::vector<std::string> unsupportedExtensions = {".xyz", ".abc", ".123"};
    for (const auto& ext : unsupportedExtensions) {
        editor.setFilename("test" + ext);
        ASSERT_EQ(editor.getCurrentHighlighter(), nullptr) 
            << ext << " should not have a highlighter";
    }
}

/**
 * Test rendering output with highlighting
 */
TEST_F(AutomatedSyntaxHighlightingTest, RenderingWithHighlighting) {
    TestEditor editor;
    
    // Setup with highlighting enabled
    editor.enableSyntaxHighlighting(true);
    editor.setFilename("test.cpp");
    
    // Add C++ code
    editor.addLine("#include <iostream>");
    editor.addLine("int main() {");
    editor.addLine("    return 0;");
    editor.addLine("}");
    
    // Get rendered output with highlighting
    auto styledOutput = editor.renderBuffer();
    
    // Disable highlighting
    editor.enableSyntaxHighlighting(false);
    
    // Get rendered output without highlighting
    auto plainOutput = editor.renderBuffer();
    
    // The outputs should be different
    ASSERT_NE(styledOutput, plainOutput) << "Styled and plain outputs should differ";
} 