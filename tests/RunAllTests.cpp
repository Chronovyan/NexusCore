#define RUN_ALL_TESTS_INCLUDE
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
    // Set up all flags to disable logging
    DISABLE_ALL_LOGGING_FOR_TESTS = true;
    ErrorReporter::debugLoggingEnabled = false;
    ErrorReporter::suppressAllWarnings = true;
    ErrorReporter::setSeverityThreshold(EditorException::Severity::Error);
    
    // Output settings to stdout (not cerr which we'll redirect)
    std::cout << "\n[INFO] Starting tests with the following settings:" << std::endl;
    std::cout << "  * DISABLE_ALL_LOGGING_FOR_TESTS = " 
              << (DISABLE_ALL_LOGGING_FOR_TESTS ? "true" : "false") << std::endl;
    std::cout << "  * ErrorReporter::suppressAllWarnings = " 
              << (ErrorReporter::suppressAllWarnings ? "true" : "false") << std::endl;
    std::cout << "  * ErrorReporter::debugLoggingEnabled = " 
              << (ErrorReporter::debugLoggingEnabled ? "true" : "false") << std::endl;
    std::cout << "  * ErrorReporter::severityThreshold = " 
              << static_cast<int>(ErrorReporter::severityThreshold) << std::endl;
    std::cout << "  * SyntaxHighlightingManager::debugLoggingEnabled = " 
              << (SyntaxHighlightingManager::isDebugLoggingEnabled() ? "true" : "false") << std::endl;
    
    // CRITICAL: Redirect stderr output
    std::cout << "[INFO] Redirecting std::cerr to NULL - all warnings will be suppressed" << std::endl;
    suppressAllConsoleOutput();
    
    // Run the tests
    std::cout << "[INFO] Running tests..." << std::endl;
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    
    // Restore stdout after tests have completed
    restoreConsoleOutput();
    
    std::cout << "[INFO] Test run completed - console output restored." << std::endl;
    return ret;
} 