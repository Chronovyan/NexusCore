#include <gtest/gtest.h>
#include "../MultiCursor.h"
#include "../TextBuffer.h"

// Mock ITextBuffer implementation for testing
class MockTextBuffer : public ITextBuffer {
public:
    MockTextBuffer() {
        // Initialize with some test data
        lines_ = {
            "Line 1 with some text",
            "Line 2 with repeated text repeated",
            "Line 3 with different content",
            "Line 4 with repeated text repeated",
            "Line 5 with final line"
        };
    }
    
    virtual ~MockTextBuffer() = default;
    
    // Implement required ITextBuffer methods
    bool isEmpty() const override { return lines_.empty(); }
    size_t lineCount() const override { return lines_.size(); }
    const std::string& getLine(size_t lineIndex) const override { 
        if (lineIndex >= lines_.size()) {
            static const std::string empty;
            return empty;
        }
        return lines_[lineIndex]; 
    }
    
    // The following methods are required by the interface but not used in tests
    void addLine(const std::string& text) override {}
    void insertLine(size_t lineIndex, const std::string& text) override {}
    void deleteLine(size_t lineIndex) override {}
    void replaceLine(size_t lineIndex, const std::string& text) override {}
    void clear() override {}
    void load(const std::vector<std::string>& lines) override {}
    std::vector<std::string> getAllLines() const override { return lines_; }
    std::string getLineSegment(size_t lineIndex, size_t startCol, size_t endCol) const override { return ""; }
    void insertText(size_t line, size_t col, const std::string& text) override {}
    void deleteText(size_t startLine, size_t startCol, size_t endLine, size_t endCol) override {}
    size_t lineLength(size_t lineIndex) const override { return getLine(lineIndex).length(); }
    
private:
    std::vector<std::string> lines_;
};

// Test fixture for MultiCursor tests
class MultiCursorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a fresh MultiCursor and MockTextBuffer for each test
        multiCursor = std::make_unique<MultiCursor>();
        textBuffer = std::make_unique<MockTextBuffer>();
    }
    
    std::unique_ptr<MultiCursor> multiCursor;
    std::unique_ptr<MockTextBuffer> textBuffer;
};

// Test basic cursor functionality
TEST_F(MultiCursorTest, BasicCursorOperations) {
    // Check that we start with one cursor at position (0, 0)
    EXPECT_EQ(multiCursor->getCursorCount(), 1);
    EXPECT_EQ(multiCursor->getPrimaryCursorPosition().line, 0);
    EXPECT_EQ(multiCursor->getPrimaryCursorPosition().column, 0);
    
    // Set primary cursor position
    multiCursor->setPrimaryCursorPosition({1, 5});
    EXPECT_EQ(multiCursor->getPrimaryCursorPosition().line, 1);
    EXPECT_EQ(multiCursor->getPrimaryCursorPosition().column, 5);
    
    // Add another cursor
    bool added = multiCursor->addCursor({2, 10});
    EXPECT_TRUE(added);
    EXPECT_EQ(multiCursor->getCursorCount(), 2);
    
    // Try to add a cursor at the same position (should fail)
    added = multiCursor->addCursor({1, 5});
    EXPECT_FALSE(added);
    EXPECT_EQ(multiCursor->getCursorCount(), 2);
    
    // Remove secondary cursor
    bool removed = multiCursor->removeCursor({2, 10});
    EXPECT_TRUE(removed);
    EXPECT_EQ(multiCursor->getCursorCount(), 1);
    
    // Try to remove primary cursor (should fail)
    removed = multiCursor->removeCursor({1, 5});
    EXPECT_FALSE(removed); // Can't remove primary cursor
    EXPECT_EQ(multiCursor->getCursorCount(), 1);
}

// Test cursor movement
TEST_F(MultiCursorTest, CursorMovement) {
    // Set up multiple cursors
    multiCursor->setPrimaryCursorPosition({1, 5});
    multiCursor->addCursor({2, 10});
    multiCursor->addCursor({3, 15});
    
    // Move all cursors up
    multiCursor->moveCursors("up", *textBuffer);
    
    // Get all cursor positions after movement
    auto positions = multiCursor->getAllCursorPositions();
    EXPECT_EQ(positions.size(), 3);
    EXPECT_EQ(positions[0].line, 0); // Primary moved from 1,5 to 0,5
    EXPECT_EQ(positions[1].line, 1); // Secondary moved from 2,10 to 1,10
    EXPECT_EQ(positions[2].line, 2); // Secondary moved from 3,15 to 2,15
    
    // Move all cursors right
    multiCursor->moveCursors("right", *textBuffer);
    
    // Get all cursor positions after movement
    positions = multiCursor->getAllCursorPositions();
    EXPECT_EQ(positions[0].column, 6); // Primary moved from 0,5 to 0,6
    EXPECT_EQ(positions[1].column, 11); // Secondary moved from 1,10 to 1,11
    EXPECT_EQ(positions[2].column, 16); // Secondary moved from 2,15 to 2,16
}

