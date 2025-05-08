#include <iostream>
#include <string>
#include <vector>
#include "EditorTestableFixed.h"

int main() {
    std::cout << "=== Selection and Clipboard Operations Tests ===" << std::endl;
    std::string output;

    // Test 1: Basic Selection
    std::vector<std::string> basicSelectionTest = {
        "clear",
        "add The quick brown fox jumps over the lazy dog",
        "view",
        "setcursor 0 4", // Position at 'q'
        "cursor",
        "selstart",
        "setcursor 0 9", // Position after 'quick'
        "cursor",
        "selend",
        "selshow",
        "view"
    };

    // Test 2: Copy and Paste
    std::vector<std::string> copyPasteTest = {
        "clear",
        "add First line with text",
        "view",
        "setcursor 0 6", // Position at 'l'
        "cursor",
        "selstart",
        "setcursor 0 10", // Position after 'line'
        "cursor",
        "selend",
        "selshow",
        "copy",
        "setcursor 0 16", // After 'with'
        "cursor",
        "paste",
        "view",
        "cursor"
    };

    // Test 3: Cut and Paste
    std::vector<std::string> cutPasteTest = {
        "clear",
        "add Text to be cut and pasted",
        "view",
        "setcursor 0 5", // At space after 'Text'
        "cursor",
        "selstart",
        "setcursor 0 12", // Position after 'be cut'
        "cursor",
        "selend",
        "selshow",
        "cut",
        "view",
        "cursor",
        "setcursor 0 9", // After 'and'
        "cursor",
        "paste",
        "view",
        "cursor"
    };

    // Test 4: Select Word
    std::vector<std::string> selectWordTest = {
        "clear",
        "add Multiple words for testing selection",
        "view",
        "setcursor 0 5", // Inside 'Multiple'
        "cursor",
        "selword",
        "selshow",
        "view",
        "setcursor 0 15", // Inside 'for'
        "cursor",
        "selword",
        "selshow",
        "view"
    };

    // Test 5: Delete Word
    std::vector<std::string> deleteWordTest = {
        "clear",
        "add Words to delete in this test",
        "view",
        "setcursor 0 0", // At the beginning
        "cursor",
        "delword", // Delete 'Words'
        "view",
        "cursor",
        "setcursor 0 3", // At 'delete'
        "cursor",
        "delword", // Delete 'delete'
        "view",
        "cursor"
    };

    // Test 6: Selection Clear
    std::vector<std::string> selectionClearTest = {
        "clear",
        "add Testing selection clear function",
        "view",
        "setcursor 0 8", // At 'selection'
        "cursor",
        "selstart",
        "setcursor 0 17", // After 'selection'
        "cursor",
        "selend",
        "selshow",
        "selclear",
        "selshow",
        "view",
        "cursor"
    };

    // Test 7: Selection Edge Cases
    std::vector<std::string> selectionEdgeCasesTest = {
        "clear",
        "add First line",
        "add Second line",
        "view",
        // Select across lines
        "setcursor 0 8", // At end of 'First line'
        "cursor",
        "selstart",
        "setcursor 1 5", // Middle of 'Second line'
        "cursor",
        "selend",
        "selshow", // Should show newline
        "view",
        // Empty selection (start = end)
        "setcursor 0 0",
        "cursor",
        "selstart",
        "selend",
        "selshow", // Should show no selection
        "view"
    };

    // Run each test
    std::cout << "\n=== Test 1: Basic Selection ===" << std::endl;
    EditorTestableFixed::runWithInputs(basicSelectionTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 2: Copy and Paste ===" << std::endl;
    EditorTestableFixed::runWithInputs(copyPasteTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 3: Cut and Paste ===" << std::endl;
    EditorTestableFixed::runWithInputs(cutPasteTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 4: Select Word ===" << std::endl;
    EditorTestableFixed::runWithInputs(selectWordTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 5: Delete Word ===" << std::endl;
    EditorTestableFixed::runWithInputs(deleteWordTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 6: Selection Clear ===" << std::endl;
    EditorTestableFixed::runWithInputs(selectionClearTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 7: Selection Edge Cases ===" << std::endl;
    EditorTestableFixed::runWithInputs(selectionEdgeCasesTest, output);
    std::cout << output << std::endl;

    std::cout << "\n=== Selection and Clipboard Tests Complete ===" << std::endl;
    return 0;
} 