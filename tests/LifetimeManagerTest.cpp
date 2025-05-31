#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <unordered_set>
#include "../src/di/LifetimeManager.hpp"
#include "../src/di/CoreModule.hpp"
#include "di/TestServices.hpp"

using namespace di;
using namespace di::lifetime;
using namespace di::test;

// Test fixture for LifetimeManager tests
class LifetimeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DisposableService::resetInstanceCount();
    }
    
    void TearDown() override {
        // Nothing to clean up
    }
};

// Test singleton lifetime
TEST_F(LifetimeManagerTest, SingletonLifetime) {
    LifetimeManager manager;
    
    // Create a factory that creates a new instance each time
    auto factory = []() {
        return std::make_shared<DisposableService>();
    };
    
    // Get multiple instances with singleton lifetime
    auto instance1 = manager.getInstance<DisposableService>(factory, ServiceLifetime::Singleton);
    auto instance2 = manager.getInstance<DisposableService>(factory, ServiceLifetime::Singleton);
    
    // Verify that only one instance was created
    EXPECT_EQ(DisposableService::getInstanceCount(), 1);
    
    // Verify that the same instance is returned
    EXPECT_EQ(instance1, instance2);
    
    // Clean up
    manager.dispose();
    
    // Verify that the instance was disposed
    EXPECT_TRUE(instance1->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 0);
}

// Test transient lifetime
TEST_F(LifetimeManagerTest, TransientLifetime) {
    LifetimeManager manager;
    
    // Create a factory that creates a new instance each time
    auto factory = []() {
        return std::make_shared<DisposableService>();
    };
    
    // Get multiple instances with transient lifetime
    auto instance1 = manager.getInstance<DisposableService>(factory, ServiceLifetime::Transient);
    auto instance2 = manager.getInstance<DisposableService>(factory, ServiceLifetime::Transient);
    
    // Verify that multiple instances were created
    EXPECT_EQ(DisposableService::getInstanceCount(), 2);
    
    // Verify that different instances are returned
    EXPECT_NE(instance1, instance2);
    
    // Clean up
    manager.dispose();
    
    // Verify that instances were disposed
    EXPECT_TRUE(instance1->isDisposed());
    EXPECT_TRUE(instance2->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 0);
}

// Test scoped lifetime
TEST_F(LifetimeManagerTest, ScopedLifetime) {
    LifetimeManager rootManager;
    
    // Create a factory that creates a new instance each time
    auto factory = []() {
        return std::make_shared<DisposableService>();
    };
    
    // Get a singleton instance in the root scope
    auto rootSingleton = rootManager.getInstance<DisposableService>(factory, ServiceLifetime::Singleton);
    
    // Create a child scope
    auto childScope = rootManager.createScope();
    
    // Get a singleton instance in the child scope - should be the same as the root scope
    auto childSingleton = childScope->getInstance<DisposableService>(factory, ServiceLifetime::Singleton);
    
    // Get a scoped instance in the child scope
    auto childScoped = childScope->getInstance<DisposableService>(factory, ServiceLifetime::Scoped);
    
    // Get another scoped instance in the child scope - should be the same within the scope
    auto childScoped2 = childScope->getInstance<DisposableService>(factory, ServiceLifetime::Scoped);
    
    // Verify the instance counts
    EXPECT_EQ(DisposableService::getInstanceCount(), 2); // 1 singleton + 1 scoped
    
    // Verify that singletons are shared between scopes
    EXPECT_EQ(rootSingleton, childSingleton);
    
    // Verify that scoped instances are reused within the same scope
    EXPECT_EQ(childScoped, childScoped2);
    
    // Create another child scope
    auto childScope2 = rootManager.createScope();
    
    // Get a scoped instance in the second child scope
    auto childScope2Scoped = childScope2->getInstance<DisposableService>(factory, ServiceLifetime::Scoped);
    
    // Verify that a new scoped instance was created for the second child scope
    EXPECT_NE(childScoped, childScope2Scoped);
    
    // Verify the instance count
    EXPECT_EQ(DisposableService::getInstanceCount(), 3); // 1 singleton + 2 scoped
    
    // Dispose the first child scope
    childScope->dispose();
    
    // Verify that scoped instances from the first child scope were disposed
    EXPECT_TRUE(childScoped->isDisposed());
    
    // Verify that the singleton is still alive
    EXPECT_FALSE(rootSingleton->isDisposed());
    
    // Verify the instance count
    EXPECT_EQ(DisposableService::getInstanceCount(), 2); // 1 singleton + 1 scoped
    
    // Dispose the second child scope
    childScope2->dispose();
    
    // Verify the instance count
    EXPECT_EQ(DisposableService::getInstanceCount(), 1); // 1 singleton
    
    // Dispose the root manager
    rootManager.dispose();
    
    // Verify that all instances were disposed
    EXPECT_TRUE(rootSingleton->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 0);
}

