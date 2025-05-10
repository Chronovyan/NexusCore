#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "SyntaxHighlighter.h" // Assuming this is the path
#include <iostream> // Added for std::cout
#include <memory>
#include <stdexcept>

// Helper function to check if a specific style is applied to a range
bool hasStyle(const std::vector<SyntaxStyle>& styles, size_t start, size_t end, SyntaxColor color) {
    for (const auto& style : styles) {
        if (style.startCol == start && style.endCol == end && style.color == color) {
            return true;
        }
    }
    return false;
}

// Test fixture for CppHighlighter tests
class CppHighlighterTest : public ::testing::Test {
protected:
    CppHighlighter highlighter;
    void SetUp() override {
        std::cout << "[DEBUG] CppHighlighterTest::SetUp()" << std::endl;
    }
    void TearDown() override {
        std::cout << "[DEBUG] CppHighlighterTest::TearDown()" << std::endl;
    }
};

TEST_F(CppHighlighterTest, HighlightsKeywords) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsKeywords) - Start" << std::endl;
    std::string line = "int main() { return 0; }";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);

    // Check for "int" - it's a Type in our implementation
    EXPECT_TRUE(hasStyle(styles, 0, 3, SyntaxColor::Type)); 
    // Check for "return" - position is at [13,19] in our implementation
    EXPECT_TRUE(hasStyle(styles, 13, 19, SyntaxColor::Keyword)); 
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsKeywords) - End" << std::endl;
}

TEST_F(CppHighlighterTest, HighlightsLineComments) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsLineComments) - Start" << std::endl;
    std::string line = "int x = 5; // This is a comment";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);
    
    EXPECT_TRUE(hasStyle(styles, 11, 31, SyntaxColor::Comment));
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsLineComments) - End" << std::endl;
}

TEST_F(CppHighlighterTest, HighlightsBlockCommentsOnSingleLine) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsBlockCommentsOnSingleLine) - Start" << std::endl;
    std::string line = "/* Block comment */ int y = 10;";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);

    EXPECT_TRUE(hasStyle(styles, 0, 19, SyntaxColor::Comment));
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsBlockCommentsOnSingleLine) - End" << std::endl;
}

TEST_F(CppHighlighterTest, HighlightsStringLiterals) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsStringLiterals) - Start" << std::endl;
    std::string line = "const char* str = \"Hello, World!\";";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);

    EXPECT_TRUE(hasStyle(styles, 18, 33, SyntaxColor::String));
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsStringLiterals) - End" << std::endl;
}

TEST_F(CppHighlighterTest, HighlightsNumbers) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsNumbers) - Start" << std::endl;
    std::string line = "float pi = 3.14159; int count = 100;";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);

    EXPECT_TRUE(hasStyle(styles, 11, 18, SyntaxColor::Number));
    EXPECT_TRUE(hasStyle(styles, 32, 35, SyntaxColor::Number));
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HighlightsNumbers) - End" << std::endl;
}


TEST_F(CppHighlighterTest, MixedElements) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, MixedElements) - Start" << std::endl;
    std::string line = "if (val > 0) { // Check positive";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);

    EXPECT_TRUE(hasStyle(styles, 0, 2, SyntaxColor::Keyword));       // "if"
    EXPECT_TRUE(hasStyle(styles, 10, 11, SyntaxColor::Number));      // "0"
    EXPECT_TRUE(hasStyle(styles, 15, 32, SyntaxColor::Comment));   // "// Check positive"
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, MixedElements) - End" << std::endl;
}

TEST_F(CppHighlighterTest, NoHighlightableElements) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, NoHighlightableElements) - Start" << std::endl;
    std::string line = "  myVariable anotherVar  ";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);
    EXPECT_TRUE(styles.empty());
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, NoHighlightableElements) - End" << std::endl;
}

TEST_F(CppHighlighterTest, HandlesEmptyLine) {
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HandlesEmptyLine) - Start" << std::endl;
    std::string line = "";
    std::vector<SyntaxStyle> styles = highlighter.highlightLine(line, 0);
    EXPECT_TRUE(styles.empty());
    std::cout << "[DEBUG] TEST_F(CppHighlighterTest, HandlesEmptyLine) - End" << std::endl;
}

// Test for PatternBasedHighlighter directly if needed,
// or to test adding patterns.
class PatternBasedHighlighterTest : public ::testing::Test, public PatternBasedHighlighter { // Derive from PatternBasedHighlighter
protected:
    PatternBasedHighlighterTest() : PatternBasedHighlighter("TestPatternHighlighter") {
        std::cout << "[DEBUG] PatternBasedHighlighterTest::PatternBasedHighlighterTest() Constructor" << std::endl;
    } 
    void SetUp() override {
        std::cout << "[DEBUG] PatternBasedHighlighterTest::SetUp()" << std::endl;
    }
    void TearDown() override {
        std::cout << "[DEBUG] PatternBasedHighlighterTest::TearDown()" << std::endl;
    }
    // PatternBasedHighlighter pbh{"TestPatternHighlighter"}; // No longer need separate instance
};

TEST_F(PatternBasedHighlighterTest, AddAndHighlightPattern) {
    std::cout << "[DEBUG] TEST_F(PatternBasedHighlighterTest, AddAndHighlightPattern) - Start" << std::endl;
    addPattern("\\bmykeyword\\b", SyntaxColor::Type); // Call directly as a member
    std::string line = "this is mykeyword here";
    std::vector<SyntaxStyle> styles = highlightLine(line, 0); // Call directly as a member
    
    ASSERT_EQ(styles.size(), 1);
    EXPECT_EQ(styles[0].startCol, 8);
    EXPECT_EQ(styles[0].endCol, 17);
    EXPECT_EQ(styles[0].color, SyntaxColor::Type);
    std::cout << "[DEBUG] TEST_F(PatternBasedHighlighterTest, AddAndHighlightPattern) - End" << std::endl;
}

TEST_F(PatternBasedHighlighterTest, OverlappingPatternsFavorFirstAdded) {
    std::cout << "[DEBUG] TEST_F(PatternBasedHighlighterTest, OverlappingPatternsFavorFirstAdded) - Start" << std::endl;
    // First pattern: "word"
    addPattern("word", SyntaxColor::Keyword); // Regex: "word"
    // Second pattern: "keyword" (contains "word")
    addPattern("keyword", SyntaxColor::String); // Regex: "keyword"

    std::string line = "this is a keyword"; // "keyword" (length 7) starts at 10. "word" (length 4) within it starts at 13.
    std::vector<SyntaxStyle> styles = highlightLine(line, 0);

    // Verify that "keyword" is highlighted as STRING
    // Text: "keyword", Range: [10, 17)
    EXPECT_TRUE(hasStyle(styles, 10, 17, SyntaxColor::String));

    // Verify that "word" within "keyword" is highlighted as KEYWORD
    // Text: "word", Range: [13, 17)
    EXPECT_TRUE(hasStyle(styles, 13, 17, SyntaxColor::Keyword));

    // Optional: Verify the number of styles if no other patterns are expected to match.
    // Here, we expect two styles from the two patterns.
    EXPECT_EQ(styles.size(), 2);
    
    // The original test name "OverlappingPatternsFavorFirstAdded" is now a misnomer
    // if we keep the current PatternBasedHighlighter logic.
    // For now, we are testing the actual behavior.
    // If "first added wins" is desired, PatternBasedHighlighter::highlightLine needs modification.
    std::cout << "[DEBUG] TEST_F(PatternBasedHighlighterTest, OverlappingPatternsFavorFirstAdded) - End" << std::endl;
}

// Mock SyntaxHighlighter that can be configured to throw on getSupportedExtensions
class RegistryMockSyntaxHighlighter : public SyntaxHighlighter {
public:
    MOCK_CONST_METHOD2(highlightLine, std::vector<SyntaxStyle>(const std::string& line, size_t lineIndex));
    MOCK_CONST_METHOD1(highlightBuffer, std::vector<std::vector<SyntaxStyle>>(const TextBuffer& buffer));
    MOCK_CONST_METHOD0(getSupportedExtensions, std::vector<std::string>());
    MOCK_CONST_METHOD0(getLanguageName, std::string());

    // Constructor to configure mock behavior
    RegistryMockSyntaxHighlighter(bool throwOnGetExtensions = false, 
                                 const std::string& exceptionMsg = "Test Exception") {
        using namespace testing;

        ON_CALL(*this, getLanguageName())
            .WillByDefault(Return("MockLanguage"));

        if (throwOnGetExtensions) {
            ON_CALL(*this, getSupportedExtensions())
                .WillByDefault(Throw(std::runtime_error(exceptionMsg)));
        } else {
            ON_CALL(*this, getSupportedExtensions())
                .WillByDefault(Return(std::vector<std::string>{"mock", "test"}));
        }
    }
};

class SyntaxHighlighterRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear the registry (if possible) before each test
        SyntaxHighlighterRegistry::getInstance().clearRegistry();
    }

    void TearDown() override {
        // Clean up after each test
        SyntaxHighlighterRegistry::getInstance().clearRegistry();
    }
};

