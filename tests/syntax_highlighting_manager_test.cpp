#include "gtest/gtest.h"
#include "gmock/gmock.h" // Added for GMock utilities
#include "SyntaxHighlightingManager.h" // Adjust path as needed
#include "SyntaxHighlighter.h"       // For SyntaxHighlighter interface and mocks
#include "TextBuffer.h"              // For TextBuffer
#include "EditorError.h"             // For ErrorReporter (to verify logging context if possible)
#include <memory>
#include <chrono>
#include <thread>
#include <iostream> // Added for std::cout
#include <vector>
#include <atomic>
#include <algorithm>
#include <future>
#include <random>

// Custom action for returning a unique_ptr<vector<SyntaxStyle>>
ACTION_P(ReturnStyleVector, style) {
    auto result = std::make_unique<std::vector<SyntaxStyle>>();
    result->push_back(style);
    return result;
}

// Mock SyntaxHighlighter that can be configured to throw
class MockSyntaxHighlighter : public SyntaxHighlighter {
public:
    MockSyntaxHighlighter() {
        // Set up default behavior
        ON_CALL(*this, highlightLine(testing::_, testing::_))
            .WillByDefault([this](const std::string& line, size_t lineIndex) -> std::unique_ptr<std::vector<SyntaxStyle>> {
                try {
                    if (throw_on_highlight_line_) {
                        // Use logic_error which doesn't need heap allocation
                        throw std::logic_error(exception_message_);
                    }
                    
                    // Generate a simple style vector based on line content for testing
                    auto styles = std::make_unique<std::vector<SyntaxStyle>>();
                    if (!line.empty()) {
                        // Add a simple style that covers the entire line
                        styles->push_back(SyntaxStyle(0, line.length(), SyntaxColor::Keyword));
                    }
                    // Return by moving the unique_ptr to avoid potential memory issues
                    return styles;
                } catch (const std::exception& e) {
                    // Rethrow with a clear message
                    throw std::logic_error(std::string("Mock exception: ") + e.what());
                } catch (...) {
                    // Convert unknown exceptions to standard ones
                    throw std::logic_error("Unknown exception in mock");
                }
            });
        
        ON_CALL(*this, getSupportedExtensions())
            .WillByDefault([]() {
                return std::vector<std::string>{".cpp", ".h"};
            });
        
        ON_CALL(*this, getLanguageName())
            .WillByDefault([]() {
                return "C++";
            });
            
        ON_CALL(*this, highlightBuffer(testing::_))
            .WillByDefault([this](const TextBuffer& buffer) -> std::vector<std::vector<SyntaxStyle>> {
                try {
                    // Create a vector of vector for each line
                    std::vector<std::vector<SyntaxStyle>> result;
                    result.reserve(buffer.lineCount()); // Pre-reserve to avoid reallocations
                    
                    for (size_t i = 0; i < buffer.lineCount(); ++i) {
                        const std::string& line = buffer.getLine(i);
                        std::vector<SyntaxStyle> lineStyles;
                        if (!line.empty() && !throw_on_highlight_line_) {
                            lineStyles.push_back(SyntaxStyle(0, line.length(), SyntaxColor::Keyword));
                        }
                        result.push_back(std::move(lineStyles)); // Move to avoid copies
                    }
                    return result;
                } catch (const std::exception& e) {
                    // Log the error and return an empty result
                    std::cerr << "Exception in highlightBuffer mock: " << e.what() << std::endl;
                    return std::vector<std::vector<SyntaxStyle>>();
                }
            });
    }

    MOCK_METHOD(std::unique_ptr<std::vector<SyntaxStyle>>, highlightLine, (const std::string& line, size_t lineIndex), (const, override));
    MOCK_METHOD(std::vector<std::vector<SyntaxStyle>>, highlightBuffer, (const TextBuffer& buffer), (const, override));
    MOCK_METHOD(std::vector<std::string>, getSupportedExtensions, (), (const, override));
    MOCK_METHOD(std::string, getLanguageName, (), (const, override));

    void setThrowOnHighlightLine(bool should_throw, const std::string& exception_message = "Test Exception") {
        throw_on_highlight_line_ = should_throw;
        exception_message_ = exception_message;
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
        
        try {
            // Create new buffer with known content
            text_buffer_ = TextBuffer(); // Reset the buffer
            // TextBuffer starts with an empty line at index 0
            text_buffer_.addLine("Line 1 content");
            text_buffer_.addLine("Line 2 content");
            
            // Create the mock with NiceMock to suppress irrelevant warnings
            mock_highlighter_ = std::make_shared<testing::NiceMock<MockSyntaxHighlighter>>();
            
            // Set up the test environment
            manager_.setHighlighter(mock_highlighter_);
            manager_.setBuffer(&text_buffer_);
            manager_.setEnabled(true);
            
            std::cout << "[DEBUG] SyntaxHighlightingManagerTest::SetUp() - End" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[ERROR] Exception in SetUp: " << e.what() << std::endl;
            throw; // Re-throw to let the test framework know setup failed
        }
    }

    void TearDown() override {
        std::cout << "[DEBUG] SyntaxHighlightingManagerTest::TearDown() - Start" << std::endl;
        
        try {
            // Clean up resources in reverse order of acquisition
            manager_.setHighlighter(nullptr);
            manager_.setBuffer(nullptr);
            mock_highlighter_.reset();
            
            std::cout << "[DEBUG] SyntaxHighlightingManagerTest::TearDown() - End" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[ERROR] Exception in TearDown: " << e.what() << std::endl;
            // We don't re-throw in TearDown as it would mask test failures
        }
    }
};

TEST_F(SyntaxHighlightingManagerTest, InitialStateIsEnabled) {
    // Verify default initial state is enabled
    EXPECT_TRUE(manager_.isEnabled());
}

TEST_F(SyntaxHighlightingManagerTest, EnableDisableToggleWorks) {
    // Initially the manager is enabled (from SetUp)
    EXPECT_TRUE(manager_.isEnabled());
    
    // Test disabling
    manager_.setEnabled(false);
    EXPECT_FALSE(manager_.isEnabled());
    
    // Validate that getHighlightingStyles returns empty when disabled
    auto styles = manager_.getHighlightingStyles(0, 0);
    EXPECT_EQ(styles.size(), 1);
    EXPECT_TRUE(styles[0].empty());
    
    // Re-enable and verify
    manager_.setEnabled(true);
    EXPECT_TRUE(manager_.isEnabled());
}

TEST_F(SyntaxHighlightingManagerTest, HighlightLineCatchesExceptionFromHighlighter) {
    // Create a non-mocked highlighter that will throw controlled exceptions
    class DirectExceptionHighlighter : public SyntaxHighlighter {
    public:
        std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string&, size_t) const override {
            // Always throw exception - no heap manipulation in the exception path
            throw std::runtime_error("Direct exception without mock framework"); 
        }
        
        std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer&) const override {
            throw std::runtime_error("Direct exception without mock framework");
        }
        
        std::vector<std::string> getSupportedExtensions() const override {
            return {".cpp"};
        }
        
        std::string getLanguageName() const override {
            return "DirectTest";
        }
    };
    
    // Create a separate SyntaxHighlightingManager instance to avoid any test fixture state issues
    SyntaxHighlightingManager localManager;
    TextBuffer localBuffer;
    
    // Add some content to the buffer
    localBuffer.addLine("Test content");
    
    // Set up a local highlighter that will throw
    auto directHighlighter = std::make_shared<DirectExceptionHighlighter>();
    
    // Apply the highlighter and buffer to the manager
    localManager.setBuffer(&localBuffer);
    localManager.setHighlighter(directHighlighter);
    localManager.setEnabled(true);
    
    // Test exception handling
    std::vector<std::vector<SyntaxStyle>> styles;
    ASSERT_NO_THROW({
        // Invalidate line 0 to ensure highlighting will be attempted
        localManager.invalidateLine(0);
        
        // Get styles which should trigger highlighting and catch the exception
        styles = localManager.getHighlightingStyles(0, 0);
    });
    
    // Verify result is valid but empty due to exception
    ASSERT_EQ(styles.size(), 1);
    EXPECT_TRUE(styles[0].empty());
    
    // Explicitly clean up resources
    localManager.setHighlighter(nullptr);
    localManager.setBuffer(nullptr);
}

TEST_F(SyntaxHighlightingManagerTest, GetHighlightingStylesReturnsEmptyWhenHighlighterThrows) {
    // Create a non-mocked highlighter that will throw controlled exceptions
    class DirectExceptionHighlighter2 : public SyntaxHighlighter {
    public:
        std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string&, size_t) const override {
            // Always throw exception - no heap manipulation in the exception path
            throw std::runtime_error("Direct exception without mock framework");
        }
        
        std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer&) const override {
            throw std::runtime_error("Direct exception without mock framework");
        }
        
        std::vector<std::string> getSupportedExtensions() const override {
            return {".cpp"};
        }
        
        std::string getLanguageName() const override {
            return "DirectTest2";
        }
    };
    
    // Create a separate SyntaxHighlightingManager instance to avoid any test fixture state issues
    SyntaxHighlightingManager localManager;
    TextBuffer localBuffer;
    
    // Add some content to the buffer
    localBuffer.addLine("Test content");
    localBuffer.addLine("More test content");
    
    // Set up a local highlighter that will throw
    auto directHighlighter = std::make_shared<DirectExceptionHighlighter2>();
    
    // Apply the highlighter and buffer to the manager
    localManager.setBuffer(&localBuffer);
    localManager.setHighlighter(directHighlighter);
    localManager.setEnabled(true);
    
    // Make sure all lines will be highlighted
    localManager.invalidateAllLines();
    
    // Test get styles with exception handling
    std::vector<std::vector<SyntaxStyle>> styles;
    ASSERT_NO_THROW({
        styles = localManager.getHighlightingStyles(0, 1);
    });
    
    // Verify result has right size but both lines are empty due to exception
    ASSERT_EQ(styles.size(), 2);
    EXPECT_TRUE(styles[0].empty());
    EXPECT_TRUE(styles[1].empty());
    
    // Explicitly clean up resources
    localManager.setHighlighter(nullptr);
    localManager.setBuffer(nullptr);
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

// Tests for Cache Logic
TEST_F(SyntaxHighlightingManagerTest, CacheHitsAfterHighlighting) {
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Set expectations with specific behavior
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3)  // Once for each line (lines 0, 1, and 2)
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    // Initial highlighting should call the highlighter for all three lines
    manager_.invalidateAllLines();
    auto styles1 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles1.size(), 3);
    
    // Second request without invalidation should use cache (no more calls to highlighter)
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(0); // Should not be called again if cache is used
    
    auto styles2 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles2.size(), 3);
}

TEST_F(SyntaxHighlightingManagerTest, CacheMissAfterInvalidateLine) {
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Initial highlighting to populate cache - expect all lines to be highlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3) // Once for each line
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    auto styles1 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles1.size(), 3);
    
    // Clear expectations and set up for next test phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Invalidate line 0 and expect only that line to be rehighlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 0))
        .Times(1)
        .WillOnce(ReturnStyleVector(testStyle));
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 1))
        .Times(0); // Line 1 should still be in cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 2))
        .Times(0); // Line 2 should still be in cache
    
    manager_.invalidateLine(0);
    auto styles2 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles2.size(), 3);
}

TEST_F(SyntaxHighlightingManagerTest, CacheMissAfterInvalidateLines) {
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Initial highlighting to populate cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3) // Once for each line
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    auto styles1 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles1.size(), 3);
    
    // Clear expectations and set up for next test phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Invalidate a range of lines and expect them to be rehighlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(2) // Lines 0 and 1 should be rehighlighted
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    manager_.invalidateLines(0, 1);
    auto styles2 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles2.size(), 3);
}

TEST_F(SyntaxHighlightingManagerTest, CacheMissAfterInvalidateAllLines) {
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Initial highlighting to populate cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3) // Once for each line
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    auto styles1 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles1.size(), 3);
    
    // Clear expectations and set up for next test phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Invalidate all lines and expect all to be rehighlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3) // All lines should be rehighlighted
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    manager_.invalidateAllLines();
    auto styles2 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles2.size(), 3);
}

TEST_F(SyntaxHighlightingManagerTest, DISABLED_CacheUpdatesAfterTimeout) {
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Initial highlighting to populate cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(2) // Once for each line
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    auto styles1 = manager_.getHighlightingStyles(0, 1);
    EXPECT_EQ(styles1.size(), 2);
    
    // Clear expectations and set up for next test phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Wait a bit longer than the cache entry lifetime
    // The SyntaxHighlightingManager header indicates CACHE_ENTRY_LIFETIME_MS = 10000 (10 seconds)
    // For testing purposes, we'll wait for just over that time
    std::this_thread::sleep_for(std::chrono::milliseconds(11000));
    
    // After cache timeout, expect lines to be rehighlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(2) // Both lines should be rehighlighted
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    auto styles2 = manager_.getHighlightingStyles(0, 1);
    EXPECT_EQ(styles2.size(), 2);
}

TEST_F(SyntaxHighlightingManagerTest, HighlightingTimeoutSettings) {
    // Verify default timeout setting
    EXPECT_EQ(manager_.getHighlightingTimeout(), SyntaxHighlightingManager::DEFAULT_HIGHLIGHTING_TIMEOUT_MS);
    
    // Set a custom timeout
    size_t customTimeout = 100;
    manager_.setHighlightingTimeout(customTimeout);
    EXPECT_EQ(manager_.getHighlightingTimeout(), customTimeout);
}

TEST_F(SyntaxHighlightingManagerTest, ContextLinesSettings) {
    // Verify default context lines setting
    EXPECT_EQ(manager_.getContextLines(), SyntaxHighlightingManager::DEFAULT_CONTEXT_LINES);
    
    // Set custom context lines
    size_t customContextLines = 50;
    manager_.setContextLines(customContextLines);
    EXPECT_EQ(manager_.getContextLines(), customContextLines);
}

// Tests for Visible Range functionality
TEST_F(SyntaxHighlightingManagerTest, VisibleRangeAffectsCacheLifetime) {
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Set a visible range and ensure it affects which lines are prioritized
    manager_.setVisibleRange(0, 0); // Only line 0 is visible
    
    // Initial highlighting for all lines
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3) // Once for each line
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    auto styles1 = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(styles1.size(), 3);
    
    // Clear expectations and set up for next test phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // After a cache cleanup, the non-visible lines might be cleaned up first
    // This is implementation-dependent, but we can verify the behavior
    
    // Request highlighting for just the visible line
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 0))
        .Times(0); // Line 0 should still be in cache as it's in the visible range
    
    auto styles2 = manager_.getHighlightingStyles(0, 0);
    EXPECT_EQ(styles2.size(), 1);
}

// Thread safety tests - basic verification
TEST_F(SyntaxHighlightingManagerTest, DISABLED_ConcurrentInvalidationsAreHandledSafely) {
    // This test verifies that concurrent invalidation operations don't cause crashes
    // or undefined behavior. We're not testing for specific thread interleavings,
    // just basic thread safety.
    
    // Create a style for testing
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // Set up default behavior for any highlighting that might happen
    ON_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .WillByDefault(ReturnStyleVector(testStyle));
    
    const int numThreads = 5;
    const int operationsPerThread = 10;
    
    std::vector<std::thread> threads;
    
    // Create threads that perform concurrent invalidations
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, operationsPerThread]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                // Mix of different invalidation operations
                switch (j % 3) {
                    case 0:
                        manager_.invalidateLine(0);
                        break;
                    case 1:
                        manager_.invalidateLines(0, 1);
                        break;
                    case 2:
                        manager_.invalidateAllLines();
                        break;
                }
                
                // Also mix in some highlighting requests
                if (j % 2 == 0) {
                    manager_.getHighlightingStyles(0, 1);
                }
                
                // Small sleep to increase chance of thread interleaving
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // If we got here without crashing or exceptions, basic thread safety is working
    SUCCEED() << "Concurrent operations completed without crashes or exceptions";
}

// TODO: Add tests for SyntaxHighlighterRegistry error handling
// For example, what happens if a highlighter passed to registerHighlighter
// throws an exception during getSupportedExtensions?
// What happens if getHighlighterForExtension encounters an issue? (less likely with current logic)

TEST_F(SyntaxHighlightingManagerTest, DebugSetUpBufferLineCount) {
    std::cout << "Buffer line count: " << text_buffer_.lineCount() << std::endl;
    for (size_t i = 0; i < text_buffer_.lineCount(); ++i) {
        std::cout << "Line " << i << ": \"" << text_buffer_.getLine(i) << "\"" << std::endl;
    }
    SUCCEED();
}

// Simple exception-throwing highlighter without using mocks
class ExceptionThrowingHighlighter : public SyntaxHighlighter {
public:
    ExceptionThrowingHighlighter() = default;
    
    std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(
        const std::string&, size_t) const override {
        // Use logic_error to avoid heap allocation during exception
        throw std::logic_error("Intentional exception from ExceptionThrowingHighlighter");
    }
    
    std::vector<std::vector<SyntaxStyle>> highlightBuffer(
        const TextBuffer&) const override {
        throw std::logic_error("Intentional exception from ExceptionThrowingHighlighter");
    }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {".test"};
    }
    
    std::string getLanguageName() const override {
        return "TestLanguage";
    }
};

// Test specifically for exception handling without using mocks
TEST_F(SyntaxHighlightingManagerTest, ExceptionThrowingHighlighterTest) {
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, ExceptionThrowingHighlighterTest) - Start" << std::endl;
    
    // Create and set a dedicated exception-throwing highlighter
    auto exceptionHighlighter = std::make_shared<ExceptionThrowingHighlighter>();
    manager_.setHighlighter(exceptionHighlighter);
    
    // Test that the exception is properly caught by the manager
    std::vector<std::vector<SyntaxStyle>> styles;
    ASSERT_NO_THROW({
        manager_.invalidateLine(0);
        styles = manager_.getHighlightingStyles(0, 0);
    });
    
    // Verify the result is valid but contains empty styles due to the exception
    ASSERT_EQ(styles.size(), 1);
    EXPECT_TRUE(styles[0].empty());
    
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, ExceptionThrowingHighlighterTest) - End" << std::endl;
}

// Test without fixture to completely isolate exception handling
TEST(StandaloneExceptionTest, HighlightingManagerHandlesExceptions) {
    std::cout << "[DEBUG] Standalone exception test starting..." << std::endl;
    
    // Create all objects locally to control lifetime
    SyntaxHighlightingManager manager;
    TextBuffer buffer;
    
    buffer.addLine("Line 1 for testing");
    buffer.addLine("Line 2 for testing");
    
    // Create a highlighter that always throws
    class SimpleExceptionHighlighter : public SyntaxHighlighter {
    public:
        std::unique_ptr<std::vector<SyntaxStyle>> highlightLine(const std::string&, size_t) const override {
            throw std::logic_error("Simple test exception");
        }
        
        std::vector<std::vector<SyntaxStyle>> highlightBuffer(const TextBuffer&) const override {
            throw std::logic_error("Simple test exception");
        }
        
        std::vector<std::string> getSupportedExtensions() const override {
            return {".txt"};
        }
        
        std::string getLanguageName() const override {
            return "Test";
        }
    };
    
    // Setup manager with the exception-throwing highlighter
    auto highlighter = std::make_shared<SimpleExceptionHighlighter>();
    manager.setBuffer(&buffer);
    manager.setHighlighter(highlighter);
    
    // Try to get highlighting styles - should not throw
    std::vector<std::vector<SyntaxStyle>> styles;
    ASSERT_NO_THROW({
        styles = manager.getHighlightingStyles(0, 1);
    });
    
    // Verify styles are valid but empty
    ASSERT_EQ(styles.size(), 2);
    EXPECT_TRUE(styles[0].empty());
    EXPECT_TRUE(styles[1].empty());
    
    // Clean up explicitly
    manager.setHighlighter(nullptr);
    manager.setBuffer(nullptr);
    
    std::cout << "[DEBUG] Standalone exception test completed successfully" << std::endl;
}

TEST_F(SyntaxHighlightingManagerTest, DisabledStateReturnsEmptyStyles) {
    // Test that a disabled manager returns empty styling
    manager_.setEnabled(false);
    
    // Create some test expectations for verification
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // When disabled, the highlighter should not be called at all
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(0);
    
    // Request styling while disabled
    auto styles = manager_.getHighlightingStyles(0, 2);
    
    // Verify we got the right number of lines back
    ASSERT_EQ(styles.size(), 3);
    
    // Each line should have empty styling when manager is disabled
    for (const auto& lineStyles : styles) {
        EXPECT_TRUE(lineStyles.empty());
    }
}

TEST_F(SyntaxHighlightingManagerTest, ReenabledStateResumesHighlighting) {
    // Test that when re-enabled after being disabled, highlighting works again
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // First disable the manager
    manager_.setEnabled(false);
    
    // Request styling while disabled (should not call highlighter)
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(0);
    auto disabledStyles = manager_.getHighlightingStyles(0, 2);
    
    // Re-enable the manager
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    manager_.setEnabled(true);
    
    // Now the highlighter should be called for each line
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3)  // For lines 0, 1, and 2
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    // Request styling while enabled
    auto enabledStyles = manager_.getHighlightingStyles(0, 2);
    
    // Verify we got the right number of lines back
    ASSERT_EQ(enabledStyles.size(), 3);
    
    // Each line should have non-empty styling when manager is enabled
    for (const auto& lineStyles : enabledStyles) {
        EXPECT_FALSE(lineStyles.empty());
        ASSERT_EQ(lineStyles.size(), 1);
        EXPECT_EQ(lineStyles[0].color, SyntaxColor::Keyword);
    }
}

TEST_F(SyntaxHighlightingManagerTest, InvalidateLineRemovesFromCache) {
    // Test that invalidateLine removes a specific line from the cache
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // First request highlighting to populate the cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3)  // For lines 0, 1, and 2
        .WillRepeatedly(ReturnStyleVector(testStyle));
    auto initialStyles = manager_.getHighlightingStyles(0, 2);
    
    // Clear expectations for the next phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Now, invalidate just line 1
    manager_.invalidateLine(1);
    
    // Set expectations for the next request - only line 1 should be re-highlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 0))
        .Times(0);  // Line 0 should still be cached
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 1))
        .Times(1)   // Line 1 was invalidated and needs re-highlighting
        .WillOnce(ReturnStyleVector(testStyle));
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, 2))
        .Times(0);  // Line 2 should still be cached
    
    // Request highlighting again
    auto updatedStyles = manager_.getHighlightingStyles(0, 2);
    
    // Verify the result
    ASSERT_EQ(updatedStyles.size(), 3);
    for (const auto& lineStyles : updatedStyles) {
        ASSERT_EQ(lineStyles.size(), 1);
        EXPECT_EQ(lineStyles[0].color, SyntaxColor::Keyword);
    }
}

TEST_F(SyntaxHighlightingManagerTest, VerifyInvalidateAllLinesCleanupBehavior) {
    // Test specific behavior of invalidateAllLines and verify it cleans up properly
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // First request highlighting to populate the cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3)  // For lines 0, 1, and 2
        .WillRepeatedly(ReturnStyleVector(testStyle));
    auto initialStyles = manager_.getHighlightingStyles(0, 2);
    
    // Verify initial cache state
    ASSERT_EQ(initialStyles.size(), 3);
    for (const auto& lineStyles : initialStyles) {
        ASSERT_EQ(lineStyles.size(), 1);
    }
    
    // Clear expectations for the next phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Now, invalidate all lines - this should clear the entire cache
    manager_.invalidateAllLines();
    
    // Expect all lines to be re-highlighted
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3)  // Lines 0, 1, and 2 will all need to be re-highlighted
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    // Request highlighting again
    auto updatedStyles = manager_.getHighlightingStyles(0, 2);
    
    // Verify that the manager properly re-highlighted all lines
    ASSERT_EQ(updatedStyles.size(), 3);
    for (const auto& lineStyles : updatedStyles) {
        ASSERT_EQ(lineStyles.size(), 1);
        EXPECT_EQ(lineStyles[0].color, SyntaxColor::Keyword);
    }
}

TEST_F(SyntaxHighlightingManagerTest, CacheManagementWithBufferChanges) {
    // Test that buffer changes are properly handled by the cache
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    
    // First request highlighting to populate the cache
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(3)  // For lines 0, 1, and 2
        .WillRepeatedly(ReturnStyleVector(testStyle));
    auto initialStyles = manager_.getHighlightingStyles(0, 2);
    
    // Clear expectations for the next phase
    testing::Mock::VerifyAndClearExpectations(mock_highlighter_.get());
    
    // Simulate a buffer change - this would normally happen through the editor
    // We'll simulate this by updating the buffer and invalidating the lines
    text_buffer_.addLine("New line content");  // Add a new line, now we have 4 lines
    
    // Invalidate the affected range (the whole buffer in this simple case)
    manager_.invalidateAllLines();
    
    // Expect all lines to be re-highlighted, now including the new line
    EXPECT_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .Times(4)  // Now we have 4 lines (0-3)
        .WillRepeatedly(ReturnStyleVector(testStyle));
    
    // Request highlighting for all lines
    auto updatedStyles = manager_.getHighlightingStyles(0, 3);
    
    // Verify that the manager properly handled the buffer change
    ASSERT_EQ(updatedStyles.size(), 4);  // Should now have 4 lines of styles
    for (const auto& lineStyles : updatedStyles) {
        ASSERT_EQ(lineStyles.size(), 1);
        EXPECT_EQ(lineStyles[0].color, SyntaxColor::Keyword);
    }
}

// Thread-safety test for concurrent reads
TEST_F(SyntaxHighlightingManagerTest, ConcurrentReadsAreThreadSafe) {
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, ConcurrentReadsAreThreadSafe) - Start" << std::endl;
    
    // Prepare a buffer with fewer lines for faster processing
    const size_t LINE_COUNT = 20; 
    
    // Reset the text buffer with multiple lines
    text_buffer_ = TextBuffer(); // Start fresh
    for (size_t i = 0; i < LINE_COUNT; ++i) {
        std::string line = "Line " + std::to_string(i) + " content with some C++ syntax: for (int i = 0; i < 10; i++) { }";
        text_buffer_.addLine(line);
    }
    
    // Configure mock highlighter with default styling
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    ON_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .WillByDefault(ReturnStyleVector(testStyle));
    
    // Configure the manager to use the updated buffer
    manager_.setBuffer(&text_buffer_);
    manager_.setHighlighter(mock_highlighter_);
    manager_.setEnabled(true);
    
    // Set a reasonable timeout
    manager_.setHighlightingTimeout(100);
    
    // Invalidate all lines to ensure they need highlighting
    manager_.invalidateAllLines();
    
    // Do an initial highlighting of just a few lines to ensure the mechanism works
    auto initialStyles = manager_.getHighlightingStyles(0, 5);
    ASSERT_EQ(initialStyles.size(), 6) << "The initial highlighting didn't return the expected number of lines";
    
    // Atomic counter to track if any thread encounters issues
    std::atomic<bool> encounteredIssues(false);
    
    // Vector to store results from each thread
    std::vector<std::future<bool>> results;
    
    // Create multiple threads that will read concurrently
    const size_t THREAD_COUNT = 4;
    
    for (size_t t = 0; t < THREAD_COUNT; ++t) {
        // Each thread will request highlighting for different parts of the buffer
        results.push_back(std::async(std::launch::async, [this, t, LINE_COUNT]() {
            try {
                // Calculate a range for this thread to request
                size_t startLine = (t * LINE_COUNT) / THREAD_COUNT;
                size_t endLine = ((t + 1) * LINE_COUNT) / THREAD_COUNT - 1;
                
                // Add some randomization to increase chance of concurrent access
                std::this_thread::sleep_for(std::chrono::milliseconds(t * 2));
                
                // Request styles for a range of lines
                std::vector<std::vector<SyntaxStyle>> styles = manager_.getHighlightingStyles(startLine, endLine);
                
                // Verify the result has the correct dimensions
                if (styles.size() != (endLine - startLine + 1)) {
                    std::cerr << "Thread " << t << ": Expected " << (endLine - startLine + 1) 
                              << " lines, got " << styles.size() << std::endl;
                    return false;
                }
                
                // In a concurrent environment, we cannot guarantee that all lines will be 
                // highlighted in a single call due to timeout, so we just make sure some were
                bool allLinesEmpty = true;
                for (const auto& lineStyles : styles) {
                    if (!lineStyles.empty()) {
                        allLinesEmpty = false;
                        break;
                    }
                }
                
                if (allLinesEmpty) {
                    std::cerr << "Thread " << t << ": All lines had empty styles - unexpected in concurrent testing" << std::endl;
                    return false;
                }
                
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Thread " << t << " encountered exception: " << e.what() << std::endl;
                return false;
            } catch (...) {
                std::cerr << "Thread " << t << " encountered unknown exception" << std::endl;
                return false;
            }
        }));
    }
    
    // Wait for all threads and check results
    for (auto& result : results) {
        if (!result.get()) {
            encounteredIssues = true;
        }
    }
    
    EXPECT_FALSE(encounteredIssues) << "One or more threads encountered issues during concurrent reads";
    
    // Reset the timeout to default
    manager_.setHighlightingTimeout(SyntaxHighlightingManager::DEFAULT_HIGHLIGHTING_TIMEOUT_MS);
    
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, ConcurrentReadsAreThreadSafe) - End" << std::endl;
}

