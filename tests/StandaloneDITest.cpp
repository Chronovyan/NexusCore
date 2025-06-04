#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "../src/di/Injector.hpp"
#include "../src/di/CoreModule.hpp"
#include "di/TestServices.hpp"

// This test demonstrates the standalone DI implementation
// It uses both the modern and legacy interfaces to show how they work together
using namespace di;
using namespace di::test;

// A more advanced greeter that also uses a configuration service
class ConfigurableGreeter : public IGreeter {
public:
    // Constructor injection for multiple dependencies
    ConfigurableGreeter(
        std::shared_ptr<ISimpleLogger> logger,
        const std::string& greeting_format
    ) : 
        logger_(logger),
        greeting_format_(greeting_format) {
        logger_->log("ConfigurableGreeter created with format: " + greeting_format);
    }

    std::string greet(const std::string& name) const override {
        std::string message = greeting_format_;
        // Replace {name} with the actual name
        size_t pos = message.find("{name}");
        if (pos != std::string::npos) {
            message.replace(pos, 6, name);
        }
        logger_->log("Custom greeting: " + message);
        return message;
    }

private:
    std::shared_ptr<ISimpleLogger> logger_;
    std::string greeting_format_;
};

// Test fixture for DI tests
class StandaloneDITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure the injector with core services from CoreModule
        CoreModule::configure(injector);
        
        // Register services using the modern style (without injector reference)
        // This is the recommended approach for new code
        injector.registerFactory<IGreeter>([]() {
            // This service will be created with a logger obtained elsewhere
            // In a real application, you might inject other dependencies here
            auto logger = std::make_shared<ConsoleLogger>();
            return std::make_shared<SimpleGreeter>(logger);
        });

        // Register a service using the legacy style (with injector reference)
        // This demonstrates backward compatibility
        injector.registerFactory<ConfigurableGreeter>([](Injector& inj) {
            // This service resolves its dependencies from the injector
            auto logger = inj.resolve<ISimpleLogger>();
            return std::make_shared<ConfigurableGreeter>(logger, "Greetings, {name}!");
        });
    }
    
    Injector injector;
};

// Test basic dependency resolution using the modern interface
TEST_F(StandaloneDITest, BasicResolution) {
    // Resolve the logger using the modern interface (get<T>)
    auto logger = injector.get<ISimpleLogger>();
    ASSERT_NE(logger, nullptr);
    
    // Resolve the greeter using the modern interface
    auto greeter = injector.get<IGreeter>();
    ASSERT_NE(greeter, nullptr);
    
    // Use the resolved service
    std::string greeting = greeter->greet("World");
    EXPECT_EQ(greeting, "Hello, World!");
}

// Test the modern interface for DI
TEST_F(StandaloneDITest, ModernInterface) {
    // The modern interface uses get<T>() for clearer intent
    auto logger = injector.get<ISimpleLogger>();
    ASSERT_NE(logger, nullptr);
    
    // Test the logging functionality
    logger->log("Testing modern interface");
    
    // We can also resolve the ConfigurableGreeter specifically if needed
    auto configGreeter = injector.get<ConfigurableGreeter>();
    ASSERT_NE(configGreeter, nullptr);
    
    // Test the custom greeting
    std::string greeting = configGreeter->greet("Modern User");
    EXPECT_EQ(greeting, "Greetings, Modern User!");
}

// Test the legacy interface for backward compatibility
TEST_F(StandaloneDITest, LegacyInterface) {
    // The legacy interface uses resolve<T>()
    auto logger = injector.resolve<ISimpleLogger>();
    ASSERT_NE(logger, nullptr);
    
    // Test the logging functionality
    logger->log("Testing legacy interface");
    
    // We can also resolve IGreeter via the legacy interface
    // This demonstrates that both interfaces work with all registered services
    auto greeter = injector.resolve<IGreeter>();
    ASSERT_NE(greeter, nullptr);
    
    std::string greeting = greeter->greet("Legacy User");
    EXPECT_EQ(greeting, "Hello, Legacy User!");
}

// Test that demonstrates mixed registration and resolution styles
TEST_F(StandaloneDITest, MixedStyles) {
    // Create a new injector for this test
    Injector mixedInjector;
    
    // Register some services using different styles
    mixedInjector.registerFactory<ISimpleLogger>([]() {
        return std::make_shared<ConsoleLogger>();
    });
    
    // Register using legacy style with injector access
    mixedInjector.registerFactory<IGreeter>([](Injector& inj) {
        auto logger = inj.resolve<ISimpleLogger>();
        return std::make_shared<SimpleGreeter>(logger);
    });
    
    // Resolve using both styles
    auto logger1 = mixedInjector.get<ISimpleLogger>();
    auto logger2 = mixedInjector.resolve<ISimpleLogger>();
    
    // NOTE: With this implementation, services are not singletons by default
    // Each call to get<T>() or resolve<T>() creates a new instance
    // To implement singleton behavior, you would need to cache the instances
    // or implement a decorator pattern for the factories
    EXPECT_NE(logger1, logger2);
    
    // Both interfaces can resolve all registered services
    auto greeter1 = mixedInjector.get<IGreeter>();
    auto greeter2 = mixedInjector.resolve<IGreeter>();
    
    // These are different instances
    EXPECT_NE(greeter1, greeter2);
    
    // But they should both work the same way
    EXPECT_EQ(greeter1->greet("User"), greeter2->greet("User"));
}

// Only include a main function when not built as part of RunAllTests
#ifndef RUN_ALL_TESTS_INCLUDE
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif 