#include <gtest/gtest.h>
#include "../src/core/Document.h"
#include <fstream>
#include <filesystem>

using namespace ai_editor;

class DocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        doc = std::make_unique<Document>();
        testFilePath = "test_document.txt";
    }

    void TearDown() override {
        // Clean up test file if it exists
        if (std::filesystem::exists(testFilePath)) {
            std::filesystem::remove(testFilePath);
        }
    }

    std::unique_ptr<Document> doc;
    std::string testFilePath;
};

TEST_F(DocumentTest, NewDocumentIsEmpty) {
    doc->newDocument();
    EXPECT_EQ(doc->getLineCount(), 1);  // Should have one empty line
    EXPECT_TRUE(doc->getLine(0).empty());
}

TEST_F(DocumentTest, InsertText) {
    doc->newDocument();
    EXPECT_TRUE(doc->insertText(0, 0, "Hello"));
    EXPECT_EQ(doc->getLine(0), "Hello");
    
    // Insert in the middle
    EXPECT_TRUE(doc->insertText(0, 2, "LL"));
    EXPECT_EQ(doc->getLine(0), "HeLLllo");
    
    // Insert at end
    EXPECT_TRUE(doc->insertText(0, 7, " World"));
    EXPECT_EQ(doc->getLine(0), "HeLLllo World");
}

TEST_F(DocumentTest, InsertNewLine) {
    doc->newDocument();
    EXPECT_TRUE(doc->insertText(0, 0, "Line 1"));
    EXPECT_TRUE(doc->insertText(0, 6, "\nLine 2"));
    
    EXPECT_EQ(doc->getLineCount(), 2);
    EXPECT_EQ(doc->getLine(0), "Line 1");
    EXPECT_EQ(doc->getLine(1), "Line 2");
}

TEST_F(DocumentTest, DeleteText) {
    doc->newDocument();
    doc->insertText(0, 0, "Hello World");
    
    // Delete from middle
    std::string deleted = doc->deleteText(0, 2, 0, 5);
    EXPECT_EQ(deleted, "llo");
    EXPECT_EQ(doc->getLine(0), "He World");
    
    // Delete across lines
    doc->insertText(0, 2, "llo");
    doc->insertText(0, 8, "\nLine 2");
    deleted = doc->deleteText(0, 7, 1, 2);
    EXPECT_EQ(deleted, "\nLi");
    EXPECT_EQ(doc->getLine(0), "Hello Wne 2");
}

TEST_F(DocumentTest, SaveAndLoadFile) {
    // Create a test document
    doc->newDocument();
    doc->insertText(0, 0, "Line 1\nLine 2\nLine 3");
    
    // Save to file
    EXPECT_TRUE(doc->saveToFile(testFilePath));
    EXPECT_TRUE(std::filesystem::exists(testFilePath));
    
    // Create a new document and load from file
    Document loadedDoc;
    EXPECT_TRUE(loadedDoc.loadFromFile(testFilePath));
    
    // Verify content
    EXPECT_EQ(loadedDoc.getLineCount(), 3);
    EXPECT_EQ(loadedDoc.getLine(0), "Line 1");
    EXPECT_EQ(loadedDoc.getLine(1), "Line 2");
    EXPECT_EQ(loadedDoc.getLine(2), "Line 3");
}

TEST_F(DocumentTest, UndoRedo) {
    doc->newDocument();
    
    // Initial insert
    doc->insertText(0, 0, "Hello");
    EXPECT_EQ(doc->getLine(0), "Hello");
    
    // Undo
    EXPECT_TRUE(doc->undo());
    EXPECT_EQ(doc->getLine(0), "");
    
    // Redo
    EXPECT_TRUE(doc->redo());
    EXPECT_EQ(doc->getLine(0), "Hello");
    
    // Insert more text
    doc->insertText(0, 5, " World");
    EXPECT_EQ(doc->getLine(0), "Hello World");
    
    // Undo should go back to just "Hello"
    EXPECT_TRUE(doc->undo());
    EXPECT_EQ(doc->getLine(0), "Hello");
}
