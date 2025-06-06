#include "gtest/gtest.h"
#include <string>
#include <memory>

// A simple text buffer class for testing
class SimpleTextBuffer {
public:
    SimpleTextBuffer() : content(""), cursorPos(0) {}
    
    void insertText(const std::string& text) {
        content.insert(cursorPos, text);
        cursorPos += text.length();
    }
    
    void deleteChar() {
        if (cursorPos > 0 && !content.empty()) {
            content.erase(cursorPos - 1, 1);
            cursorPos--;
        }
    }
    
    void moveCursorLeft() {
        if (cursorPos > 0) cursorPos--;
    }
    
    void moveCursorRight() {
        if (cursorPos < content.length()) cursorPos++;
    }
    
    std::string getText() const { return content; }
    size_t getCursorPosition() const { return cursorPos; }
    
private:
    std::string content;
    size_t cursorPos;
};

class SimpleEditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<SimpleTextBuffer>();
    }

    void TearDown() override {
        buffer.reset();
    }

    std::unique_ptr<SimpleTextBuffer> buffer;
};

TEST_F(SimpleEditorTest, InitialState) {
    EXPECT_EQ(buffer->getText(), "");
    EXPECT_EQ(buffer->getCursorPosition(), 0);
}

TEST_F(SimpleEditorTest, InsertText) {
    buffer->insertText("Hello");
    EXPECT_EQ(buffer->getText(), "Hello");
    EXPECT_EQ(buffer->getCursorPosition(), 5);
    
    buffer->insertText(", World!");
    EXPECT_EQ(buffer->getText(), "Hello, World!");
    EXPECT_EQ(buffer->getCursorPosition(), 13);
}

TEST_F(SimpleEditorTest, MoveCursor) {
    buffer->insertText("Hello");
    
    // Move cursor left twice
    buffer->moveCursorLeft();
    buffer->moveCursorLeft();
    EXPECT_EQ(buffer->getCursorPosition(), 3);
    
    // Insert text in the middle
    buffer->insertText("p");
    EXPECT_EQ(buffer->getText(), "Helplo");
    
    // Move cursor to end
    while (buffer->getCursorPosition() < buffer->getText().length()) {
        buffer->moveCursorRight();
    }
    buffer->insertText("!");
    EXPECT_EQ(buffer->getText(), "Helplo!");
}

TEST_F(SimpleEditorTest, DeleteCharacter) {
    buffer->insertText("Hello");
    
    // Move cursor between 'l' and 'o' (before last character)
    buffer->moveCursorLeft();
    buffer->moveCursorLeft();
    EXPECT_EQ(buffer->getText(), "Hello");
    
    // Delete 'l' (the character before cursor)
    buffer->deleteChar();
    EXPECT_EQ(buffer->getText(), "Helo");
    
    // Move cursor to beginning
    while (buffer->getCursorPosition() > 0) {
        buffer->moveCursorLeft();
    }
    
    // Try to delete at beginning (should do nothing)
    buffer->deleteChar();
    EXPECT_EQ(buffer->getText(), "Helo");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
