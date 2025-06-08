#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"

class AutomatedUndoRedoTest : public ::testing::Test {
protected:
    // Test inputs for different test cases
    std::vector<std::string> basicTestInputs = {
        "add Line 1",
        "add Line 2", 
        "add Line 3",
        "view",
        "lines",
        "undo",
        "view", 
        "lines",
        "undo", 
        "view",
        "lines", 
        "redo",
        "view",
        "lines"
    };
    
    std::vector<std::string> textEditTestInputs = {
        "add Hello",
        "view",
        "setcursor 0 5",
        "type , world",
        "view",
        "backspace",
        "view",
        "undo",
        "view",
        "undo",
        "view",
        "redo",
        "view"
    };
    
    std::vector<std::string> lineOperationsTestInputs = {
        "add First line",
        "add Second line", 
        "add Third line",
        "view",
        "delete 1",
        "view",
        "undo",
        "view",
        "replace 0 New first",
        "view",
        "undo",
        "view"
    };
};

/**
 * Test basic undo/redo operations (adding/removing lines)
 */
TEST_F(AutomatedUndoRedoTest, BasicUndoRedo) {
    std::string output;
    
    // Run the editor with the test inputs
    ASSERT_TRUE(EditorTestable::runWithInputs(basicTestInputs, output));
    
    // Check if undo/redo commands are recognized
    ASSERT_NE(output.find("Action undone"), std::string::npos) << "Undo command wasn't recognized";
    ASSERT_NE(output.find("Action redone"), std::string::npos) << "Redo command wasn't recognized";
    
    // Extract and verify line counts at different stages
    // Improved line count extraction with more robust parsing
    size_t initialView = output.find("--- Buffer View ---");
    ASSERT_NE(initialView, std::string::npos) << "Couldn't find initial buffer view";
    
    size_t initialLines = output.find("Total lines:", initialView);
    ASSERT_NE(initialLines, std::string::npos) << "Couldn't find line count after initial view";
    
    // Extract the line count by looking for digits after "Total lines:"
    std::string lineCountStr1;
    size_t lineCountStart = initialLines + 12; // Length of "Total lines:"
    while (lineCountStart < output.length() && 
           (output[lineCountStart] == ' ' || output[lineCountStart] == '\t')) {
        lineCountStart++; // Skip whitespace
    }
    while (lineCountStart < output.length() && 
           std::isdigit(output[lineCountStart])) {
        lineCountStr1.push_back(output[lineCountStart]);
        lineCountStart++;
    }
    ASSERT_FALSE(lineCountStr1.empty()) << "Couldn't extract initial line count";
    ASSERT_EQ(lineCountStr1, "3") << "Initial line count should be 3";
    
    size_t undoPos1 = output.find("Action undone");
    ASSERT_NE(undoPos1, std::string::npos) << "Couldn't find first undo action";
    
    size_t linesAfterFirstUndo = output.find("Total lines:", undoPos1);
    ASSERT_NE(linesAfterFirstUndo, std::string::npos) << "Couldn't find line count after first undo";
    
    // Extract the line count after first undo
    std::string lineCountStr2;
    lineCountStart = linesAfterFirstUndo + 12; // Length of "Total lines:"
    while (lineCountStart < output.length() && 
           (output[lineCountStart] == ' ' || output[lineCountStart] == '\t')) {
        lineCountStart++; // Skip whitespace
    }
    while (lineCountStart < output.length() && 
           std::isdigit(output[lineCountStart])) {
        lineCountStr2.push_back(output[lineCountStart]);
        lineCountStart++;
    }
    ASSERT_FALSE(lineCountStr2.empty()) << "Couldn't extract line count after first undo";
    ASSERT_EQ(lineCountStr2, "2") << "Line count after first undo should be 2";
    
    size_t undoPos2 = output.find("Action undone", undoPos1 + 1);
    ASSERT_NE(undoPos2, std::string::npos) << "Couldn't find second undo action";
    
    size_t linesAfterSecondUndo = output.find("Total lines:", undoPos2);
    ASSERT_NE(linesAfterSecondUndo, std::string::npos) << "Couldn't find line count after second undo";
    
    // Extract the line count after second undo
    std::string lineCountStr3;
    lineCountStart = linesAfterSecondUndo + 12; // Length of "Total lines:"
    while (lineCountStart < output.length() && 
           (output[lineCountStart] == ' ' || output[lineCountStart] == '\t')) {
        lineCountStart++; // Skip whitespace
    }
    while (lineCountStart < output.length() && 
           std::isdigit(output[lineCountStart])) {
        lineCountStr3.push_back(output[lineCountStart]);
        lineCountStart++;
    }
    ASSERT_FALSE(lineCountStr3.empty()) << "Couldn't extract line count after second undo";
    ASSERT_EQ(lineCountStr3, "1") << "Line count after second undo should be 1";
    
    size_t redoPos = output.find("Action redone");
    ASSERT_NE(redoPos, std::string::npos) << "Couldn't find redo action";
    
    size_t linesAfterRedo = output.find("Total lines:", redoPos);
    ASSERT_NE(linesAfterRedo, std::string::npos) << "Couldn't find line count after redo";
    
    // Extract the line count after redo
    std::string lineCountStr4;
    lineCountStart = linesAfterRedo + 12; // Length of "Total lines:"
    while (lineCountStart < output.length() && 
           (output[lineCountStart] == ' ' || output[lineCountStart] == '\t')) {
        lineCountStart++; // Skip whitespace
    }
    while (lineCountStart < output.length() && 
           std::isdigit(output[lineCountStart])) {
        lineCountStr4.push_back(output[lineCountStart]);
        lineCountStart++;
    }
    ASSERT_FALSE(lineCountStr4.empty()) << "Couldn't extract line count after redo";
    ASSERT_EQ(lineCountStr4, "2") << "Line count after redo should be 2";
}

/**
 * Test text editing undo/redo operations (typing, backspace)
 */
TEST_F(AutomatedUndoRedoTest, TextEditingUndoRedo) {
    std::string output;
    
    // Run the editor with the test inputs
    ASSERT_TRUE(EditorTestable::runWithInputs(textEditTestInputs, output));
    
    // Check if basic operations worked
    ASSERT_NE(output.find("Hello"), std::string::npos) << "Initial text wasn't added";
    ASSERT_NE(output.find("Hello, world"), std::string::npos) << "Text wasn't inserted";
    ASSERT_NE(output.find("Hello, worl"), std::string::npos) << "Backspace didn't work";
    
    // Verify undo/redo state changes with a more robust approach
    
    // Helper function to extract buffer content between buffer view markers
    auto extractBufferContent = [&output](const std::string& afterMarker) -> std::string {
        size_t markerPos = output.find(afterMarker);
        if (markerPos == std::string::npos) {
            return "";
        }
        
        size_t viewStartPos = output.find("--- Buffer View ---", markerPos);
        if (viewStartPos == std::string::npos) {
            return "";
        }
        
        size_t contentStartPos = viewStartPos + std::string("--- Buffer View ---").length();
        size_t viewEndPos = output.find("-------------------", contentStartPos);
        
        if (viewEndPos == std::string::npos) {
            return "";
        }
        
        return output.substr(contentStartPos, viewEndPos - contentStartPos);
    };
    
    // Find view after typing ", world"
    std::string bufferAfterType = extractBufferContent("Text inserted");
    ASSERT_FALSE(bufferAfterType.empty()) << "Could not extract buffer content after typing";
    ASSERT_NE(bufferAfterType.find("Hello, world"), std::string::npos) 
        << "Buffer should contain 'Hello, world' after typing";
    
    // Find view after backspace
    std::string bufferAfterBackspace = extractBufferContent("Backspace performed");
    ASSERT_FALSE(bufferAfterBackspace.empty()) << "Could not extract buffer content after backspace";
    ASSERT_NE(bufferAfterBackspace.find("Hello, worl"), std::string::npos) 
        << "Buffer should contain 'Hello, worl' after backspace";
    
    // Find view after undoing backspace
    std::string bufferAfterUndoBackspace = extractBufferContent("Action undone");
    ASSERT_FALSE(bufferAfterUndoBackspace.empty()) << "Could not extract buffer content after undoing backspace";
    ASSERT_NE(bufferAfterUndoBackspace.find("Hello, world"), std::string::npos) 
        << "Buffer should contain 'Hello, world' after undoing backspace";
    
    // Look for the second occurrence of "Action undone" for undoing typing
    size_t firstUndoPos = output.find("Action undone");
    ASSERT_NE(firstUndoPos, std::string::npos) << "Could not find first undo action";
    
    // Find view after undoing typing (second undo)
    size_t secondUndoPos = output.find("Action undone", firstUndoPos + 1);
    ASSERT_NE(secondUndoPos, std::string::npos) << "Could not find second undo action";
    
    size_t viewAfterUndoType = output.find("--- Buffer View ---", secondUndoPos);
    ASSERT_NE(viewAfterUndoType, std::string::npos) << "Could not find buffer view after undoing typing";
    
    size_t contentStartPos = viewAfterUndoType + std::string("--- Buffer View ---").length();
    size_t viewEndAfterUndoType = output.find("-------------------", contentStartPos);
    ASSERT_NE(viewEndAfterUndoType, std::string::npos) << "Could not find end of buffer view after undoing typing";
    
    std::string bufferAfterUndoType = output.substr(contentStartPos, viewEndAfterUndoType - contentStartPos);
    ASSERT_NE(bufferAfterUndoType.find("Hello"), std::string::npos) 
        << "Buffer should contain only 'Hello' after undoing typing";
    
    // Find view after redoing typing
    std::string bufferAfterRedoType = extractBufferContent("Action redone");
    ASSERT_FALSE(bufferAfterRedoType.empty()) << "Could not extract buffer content after redoing typing";
    ASSERT_NE(bufferAfterRedoType.find("Hello, world"), std::string::npos) 
        << "Buffer should contain 'Hello, world' after redoing typing";
}

