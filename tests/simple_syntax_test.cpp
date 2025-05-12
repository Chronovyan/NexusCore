#include <iostream>
#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include "gtest/gtest.h" // Include for Google Test macros
#include "../src/SyntaxHighlighter.h" // Provides SyntaxColor, SyntaxStyle
#include "../src/TextBuffer.h"
#include "../src/EditorError.h" // Includes SyntaxHighlighter.h

// Simple syntax highlighter interface (could be moved to a shared test utility header if used elsewhere)
class SimpleSyntaxHighlighter {
public:
    virtual ~SimpleSyntaxHighlighter() = default;
    // Using SyntaxStyle and SyntaxColor from SyntaxHighlighter.h
    virtual std::vector<SyntaxStyle> highlightLine(const std::string& line, size_t lineIndex) const = 0;
    virtual std::string getLanguageName() const = 0;
};

// Simple C++ syntax highlighter (could be moved to a shared test utility header)
class SimpleCppHighlighter : public SimpleSyntaxHighlighter {
public:
    SimpleCppHighlighter() {}

    std::vector<SyntaxStyle> highlightLine(const std::string& line, [[maybe_unused]] size_t lineIndex) const override {
        std::vector<SyntaxStyle> styles;
        // Simplified highlighting logic

        if (line.rfind("#include", 0) == 0) {
            styles.push_back(SyntaxStyle(0, 8, SyntaxColor::Preprocessor));
        }

        size_t pos;
        if ((pos = line.find("int ")) != std::string::npos) {
            styles.push_back(SyntaxStyle(pos, pos + 3, SyntaxColor::Type));
        }
        if ((pos = line.find("return")) != std::string::npos) {
            styles.push_back(SyntaxStyle(pos, pos + 6, SyntaxColor::Keyword));
        }
        if ((pos = line.find("main")) != std::string::npos) {
            styles.push_back(SyntaxStyle(pos, pos + 4, SyntaxColor::Function));
        }

        size_t stringStart = line.find("\"");
        if (stringStart != std::string::npos) {
            size_t stringEnd = line.find("\"", stringStart + 1);
            if (stringEnd != std::string::npos) {
                styles.push_back(SyntaxStyle(stringStart, stringEnd + 1, SyntaxColor::String));
            }
        }
        return styles;
    }

    std::string getLanguageName() const override {
        return "C++ (Simple)";
    }
};

// --- Google Test Cases ---

TEST(SimpleSyntaxHighlightingTest, PreprocessorDirective) {
    TextBuffer buffer;
    buffer.addLine("#include <iostream>");
    SimpleCppHighlighter highlighter;
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(buffer.getLine(0), 0);

    bool foundPreprocessor = false;
    for (const auto& style : styles) {
        if (style.color == SyntaxColor::Preprocessor && style.startCol == 0 && style.endCol == 8) {
            foundPreprocessor = true;
            break;
        }
    }
    ASSERT_TRUE(foundPreprocessor) << "Preprocessor directive #include was not highlighted correctly.";
}

TEST(SimpleSyntaxHighlightingTest, StringLiteral) {
    TextBuffer buffer;
    buffer.addLine("    std::cout << \"Hello World\" << std::endl;");
    SimpleCppHighlighter highlighter;
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(buffer.getLine(0), 0);
    
    bool foundString = false;
    for (const auto& style : styles) {
        if (style.color == SyntaxColor::String && style.startCol == 17 && style.endCol == 30) { // "Hello World"
            foundString = true;
            break;
        }
    }
    ASSERT_TRUE(foundString) << "String literal \"Hello World\" was not highlighted correctly.";
}

TEST(SimpleSyntaxHighlightingTest, KeywordAndType) {
    TextBuffer buffer;
    buffer.addLine("int main() { return 0; }");
    SimpleCppHighlighter highlighter;
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(buffer.getLine(0), 0);

    bool foundInt = false;
    bool foundReturn = false;

    for (const auto& style : styles) {
        if (style.color == SyntaxColor::Type && style.startCol == 0 && style.endCol == 3) { // "int"
            foundInt = true;
        }
        if (style.color == SyntaxColor::Keyword && style.startCol == 13 && style.endCol == 19) { // "return"
            foundReturn = true;
        }
    }
    ASSERT_TRUE(foundInt) << "Type 'int' was not highlighted correctly.";
    ASSERT_TRUE(foundReturn) << "Keyword 'return' was not highlighted correctly.";
}
