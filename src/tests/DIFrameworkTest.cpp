
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "../src/di/DIFramework.hpp"
#include "../src/di/ServiceCollection.hpp"

// Simple test to ensure the DIFramework compiles
TEST(DIFrameworkTest, BasicCompilationTest) {
    auto framework = std::make_shared<di::DIFramework>();
    EXPECT_TRUE(framework != nullptr);
}

// Main function that runs the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
