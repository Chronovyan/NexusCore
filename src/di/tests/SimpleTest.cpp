#include <gtest/gtest.h>
#include <di/DIFramework.hpp>
#include <iostream>

// Very simple test to verify basic functionality
TEST(SimpleTest, BasicTest) {
    std::cout << "Creating DIFramework instance" << std::endl;
    di::DIFramework framework;
    std::cout << "Successfully created DIFramework instance" << std::endl;
    
    // Just a simple check
    EXPECT_TRUE(true);
    
    std::cout << "Test completed, framework will be destroyed" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Starting SimpleTest" << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    std::cout << "Test execution completed with result: " << result << std::endl;
    return result;
} 