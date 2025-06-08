#include <gtest/gtest.h>

// Test case to verify the testing framework is working
TEST(BasicTest, FrameworkTest) {
    EXPECT_EQ(1 + 1, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