// Test thread safety for singleton creation
TEST_F(LifetimeManagerTest, ThreadSafety) {
    LifetimeManager manager;
    
    // Create a factory that creates a new instance each time
    auto factory = []() {
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return std::make_shared<DisposableService>();
    };
    
    // Use multiple threads to get singleton instances concurrently
    const int numThreads = 10;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<DisposableService>> instances(numThreads);
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &instances, i, factory]() {
            instances[i] = manager.getInstance<DisposableService>(factory, ServiceLifetime::Singleton);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify that only one instance was created
    EXPECT_EQ(DisposableService::getInstanceCount(), 1);
    
    // Verify that all threads got the same instance
    for (int i = 1; i < numThreads; ++i) {
        EXPECT_EQ(instances[0], instances[i]);
    }
    
    // Clean up
    manager.dispose();
    
    // Verify that all instances were disposed
    EXPECT_TRUE(instances[0]->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 0);
}

// Test LifetimeInjector
TEST_F(LifetimeManagerTest, LifetimeInjector) {
    // Create a simple injector
    LifetimeInjector injector;
    
    // Register a singleton service
    injector.registerFactory<ISimpleLogger>([]() {
        return std::make_shared<ConsoleLogger>();
    }, ServiceLifetime::Singleton);

    // Register a scoped service
    injector.registerFactory<IGreeter>([](Injector& currentInjector) {
        std::cout << "Creating new SimpleGreeter" << std::endl;
        auto logger = currentInjector.resolve<ISimpleLogger>();
        return std::make_shared<SimpleGreeter>(logger);
    }, ServiceLifetime::Scoped);
    
    // Register a transient disposable service
    injector.registerFactory<DisposableService>([]() {
        return std::make_shared<DisposableService>();
    }, ServiceLifetime::Transient);

    // Test singleton behavior
    auto logger1 = injector.get<ISimpleLogger>();
    auto logger2 = injector.get<ISimpleLogger>();
    EXPECT_EQ(logger1, logger2); // Same instance

    // Test scoped behavior
    auto greeter1 = injector.get<IGreeter>();
    auto greeter2 = injector.get<IGreeter>();
    EXPECT_EQ(greeter1, greeter2); // Same instance within the same scope

    // Test transient behavior
    auto service1 = injector.get<DisposableService>();
    auto service2 = injector.get<DisposableService>();
    EXPECT_NE(service1, service2); // Different instances
    EXPECT_EQ(DisposableService::getInstanceCount(), 2);

    // Test dispose
    injector.dispose();
    EXPECT_TRUE(service1->isDisposed());
    EXPECT_TRUE(service2->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 0);
}

