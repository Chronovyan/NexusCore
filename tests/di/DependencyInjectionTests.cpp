#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include "di/Injector.hpp"

// Test interfaces and implementations
namespace {

// ----- Test Interfaces -----

struct IService {
    virtual ~IService() = default;
    virtual std::string getName() const = 0;
};

struct IClient {
    virtual ~IClient() = default;
    virtual std::string getClientName() const = 0;
};

struct IDatabase {
    virtual ~IDatabase() = default;
    virtual std::string getConnectionString() const = 0;
};

struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(const std::string& message) = 0;
};

// ----- Test Implementations -----

// Simple implementation with default constructor
class BasicService : public IService {
public:
    std::string getName() const override { return "BasicService"; }
};

// Implementation with constructor dependency
class AdvancedService : public IService {
public:
    explicit AdvancedService(std::shared_ptr<ILogger> logger) 
        : _logger(logger) {
        if (_logger) {
            _logger->log("AdvancedService created");
        }
    }

    std::string getName() const override { return "AdvancedService"; }

private:
    std::shared_ptr<ILogger> _logger;
};

// Implementation with multiple dependencies
class Client : public IClient {
public:
    Client(std::shared_ptr<IService> service, std::shared_ptr<ILogger> logger)
        : _service(service), _logger(logger) {
        if (_logger) {
            _logger->log("Client created with service: " + _service->getName());
        }
    }

    std::string getClientName() const override { 
        return "Client using " + _service->getName(); 
    }

private:
    std::shared_ptr<IService> _service;
    std::shared_ptr<ILogger> _logger;
};

// Implementation with three dependencies
class ComplexClient : public IClient {
public:
    ComplexClient(
        std::shared_ptr<IService> service, 
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IDatabase> database)
        : _service(service), _logger(logger), _database(database) {
        if (_logger) {
            _logger->log("ComplexClient created with service: " + _service->getName() + 
                        " and database: " + _database->getConnectionString());
        }
    }

    std::string getClientName() const override { 
        return "ComplexClient using " + _service->getName() + 
               " and database " + _database->getConnectionString();
    }

private:
    std::shared_ptr<IService> _service;
    std::shared_ptr<ILogger> _logger;
    std::shared_ptr<IDatabase> _database;
};

class SimpleDatabase : public IDatabase {
public:
    std::string getConnectionString() const override {
        return "sqlite:memory";
    }
};

// Mock classes for testing
class MockLogger : public ILogger {
public:
    MOCK_METHOD(void, log, (const std::string&), (override));
};

} // anonymous namespace

// ===== Test Fixture =====
class DITests : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }

    void TearDown() override {
        // Common cleanup for all tests
    }

    // Helper methods if needed
};

// ===== Test Cases =====

// ----- Registration Tests -----

TEST_F(DITests, RegisterAndResolveSimpleType) {
    di::Injector injector;
    
    // Register a simple type
    injector.registerType<IService, BasicService>();
    
    // Resolve the type
    auto service = injector.resolve<IService>();
    
    // Verify
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->getName(), "BasicService");
}

TEST_F(DITests, RegisterAndResolveWithSingletonScope) {
    di::Injector injector;
    
    // Register as singleton
    injector.registerType<IService, BasicService>(di::Lifetime::Singleton);
    
    // Resolve twice
    auto service1 = injector.resolve<IService>();
    auto service2 = injector.resolve<IService>();
    
    // Verify same instance
    ASSERT_NE(service1, nullptr);
    ASSERT_NE(service2, nullptr);
    EXPECT_EQ(service1, service2);  // Should be same pointer
}

TEST_F(DITests, RegisterAndResolveWithTransientScope) {
    di::Injector injector;
    
    // Register as transient (default)
    injector.registerType<IService, BasicService>(di::Lifetime::Transient);
    
    // Resolve twice
    auto service1 = injector.resolve<IService>();
    auto service2 = injector.resolve<IService>();
    
    // Verify different instances
    ASSERT_NE(service1, nullptr);
    ASSERT_NE(service2, nullptr);
    EXPECT_NE(service1, service2);  // Should be different pointers
}

TEST_F(DITests, RegisterInstance) {
    di::Injector injector;
    
    // Create instance and register it
    auto instance = std::make_shared<BasicService>();
    injector.registerInstance<IService>(instance);
    
    // Resolve
    auto resolved = injector.resolve<IService>();
    
    // Verify same instance
    EXPECT_EQ(instance, resolved);
}

TEST_F(DITests, RegisterFactory) {
    di::Injector injector;
    
    // Register with factory function
    bool factoryCalled = false;
    injector.registerFactory<IService>(
        [&factoryCalled](di::Injector&) {
            factoryCalled = true;
            return std::make_shared<BasicService>();
        }
    );
    
    // Resolve
    auto service = injector.resolve<IService>();
    
    // Verify
    ASSERT_NE(service, nullptr);
    EXPECT_TRUE(factoryCalled);
    EXPECT_EQ(service->getName(), "BasicService");
}

// ----- Resolution and Dependency Injection Tests -----

TEST_F(DITests, ResolveWithOneDependency) {
    di::Injector injector;
    
    // Setup mock
    auto mockLogger = std::make_shared<MockLogger>();
    EXPECT_CALL(*mockLogger, log(testing::_)).Times(1);
    
    // Register dependencies
    injector.registerInstance<ILogger>(mockLogger);
    injector.registerType<IService, AdvancedService>();
    
    // Resolve
    auto service = injector.resolve<IService>();
    
    // Verify
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->getName(), "AdvancedService");
}

TEST_F(DITests, ResolveWithTwoDependencies) {
    di::Injector injector;
    
    // Setup mock
    auto mockLogger = std::make_shared<MockLogger>();
    EXPECT_CALL(*mockLogger, log(testing::_)).Times(1);
    
    // Register dependencies
    injector.registerInstance<ILogger>(mockLogger);
    injector.registerType<IService, BasicService>();
    injector.registerType<IClient, Client>();
    
    // Resolve
    auto client = injector.resolve<IClient>();
    
    // Verify
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getClientName(), "Client using BasicService");
}

TEST_F(DITests, ResolveWithThreeDependencies) {
    di::Injector injector;
    
    // Setup mock
    auto mockLogger = std::make_shared<MockLogger>();
    EXPECT_CALL(*mockLogger, log(testing::_)).Times(1);
    
    // Register dependencies
    injector.registerInstance<ILogger>(mockLogger);
    injector.registerType<IService, BasicService>();
    injector.registerType<IDatabase, SimpleDatabase>();
    injector.registerType<IClient, ComplexClient>();
    
    // Resolve
    auto client = injector.resolve<IClient>();
    
    // Verify
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getClientName(), "ComplexClient using BasicService and database sqlite:memory");
}

TEST_F(DITests, ResolveDependencyChain) {
    di::Injector injector;
    
    // Register a chain of dependencies:
    // ComplexClient -> BasicService & MockLogger & SimpleDatabase
    auto mockLogger = std::make_shared<MockLogger>();
    EXPECT_CALL(*mockLogger, log(testing::_)).Times(1);
    
    injector.registerInstance<ILogger>(mockLogger);
    injector.registerType<IService, BasicService>();
    injector.registerType<IDatabase, SimpleDatabase>();
    injector.registerType<IClient, ComplexClient>();
    
    // Resolve the leaf node which should trigger resolving the entire chain
    auto client = injector.resolve<IClient>();
    
    // Verify
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->getClientName(), "ComplexClient using BasicService and database sqlite:memory");
}

// ----- Error Handling Tests -----

TEST_F(DITests, ResolveUnregisteredType) {
    di::Injector injector;
    
    // Attempt to resolve unregistered type
    EXPECT_THROW(injector.resolve<IService>(), std::runtime_error);
}

TEST_F(DITests, RegisterNullInstance) {
    di::Injector injector;
    
    // Attempt to register null instance
    std::shared_ptr<IService> nullService;
    EXPECT_THROW(injector.registerInstance<IService>(nullService), std::invalid_argument);
}

TEST_F(DITests, RegisterNullFactory) {
    di::Injector injector;
    
    // Attempt to register null factory
    std::function<std::shared_ptr<IService>(di::Injector&)> nullFactory;
    EXPECT_THROW(injector.registerFactory<IService>(nullFactory), std::invalid_argument);
}

TEST_F(DITests, MissingDependencyInChain) {
    di::Injector injector;
    
    // Register ComplexClient but miss some dependencies
    injector.registerType<IService, BasicService>();
    // Missing ILogger and IDatabase registrations
    injector.registerType<IClient, ComplexClient>();
    
    // Attempt to resolve should fail because of missing dependencies
    EXPECT_THROW(injector.resolve<IClient>(), std::runtime_error);
}

// ----- Child Container Tests -----

TEST_F(DITests, ChildContainerInheritsRegistrations) {
    di::Injector parentInjector;
    
    // Register in parent
    parentInjector.registerType<IService, BasicService>(di::Lifetime::Singleton);
    
    // Create child injector
    auto childInjector = parentInjector.createChildInjector();
    
    // Resolve from child
    auto service = childInjector.resolve<IService>();
    
    // Verify
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->getName(), "BasicService");
}

TEST_F(DITests, ChildContainerOverridesRegistrations) {
    di::Injector parentInjector;
    
    // Register in parent
    parentInjector.registerType<IService, BasicService>();
    
    // Create child and register same interface with different implementation
    auto childInjector = parentInjector.createChildInjector();
    auto mockLogger = std::make_shared<MockLogger>();
    EXPECT_CALL(*mockLogger, log(testing::_)).Times(1);
    childInjector.registerInstance<ILogger>(mockLogger);
    childInjector.registerType<IService, AdvancedService>();
    
    // Resolve from child
    auto service = childInjector.resolve<IService>();
    
    // Verify child's registration was used
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->getName(), "AdvancedService");
}

// ----- Introspection Tests -----

TEST_F(DITests, IsRegisteredTest) {
    di::Injector injector;
    
    // Before registration
    EXPECT_FALSE(injector.isRegistered<IService>());
    
    // After registration
    injector.registerType<IService, BasicService>();
    EXPECT_TRUE(injector.isRegistered<IService>());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 