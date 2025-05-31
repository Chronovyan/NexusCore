#include <iostream>
#include <memory>
#include <string>
#include "Injector.hpp"
#include "CoreModule.hpp"

// Additional test interfaces
class IConfigService {
public:
    virtual ~IConfigService() = default;
    virtual std::string getValue(const std::string& key) const = 0;
};

class SimpleConfigService : public IConfigService {
public:
    SimpleConfigService(std::shared_ptr<di::ISimpleLogger> logger) : logger_(logger) {
        logger_->log("SimpleConfigService created");
    }

    std::string getValue(const std::string& key) const override {
        logger_->log("Getting config value for: " + key);
        if (key == "app.name") return "DI Test App";
        if (key == "app.version") return "1.0";
        return "unknown";
    }

private:
    std::shared_ptr<di::ISimpleLogger> logger_;
};

class IUserService {
public:
    virtual ~IUserService() = default;
    virtual std::string getCurrentUser() const = 0;
};

class UserService : public IUserService {
public:
    UserService(std::shared_ptr<di::ISimpleLogger> logger, std::shared_ptr<IConfigService> config) 
        : logger_(logger), config_(config) {
        logger_->log("UserService created");
    }

    std::string getCurrentUser() const override {
        logger_->log("Getting current user");
        return "test_user";
    }

private:
    std::shared_ptr<di::ISimpleLogger> logger_;
    std::shared_ptr<IConfigService> config_;
};

// Test module that uses our CoreModule's logger
class TestModule {
public:
    static void configure(di::Injector& injector) {
        // Register concrete implementations
        injector.registerFactory<IConfigService>([](di::Injector& inj) {
            auto logger = inj.resolve<di::ISimpleLogger>();
            return std::make_shared<SimpleConfigService>(logger);
        });

        injector.registerFactory<IUserService>([](di::Injector& inj) {
            auto logger = inj.resolve<di::ISimpleLogger>();
            auto config = inj.resolve<IConfigService>();
            return std::make_shared<UserService>(logger, config);
        });
    }
};

int main() {
    try {
        std::cout << "Starting DI test program..." << std::endl;

        // Create and configure the injector
        di::Injector injector;
        
        // Configure core services
        di::CoreModule::configure(injector);
        
        // Configure test services
        TestModule::configure(injector);

        // Test the resolve<T>() interface (backward compatibility)
        auto logger = injector.resolve<di::ISimpleLogger>();
        logger->logDebug("DI system initialized using resolve<T>()");

        auto config = injector.resolve<IConfigService>();
        std::cout << "App name (via resolve): " << config->getValue("app.name") << std::endl;

        // Test the get<T>() interface
        auto userService = injector.get<IUserService>();
        std::cout << "Current user (via get): " << userService->getCurrentUser() << std::endl;

        std::cout << "DI test completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 