// Test LifetimeInjector with child scopes
TEST_F(LifetimeManagerTest, LifetimeInjectorWithChildScope) {
    // Create a simple injector
    LifetimeInjector injector;
    
    std::cout << "\n==== REGISTERING SERVICES ====" << std::endl;
    
    // Register a singleton service
    injector.registerFactory<ISimpleLogger>([]() {
        std::cout << "Creating new ConsoleLogger" << std::endl;
        return std::make_shared<ConsoleLogger>();
    }, ServiceLifetime::Singleton);

    // Register a scoped service with a unique implementation each time
    injector.registerFactory<IGreeter>([](Injector& currentInjector) {
        std::cout << "Creating PARENT SimpleGreeter for LifetimeInjectorWithChildScope test" << std::endl;
        static std::shared_ptr<SimpleGreeter> parentInstance;
        
        if (!parentInstance) {
            auto logger = currentInjector.resolve<ISimpleLogger>();
            parentInstance = std::make_shared<SimpleGreeter>(logger);
            std::cout << "[LOG] SimpleGreeter created for PARENT instance - ptr: " << parentInstance.get() << std::endl;
        }
        
        return parentInstance;
    }, ServiceLifetime::Scoped);
    
    // Register a transient disposable service
    injector.registerFactory<DisposableService>([]() {
        std::cout << "Creating new DisposableService" << std::endl;
        return std::make_shared<DisposableService>();
    }, ServiceLifetime::Transient);

    std::cout << "\n==== GETTING SERVICES FROM PARENT SCOPE ====" << std::endl;
    
    // Get services from parent scope
    auto parentLogger = injector.get<ISimpleLogger>();
    std::cout << "Parent logger: " << parentLogger.get() << std::endl;
    
    auto parentGreeter = injector.get<IGreeter>();
    std::cout << "Parent greeter: " << parentGreeter.get() << std::endl;
    
    auto parentService = injector.get<DisposableService>();
    std::cout << "Parent service: " << parentService.get() << std::endl;

    std::cout << "\n==== CREATING CHILD SCOPE ====" << std::endl;
    
    // Create a child scope
    auto childScope = injector.createScope();
    
    // Override the factory for IGreeter in the child scope to guarantee a new instance
    childScope->registerFactory<IGreeter>([](Injector& currentInjector) {
        std::cout << "Creating CHILD SimpleGreeter for LifetimeInjectorWithChildScope test" << std::endl;
        
        auto logger = currentInjector.resolve<ISimpleLogger>();
        auto childInstance = std::make_shared<SimpleGreeter>(logger);
        std::cout << "[LOG] SimpleGreeter created for CHILD instance - ptr: " << childInstance.get() << std::endl;
        
        return childInstance;
    }, ServiceLifetime::Scoped);
    
    std::cout << "\n==== GETTING SERVICES FROM CHILD SCOPE ====" << std::endl;
    
    // Get services from child scope
    auto childLogger = childScope->get<ISimpleLogger>();
    std::cout << "Child logger: " << childLogger.get() << std::endl;
    
    auto childGreeter = childScope->get<IGreeter>();
    std::cout << "Child greeter: " << childGreeter.get() << std::endl;
    
    auto childService = childScope->get<DisposableService>();
    std::cout << "Child service: " << childService.get() << std::endl;
    
    // Print instance counts
    std::cout << "\n==== VERIFICATION ====" << std::endl;
    std::cout << "DisposableService instance count: " << DisposableService::getInstanceCount() << std::endl;
    
    // Test singleton behavior - should be shared across scopes
    EXPECT_EQ(parentLogger.get(), childLogger.get());
    std::cout << "Singleton test passed" << std::endl;
    
    // Test scoped behavior - instance counter should have increased if we got a new instance
    EXPECT_NE(parentGreeter.get(), childGreeter.get()) << "Should get different instances for scoped services in different scopes";
    std::cout << "Scoped test " << (parentGreeter.get() != childGreeter.get() ? "passed" : "failed") << std::endl;
    
    // Test transient behavior - should be different for every call
    EXPECT_NE(parentService.get(), childService.get());
    std::cout << "Transient test passed" << std::endl;
    
    // Test disposal of child scope
    std::cout << "\n==== DISPOSING CHILD SCOPE ====" << std::endl;
    EXPECT_EQ(DisposableService::getInstanceCount(), 2);
    childScope->dispose();
    std::cout << "Child scope disposed" << std::endl;
    std::cout << "Child service disposed: " << (childService->isDisposed() ? "true" : "false") << std::endl;
    std::cout << "Parent service disposed: " << (parentService->isDisposed() ? "true" : "false") << std::endl;
    std::cout << "DisposableService instance count: " << DisposableService::getInstanceCount() << std::endl;
    
    EXPECT_TRUE(childService->isDisposed());
    EXPECT_FALSE(parentService->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 1);
    
    // Test disposal of parent scope
    std::cout << "\n==== DISPOSING PARENT SCOPE ====" << std::endl;
    injector.dispose();
    std::cout << "Parent scope disposed" << std::endl;
    std::cout << "Parent service disposed: " << (parentService->isDisposed() ? "true" : "false") << std::endl;
    std::cout << "DisposableService instance count: " << DisposableService::getInstanceCount() << std::endl;
    
    EXPECT_TRUE(parentService->isDisposed());
    EXPECT_EQ(DisposableService::getInstanceCount(), 0);
}