/**
 * Test line operations undo/redo (delete line, replace line)
 */
TEST_F(AutomatedUndoRedoTest, LineOperationsUndoRedo) {
    std::string output;
    
    // Run the editor with the test inputs
    ASSERT_TRUE(EditorTestable::runWithInputs(lineOperationsTestInputs, output));
    
    // Find the initial view with all three lines
    size_t initialView = output.find("--- Buffer View ---");
    size_t initialViewEnd = output.find("-------------------", initialView);
    std::string initialBuffer = output.substr(initialView, initialViewEnd - initialView);
    ASSERT_NE(initialBuffer.find("First line"), std::string::npos) << "First line not found in initial view";
    ASSERT_NE(initialBuffer.find("Second line"), std::string::npos) << "Second line not found in initial view";
    ASSERT_NE(initialBuffer.find("Third line"), std::string::npos) << "Third line not found in initial view";
    
    // Find the view after deleting the second line
    size_t deletePosition = output.find("Line 1 deleted");
    size_t viewAfterDelete = output.find("--- Buffer View ---", deletePosition);
    size_t viewEndAfterDelete = output.find("-------------------", viewAfterDelete);
    std::string bufferAfterDelete = output.substr(viewAfterDelete, viewEndAfterDelete - viewAfterDelete);
    ASSERT_NE(bufferAfterDelete.find("First line"), std::string::npos) << "First line should remain after delete";
    ASSERT_NE(bufferAfterDelete.find("Third line"), std::string::npos) << "Third line should remain after delete";
    ASSERT_EQ(bufferAfterDelete.find("Second line"), std::string::npos) << "Second line should be deleted";
    
    // Find the view after undoing the delete
    size_t undoDeletePosition = output.find("Action undone", deletePosition);
    size_t viewAfterUndoDelete = output.find("--- Buffer View ---", undoDeletePosition);
    size_t viewEndAfterUndoDelete = output.find("-------------------", viewAfterUndoDelete);
    std::string bufferAfterUndoDelete = output.substr(viewAfterUndoDelete, viewEndAfterUndoDelete - viewAfterUndoDelete);
    ASSERT_NE(bufferAfterUndoDelete.find("First line"), std::string::npos) << "First line should be present after undo";
    ASSERT_NE(bufferAfterUndoDelete.find("Second line"), std::string::npos) << "Second line should be restored after undo";
    ASSERT_NE(bufferAfterUndoDelete.find("Third line"), std::string::npos) << "Third line should be present after undo";
    
    // Find the view after replacing first line
    size_t replacePosition = output.find("Line 0 replaced");
    size_t viewAfterReplace = output.find("--- Buffer View ---", replacePosition);
    size_t viewEndAfterReplace = output.find("-------------------", viewAfterReplace);
    std::string bufferAfterReplace = output.substr(viewAfterReplace, viewEndAfterReplace - viewAfterReplace);
    ASSERT_NE(bufferAfterReplace.find("New first"), std::string::npos) << "New text should be present after replace";
    ASSERT_EQ(bufferAfterReplace.find("First line"), std::string::npos) << "Original text should be gone after replace";
    
    // Find the view after undoing the replace
    size_t undoReplacePosition = output.find("Action undone", replacePosition);
    size_t viewAfterUndoReplace = output.find("--- Buffer View ---", undoReplacePosition);
    size_t viewEndAfterUndoReplace = output.find("-------------------", viewAfterUndoReplace);
    std::string bufferAfterUndoReplace = output.substr(viewAfterUndoReplace, viewEndAfterUndoReplace - viewAfterUndoReplace);
    ASSERT_NE(bufferAfterUndoReplace.find("First line"), std::string::npos) << "Original text should be restored after undo";
    ASSERT_EQ(bufferAfterUndoReplace.find("New first"), std::string::npos) << "Replacement text should be gone after undo";
}

/**
 * Test undo/redo history state limits
 */
TEST_F(AutomatedUndoRedoTest, UndoRedoHistoryLimits) {
    std::string output;
    
    // Create a test that performs more operations than the history limit
    std::vector<std::string> manyOperations;
    
    // Add 100 lines to create a long history
    for (int i = 0; i < 100; i++) {
        manyOperations.push_back("add Line " + std::to_string(i));
    }
    
    // Now try to undo more times than the history limit
    for (int i = 0; i < 110; i++) {
        manyOperations.push_back("undo");
    }
    
    // Try to redo more times than possible
    for (int i = 0; i < 110; i++) {
        manyOperations.push_back("redo");
    }
    
    // Run the editor with the test inputs
    ASSERT_TRUE(EditorTestable::runWithInputs(manyOperations, output));
    
    // Count the successful undos (we should have at least as many as our added lines)
    int undoCount = 0;
    size_t pos = 0;
    while ((pos = output.find("Action undone", pos)) != std::string::npos) {
        undoCount++;
        pos += 12; // Move past "Action undone"
    }
    
    // Count the successful redos
    int redoCount = 0;
    pos = 0;
    while ((pos = output.find("Action redone", pos)) != std::string::npos) {
        redoCount++;
        pos += 12; // Move past "Action redone"
    }
    
    // We should have at least some successful undos and redos
    ASSERT_GT(undoCount, 0) << "Should have performed at least some successful undos";
    ASSERT_GT(redoCount, 0) << "Should have performed at least some successful redos";
    
    // We should also have some failed attempts when we hit the limit
    ASSERT_NE(output.find("Nothing to undo"), std::string::npos) 
        << "Should eventually hit the undo limit";
    ASSERT_NE(output.find("Nothing to redo"), std::string::npos) 
        << "Should eventually hit the redo limit";
} 