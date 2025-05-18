# Testing Guide

This guide covers how to run existing tests and write new tests for the AI-First TextEditor.

## Running Tests

### Run All Tests

```bash
# Build and run all tests
cd build_vs
cmake --build . --target RunAllTests
.\tests\RunAllTests.exe
```

### Run Specific Test Suites

```bash
# Run all tests in a specific test suite
.\tests\RunAllTests.exe --gtest_filter=OpenAIApiEndpointsTest*

# Run a specific test
.\tests\RunAllTests.exe --gtest_filter=OpenAIApiEndpointsTest.HandleAuthenticationErrors
```

### Run Individual Test Executables

```bash
# Run a specific test executable
.\tests\OpenAIApiEndpointsTest.exe

# Run with XML output (useful for CI)
.\tests\OpenAIApiEndpointsTest.exe --gtest_output=xml:test_results.xml
```

## Writing Tests

Tests are written using the GoogleTest framework. Here's a basic example:

```cpp
#include <gtest/gtest.h>
#include "../src/ComponentToTest.h"

// Test fixture (optional)
class ComponentTest : public ::testing::Test {
protected:
    // Set up code that runs before each test
    void SetUp() override {
        // Initialize components
    }
    
    // Clean-up code that runs after each test
    void TearDown() override {
        // Clean up resources
    }
    
    // Components used in tests
    ComponentToTest component;
};

// Basic test using the fixture
TEST_F(ComponentTest, FunctionNameReturnsExpectedValue) {
    // Arrange
    // (Setup is done in SetUp())
    
    // Act
    bool result = component.functionName();
    
    // Assert
    EXPECT_TRUE(result);
}

// Simple test without fixture
TEST(StandaloneTest, SimpleAssertion) {
    // Arrange
    int value = 42;
    
    // Act
    int result = value * 2;
    
    // Assert
    EXPECT_EQ(84, result);
}
```

## Using Test Mocks

### Mocking OpenAI API

The `MockOpenAI_API_Client` allows testing components that use the OpenAI API without making actual API calls.

```cpp
// Example of mocking API responses
MockOpenAI_API_Client mockClient;

// Create a mock response
ApiResponse mockResponse;
mockResponse.success = true;
mockResponse.content = "Test response content";

// Add a tool call to the response
ApiToolCall toolCall;
toolCall.id = "call_123";
toolCall.function.name = "write_file_content";
toolCall.function.arguments = R"({"filename": "test.cpp", "content": "int main() { return 0; }"})";
mockResponse.tool_calls.push_back(toolCall);

// Prime the mock client to return this response
mockClient.primeResponse(mockResponse);

// Use the mock client in your component
ComponentUsingApi component(mockClient);
component.performAction();  // Will use the mocked response
```

### Mocking WorkspaceManager

The `MockWorkspaceManager` simulates file operations without actually writing to disk:

```cpp
// Create a mock workspace manager
MockWorkspaceManager mockWorkspace("test_workspace");

// Use in tests
AIAgentOrchestrator orchestrator(mockApiClient, uiModel, mockWorkspace);
orchestrator.handleSubmitUserPrompt("Create a project");

// Verify file operations
EXPECT_TRUE(mockWorkspace.fileExists("main.cpp"));
```

## Test Coverage

To generate test coverage reports:

```bash
# Build with coverage enabled (GCC/Clang)
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make

# Run the tests
./tests/RunAllTests

# Generate coverage report with lcov
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

## Troubleshooting Tests

### Common Issues

1. **Test failures due to unchanged cache:**
   ```bash
   # Rebuild completely
   rm -rf build && mkdir build && cd build
   cmake .. && make
   ```

2. **Failed assertions in virtual method tests:**
   Ensure the base class methods are declared virtual with proper override specifiers.

3. **OpenAI API test failures:**
   Check if the `.env` file is properly set up in the test directory.

4. **Timeout in tests:**
   Add the following to increase timeout:
   ```cpp
   TEST(LongRunningTest, Example) {
       testing::FLAGS_gtest_death_test_style = "threadsafe";
       // Your test code
   }
   ```

### Debugging Tests

For Visual Studio:
1. Set `RunAllTests` as the startup project
2. Set breakpoints in your test code
3. Run with the debugger (F5)

For command line:
```bash
# Run with verbose output
.\tests\RunAllTests.exe --gtest_filter=FailingTest* --gtest_also_run_disabled_tests --gtest_print_time
``` 