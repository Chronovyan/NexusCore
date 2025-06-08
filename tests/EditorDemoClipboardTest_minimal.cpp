#include <gtest/gtest.h>
#include "../src/EditorDemoWindow.h"

namespace ai_editor {

class EditorDemoWindowTest : public ::testing::Test {
protected:
    EditorDemoWindow editor_;
};

TEST_F(EditorDemoWindowTest, CanBeCreated) {
    // Just test that we can create an instance
    EXPECT_TRUE(true);
}

} // namespace ai_editor

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
