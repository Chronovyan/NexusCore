#include "gtest/gtest.h"
#include "../src/TextBuffer.h"
#include <string>
#include <memory>
#include <vector>

class TextBufferEncodingTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<TextBuffer>();
    }
    
    // Helper function to insert text that might contain newlines
    void insertText(const std::string& text) {
        // Split the text into lines and add them
        size_t start = 0;
        size_t end = text.find('\n');
        bool first = true;
        
        while (end != std::string::npos) {
            std::string line = text.substr(start, end - start);
            if (first) {
                buffer->setLine(0, line);
                first = false;
            } else {
                buffer->addLine(line);
            }
            start = end + 1;
            end = text.find('\n', start);
        }
        // Add the last line if there's no trailing newline
        if (start < text.length()) {
            if (first) {
                buffer->setLine(0, text.substr(start));
            } else {
                buffer->addLine(text.substr(start));
            }
        }
    }
    
    // Helper function to get text content
    std::string getText() const {
        return getBufferContent();
    }
    
    // Helper function to get text content as a single string
    std::string getBufferContent() const {
        std::string content;
        for (size_t i = 0; i < buffer->lineCount(); ++i) {
            if (i > 0) content += "\n";
            content += buffer->getLine(i);
        }
        return content;
    }
    
    void TearDown() override {
        buffer.reset();
    }
    
    std::unique_ptr<TextBuffer> buffer;
};

TEST_F(TextBufferEncodingTest, HandlesBasicUTF8) {
    // Test basic ASCII characters
    insertText("Hello, World!");
    EXPECT_EQ(getText(), "Hello, World!");
    
    // Test non-ASCII characters
    buffer->clear(true);
    insertText("Привет, мир!");  // Russian
    EXPECT_EQ(getText(), "Привет, мир!");
    
    buffer->clear(true);
    insertText("こんにちは世界");  // Japanese
    EXPECT_EQ(getText(), "こんにちは世界");
    
    buffer->clear(true);
    insertText("مرحبا بالعالم");  // Arabic
    EXPECT_EQ(getText(), "مرحبا بالعالم");
}

TEST_F(TextBufferEncodingTest, HandlesMixedEncoding) {
    // Test mixing different scripts
    std::string mixed = "English 中文 русский 日本語 العربية";
    insertText(mixed);
    EXPECT_EQ(getText(), mixed);
    
    // Test with emojis and special characters
    buffer->clear(true);
    std::string withEmoji = "Test 😊 emoji 测试 🚀";
    insertText(withEmoji);
    EXPECT_EQ(getText(), withEmoji);
}

TEST_F(TextBufferEncodingTest, HandlesSurrogatePairs) {
    // Test characters outside BMP (Basic Multilingual Plane)
    // These require surrogate pairs in UTF-16
    insertText("😊");  // Smiling face emoji
    EXPECT_EQ(getText(), "😊");
    
    // Test multiple surrogate pairs
    buffer->clear(true);
    insertText("😊😊😊");
    EXPECT_EQ(getText(), "😊😊😊");
}

TEST_F(TextBufferEncodingTest, HandlesInvalidUTF8) {
    // Test with invalid UTF-8 sequence
    buffer->clear(true);
    // This is an invalid UTF-8 sequence (starts with 0x80 which is a continuation byte)
    std::string invalid = "abc\x80\xff\xfe\xfd";
    insertText(invalid);
    // The buffer should handle this gracefully, but we don't validate UTF-8
    EXPECT_EQ(getText(), invalid);
    
    // The behavior here depends on how you want to handle invalid UTF-8
    // Either way, it shouldn't crash
    std::string result = getText();
    EXPECT_FALSE(result.empty());
}