// Test selection operations
TEST_F(MultiCursorTest, SelectionOperations) {
    // Set up multiple cursors
    multiCursor->setPrimaryCursorPosition({1, 5});
    multiCursor->addCursor({2, 10});
    
    // Start selection for all cursors
    multiCursor->startSelection(0);
    multiCursor->startSelection(1);
    
    // Check that selections are created
    EXPECT_TRUE(multiCursor->hasSelection(0));
    EXPECT_TRUE(multiCursor->hasSelection(1));
    
    // Move cursors to create selections
    multiCursor->setPrimaryCursorPosition({1, 10});
    
    // Update selections
    multiCursor->updateSelection(0);
    
    // Check selection ranges
    auto selection0 = multiCursor->getSelection(0);
    EXPECT_EQ(selection0.start.line, 1);
    EXPECT_EQ(selection0.start.column, 5);
    EXPECT_EQ(selection0.end.line, 1);
    EXPECT_EQ(selection0.end.column, 10);
    
    // Set selection range directly
    multiCursor->setSelectionRange({3, 5}, {3, 15}, 1);
    
    // Check selection range
    auto selection1 = multiCursor->getSelection(1);
    EXPECT_EQ(selection1.start.line, 3);
    EXPECT_EQ(selection1.start.column, 5);
    EXPECT_EQ(selection1.end.line, 3);
    EXPECT_EQ(selection1.end.column, 15);
    
    // Clear selection for cursor 0
    multiCursor->clearSelection(0);
    EXPECT_FALSE(multiCursor->hasSelection(0));
    EXPECT_TRUE(multiCursor->hasSelection(1));
    
    // Clear all selections
    multiCursor->clearAllSelections();
    EXPECT_FALSE(multiCursor->hasSelection(0));
    EXPECT_FALSE(multiCursor->hasSelection(1));
}

// Test adding cursors at all occurrences of a pattern
TEST_F(MultiCursorTest, AddCursorsAtAllOccurrences) {
    // Set primary cursor position
    multiCursor->setPrimaryCursorPosition({0, 0});
    
    // Add cursors at all occurrences of "repeated"
    size_t added = multiCursor->addCursorsAtAllOccurrences("repeated", *textBuffer);
    EXPECT_EQ(added, 4); // "repeated" appears twice in line 1 and twice in line 3
    EXPECT_EQ(multiCursor->getCursorCount(), 5); // 1 primary + 4 new
    
    // Check cursor positions
    auto positions = multiCursor->getAllCursorPositions();
    
    // Check if we have cursors at the positions where "repeated" appears
    bool foundLine1First = false;
    bool foundLine1Second = false;
    bool foundLine3First = false;
    bool foundLine3Second = false;
    
    for (const auto& pos : positions) {
        if (pos.line == 1 && pos.column == 10) foundLine1First = true;
        if (pos.line == 1 && pos.column == 28) foundLine1Second = true;
        if (pos.line == 3 && pos.column == 10) foundLine3First = true;
        if (pos.line == 3 && pos.column == 28) foundLine3Second = true;
    }
    
    EXPECT_TRUE(foundLine1First);
    EXPECT_TRUE(foundLine1Second);
    EXPECT_TRUE(foundLine3First);
    EXPECT_TRUE(foundLine3Second);
}

// Test adding cursors at column
TEST_F(MultiCursorTest, AddCursorsAtColumn) {
    // Set primary cursor position
    multiCursor->setPrimaryCursorPosition({0, 0});
    
    // Add cursors at column 5 for lines 1-4
    size_t added = multiCursor->addCursorsAtColumn(1, 4, 5, *textBuffer);
    EXPECT_EQ(added, 4); // 4 new cursors added
    EXPECT_EQ(multiCursor->getCursorCount(), 5); // 1 primary + 4 new
    
    // Check cursor positions
    auto positions = multiCursor->getAllCursorPositions();
    
    // Verify that we have cursors at column 5 for lines 1-4
    bool foundLine1 = false;
    bool foundLine2 = false;
    bool foundLine3 = false;
    bool foundLine4 = false;
    
    for (const auto& pos : positions) {
        if (pos.line == 1 && pos.column == 5) foundLine1 = true;
        if (pos.line == 2 && pos.column == 5) foundLine2 = true;
        if (pos.line == 3 && pos.column == 5) foundLine3 = true;
        if (pos.line == 4 && pos.column == 5) foundLine4 = true;
    }
    
    EXPECT_TRUE(foundLine1);
    EXPECT_TRUE(foundLine2);
    EXPECT_TRUE(foundLine3);
    EXPECT_TRUE(foundLine4);
}

// Test merging overlapping selections
TEST_F(MultiCursorTest, MergeOverlappingSelections) {
    // Set up multiple cursors with overlapping selections
    multiCursor->setPrimaryCursorPosition({1, 5});
    multiCursor->startSelection(0);
    multiCursor->setPrimaryCursorPosition({1, 15});
    multiCursor->updateSelection(0);
    
    multiCursor->addCursor({1, 10});
    multiCursor->startSelection(1);
    CursorPosition pos = {1, 20};
    multiCursor->setPrimaryCursorPosition(pos); // This should move the primary cursor
    multiCursor->updateSelection(1);
    
    // Verify we have two overlapping selections
    auto selections = multiCursor->getAllSelections();
    EXPECT_EQ(selections.size(), 2);
    
    // Merge overlapping selections
    size_t afterMerge = multiCursor->mergeOverlappingSelections();
    EXPECT_EQ(afterMerge, 1); // Should now have 1 selection after merging
    
    // Check that the merged selection covers the entire range
    selections = multiCursor->getAllSelections();
    EXPECT_EQ(selections.size(), 1);
    EXPECT_EQ(selections[0].start.line, 1);
    EXPECT_EQ(selections[0].start.column, 5);
    EXPECT_EQ(selections[0].end.line, 1);
    EXPECT_EQ(selections[0].end.column, 20);
}

// Main test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 