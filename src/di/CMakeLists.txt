cmake_minimum_required(VERSION 3.10)

# Project declaration for the standalone build
project(DIFramework VERSION 1.0.0 LANGUAGES CXX)

# Add the DI library
add_library(di
    Injector.hpp
    ModuleManager.hpp
    ModuleManager.cpp
    CoreModule.hpp
    DIPatterns.hpp
    LifetimeManager.hpp
    AppDebugLog.h
)

# Set include directories for the library
target_include_directories(di PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Set C++17 for the library
target_compile_features(di PUBLIC cxx_std_17)

# Create a simple DI test executable with a unique name
add_executable(standalone_di_test
    di_test.cpp
)

# Create a lifetime manager example executable
add_executable(lifetime_example
    LifetimeExample.cpp
)

# Link the test with the DI library
target_link_libraries(standalone_di_test PRIVATE di)

# Link the lifetime example with the DI library
target_link_libraries(lifetime_example PRIVATE di)

# Set C++17 for the executables
target_compile_features(standalone_di_test PUBLIC cxx_std_17)
target_compile_features(lifetime_example PUBLIC cxx_std_17)

# Tests could be added in the future 

# DI Framework CMakeLists.txt

# Make sure the tests directory exists
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Define source files for the DIFramework library
set(DI_FRAMEWORK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/DIFramework.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LifetimeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Injector.cpp
)

# Create the static library
add_library(DIFramework STATIC ${DI_FRAMEWORK_SOURCES})

# Set include directories for the library
target_include_directories(DIFramework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Create a simple test file for the DI framework
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/tests/DITest.cpp
"#include <gtest/gtest.h>
#include <di/DIFramework.hpp>
#include <iostream>

TEST(DITest, SimpleTest) {
    di::DIFramework framework;
    EXPECT_TRUE(true);
}
")

# Add test executable
add_executable(DITest 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/DITest.cpp
)

# Link with DIFramework and gtest
target_link_libraries(DITest
    DIFramework
    gtest
    gtest_main
)

# Add to CTest
add_test(NAME DITest COMMAND DITest)

# Create a comprehensive test file for the DI framework
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/tests/DIFrameworkTest.cpp
"#include <gtest/gtest.h>
#include <di/DIFramework.hpp>
#include <di/LifetimeManager.hpp>
#include <di/Injector.hpp>
#include <iostream>
#include <memory>
#include <string>

using namespace di;
using namespace di::lifetime;

// Helper interfaces and classes for testing
namespace di::test {

// Interface for a greeter
class IGreeter {
public:
    virtual ~IGreeter() = default;
    virtual std::string greet(const std::string& name) const = 0;
};

// Simple greeter implementation
class SimpleGreeter : public IGreeter {
public:
    std::string greet(const std::string& name) const override {
        return \"Hello, \" + name + \"!\";
    }
};

// Disposable greeter implementation
class DisposableGreeter : public IGreeter, public IDisposable {
public:
    DisposableGreeter() {
        std::cout << \"DisposableGreeter created: \" << this << std::endl;
    }
    
    ~DisposableGreeter() {
        std::cout << \"DisposableGreeter destroyed: \" << this << std::endl;
    }
    
    std::string greet(const std::string& name) const override {
        return \"Hello, \" + name + \" (from disposable greeter)!\";
    }
    
    void dispose() override {
        std::cout << \"DisposableGreeter::dispose called: \" << this << std::endl;
        isDisposed_ = true;
    }
    
    bool isDisposed() const {
        return isDisposed_;
    }
    
private:
    bool isDisposed_ = false;
};

// A service that depends on IGreeter
class GreetingService {
public:
    GreetingService(std::shared_ptr<IGreeter> greeter) : greeter_(greeter) {
        std::cout << \"GreetingService created with greeter: \" << greeter_.get() << std::endl;
    }
    
    std::string generateGreeting(const std::string& name) const {
        return greeter_->greet(name) + \" Welcome to our service!\";
    }
    
private:
    std::shared_ptr<IGreeter> greeter_;
};

// A disposable service for testing disposal
class CounterService : public IDisposable {
public:
    CounterService() {
        std::cout << \"CounterService created: \" << this << std::endl;
        counter_++;
        instanceId_ = counter_;
    }
    
    ~CounterService() {
        std::cout << \"CounterService destroyed: \" << this << std::endl;
    }
    
    void dispose() override {
        std::cout << \"CounterService::dispose called for instance \" << instanceId_ << \": \" << this << std::endl;
        isDisposed_ = true;
    }
    
    bool isDisposed() const {
        return isDisposed_;
    }
    
    int getInstanceId() const {
        return instanceId_;
    }
    
    static void resetCounter() {
        counter_ = 0;
    }
    
private:
    bool isDisposed_ = false;
    int instanceId_ = 0;
    static int counter_;
};

int CounterService::counter_ = 0;

} // namespace di::test

class DIFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        di::test::CounterService::resetCounter();
    }
};

// Basic compilation test
TEST_F(DIFrameworkTest, BasicCompilationTest) {
    std::cout << \"[\" << __func__ << \"] Creating DIFramework instance\" << std::endl;
    DIFramework framework;
    std::cout << \"[\" << __func__ << \"] Destroying DIFramework instance\" << std::endl;
    
    // Just a simple check that we can create and destroy the framework
    EXPECT_TRUE(true);
}

// Test registering and resolving a transient service
TEST_F(DIFrameworkTest, TransientServiceRegistration) {
    DIFramework framework;
    
    // Register a transient service
    framework.registerType<di::test::IGreeter, di::test::SimpleGreeter>(ServiceLifetime::Transient);
    
    // Resolve the service twice
    auto greeter1 = framework.resolve<di::test::IGreeter>();
    auto greeter2 = framework.resolve<di::test::IGreeter>();
    
    // Check that we got valid instances
    EXPECT_NE(greeter1, nullptr);
    EXPECT_NE(greeter2, nullptr);
    
    // Transient services should be different instances
    EXPECT_NE(greeter1, greeter2);
    
    // Test the functionality
    EXPECT_EQ(greeter1->greet(\"Alice\"), \"Hello, Alice!\");
    EXPECT_EQ(greeter2->greet(\"Bob\"), \"Hello, Bob!\");
}

// Test registering and resolving a singleton service
TEST_F(DIFrameworkTest, SingletonServiceRegistration) {
    DIFramework framework;
    
    // Register a singleton service
    framework.registerType<di::test::IGreeter, di::test::SimpleGreeter>(ServiceLifetime::Singleton);
    
    // Resolve the service twice
    auto greeter1 = framework.resolve<di::test::IGreeter>();
    auto greeter2 = framework.resolve<di::test::IGreeter>();
    
    // Check that we got valid instances
    EXPECT_NE(greeter1, nullptr);
    EXPECT_NE(greeter2, nullptr);
    
    // Singleton services should be the same instance
    EXPECT_EQ(greeter1, greeter2);
    
    // Test the functionality
    EXPECT_EQ(greeter1->greet(\"Alice\"), \"Hello, Alice!\");
    EXPECT_EQ(greeter2->greet(\"Bob\"), \"Hello, Bob!\");
}

// Test registering and resolving a service with a factory function
TEST_F(DIFrameworkTest, FactoryFunctionRegistration) {
    DIFramework framework;
    
    // Register a service with a custom factory function
    framework.registerFactory<di::test::IGreeter>([](){ 
        return std::make_shared<di::test::SimpleGreeter>(); 
    });
    
    // Resolve the service
    auto greeter = framework.resolve<di::test::IGreeter>();
    
    // Check that we got a valid instance
    EXPECT_NE(greeter, nullptr);
    
    // Test the functionality
    EXPECT_EQ(greeter->greet(\"Charlie\"), \"Hello, Charlie!\");
}

// Test registering and resolving dependent services
TEST_F(DIFrameworkTest, DependentServiceRegistration) {
    DIFramework framework;
    
    // Register the dependency
    framework.registerType<di::test::IGreeter, di::test::SimpleGreeter>();
    
    // Register the service that depends on IGreeter
    framework.registerFactory<di::test::GreetingService>([&framework](){ 
        auto greeter = framework.resolve<di::test::IGreeter>();
        return std::make_shared<di::test::GreetingService>(greeter);
    });
    
    // Resolve the service
    auto service = framework.resolve<di::test::GreetingService>();
    
    // Check that we got a valid instance
    EXPECT_NE(service, nullptr);
    
    // Test the functionality
    EXPECT_EQ(service->generateGreeting(\"Dave\"), 
              \"Hello, Dave! Welcome to our service!\");
}

// Test registering and resolving a service in a scope
TEST_F(DIFrameworkTest, ScopedServiceRegistration) {
    DIFramework framework;
    
    // Register a scoped service
    framework.registerType<di::test::CounterService, di::test::CounterService>(ServiceLifetime::Scoped);
    
    // Create a scope
    auto scope = framework.createScope();
    
    // Resolve the service twice in the scope
    auto service1 = scope->resolve<di::test::CounterService>();
    auto service2 = scope->resolve<di::test::CounterService>();
    
    // Check that we got valid instances
    EXPECT_NE(service1, nullptr);
    EXPECT_NE(service2, nullptr);
    
    // Scoped services should be the same instance within a scope
    EXPECT_EQ(service1, service2);
    EXPECT_EQ(service1->getInstanceId(), 1);
    
    // Create another scope
    auto scope2 = framework.createScope();
    
    // Resolve the service in the second scope
    auto service3 = scope2->resolve<di::test::CounterService>();
    
    // Check that we got a valid instance
    EXPECT_NE(service3, nullptr);
    
    // The service in the second scope should be a different instance
    EXPECT_NE(service1, service3);
    EXPECT_EQ(service3->getInstanceId(), 2);
}

// Test disposal of disposable services
TEST_F(DIFrameworkTest, DisposableServiceDisposal) {
    {
        DIFramework framework;
        
        // Register a disposable service
        framework.registerType<di::test::DisposableGreeter, di::test::DisposableGreeter>();
        
        // Resolve the service
        auto greeter = framework.resolve<di::test::DisposableGreeter>();
        
        // Check that we got a valid instance
        EXPECT_NE(greeter, nullptr);
        EXPECT_FALSE(greeter->isDisposed());
        
        // Test the functionality
        EXPECT_EQ(greeter->greet(\"Eve\"), \"Hello, Eve (from disposable greeter)!\");
        
        // Create a scope
        auto scope = framework.createScope();
        
        // Register a scoped disposable service
        scope->registerType<di::test::CounterService, di::test::CounterService>(ServiceLifetime::Scoped);
        
        // Resolve the service
        auto service = scope->resolve<di::test::CounterService>();
        
        // Check that we got a valid instance
        EXPECT_NE(service, nullptr);
        EXPECT_FALSE(service->isDisposed());
        
        // Manually dispose the scope
        scope->dispose();
        
        // The service should now be disposed
        EXPECT_TRUE(service->isDisposed());
    }
    // The framework and all its services should be disposed when it goes out of scope
}

// Main function to run the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
")

# Create a second test executable using the comprehensive test file
add_executable(DI_Framework_Test 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/DIFrameworkTest.cpp
)

# Link with DIFramework and gtest
target_link_libraries(DI_Framework_Test
    DIFramework
    gtest
    gtest_main
)

# Add to CTest
add_test(NAME DI_Framework_Test COMMAND DI_Framework_Test)

# Create a simple test executable for debugging
add_executable(SimpleTest 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/SimpleTest.cpp
)

# Link with DIFramework and gtest
target_link_libraries(SimpleTest
    DIFramework
    gtest
    gtest_main
)

# Add to CTest
add_test(NAME SimpleTest COMMAND SimpleTest)

# Create the SingletonDebugTest executable for debugging
add_executable(SingletonDebugTest 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/SingletonDebugTest.cpp
)

# Link with DIFramework and gtest
target_link_libraries(SingletonDebugTest
    DIFramework
    gtest
    gtest_main
)

# Add to CTest
add_test(NAME SingletonDebugTest COMMAND SingletonDebugTest)

# Install the library and headers
install(TARGETS DIFramework
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES 
    DIFramework.hpp
    LifetimeManager.hpp
    Injector.hpp
    DESTINATION include/di
)

# Add an executable for the request scope example
add_executable(request_scope_example RequestScopeExample.cpp)
target_link_libraries(request_scope_example PRIVATE DIFramework ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(request_scope_example PROPERTIES FOLDER "Tests/DI")

# Add test for RequestScopeExample
add_test(NAME RequestScopeExample COMMAND request_scope_example) 