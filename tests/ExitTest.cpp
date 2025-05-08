#include <iostream>
#include <string>
#include <vector>
#include "EditorTestable.h"

int main() {
    std::cout << "=== Testing Exit Commands ===" << std::endl;
    std::string output;

    // Test 1: "quit" command
    std::vector<std::string> quitTest = {
        "add This is a test line",
        "view",
        "quit",
        "add This should not be executed" // This should not be executed
    };

    // Test 2: "exit" command
    std::vector<std::string> exitTest = {
        "add This is a test line",
        "view",
        "exit",
        "add This should not be executed" // This should not be executed
    };

    // Run each test
    std::cout << "\n=== Test 1: 'quit' Command ===" << std::endl;
    EditorTestable::runWithInputs(quitTest, output);
    std::cout << output << std::endl;

    output.clear();
    std::cout << "\n=== Test 2: 'exit' Command ===" << std::endl;
    EditorTestable::runWithInputs(exitTest, output);
    std::cout << output << std::endl;

    std::cout << "\n=== Exit Command Tests Complete ===" << std::endl;
    return 0;
} 