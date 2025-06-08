#pragma once

#include "../src/EditorErrorReporter.h"

// Mock implementation of IErrorReporter for testing
class MockErrorReporter : public IErrorReporter {
public:
    void reportError(const std::string& source, const std::string& message, int level) override {
        lastError_ = message;
        errorSource_ = source;
        errorLevel_ = level;
        errorReported_ = true;
    }
    
    void reportError(const std::string& source, const std::string& message, const std::string& details) override {
        lastError_ = message + ": " + details;
        errorSource_ = source;
        errorReported_ = true;
    }
    
    bool wasErrorReported() const { return errorReported_; }
    const std::string& getLastError() const { return lastError_; }
    const std::string& getLastErrorSource() const { return errorSource_; }
    int getLastErrorLevel() const { return errorLevel_; }
    
    void reset() {
        errorReported_ = false;
        lastError_.clear();
        errorSource_.clear();
        errorLevel_ = 0;
    }
    
private:
    bool errorReported_ = false;
    std::string lastError_;
    std::string errorSource_;
    int errorLevel_ = 0;
};