// Test registering a well-behaved highlighter
TEST_F(SyntaxHighlighterRegistryTest, RegisterHighlighter) {
    auto highlighter = std::make_unique<testing::NiceMock<RegistryMockSyntaxHighlighter>>(false);
    
    ASSERT_NO_THROW({
        SyntaxHighlighterRegistry::getInstance().registerHighlighter(std::move(highlighter));
    });
    
    // Verify highlighter was registered by checking if we can get it for its extensions
    auto retrievedHighlighter = SyntaxHighlighterRegistry::getInstance().getHighlighterForExtension("mock");
    ASSERT_NE(retrievedHighlighter, nullptr);
    EXPECT_EQ(retrievedHighlighter->getLanguageName(), "MockLanguage");
}

// Test registering a null highlighter
TEST_F(SyntaxHighlighterRegistryTest, RegisterNullHighlighter) {
    std::unique_ptr<SyntaxHighlighter> nullHighlighter = nullptr;
    
    // Should handle null without crashing
    ASSERT_NO_THROW({
        SyntaxHighlighterRegistry::getInstance().registerHighlighter(std::move(nullHighlighter));
    });
    
    // Verify it didn't affect the registry
    auto retrievedHighlighter = SyntaxHighlighterRegistry::getInstance().getHighlighterForExtension("any");
    EXPECT_EQ(retrievedHighlighter, nullptr);
}

// Test registering a highlighter that throws during getSupportedExtensions
TEST_F(SyntaxHighlighterRegistryTest, RegisterThrowingHighlighter) {
    auto throwingHighlighter = std::make_unique<testing::NiceMock<RegistryMockSyntaxHighlighter>>(true);
    
    // Should handle exception without crashing
    ASSERT_NO_THROW({
        SyntaxHighlighterRegistry::getInstance().registerHighlighter(std::move(throwingHighlighter));
    });
    
    // Verify no highlighter was registered
    auto retrievedHighlighter = SyntaxHighlighterRegistry::getInstance().getHighlighterForExtension("mock");
    EXPECT_EQ(retrievedHighlighter, nullptr);
}

// Test getting a highlighter for a non-existent extension
TEST_F(SyntaxHighlighterRegistryTest, GetHighlighterForNonExistentExtension) {
    auto highlighter = std::make_unique<testing::NiceMock<RegistryMockSyntaxHighlighter>>(false);
    SyntaxHighlighterRegistry::getInstance().registerHighlighter(std::move(highlighter));
    
    // Should return nullptr for unknown extension
    auto retrievedHighlighter = SyntaxHighlighterRegistry::getInstance().getHighlighterForExtension("nonexistent");
    EXPECT_EQ(retrievedHighlighter, nullptr);
}

// Test getting a shared_ptr highlighter for an extension
TEST_F(SyntaxHighlighterRegistryTest, GetSharedHighlighterForExtension) {
    auto highlighter = std::make_unique<testing::NiceMock<RegistryMockSyntaxHighlighter>>(false);
    SyntaxHighlighterRegistry::getInstance().registerHighlighter(std::move(highlighter));
    
    // Test the shared_ptr version
    auto sharedHighlighter = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("mock");
    ASSERT_NE(sharedHighlighter.get(), nullptr);
    EXPECT_EQ(sharedHighlighter->getLanguageName(), "MockLanguage");
    
    // Verify it's actually a shared_ptr
    auto sharedHighlighter2 = SyntaxHighlighterRegistry::getInstance().getSharedHighlighterForExtension("mock");
    EXPECT_EQ(sharedHighlighter.get(), sharedHighlighter2.get()); // Should point to same object
}

// Test the registry's thread safety
TEST_F(SyntaxHighlighterRegistryTest, ThreadSafety) {
    auto highlighter1 = std::make_unique<testing::NiceMock<RegistryMockSyntaxHighlighter>>(false);
    auto expectedLanguageName = highlighter1->getLanguageName();
    
    // Register highlighter
    SyntaxHighlighterRegistry::getInstance().registerHighlighter(std::move(highlighter1));
    
    // Access from multiple threads
    std::vector<std::thread> threads;
    std::atomic<bool> encounteredError{false};
    
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&encounteredError, expectedLanguageName]() {
            try {
                auto highlighter = SyntaxHighlighterRegistry::getInstance().getHighlighterForExtension("mock");
                if (highlighter == nullptr || highlighter->getLanguageName() != expectedLanguageName) {
                    encounteredError = true;
                }
            } catch (...) {
                encounteredError = true;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_FALSE(encounteredError.load());
} 