#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockUtils.h"
#include "TestDeclarations.h"
#include "../src/SyntaxHighlightingManager.h"
#include "../src/EditorError.h"
#include <iostream>
#include <fstream>
#include <string>

// Custom null buffer that discards all output
class NullBuffer : public std::streambuf {
public:
    int overflow(int c) override { return c; }
};

// Global null buffer instance - must be declared here
static NullBuffer null_buffer;

// Store the original std::cerr buffer
static std::streambuf* original_cerr_buffer = nullptr;

// Function to globally redirect std::cerr to the null buffer
void suppressAllConsoleOutput() {
    // Save the original buffer
    original_cerr_buffer = std::cerr.rdbuf();
    
    // Redirect std::cerr to the null buffer
    std::cerr.rdbuf(&null_buffer);
}

// Function to restore the original std::cerr buffer
void restoreConsoleOutput() {
    if (original_cerr_buffer) {
        std::cerr.rdbuf(original_cerr_buffer);
    }
}

int main(int argc, char **argv) {
    // Initialize the Google Test framework
    ::testing::InitGoogleTest(&argc, argv);
    
    // Configure error reporting and logging for tests
    
    // Disable ALL logging for tests - prevents excessive output
    DISABLE_ALL_LOGGING_FOR_TESTS = true;
    
    // For any logging that still happens, set a high severity threshold 
    // to avoid cluttering test output with debug/warning messages
    ErrorReporter::debugLoggingEnabled = false;
    ErrorReporter::suppressAllWarnings = true;
    ErrorReporter::setSeverityThreshold(EditorException::Severity::EDITOR_ERROR);
    
    // Silence cout output during tests to keep output clean
    // This redirects std::cout to a null stream for the duration of the tests
    std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
    std::ostringstream strCout;
    std::cout.rdbuf(strCout.rdbuf());
    
    // Run all the tests
    int result = RUN_ALL_TESTS();
    
    // Restore cout for normal output after tests
    std::cout.rdbuf(oldCoutStreamBuf);
    
    return result;
} 