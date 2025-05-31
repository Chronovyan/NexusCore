#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include "../src/di/LifetimeManager.hpp"

using namespace di::lifetime;

// Test fixture
class LifetimeManagerFixedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any global state
    }
};

// Simple disposable service
class DisposableTestService : public IDisposable {
public:
    DisposableTestService() {
        std::cout << "DisposableTestService constructed: " << this << std::endl;
    }
    
    ~DisposableTestService() {
        std::cout << "DisposableTestService destroyed: " << this << " (disposed=" << disposed_ << ")" << std::endl;
    }
    
    void dispose() override {
        std::cout << "DisposableTestService::dispose called: " << this << std::endl;
        disposed_ = true;
    }
    
    bool isDisposed() const {
        return disposed_;
    }
    
private:
    bool disposed_ = false;
};

// A service that requires a logger
class GreeterService {
public:
    GreeterService() {
        std::cout << "GreeterService constructed: " << this << std::endl;
    }
    
    ~GreeterService() {
        std::cout << "GreeterService destroyed: " << this << std::endl;
    }
    
    std::string greet() {
        return "Hello, world!";
    }
};

// Test scope creation and disposal
TEST_F(LifetimeManagerFixedTest, ScopedServicesDisposal) {
    // Create a parent lifetime manager
    LifetimeManager parentManager;
    
    // Function to create a service
    auto createService = []() {
        return std::make_shared<DisposableTestService>();
    };
    
    // Get a scoped service from the parent manager
    auto parentService = parentManager.getInstance<DisposableTestService>(createService, ServiceLifetime::Scoped);
    EXPECT_FALSE(parentService->isDisposed());
    
    // Create a child scope
    auto childScope = parentManager.createScope();
    
    // Get a scoped service from the child scope
    auto childService = childScope->getInstance<DisposableTestService>(createService, ServiceLifetime::Scoped);
    EXPECT_FALSE(childService->isDisposed());
    
    // Dispose the child scope
    childScope->dispose();
    
    // Verify that the child service was disposed
    EXPECT_TRUE(childService->isDisposed());
    
    // Verify that the parent service is still not disposed
    EXPECT_FALSE(parentService->isDisposed());
    
    // Dispose the parent scope
    parentManager.dispose();
    
    // Verify that the parent service was disposed
    EXPECT_TRUE(parentService->isDisposed());
}

// Test LifetimeInjector scope creation
TEST_F(LifetimeManagerFixedTest, LifetimeInjectorScopedServicesDisposal) {
    // Create a parent injector
    LifetimeInjector parentInjector;
    
    // Register a scoped service
    parentInjector.registerFactory<DisposableTestService>([]() {
        return std::make_shared<DisposableTestService>();
    }, ServiceLifetime::Scoped);
    
    // Register a greeter service that depends on the logger
    parentInjector.registerFactory<GreeterService>([]() {
        return std::make_shared<GreeterService>();
    }, ServiceLifetime::Scoped);
    
    // Get a scoped service from the parent injector
    auto parentService = parentInjector.resolve<DisposableTestService>();
    EXPECT_FALSE(parentService->isDisposed());
    
    // Get a greeter service from the parent injector
    auto parentGreeter = parentInjector.resolve<GreeterService>();
    
    // Create a child scope
    auto childScope = parentInjector.createScope();
    
    // Get a scoped service from the child scope
    auto childService = childScope->resolve<DisposableTestService>();
    EXPECT_FALSE(childService->isDisposed());
    
    // Get a greeter service from the child scope
    auto childGreeter = childScope->resolve<GreeterService>();
    
    // Verify that parent and child services are different
    EXPECT_NE(parentService, childService);
    EXPECT_NE(parentGreeter, childGreeter);
    
    // Dispose the child scope
    childScope->dispose();
    
    // Verify that the child service was disposed
    EXPECT_TRUE(childService->isDisposed());
    
    // Verify that the parent service is still not disposed
    EXPECT_FALSE(parentService->isDisposed());
    
    // Dispose the parent scope
    parentInjector.dispose();
    
    // Verify that the parent service was disposed
    EXPECT_TRUE(parentService->isDisposed());
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 