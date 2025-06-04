#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "../src/di/DIFramework.hpp"
#include "../src/di/ServiceCollection.hpp"
#include "di/TestServices.hpp"

using namespace di;
using namespace di::test;

// Implementation of SimpleGreeter without constructor dependency for this test
class TestSimpleGreeter : public di::test::IGreeter {
public:
    std::string greet(const std::string& name) const override {
        return "Hello, " + name + "!";
    }
};

class FancyGreeter : public di::test::IGreeter {
public:
    std::string greet(const std::string& name) const override {
        return "Greetings, " + name + "!";
    }
};

class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;
    virtual std::string getCurrentTime() = 0;
};

class SimpleTimeProvider : public ITimeProvider {
public:
    std::string getCurrentTime() override {
        return "12:00 PM";
    }
};

class INotifier {
public:
    virtual ~INotifier() = default;
    virtual void notify(const std::string& message) = 0;
};

class SimpleNotifier : public INotifier {
public:
    void notify(const std::string& message) override {
        lastMessage_ = message;
    }

    std::string getLastMessage() const {
        return lastMessage_;
    }

private:
    std::string lastMessage_;
};

// Service that depends on other services
class GreetingService {
public:
    GreetingService(
        std::shared_ptr<IGreeter> greeter,
        std::shared_ptr<ITimeProvider> timeProvider,
        std::shared_ptr<INotifier> notifier)
        : greeter_(greeter)
        , timeProvider_(timeProvider)
        , notifier_(notifier) {
    }

    std::string greetWithTime(const std::string& name) {
        auto greeting = greeter_->greet(name);
        auto time = timeProvider_->getCurrentTime();
        auto message = greeting + " The time is " + time + ".";
        notifier_->notify(message);
        return message;
    }

private:
    std::shared_ptr<IGreeter> greeter_;
    std::shared_ptr<ITimeProvider> timeProvider_;
    std::shared_ptr<INotifier> notifier_;
};

// Disposable service for testing cleanup
class IDisposableService : public lifetime::IDisposable {
public:
    virtual ~IDisposableService() = default;
    virtual bool isDisposed() const = 0;
};

class DisposableService : public IDisposableService {
public:
    bool isDisposed() const override {
        return disposed_;
    }

    void dispose() override {
        disposed_ = true;
    }

private:
    bool disposed_ = false;
};

// Test fixture
class DIFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        framework_ = std::make_shared<DIFramework>();
    }

    std::shared_ptr<DIFramework> framework_;
};

// Tests
TEST_F(DIFrameworkTest, RegisterAndResolveTransientService) {
    // Register a transient service
    framework_->registerFactory<IGreeter>([]() {
        return std::make_shared<TestSimpleGreeter>();
    });

    // Resolve the service twice
    auto greeter1 = framework_->get<IGreeter>();
    auto greeter2 = framework_->get<IGreeter>();

    // Verify that the service works
    EXPECT_EQ("Hello, World!", greeter1->greet("World"));

    // Verify that we got different instances
    EXPECT_NE(greeter1, greeter2);
}

TEST_F(DIFrameworkTest, RegisterAndResolveSingletonService) {
    // Register a singleton service
    framework_->registerFactory<IGreeter>(
        []() { return std::make_shared<TestSimpleGreeter>(); },
        lifetime::ServiceLifetime::Singleton);

    // Resolve the service twice
    auto greeter1 = framework_->get<IGreeter>();
    auto greeter2 = framework_->get<IGreeter>();

    // Verify that the service works
    EXPECT_EQ("Hello, World!", greeter1->greet("World"));

    // Verify that we got the same instance
    EXPECT_EQ(greeter1, greeter2);
}

TEST_F(DIFrameworkTest, RegisterAndResolveScopedService) {
    // Register a scoped service
    framework_->registerFactory<IGreeter>(
        []() { return std::make_shared<TestSimpleGreeter>(); },
        lifetime::ServiceLifetime::Scoped);

    // Resolve the service twice in the same scope
    auto greeter1 = framework_->get<IGreeter>();
    auto greeter2 = framework_->get<IGreeter>();

    // Verify that we got the same instance in the same scope
    EXPECT_EQ(greeter1, greeter2);

    // Create a new scope
    auto scope = framework_->createScope();

    // Resolve the service in the new scope
    auto greeter3 = scope->get<IGreeter>();

    // Verify that we got a different instance in the new scope
    EXPECT_NE(greeter1, greeter3);
}