TEST_F(TextBufferEncodingTest, PreservesLineEndings) {
    // Test different line endings
    std::string lf = "Line1\nLine2";
    std::string crlf = "Line1\r\nLine2";
    std::string cr = "Line1\rLine2";
    
    // Test LF line endings
    insertText(lf);
    EXPECT_EQ(getText(), "Line1\nLine2");
    
    // Test CRLF line endings (will be converted to LF)
    buffer->clear(true);
    insertText(crlf);
    EXPECT_EQ(getText(), "Line1\nLine2");
    
    // Test CR line endings (will be converted to LF)
    buffer->clear(true);
    insertText(cr);
    EXPECT_EQ(getText(), "Line1\nLine2");
}

TEST_F(TextBufferEncodingTest, HandlesZeroWidthCharacters) {
    // Test with zero-width spaces and joiners
    std::string withZwsp = "Zero\u200BWidth\u200CJoiner";
    insertText(withZwsp);
    EXPECT_EQ(getText(), withZwsp);
    
    // Test with combining characters
    std::string combining = "A\u0301";  // A with acute accent
    buffer->clear(true);
    insertText(combining);
    EXPECT_EQ(getText(), combining);
}

TEST_F(TextBufferEncodingTest, HandlesBidirectionalText) {
    // Test with mixed RTL and LTR text
    std::string bidi = "English עברית العربية";
    insertText(bidi);
    EXPECT_EQ(getText(), bidi);
    
    // Test with RTL text containing numbers (which are LTR)
    std::string bidiWithNumbers = "עברית 123 עברית";
    buffer->clear(true);
    insertText(bidiWithNumbers);
    EXPECT_EQ(getText(), bidiWithNumbers);
}

TEST_F(TextBufferEncodingTest, HandlesLargeUnicodeStrings) {
    // Test with a large string containing various Unicode characters
    std::string largeText;
    const int count = 1000;
    
    // Add characters from different Unicode blocks
    for (int i = 0; i < count; i++) {
        switch (i % 4) {
            case 0: largeText += "A"; break;  // ASCII
            case 1: largeText += "α"; break;  // Greek
            case 2: largeText += "あ"; break; // Hiragana
            case 3: largeText += "😊"; break; // Emoji
        }
    }
    
    insertText(largeText);
    std::string result = getText();
    
    // Basic sanity checks
    EXPECT_EQ(result.length(), largeText.length());
    EXPECT_EQ(result, largeText);
}

TEST_F(TextBufferEncodingTest, HandlesUnicodeNormalization) {
    // Test that the buffer preserves Unicode normalization forms
    // Note: This test assumes the buffer doesn't normalize internally
    // If it does, we should test that normalization is consistent
    
    // Same character represented in different ways
    std::string nfc = "é";           // Single codepoint
    std::string nfd = "e\u0301";     // e + combining acute accent
    
    insertText(nfc);
    std::string result1 = getText();
    
    buffer->clear(true);
    insertText(nfd);
    std::string result2 = getText();
    
    // They should be preserved as-is
    EXPECT_EQ(result1, nfc);
    EXPECT_EQ(result2, nfd);
    
    // Length should be different
    EXPECT_NE(result1.length(), result2.length());
}

TEST_F(TextBufferEncodingTest, HandlesMixedLineEndings) {
    // Test with mixed line endings in the same text
    std::string mixed = "Line1\nLine2\r\nLine3\rLine4";
    insertText(mixed);
    
    // Should convert all line endings to LF
    std::string result = getText();
    EXPECT_EQ(result, "Line1\nLine2\nLine3\nLine4");
    
    // Test line counting - should be 4 lines
    EXPECT_EQ(buffer->lineCount(), 4);
}

TEST_F(TextBufferEncodingTest, HandlesUnicodeWhitespace) {
    // Test with various Unicode whitespace characters
    std::string spaces = "Regular space\u00A0Non-breaking space\u2003Em space\u3000Ideographic space";
    insertText(spaces);
    EXPECT_EQ(getText(), spaces);
    
    // Note: Cursor movement tests are not applicable to TextBuffer's interface
    // due to differences in cursor position handling
}

// Add more tests for other encoding-related functionality as needed