// Thread-safety test for concurrent reads and writes
TEST_F(SyntaxHighlightingManagerTest, ConcurrentReadsAndWritesAreThreadSafe) {
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, ConcurrentReadsAndWritesAreThreadSafe) - Start" << std::endl;
    
    // Prepare a smaller buffer to test with
    const size_t LINE_COUNT = 20;
    
    // Reset the text buffer with multiple lines
    text_buffer_ = TextBuffer(); // Start fresh
    for (size_t i = 0; i < LINE_COUNT; ++i) {
        std::string line = "Line " + std::to_string(i) + " content with some C++ syntax: for (int i = 0; i < 10; i++) { }";
        text_buffer_.addLine(line);
    }
    
    // Configure mock highlighter with default styling
    SyntaxStyle testStyle(0, 5, SyntaxColor::Keyword);
    ON_CALL(*mock_highlighter_, highlightLine(testing::_, testing::_))
        .WillByDefault(ReturnStyleVector(testStyle));
    
    // Configure the manager to use the updated buffer
    manager_.setBuffer(&text_buffer_);
    manager_.setHighlighter(mock_highlighter_);
    manager_.setEnabled(true);
    
    // Set a reasonable timeout
    manager_.setHighlightingTimeout(100);
    
    // Do an initial highlighting of just a few lines
    auto initialStyles = manager_.getHighlightingStyles(0, 3);
    ASSERT_EQ(initialStyles.size(), 4) << "The initial highlighting didn't return the expected number of lines";
    
    // Atomic counter to track if any thread encounters issues
    std::atomic<bool> encounteredIssues(false);
    
    // Vector to store results from each thread
    std::vector<std::future<bool>> results;
    
    // Create a mix of reader and writer threads
    const size_t READER_THREADS = 3;
    const size_t WRITER_THREADS = 2;
    const size_t TOTAL_THREADS = READER_THREADS + WRITER_THREADS;
    
    // Random number generator for unpredictable operations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lineDist(0, LINE_COUNT - 1);
    std::uniform_int_distribution<> opDist(0, 3);  // For operation selection
    std::uniform_int_distribution<> sleepDist(1, 5); // For sleep times
    
    // Create reader threads
    for (size_t t = 0; t < READER_THREADS; ++t) {
        results.push_back(std::async(std::launch::async, [this, t, LINE_COUNT, &gen, &lineDist, &sleepDist]() {
            try {
                // Each reader makes a few requests
                for (int i = 0; i < 3; ++i) {
                    // Calculate a random range for this thread to request (small range to finish faster)
                    size_t startLine = lineDist(gen) % (LINE_COUNT - 3);
                    size_t endLine = std::min(startLine + 3, LINE_COUNT - 1);
                    
                    // Add some randomized sleep to increase chance of concurrency
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDist(gen)));
                    
                    // Request styles for the range
                    std::vector<std::vector<SyntaxStyle>> styles = manager_.getHighlightingStyles(startLine, endLine);
                    
                    // Verify the result has the correct dimensions
                    if (styles.size() != (endLine - startLine + 1)) {
                        std::cerr << "Reader " << t << ": Expected " << (endLine - startLine + 1) 
                                << " lines, got " << styles.size() << std::endl;
                        return false;
                    }
                    
                    // We don't need to check contents as they might be legitimately empty during concurrent ops
                }
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Reader " << t << " encountered exception: " << e.what() << std::endl;
                return false;
            } catch (...) {
                std::cerr << "Reader " << t << " encountered unknown exception" << std::endl;
                return false;
            }
        }));
    }
    
    // Create writer threads that will invalidate lines
    for (size_t t = 0; t < WRITER_THREADS; ++t) {
        results.push_back(std::async(std::launch::async, [this, t, LINE_COUNT, &gen, &lineDist, &opDist, &sleepDist]() {
            try {
                // Each writer performs a few operations
                for (int i = 0; i < 5; ++i) {
                    // Randomly choose between different operations
                    int operation = opDist(gen);
                    
                    // Add some randomized sleep to increase chance of concurrency
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDist(gen)));
                    
                    switch (operation) {
                        case 0: {
                            // Invalidate a single random line
                            size_t line = lineDist(gen);
                            manager_.invalidateLine(line);
                            break;
                        }
                        case 1: {
                            // Invalidate a range of lines
                            size_t startLine = lineDist(gen) % (LINE_COUNT / 2);
                            size_t endLine = std::min(startLine + 3, LINE_COUNT - 1);
                            manager_.invalidateLines(startLine, endLine);
                            break;
                        }
                        case 2: {
                            // Toggle the enabled state briefly
                            bool currentState = manager_.isEnabled();
                            manager_.setEnabled(!currentState);
                            // Don't leave it disabled, re-enable it after a very short delay
                            if (currentState) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                manager_.setEnabled(true);
                            }
                            break;
                        }
                        default: {
                            // Invalidate all lines (rarely)
                            if (i == 2) { // Only do this once in the middle
                                manager_.invalidateAllLines();
                            } else {
                                // Default to another line invalidation
                                manager_.invalidateLine(lineDist(gen));
                            }
                            break;
                        }
                    }
                }
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Writer " << t << " encountered exception: " << e.what() << std::endl;
                return false;
            } catch (...) {
                std::cerr << "Writer " << t << " encountered unknown exception" << std::endl;
                return false;
            }
        }));
    }
    
    // Wait for all threads and check results
    for (auto& result : results) {
        if (!result.get()) {
            encounteredIssues = true;
        }
    }
    
    // Verify the manager is still in a consistent state
    EXPECT_FALSE(encounteredIssues) << "One or more threads encountered issues during concurrent operations";
    
    // Final check - verify that after all this concurrent activity, we can still get highlighting
    // First re-enable manager and repopulate cache
    manager_.setEnabled(true);
    manager_.invalidateAllLines(); // Clear any partial state
    
    // Highlight a small range and verify we get results
    auto finalStyles = manager_.getHighlightingStyles(0, 2);
    EXPECT_EQ(finalStyles.size(), 3) << "Manager should still produce correct results after concurrent operations";
    
    // Reset the timeout to default
    manager_.setHighlightingTimeout(SyntaxHighlightingManager::DEFAULT_HIGHLIGHTING_TIMEOUT_MS);
    
    std::cout << "[DEBUG] TEST_F(SyntaxHighlightingManagerTest, ConcurrentReadsAndWritesAreThreadSafe) - End" << std::endl;
} 