TEST_F(DIFrameworkTest, RegisterAndResolveExistingInstance) {
    // Create an instance
    auto greeter = std::make_shared<TestSimpleGreeter>();

    // Register the instance
    framework_->registerSingleton<IGreeter>(greeter);

    // Resolve the service
    auto resolvedGreeter = framework_->get<IGreeter>();

    // Verify that we got the same instance
    EXPECT_EQ(greeter, resolvedGreeter);
}

TEST_F(DIFrameworkTest, RegisterAndResolveConcreteTypeForInterface) {
    // Register a concrete type for an interface
    framework_->registerType<IGreeter, TestSimpleGreeter>();

    // Resolve the service
    auto greeter = framework_->get<IGreeter>();

    // Verify that the service works
    EXPECT_EQ("Hello, World!", greeter->greet("World"));
}

TEST_F(DIFrameworkTest, RegisterAndResolveDependentService) {
    // Register the dependencies
    framework_->registerFactory<IGreeter>([]() {
        return std::make_shared<TestSimpleGreeter>();
    });

    framework_->registerFactory<ITimeProvider>([]() {
        return std::make_shared<SimpleTimeProvider>();
    });

    framework_->registerFactory<INotifier>([]() {
        return std::make_shared<SimpleNotifier>();
    });

    // Register the dependent service
    framework_->registerFactory<GreetingService>([this]() {
        auto greeter = framework_->get<IGreeter>();
        auto timeProvider = framework_->get<ITimeProvider>();
        auto notifier = framework_->get<INotifier>();
        return std::make_shared<GreetingService>(greeter, timeProvider, notifier);
    });

    // Resolve the service
    auto greetingService = framework_->get<GreetingService>();

    // Verify that the service works
    auto message = greetingService->greetWithTime("World");
    EXPECT_EQ("Hello, World! The time is 12:00 PM.", message);

    // Verify that the notifier was called
    auto notifier = framework_->get<INotifier>();
    auto simpleNotifier = std::dynamic_pointer_cast<SimpleNotifier>(notifier);
    EXPECT_EQ("Hello, World! The time is 12:00 PM.", simpleNotifier->getLastMessage());
}

TEST_F(DIFrameworkTest, DisposeScopedServices) {
    // Register a disposable service as scoped
    framework_->registerFactory<IDisposableService>(
        []() { return std::make_shared<DisposableService>(); },
        lifetime::ServiceLifetime::Scoped);

    // Create a scope
    auto scope = framework_->createScope();

    // Resolve the service in the scope
    auto service = scope->get<IDisposableService>();

    // Verify that the service is not disposed yet
    EXPECT_FALSE(service->isDisposed());

    // Dispose the scope
    scope->dispose();

    // Verify that the service is now disposed
    EXPECT_TRUE(service->isDisposed());
}

TEST_F(DIFrameworkTest, RegisterAndResolveWithServiceCollection) {
    // Create a service collection
    ServiceCollection services;

    // Register services
    services.addSingleton<IGreeter>([]() {
        return std::make_shared<TestSimpleGreeter>();
    });

    services.addTransient<ITimeProvider, SimpleTimeProvider>();

    auto notifier = std::make_shared<SimpleNotifier>();
    services.addSingleton<INotifier>(notifier);

    // Build the service provider
    auto provider = services.buildServiceProvider();

    // Resolve the services
    auto greeter = provider->get<IGreeter>();
    auto timeProvider = provider->get<ITimeProvider>();
    auto resolvedNotifier = provider->get<INotifier>();

    // Verify that the services work
    EXPECT_EQ("Hello, World!", greeter->greet("World"));
    EXPECT_EQ("12:00 PM", timeProvider->getCurrentTime());
    EXPECT_EQ(notifier, resolvedNotifier);

    // Verify that singletons are the same instance
    auto greeter2 = provider->get<IGreeter>();
    EXPECT_EQ(greeter, greeter2);

    // Verify that transients are different instances
    auto timeProvider2 = provider->get<ITimeProvider>();
    EXPECT_NE(timeProvider, timeProvider2);
}

// Main function that runs the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 