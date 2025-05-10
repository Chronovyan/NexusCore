#include "gtest/gtest.h"
#include "gmock/gmock.h" // Added for GMock utilities
#include "SyntaxHighlightingManager.h" // Adjust path as needed
#include "SyntaxHighlighter.h"       // For SyntaxHighlighter interface and mocks
#include "TextBuffer.h"              // For TextBuffer
#include "EditorError.h"             // For ErrorReporter (to verify logging context if possible)
#include <memory>
#include <iostream> // Added for std::cout

// Mock SyntaxHighlighter that can be configured to throw
class MockSyntaxHighlighter : public SyntaxHighlighter {
public:
    MOCK_CONST_METHOD2(highlightLine, std::vector<SyntaxStyle>(const std::string& line, size_t lineIndex));
    MOCK_CONST_METHOD1(highlightBuffer, std::vector<std::vector<SyntaxStyle>>(const TextBuffer& buffer));
    MOCK_CONST_METHOD0(getSupportedExtensions, std::vector<std::string>());
    MOCK_CONST_METHOD0(getLanguageName, std::string());

    void setThrowOnHighlightLine(bool should_throw, const std::string& exception_message = "Test Exception") {
        throw_on_highlight_line_ = should_throw;
        exception_message_ = exception_message;
        if (should_throw) {
            ON_CALL(*this, highlightLine)
                .WillByDefault(testing::Throw(std::runtime_error(exception_message_)));
        } else {
            ON_CALL(*this, highlightLine)
                .WillByDefault(testing::Return(std::vector<SyntaxStyle>{})); // Default non-throwing behavior
        }
    }

private:
    bool throw_on_highlight_line_ = false;
    std::string exception_message_;
};

class SyntaxHighlightingManagerTest : public ::testing::Test {
protected:
    SyntaxHighlightingManager manager_;
    std::shared_ptr<testing::NiceMock<MockSyntaxHighlighter>> mock_highlighter_;
    TextBuffer text_buffer_;

    void SetUp() override {
        std::cout << "[DEBUG] SyntaxHighlightingManagerTest::SetUp() - Start" << std::endl;
        mock_highlighter_ = std::make_shared<testing::NiceMock<MockSyntaxHighlighter>>();
        manager_.setHighlighter(mock_highlighter_);
        text_buffer_.addLine("Hello World");
        text_buffer_.addLine("Test line for highlighting");
        manager_.setBuffer(&text_buffer_);
        manager_.setEnabled(true);
        std::cout << "[DEBUG] SyntaxHighlightingManagerTest::SetUp() - End" << std::endl;
    }

    void TearDown() override {
        std::cout << "[DEBUG] SyntaxHighlightingManagerTest::TearDown() - Start" << std::endl;
        ON_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
            .WillByDefault(testing::Return(std::vector<SyntaxStyle>{})); // Reset to default non-throwing behavior
        manager_.setHighlighter(nullptr);
        manager_.setBuffer(nullptr); // Also null out buffer
        mock_highlighter_.reset(); // Release the mock
        std::cout << "[DEBUG] SyntaxHighlightingManagerTest::TearDown() - Mock highlighter reset and detached." << std::endl;
        std::cout << "[DEBUG] SyntaxHighlightingManagerTest::TearDown() - End" << std::endl;
    }
};

TEST_F(SyntaxHighlightingManagerTest, HighlightLineCatchesExceptionFromHighlighter) {
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, HighlightLineCatchesExceptionFromHighlighter) - Start" << std::endl;
    mock_highlighter_->setThrowOnHighlightLine(true, "Highlighter failed!");

    // The manager's highlightLine method is private, it's called internally by getHighlightingStyles.
    // We expect getHighlightingStyles to catch the exception from the mock_highlighter_,
    // log it (implicitly via ErrorReporter), and return empty styles for the problematic line.
    
    // Invalidate to ensure highlighting is attempted
    manager_.invalidateLine(0);
    std::vector<std::vector<SyntaxStyle>> styles = manager_.getHighlightingStyles(0, 0);

    ASSERT_EQ(styles.size(), 1);
    EXPECT_TRUE(styles[0].empty()); // Expect empty styles due to exception

    // Further testing could involve redirecting stderr to check for ErrorReporter output,
    // but that's more complex. For now, we verify graceful handling.
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, HighlightLineCatchesExceptionFromHighlighter) - End" << std::endl;
}

TEST_F(SyntaxHighlightingManagerTest, GetHighlightingStylesReturnsEmptyWhenHighlighterThrows) {
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, GetHighlightingStylesReturnsEmptyWhenHighlighterThrows) - Start" << std::endl;
    // Configure the mock highlighter to throw an exception when highlightLine is called.
    ON_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .WillByDefault(testing::Throw(std::runtime_error("Mock highlighter failed")));

    manager_.setHighlighter(mock_highlighter_);
    manager_.setBuffer(&text_buffer_);
    manager_.setEnabled(true);
    manager_.invalidateAllLines(); // Ensure highlighting is attempted

    std::vector<std::vector<SyntaxStyle>> result;
    // We expect this call not to throw an exception itself, but to handle the one from the mock.
    ASSERT_NO_THROW({
        result = manager_.getHighlightingStyles(0, text_buffer_.lineCount() - 1);
    });

    // Check that the result contains empty styles for all lines due to the error.
    ASSERT_EQ(result.size(), text_buffer_.lineCount());
    for (const auto& line_styles : result) {
        EXPECT_TRUE(line_styles.empty());
    }
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, GetHighlightingStylesReturnsEmptyWhenHighlighterThrows) - End" << std::endl;
}

TEST_F(SyntaxHighlightingManagerTest, SetHighlighterHandlesNull) {
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, SetHighlighterHandlesNull) - Start" << std::endl;
    ASSERT_NO_THROW(manager_.setHighlighter(nullptr));
    // After setting a null highlighter, getHighlightingStyles should return empty styles
    // without attempting to call the highlighter.
    auto styles = manager_.getHighlightingStyles(0,0);
    ASSERT_EQ(styles.size(), 1);
    EXPECT_TRUE(styles[0].empty());
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, SetHighlighterHandlesNull) - End" << std::endl;
}


// TODO: Add tests for SyntaxHighlighterRegistry error handling
// For example, what happens if a highlighter passed to registerHighlighter
// throws an exception during getSupportedExtensions?
// What happens if getHighlighterForExtension encounters an issue? (less likely with current logic) 