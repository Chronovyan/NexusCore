#include <gtest/gtest.h>
#include <string>
#include <windows.h>

class ClipboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Open the clipboard
        ASSERT_TRUE(OpenClipboard(nullptr));
        
        // Clear the clipboard
        EmptyClipboard();
    }
    
    void TearDown() override {
        // Close the clipboard
        CloseClipboard();
    }
    
    // Helper function to set clipboard text
    bool setClipboardText(const std::string& text) {
        // Allocate global memory for the text
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (!hMem) return false;
        
        // Lock the memory and copy the text
        char* pMem = static_cast<char*>(GlobalLock(hMem));
        if (!pMem) {
            GlobalFree(hMem);
            return false;
        }
        
        memcpy(pMem, text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        
        // Set the clipboard data
        bool result = SetClipboardData(CF_TEXT, hMem) != nullptr;
        
        // The system now owns the memory, don't free it
        if (!result) {
            GlobalFree(hMem);
        }
        
        return result;
    }
    
    // Helper function to get clipboard text
    std::string getClipboardText() {
        if (!IsClipboardFormatAvailable(CF_TEXT)) {
            return "";
        }
        
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (!hData) {
            return "";
        }
        
        char* pszText = static_cast<char*>(GlobalLock(hData));
        if (!pszText) {
            return "";
        }
        
        std::string text(pszText);
        GlobalUnlock(hData);
        
        return text;
    }
};

TEST_F(ClipboardTest, SetAndGetText) {
    const std::string testText = "Hello, Clipboard!";
    
    // Set the clipboard text
    ASSERT_TRUE(setClipboardText(testText));
    
    // Get the clipboard text
    std::string clipboardText = getClipboardText();
    
    // Verify the text matches
    EXPECT_EQ(clipboardText, testText);
}

TEST_F(ClipboardTest, EmptyClipboard) {
    // Set some text first
    ASSERT_TRUE(setClipboardText("Test"));
    
    // Clear the clipboard
    EmptyClipboard();
    
    // Verify clipboard is empty
    std::string clipboardText = getClipboardText();
    EXPECT_TRUE(clipboardText.empty());
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
