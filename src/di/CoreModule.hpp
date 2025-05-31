#pragma once

#include "Injector.hpp"
#include <iostream>

namespace di {

/**
 * Simple logger interface for demo purposes
 * 
 * This interface demonstrates a basic service that can be registered with the DI container.
 * It provides logging functionality that can be used by other services.
 */
class ISimpleLogger {
public:
    virtual ~ISimpleLogger() = default;
    
    /**
     * Log a standard message
     * @param message The message to log
     */
    virtual void log(const std::string& message) = 0;
    
    /**
     * Log a debug message
     * @param message The debug message to log
     */
    virtual void logDebug(const std::string& message) = 0;
    
    /**
     * Log an error message
     * @param message The error message to log
     */
    virtual void logError(const std::string& message) = 0;
};

/**
 * Simple console logger implementation
 * 
 * A concrete implementation of ISimpleLogger that writes messages to the console.
 * This class is used as an example of a service that can be registered with the DI container.
 */
class ConsoleLogger : public ISimpleLogger {
public:
    void log(const std::string& message) override {
        std::cout << "[LOG] " << message << std::endl;
    }
    
    void logDebug(const std::string& message) override {
        std::cout << "[DEBUG] " << message << std::endl;
    }
    
    void logError(const std::string& message) override {
        std::cerr << "[ERROR] " << message << std::endl;
    }
};

/**
 * CoreModule registers essential services for the application.
 * This module should be configured first before any other modules.
 * 
 * The CoreModule pattern demonstrates a way to organize service registrations
 * in logical groups. Each module is responsible for registering a related set
 * of services, making the DI configuration more modular and maintainable.
 * 
 * Example usage:
 * ```cpp
 * Injector injector;
 * CoreModule::configure(injector);
 * 
 * // After configuration, services can be resolved
 * auto logger = injector.get<ISimpleLogger>();
 * logger->log("Application started");
 * ```
 */
class CoreModule {
public:
    /**
     * Configure the core services with the injector.
     * 
     * This method registers all core services with the provided injector.
     * It should be called early in the application lifecycle, before
     * other modules are configured.
     * 
     * @param injector The injector to configure
     */
    static void configure(Injector& injector) {
        std::cout << "Configuring CoreModule..." << std::endl;
        
        // Register a simple logger as a core service
        injector.registerFactory<ISimpleLogger>([]() {
            std::cout << "Creating new ConsoleLogger instance" << std::endl;
            return std::make_shared<ConsoleLogger>();
        });
        
        // Here we would register other core services
        
        std::cout << "CoreModule configured successfully" << std::endl;
    }
};

} // namespace di 