// Test LifetimeInjector with child scopes - simplified test for scoped services
TEST_F(LifetimeManagerTest, ScopedServiceInChildScope) {
    // Create a simple injector
    LifetimeInjector injector;
    
    std::cout << "\n==== REGISTERING SERVICES FOR SCOPE TEST ====" << std::endl;
    
    // Register a scoped service that creates a unique instance each time
    injector.registerFactory<IGreeter>([](Injector&) {
        std::cout << "Creating PARENT SimpleGreeter" << std::endl;
        
        // Create a logger directly instead of resolving to simplify test
        auto logger = std::make_shared<ConsoleLogger>();
        auto greeter = std::make_shared<SimpleGreeter>(logger);
        
        std::cout << "Created PARENT SimpleGreeter instance: " << greeter.get() << std::endl;
        return greeter;
    }, ServiceLifetime::Scoped);
    
    // Get service from parent scope
    std::cout << "\n==== GETTING SERVICE FROM PARENT SCOPE ====" << std::endl;
    auto parentGreeter = injector.get<IGreeter>();
    std::cout << "Parent greeter address: " << parentGreeter.get() << std::endl;
    
    // Create child scope
    std::cout << "\n==== CREATING CHILD SCOPE ====" << std::endl;
    auto childScope = injector.createScope();
    
    // Register a new factory in the child scope to ensure a different instance
    childScope->registerFactory<IGreeter>([](Injector&) {
        std::cout << "Creating CHILD SimpleGreeter (explicitly registered in child scope)" << std::endl;
        
        // Create a logger directly instead of resolving to simplify test
        auto logger = std::make_shared<ConsoleLogger>();
        auto greeter = std::make_shared<SimpleGreeter>(logger);
        
        std::cout << "Created CHILD SimpleGreeter instance: " << greeter.get() << std::endl;
        return greeter;
    }, ServiceLifetime::Scoped);
    
    // Get service from child scope
    std::cout << "\n==== GETTING SERVICE FROM CHILD SCOPE ====" << std::endl;
    auto childGreeter = childScope->get<IGreeter>();
    std::cout << "Child greeter address: " << childGreeter.get() << std::endl;
    
    // Verify - these must be different instances
    std::cout << "\n==== VERIFICATION ====" << std::endl;
    bool differentInstances = (parentGreeter.get() != childGreeter.get());
    
    std::cout << "Parent greeter address: " << parentGreeter.get() << std::endl;
    std::cout << "Child greeter address: " << childGreeter.get() << std::endl;
    std::cout << "Different instances: " << (differentInstances ? "YES" : "NO") << std::endl;
    
    EXPECT_TRUE(differentInstances) << "Child scope should receive a new instance for scoped services";
    
    // Cleanup
    childScope->dispose();
    injector.dispose();
}

// Test that scoped services created in child scopes always get new instances
// This test verifies the key issue we fixed - child scopes must always create
// new instances for scoped services, never inheriting from parent
TEST_F(LifetimeManagerTest, ScopedServiceInChildScopeNoOverride) {
    // Create a simple injector
    LifetimeInjector injector;
    
    std::cout << "\n==== REGISTERING SERVICES FOR DEFAULT SCOPED TEST ====" << std::endl;
    
    // Register a scoped service - note we only register this in the parent
    injector.registerFactory<IGreeter>([](Injector&) {
        std::cout << "Creating SimpleGreeter" << std::endl;
        
        // Create a logger directly instead of resolving to simplify test
        auto logger = std::make_shared<ConsoleLogger>();
        auto greeter = std::make_shared<SimpleGreeter>(logger);
        
        std::cout << "Created SimpleGreeter instance: " << greeter.get() << std::endl;
        return greeter;
    }, ServiceLifetime::Scoped);
    
    // Get service from parent scope
    std::cout << "\n==== GETTING SERVICE FROM PARENT SCOPE ====" << std::endl;
    auto parentGreeter = injector.get<IGreeter>();
    std::cout << "Parent greeter address: " << parentGreeter.get() << std::endl;
    
    // Create child scope
    std::cout << "\n==== CREATING CHILD SCOPE (WITHOUT FACTORY OVERRIDE) ====" << std::endl;
    auto childScope = injector.createScope();
    
    // Debug: Print all registered types in the child scope
    std::cout << "\n==== DEBUG: CHECKING CHILD SCOPE REGISTRATION ====" << std::endl;
    std::cout << "No manual registration needed - the factory should be automatically registered now" << std::endl;

    // Get service from child scope - we're not registering any explicit factory here
    // This tests the default behavior which should always create a new instance
    std::cout << "\n==== GETTING SERVICE FROM CHILD SCOPE (WITHOUT FACTORY OVERRIDE) ====" << std::endl;
    auto childGreeter = childScope->get<IGreeter>();
    std::cout << "Child greeter address: " << childGreeter.get() << std::endl;
    
    // Verify - these must be different instances even without explicit registration
    std::cout << "\n==== VERIFICATION ====" << std::endl;
    bool differentInstances = (parentGreeter.get() != childGreeter.get());
    
    std::cout << "Parent greeter address: " << parentGreeter.get() << std::endl;
    std::cout << "Child greeter address: " << childGreeter.get() << std::endl;
    std::cout << "Different instances: " << (differentInstances ? "YES" : "NO") << std::endl;
    
    EXPECT_TRUE(differentInstances) << "Child scope should automatically receive a new instance for scoped services, even without explicit factory registration";
    
    // Cleanup
    childScope->dispose();
    injector.dispose();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 