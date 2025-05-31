#pragma once

#include <memory>
#include <string>
#include <atomic>
#include "../../src/di/LifetimeManager.hpp"
#include "../../src/di/CoreModule.hpp"
#include <iostream>

namespace di {
namespace test {

// Define the IGreeter interface for testing
class IGreeter {
public:
    virtual ~IGreeter() = default;
    virtual std::string greet(const std::string& name) const = 0;
};

// A simple greeter implementation
class SimpleGreeter : public IGreeter {
public:
    SimpleGreeter(std::shared_ptr<ISimpleLogger> logger) : logger_(logger) {
        logger_->log("SimpleGreeter created");
    }

    std::string greet(const std::string& name) const override {
        logger_->log("Greeting: " + name);
        return "Hello, " + name + "!";
    }

private:
    std::shared_ptr<ISimpleLogger> logger_;
};

// A disposable service for testing
class DisposableService : public lifetime::IDisposable {
public:
    DisposableService() {
        instanceCount_++;
        std::cout << "DisposableService constructed, instance: " << this 
                  << ", count: " << instanceCount_ << std::endl;
    }
    
    ~DisposableService() {
        std::cout << "DisposableService destructor called, instance: " << this
                  << ", disposed: " << (disposed_ ? "true" : "false")
                  << ", count before: " << instanceCount_ << std::endl;
        
        // Only decrement if not already disposed to avoid double counting
        if (!disposed_) {
            instanceCount_--;
        }
    }
    
    void dispose() override {
        std::cout << "DisposableService::dispose() called, instance: " << this 
                  << ", already disposed: " << (disposed_ ? "true" : "false")
                  << ", count before: " << instanceCount_ << std::endl;
                  
        if (!disposed_) {
            disposed_ = true;
            instanceCount_--;
            std::cout << "DisposableService marked as disposed, new count: " << instanceCount_ << std::endl;
        }
    }
    
    bool isDisposed() const {
        return disposed_;
    }
    
    static int getInstanceCount() {
        return instanceCount_;
    }
    
    static void resetInstanceCount() {
        instanceCount_ = 0;
    }

private:
    bool disposed_ = false;
    static std::atomic<int> instanceCount_;
};

// Initialize the static atomic variable
std::atomic<int> DisposableService::instanceCount_(0);

} // namespace test
} // namespace di 