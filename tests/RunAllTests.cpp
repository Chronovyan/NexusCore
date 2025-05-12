#include "gtest/gtest.h"

// Include all test function declarations
#include "TestDeclarations.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 