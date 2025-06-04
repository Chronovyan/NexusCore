# Troubleshooting Guide

This guide covers common issues and their solutions for the AI-First TextEditor.

## Build Issues

### CMake Configuration Errors

```
CMake Error: The following variables are used in this project, but they are set to NOTFOUND
```

**Solution:** Install required dependencies or ensure they're in your PATH.

```bash
# For Ubuntu/Debian
sudo apt-get install build-essential cmake libssl-dev

# For macOS
brew install cmake openssl
```

### Compilation Errors

```
error: 'filesystem' is not a member of 'std'
```

**Solution:** Ensure you're using C++17 or higher.

```bash
# CMake command with explicit C++17 flag
cmake -DCMAKE_CXX_STANDARD=17 ..
```

### Linker Errors

```
undefined reference to `cpr::Session::Session()'
```

**Solution:** Check if dependencies are properly built.

```bash
# Clean build and rebuild
rm -rf build && mkdir build && cd build
cmake .. && make
```

## Runtime Issues

### API Key Not Found

```
Error: Missing API key in environment variables or .env file
```

**Solution:** Create a `.env` file in the executable directory with your OpenAI API key.

```
OPENAI_API_KEY=your_api_key_here
```

### API Request Failures

```
Error: OpenAI API request failed: 401 Unauthorized
```

**Solution:** Check your API key and ensure it's valid.

```bash
# Test your API key with curl
curl -H "Authorization: Bearer your_api_key_here" https://api.openai.com/v1/models
```

### File Operation Errors

```
Error: Failed to write file: Permission denied
```

**Solution:** Ensure your application has write permissions to the workspace directory.

```bash
# Change permissions (Linux/macOS)
chmod -R 755 ./projects/my_project
```

## Testing Issues

### Test Failures

```
Value of: workspaceManager->fileExists("main.cpp")
  Actual: false
Expected: true
```

**Solution:** Ensure mock objects are correctly set up and methods are virtual in base classes.

```cpp
// Ensure methods are virtual in base class
class WorkspaceManager {
public:
    virtual bool fileExists(const std::string& filename) const;
};

// Properly override in mock
class MockWorkspaceManager : public WorkspaceManager {
public:
    bool fileExists(const std::string& filename) const override;
};
```

### Timeout Issues

```
Test timeout exceeded: test took too long to complete
```

**Solution:** Increase test timeout or optimize test execution.

```cpp
// Add in your test file
testing::GTEST_FLAG(timeout) = 120; // 120 seconds timeout
```

## Performance Issues

### Slow AI Response

**Solution:** Configure the API client for better performance.

```cpp
// Reduce the complexity of the request
apiClient.setTemperature(0.0); // More deterministic
apiClient.setMaxTokens(1000);  // Shorter response
```

### High Memory Usage

**Solution:** Optimize buffer management and limit history size.

```cpp
// In your application configuration
textBuffer.setMaxHistorySize(50); // Limit undo history
uiModel.setMaxChatHistorySize(100); // Limit chat history
```

## Debugging Tips

### Enable Debug Logging

```cpp
// Enable debug logging
ErrorReporter::enableDebugLogging();
```

### Check HTTP Request/Response

```cpp
// Inspect raw HTTP traffic (add before API calls)
apiClient.setVerboseLogging(true);
```

### Debug Test Failures

```bash
# Run with verbose output
.\tests\RunAllTests.exe --gtest_filter=FailingTest* --gtest_also_run_disabled_tests --gtest_print_time
```

### Common Error Messages and Solutions

| Error Message | Possible Cause | Solution |
|---------------|----------------|----------|
| `Failed to load OpenGL` | Missing graphics drivers | Update graphics drivers |
| `Failed to read .env file` | Missing or incorrectly formatted file | Check file permissions and format |
| `Error 429: Rate limit exceeded` | Too many API requests | Implement backoff in your code or reduce request frequency |
| `JSON parse error` | Malformed JSON in API response | Check your API request format |
| `Assertion failed: buffer_ != nullptr` | Memory allocation failure | Check system resources | 