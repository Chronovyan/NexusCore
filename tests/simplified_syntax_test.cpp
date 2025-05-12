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
class SimplifiedSyntaxHighlighterInterfaceTestVersion : public SyntaxHighlighter {
public:
    virtual ~SimplifiedSyntaxHighlighterInterfaceTestVersion() = default;
    // Using SyntaxStyle and SyntaxColor from SyntaxHighlighter.h (from ../src/SyntaxHighlighter.h)
    virtual std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string& line, size_t lineIndex) const override = 0;
    // getLanguageName is already in SyntaxHighlighter base class
    // virtual std::string getLanguageName() const = 0; 

    // Provide empty implementations for other pure virtuals from SyntaxHighlighter base if not testing them here
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer& buffer) const override {
        std::vector<std::vector<SyntaxStyle>> allStyles;
        for (size_t i = 0; i < buffer.lineCount(); ++i) {
            auto lineStyles = highlightLine(buffer.getLine(i), i);
            if (lineStyles) {
                allStyles.push_back(std::move(*lineStyles));
            } else {
                allStyles.push_back({});
            }
        }
        return allStyles;
    }
    std::vector<std::string> getSupportedExtensions() const override {
        return { ".simpl" }; // Example
    }
};

// Simple C++ syntax highlighter (could be moved to a shared test utility header)
class SimplifiedCppHighlighterTestVersion : public SimplifiedSyntaxHighlighterInterfaceTestVersion {
public:
    SimplifiedCppHighlighterTestVersion() {}

    std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string& line, [[maybe_unused]] size_t lineIndex) const override {
        auto styles = std::make_unique<std::vector<SyntaxStyle>>();
        // Simplified highlighting logic

        if (line.rfind("#include", 0) == 0) {
            styles->push_back(SyntaxStyle(0, 8, SyntaxColor::Preprocessor));
        }

        size_t pos;
        if ((pos = line.find("int ")) != std::string::npos) {
            styles->push_back(SyntaxStyle(pos, pos + 3, SyntaxColor::Type));
        }
        if ((pos = line.find("return")) != std::string::npos) {
            styles->push_back(SyntaxStyle(pos, pos + 6, SyntaxColor::Keyword));
        }
        if ((pos = line.find("main")) != std::string::npos) {
            styles->push_back(SyntaxStyle(pos, pos + 4, SyntaxColor::Function));
        }

        size_t stringStart = line.find("\"");
        if (stringStart != std::string::npos) {
            size_t stringEnd = line.find("\"", stringStart + 1);
            if (stringEnd != std::string::npos) {
                styles->push_back(SyntaxStyle(stringStart, stringEnd + 1, SyntaxColor::String));
            }
        }
        return styles;
    }

    std::string getLanguageName() const override {
        return "C++ (Simplified Test Version)";
    }
};

// --- Google Test Cases ---

TEST(SimplifiedSyntaxHighlightingTest, PreprocessorDirective) {
    TextBuffer buffer;
    buffer.addLine("#include <iostream>");
    SimplifiedCppHighlighterTestVersion highlighter;
    auto stylesPtr = highlighter.highlightLine(buffer.getLine(0), 0);
    ASSERT_TRUE(stylesPtr != nullptr);
    const auto& styles = *stylesPtr;

    bool foundPreprocessor = false;
    for (const auto& style : styles) {
        if (style.color == SyntaxColor::Preprocessor && style.startCol == 0 && style.endCol == 8) {
            foundPreprocessor = true;
            break;
        }
    }
    ASSERT_TRUE(foundPreprocessor) << "Preprocessor directive #include was not highlighted correctly.";
}

TEST(SimplifiedSyntaxHighlightingTest, StringLiteral) {
    TextBuffer buffer;
    buffer.addLine("    std::cout << \"Hello World\" << std::endl;");
    SimplifiedCppHighlighterTestVersion highlighter;
    auto stylesPtr = highlighter.highlightLine(buffer.getLine(0), 0);
    ASSERT_TRUE(stylesPtr != nullptr);
    const auto& styles = *stylesPtr;
    
    bool foundString = false;
    for (const auto& style : styles) {
        if (style.color == SyntaxColor::String && style.startCol == 17 && style.endCol == 30) { // "Hello World"
            foundString = true;
            break;
        }
    }
    ASSERT_TRUE(foundString) << "String literal \"Hello World\" was not highlighted correctly.";
}

TEST(SimplifiedSyntaxHighlightingTest, KeywordAndType) {
    TextBuffer buffer;
    buffer.addLine("int main() { return 0; }");
    SimplifiedCppHighlighterTestVersion highlighter;
    auto stylesPtr = highlighter.highlightLine(buffer.getLine(0), 0);
    ASSERT_TRUE(stylesPtr != nullptr);
    const auto& styles = *stylesPtr;

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
