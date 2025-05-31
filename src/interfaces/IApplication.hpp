#pragma once

#include <string>

/**
 * @interface IApplication
 * @brief Interface for the main application
 * 
 * This interface defines the contract for the main application class,
 * providing methods for initialization, running, and shutdown.
 */
class IApplication {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IApplication() = default;
    
    /**
     * @brief Initialize the application
     * 
     * @param argc Number of command line arguments
     * @param argv Array of command line arguments
     * @return True if initialization succeeded, false otherwise
     */
    virtual bool initialize(int argc, char** argv) = 0;
    
    /**
     * @brief Run the application
     * 
     * @return Exit code (0 for success, non-zero for error)
     */
    virtual int run() = 0;
    
    /**
     * @brief Shut down the application
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if the application is running
     * 
     * @return True if the application is running, false otherwise
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief Get the application name
     * 
     * @return The application name
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get the application version
     * 
     * @return The application version
     */
    virtual std::string getVersion() const = 0;
}; 