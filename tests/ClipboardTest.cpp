#define IMGUI_DEFINE_MATH_OPERATORS
#include <gtest/gtest.h>
#include <imgui.h>
#include <imgui_internal.h>

TEST(ClipboardTest, BasicClipboardOperation) {
    // Test setting and getting text from the system clipboard
    const char* testText = "Hello, Clipboard!";
    
    // Set the clipboard text
    ImGui::SetClipboardText(testText);
    
    // Get the clipboard text
    const char* clipboardText = ImGui::GetClipboardText();
    
    // Verify the text matches
    EXPECT_STREQ(clipboardText, testText);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
