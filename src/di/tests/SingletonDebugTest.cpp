#include <gtest/gtest.h>
#include <di/DIFramework.hpp>
#include <iostream>
#include <memory>
#include <string>

using namespace di;
using namespace di::lifetime;

// Simple test service
class TestService {
public:
    TestService() {
        std::cout << "TestService created: " << this << std::endl;
    }
    
    ~TestService() {
        std::cout << "TestService destroyed: " << this << std::endl;
    }
    
    std::string getMessage() const {
        return "Hello from TestService!";
    }
};

// Test fixture
class SingletonDebugTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "--- Test starting ---" << std::endl;
    }
    
    void TearDown() override {
        std::cout << "--- Test ending ---" << std::endl;
    }
};

// Test just creating a framework instance
TEST_F(SingletonDebugTest, BasicFrameworkCreation) {
    std::cout << "Creating DIFramework instance" << std::endl;
    DIFramework framework;
    std::cout << "Framework created successfully" << std::endl;
}

// Test singleton registration only
TEST_F(SingletonDebugTest, RegisterSingleton) {
    std::cout << "Creating DIFramework instance" << std::endl;
    DIFramework framework;
    
    std::cout << "Registering singleton service" << std::endl;
    framework.registerType<TestService, TestService>(ServiceLifetime::Singleton);
    
    std::cout << "Service registered successfully" << std::endl;
}

// Test singleton resolution
TEST_F(SingletonDebugTest, ResolveSingleton) {
    std::cout << "Creating DIFramework instance" << std::endl;
    DIFramework framework;
    
    std::cout << "Registering singleton service" << std::endl;
    framework.registerType<TestService, TestService>(ServiceLifetime::Singleton);
    
    std::cout << "Resolving service" << std::endl;
    std::shared_ptr<TestService> service = nullptr;
    try {
        service = framework.resolve<TestService>();
        std::cout << "Service resolved successfully: " << service.get() << std::endl;
        EXPECT_NE(service, nullptr);
    }
    catch (const std::exception& ex) {
        std::cout << "Exception during resolve: " << ex.what() << std::endl;
        FAIL() << "Exception during resolve: " << ex.what();
    }
}

// Main function
int main(int argc, char** argv) {
    std::cout << "Starting SingletonDebugTest" << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    auto result = RUN_ALL_TESTS();
    std::cout << "Tests completed with result: " << result << std::endl;
    return result;
} 