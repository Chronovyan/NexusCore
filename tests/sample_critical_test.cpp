#include <gtest/gtest.h>
#include "critical_tests.h"

// Basic fixture for demonstration
class SampleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code runs before each test
    }

    void TearDown() override {
        // Cleanup code runs after each test
    }
};

// Regular test - won't run in pre-commit hook
TEST_F(SampleTest, BasicTest) {
    // This test will only run in the full test suite
    EXPECT_TRUE(true);
}

// Critical test - will run in pre-commit hook
TEST_F(SampleTest, CRITICAL(CoreFunctionality)) {
    // This test will run in both pre-commit hook and full test suite
    // Keep critical tests very fast and focused on core functionality
    EXPECT_TRUE(true);
}

// Another way to tag a test as critical
TEST(StandaloneTest, BasicTest_Critical) {
    // This test will run in pre-commit hook due to "_Critical" suffix
    EXPECT_TRUE(true);
}

/*
 * Tips for Critical Tests:
 * 1. Keep them fast - they run on every commit
 * 2. Focus on core functionality only
 * 3. Minimize external dependencies
 * 4. Avoid testing UI or complex integrations
 * 5. Use meaningful assertions that catch real issues
 */ 