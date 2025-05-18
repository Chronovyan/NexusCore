#ifndef APP_DEBUG_LOG_H
#define APP_DEBUG_LOG_H

// Suppress warnings for localtime on Windows
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <fstream>
#include <iostream>  // For std::cerr
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

class AppDebugLog {
public:
    static AppDebugLog& getInstance() {
        static AppDebugLog instance;
        return instance;
    }

    void initialize(const std::string& appName) {
        if (isInitialized) return;
        
        try {
            // Create logs directory if it doesn't exist
            std::filesystem::create_directories("logs");
            
            // Use app name and timestamp for log file
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            
            // Format time for filename
            #ifdef _MSC_VER
            // Windows-specific thread-safe time formatting
            struct tm timeinfo;
            localtime_s(&timeinfo, &time);
            ss << "logs/" << appName << "_" 
               << std::put_time(&timeinfo, "%Y%m%d_%H%M%S") 
               << ".log";
            #else
            // Unix/other platforms
            ss << "logs/" << appName << "_" 
               << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") 
               << ".log";
            #endif
            
            logFilePath = ss.str();
            logFile.open(logFilePath, std::ios::out);
            
            if (logFile.is_open()) {
                isInitialized = true;
                log("Log initialized for " + appName);
                log("Log file: " + logFilePath);
            }
            else {
                // Fallback to console if file can't be created
                std::cerr << "Failed to open log file: " << logFilePath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error initializing log: " << e.what() << std::endl;
        }
    }

    void log(const std::string& message) {
        if (!isInitialized) {
            std::cerr << "WARNING: Log not initialized: " << message << std::endl;
            return;
        }
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        try {
            #ifdef _MSC_VER
            // Windows-specific thread-safe time formatting
            struct tm timeinfo;
            localtime_s(&timeinfo, &time);
            logFile << std::put_time(&timeinfo, "[%H:%M:%S] ") 
                    << message << std::endl;
            #else
            // Unix/other platforms
            logFile << std::put_time(std::localtime(&time), "[%H:%M:%S] ") 
                    << message << std::endl;
            #endif
            
            logFile.flush();
        }
        catch (const std::exception& e) {
            std::cerr << "Error writing to log: " << e.what() << std::endl;
        }
    }
    
    void logError(const std::string& message) {
        log("ERROR: " + message);
    }

    ~AppDebugLog() {
        if (isInitialized && logFile.is_open()) {
            log("Log closed");
            logFile.close();
        }
    }

private:
    AppDebugLog() : isInitialized(false) {}
    
    bool isInitialized;
    std::ofstream logFile;
    std::string logFilePath;
};

// Convenience macros for logging
#define LOG_INIT(appName) AppDebugLog::getInstance().initialize(appName)
#define LOG_DEBUG(message) AppDebugLog::getInstance().log(message)
#define LOG_ERROR(message) AppDebugLog::getInstance().logError(message)

#endif // APP_DEBUG_LOG_H 