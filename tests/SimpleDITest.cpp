#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "../src/di/Injector.hpp"
#include "../src/di/CoreModule.hpp"

// Test interfaces from our simplified DI implementation
using namespace di;

// Additional test interfaces
class IConfigService {
public:
    virtual ~IConfigService() = default;
    virtual std::string getValue(const std::string& key) const = 0;
};

class SimpleConfigService : public IConfigService {
public:
    SimpleConfigService(std::shared_ptr<ISimpleLogger> logger) : logger_(logger) {
        logger_->log("SimpleConfigService created");
    }

    std::string getValue(const std::string& key) const override {
        logger_->log("Getting config value for: " + key);
        if (key == "app.name") return "DI Test App";
        if (key == "app.version") return "1.0";
        return "unknown";
    }

private:
    std::shared_ptr<ISimpleLogger> logger_;
};

class IUserService {
public:
    virtual ~IUserService() = default;
    virtual std::string getCurrentUser() const = 0;
};

class UserService : public IUserService {
public:
    UserService(std::shared_ptr<ISimpleLogger> logger, std::shared_ptr<IConfigService> config) 
        : logger_(logger), config_(config) {
        logger_->log("UserService created");
    }

    std::string getCurrentUser() const override {
        logger_->log("Getting current user");
        return "test_user";
    }

private:
    std::shared_ptr<ISimpleLogger> logger_;
    std::shared_ptr<IConfigService> config_;
};

// Test fixture for DI tests
class SimpleDITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure the injector with core services
        CoreModule::configure(injector);
        
        // Configure test services
        injector.registerFactory<IConfigService>([](Injector& inj) {
            auto logger = inj.resolve<ISimpleLogger>();
            return std::make_shared<SimpleConfigService>(logger);
        });

        injector.registerFactory<IUserService>([](Injector& inj) {
            auto logger = inj.resolve<ISimpleLogger>();
            auto config = inj.resolve<IConfigService>();
            return std::make_shared<UserService>(logger, config);
        });
    }
    
    Injector injector;
};

// Test basic dependency resolution with get<T>()
TEST_F(SimpleDITest, GetInterface) {
    // Test resolving via get<T>() interface
    auto logger = injector.get<ISimpleLogger>();
    ASSERT_NE(logger, nullptr);
    
    auto config = injector.get<IConfigService>();
    ASSERT_NE(config, nullptr);
    
    auto userService = injector.get<IUserService>();
    ASSERT_NE(userService, nullptr);
}

// Test legacy dependency resolution with resolve<T>()
TEST_F(SimpleDITest, ResolveInterface) {
    // Test resolving via legacy resolve<T>() interface
    auto logger = injector.resolve<ISimpleLogger>();
    ASSERT_NE(logger, nullptr);
    
    auto config = injector.resolve<IConfigService>();
    ASSERT_NE(config, nullptr);
    
    auto userService = injector.resolve<IUserService>();
    ASSERT_NE(userService, nullptr);
}

// Test dependency chains
TEST_F(SimpleDITest, DependencyChain) {
    // Resolve a service with a dependency chain
    auto userService = injector.get<IUserService>();
    ASSERT_NE(userService, nullptr);
    
    // Verify the service works
    std::string username = userService->getCurrentUser();
    ASSERT_EQ(username, "test_user");
}

// Test mixed registration styles
TEST_F(SimpleDITest, MixedRegistrationStyles) {
    // Create a new injector for this test
    Injector mixedInjector;
    
    // Register a service using the modern style (without injector reference)
    mixedInjector.registerFactory<ISimpleLogger>([]() {
        return std::make_shared<ConsoleLogger>();
    });
    
    // Register a service using the legacy style (with injector reference)
    mixedInjector.registerFactory<IConfigService>([](Injector& inj) {
        auto logger = inj.resolve<ISimpleLogger>();
        return std::make_shared<SimpleConfigService>(logger);
    });
    
    // Resolve services using both get<T>() and resolve<T>()
    auto loggerGet = mixedInjector.get<ISimpleLogger>();
    ASSERT_NE(loggerGet, nullptr);
    
    auto configResolve = mixedInjector.resolve<IConfigService>();
    ASSERT_NE(configResolve, nullptr);
    
    // Verify the services work together
    std::string appName = configResolve->getValue("app.name");
    ASSERT_EQ(appName, "DI Test App");
}

// Test error handling for unregistered types
TEST_F(SimpleDITest, ErrorHandlingUnregisteredTypes) {
    // Create a new empty injector
    Injector emptyInjector;
    
    // Attempting to resolve an unregistered type should throw
    EXPECT_THROW(emptyInjector.get<ISimpleLogger>(), std::runtime_error);
    EXPECT_THROW(emptyInjector.resolve<ISimpleLogger>(), std::runtime_error);
}

// Only include a main function when not built as part of RunAllTests
#ifndef RUN_ALL_TESTS_INCLUDE
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif 