#include "RequestScopeManager.hpp"
#include "DIFramework.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <vector>

// Simple interface for a counter service
class ICounter {
public:
    virtual ~ICounter() = default;
    virtual int increment() = 0;
    virtual int getValue() const = 0;
};

// A scoped counter implementation - each request gets its own instance
class RequestCounter : public ICounter {
private:
    int count = 0;
    std::string requestId;
public:
    explicit RequestCounter(const std::string& requestId) : requestId(requestId) {
        std::cout << "Creating RequestCounter for request: " << requestId << std::endl;
    }

    ~RequestCounter() {
        std::cout << "Destroying RequestCounter for request: " << requestId << std::endl;
    }

    int increment() override {
        return ++count;
    }

    int getValue() const override {
        return count;
    }
};

// A global counter implementation - shared across all requests
class GlobalCounter : public ICounter {
private:
    std::atomic<int> count{0};
public:
    GlobalCounter() {
        std::cout << "Creating GlobalCounter (singleton)" << std::endl;
    }

    ~GlobalCounter() {
        std::cout << "Destroying GlobalCounter (singleton)" << std::endl;
    }

    int increment() override {
        return ++count;
    }

    int getValue() const override {
        return count.load();
    }
};

// Function to handle a simulated request
void handleRequest(di::RequestScopeManager& scopeManager, const std::string& requestId, bool simulateDelay = false) {
    std::cout << "Handling request: " << requestId << std::endl;
    
    // Create a request context that automatically manages the scope
    di::RequestContext context(scopeManager, requestId);
    
    // Get the request-scoped counter
    auto requestCounter = context.get<ICounter>("RequestCounter");
    
    // Get the singleton counter
    auto globalCounter = context.get<ICounter>("GlobalCounter");
    
    // Increment both counters
    int requestCount = requestCounter->increment();
    int globalCount = globalCounter->increment();
    
    std::cout << "Request " << requestId << ": RequestCounter = " << requestCount 
              << ", GlobalCounter = " << globalCount << std::endl;
    
    // Simulate some work
    if (simulateDelay) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Increment again
    requestCount = requestCounter->increment();
    globalCount = globalCounter->increment();
    
    std::cout << "Request " << requestId << " (after work): RequestCounter = " << requestCount 
              << ", GlobalCounter = " << globalCount << std::endl;
    
    // The scope will be automatically cleaned up when the context is destroyed
}

// Function to simulate concurrent requests
void simulateConcurrentRequests(di::RequestScopeManager& scopeManager, int numRequests) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numRequests; ++i) {
        std::string requestId = "request-" + std::to_string(i);
        threads.emplace_back(handleRequest, std::ref(scopeManager), requestId, true);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Function to simulate a second wave of requests, some reusing IDs
void simulateSecondWaveRequests(di::RequestScopeManager& scopeManager, int numRequests) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numRequests; ++i) {
        // Reuse some request IDs, create new ones for others
        std::string requestId = (i % 2 == 0) ? "request-" + std::to_string(i) : "new-request-" + std::to_string(i);
        threads.emplace_back(handleRequest, std::ref(scopeManager), requestId, false);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Setup the dependency injection container
void setupDIContainer(di::DIFramework& framework) {
    // Register the RequestCounter as a factory that takes the request ID
    framework.registerFactory<ICounter, RequestCounter>("RequestCounter", [](const std::string& requestId) {
        return std::make_shared<RequestCounter>(requestId);
    });
    
    // Register the GlobalCounter as a singleton
    framework.registerSingleton<ICounter, GlobalCounter>("GlobalCounter");
}

// Demonstrate how the scope manager cleans up inactive scopes
void demonstrateScopeCleanup(di::RequestScopeManager& scopeManager) {
    std::cout << "\nDemonstrating automatic scope cleanup...\n" << std::endl;
    
    // Create a few scopes
    {
        di::RequestContext context1(scopeManager, "cleanup-test-1");
        auto counter1 = context1.get<ICounter>("RequestCounter");
        counter1->increment();
        
        // These scopes will be created and then immediately destroyed
        {
            di::RequestContext context2(scopeManager, "cleanup-test-2");
            auto counter2 = context2.get<ICounter>("RequestCounter");
            counter2->increment();
        }
        
        {
            di::RequestContext context3(scopeManager, "cleanup-test-3");
            auto counter3 = context3.get<ICounter>("RequestCounter");
            counter3->increment();
        }
        
        // Wait for the cleanup thread to remove inactive scopes
        std::cout << "Waiting for cleanup thread to remove inactive scopes..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    
    // All scopes should be cleaned up now
    std::cout << "All test scopes should be cleaned up now." << std::endl;
}

int main() {
    try {
        std::cout << "Starting RequestScope Example..." << std::endl;
        
        // Set up the DI container
        auto framework = std::make_shared<di::DIFramework>();
        setupDIContainer(*framework);
        
        // Create the request scope manager with a 2-second timeout for inactive scopes
        di::RequestScopeManager scopeManager(framework, std::chrono::seconds(2));
        
        // Simulate a first wave of concurrent requests
        std::cout << "\nSimulating first wave of concurrent requests...\n" << std::endl;
        simulateConcurrentRequests(scopeManager, 5);
        
        // Wait a moment
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Simulate a second wave of requests
        std::cout << "\nSimulating second wave of requests (some reusing IDs)...\n" << std::endl;
        simulateSecondWaveRequests(scopeManager, 5);
        
        // Demonstrate scope cleanup
        demonstrateScopeCleanup(scopeManager);
        
        std::cout << "\nRequestScope Example completed successfully." << std::endl;
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
} 