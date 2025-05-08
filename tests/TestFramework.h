#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

// Test result structure
struct TestResult {
    bool passed;
    std::string message;
    
    TestResult(bool p, const std::string& msg) : passed(p), message(msg) {}
};

// Simple test framework class
class TestFramework {
public:
    // Register a test function with a name
    void registerTest(const std::string& name, std::function<TestResult()> testFunc) {
        tests_.push_back({name, testFunc});
    }
    
    // Run all registered tests
    void runAllTests() {
        size_t passed = 0;
        size_t total = tests_.size();
        
        std::cout << "Running " << total << " tests...\n";
        std::cout << "=============================================\n";
        
        for (const auto& test : tests_) {
            std::cout << "Test: " << test.name << "... ";
            TestResult result = test.testFunc();
            
            if (result.passed) {
                std::cout << "PASSED\n";
                passed++;
            } else {
                std::cout << "FAILED\n";
                std::cout << "  Reason: " << result.message << "\n";
            }
        }
        
        std::cout << "=============================================\n";
        std::cout << "Results: " << passed << " of " << total << " tests passed.\n";
    }
    
private:
    struct TestCase {
        std::string name;
        std::function<TestResult()> testFunc;
    };
    
    std::vector<TestCase> tests_;
};

// Input and output redirection utilities
class IORedirector {
public:
    // Set up redirection
    IORedirector() {
        // Save original buffers
        oldCin_ = std::cin.rdbuf();
        oldCout_ = std::cout.rdbuf();
        
        // Redirect to our string streams
        std::cin.rdbuf(inStream_.rdbuf());
        std::cout.rdbuf(outStream_.rdbuf());
    }
    
    // Restore original I/O
    ~IORedirector() {
        std::cin.rdbuf(oldCin_);
        std::cout.rdbuf(oldCout_);
    }
    
    // Add a line of input that will be provided to cin
    void addInput(const std::string& input) {
        inStream_ << input << '\n';
    }
    
    // Add multiple lines of input
    void addInputs(const std::vector<std::string>& inputs) {
        for (const auto& input : inputs) {
            addInput(input);
        }
    }
    
    // Get all output that was sent to cout
    std::string getOutput() const {
        return outStream_.str();
    }
    
    // Clear the output buffer
    void clearOutput() {
        outStream_.str("");
        outStream_.clear();
    }
    
private:
    std::stringstream inStream_;
    std::stringstream outStream_;
    std::streambuf* oldCin_;
    std::streambuf* oldCout_;
};

// Helper functions for assertions
namespace TestAssert {
    // Assert that two values are equal
    template<typename T>
    bool areEqual(const T& expected, const T& actual, std::string& message) {
        if (expected != actual) {
            std::stringstream ss;
            ss << "Expected: " << expected << ", Actual: " << actual;
            message = ss.str();
            return false;
        }
        return true;
    }
    
    // Assert that a string contains a substring
    bool stringContains(const std::string& str, const std::string& substr, std::string& message) {
        if (str.find(substr) == std::string::npos) {
            std::stringstream ss;
            ss << "String does not contain expected substring '" << substr << "'";
            message = ss.str();
            return false;
        }
        return true;
    }
}

#endif // TEST_FRAMEWORK_H