#ifndef EDITOR_ERROR_REPORTER_H
#define EDITOR_ERROR_REPORTER_H

#include "interfaces/IErrorReporter.hpp"
#include "EditorError.h"
#include "RetryStatsTracker.h"
#include <memory>
#include <string>
#include <map>
#include <mutex>

/**
 * @class EditorErrorReporter
 * @brief Implementation of IErrorReporter that delegates to the existing ErrorReporter
 * 
 * This class adapts the existing ErrorReporter static methods to implement
 * the IErrorReporter interface for better testability and dependency injection.
 */
class EditorErrorReporter : public IErrorReporter {
public:
    EditorErrorReporter();
    ~EditorErrorReporter() override;
    
    // IErrorReporter interface implementation
    void addLogDestination(std::unique_ptr<error_reporting::ILogDestination> destination) override;
    void clearLogDestinations() override;
    void initializeDefaultLogging() override;
    
    void enableFileLogging(
        const std::string& filePath, 
        bool append,
        int rotationType,
        size_t maxSizeBytes,
        int maxFileCount) override;
    
    void enableAsyncLogging(bool enable) override;
    void logDebug(const std::string& message) override;
    void logError(const std::string& message) override;
    void logWarning(const std::string& message) override;
    void logUnknownException(const std::string& context) override;
    
    void configureAsyncQueue(
        size_t maxQueueSize,
        error_reporting::QueueOverflowPolicy overflowPolicy) override;
    
    error_reporting::AsyncQueueStats getAsyncQueueStats() override;
    void setSeverityThreshold(error_reporting::Severity threshold) override;
    void flushLogs() override;
    
    void logRetryAttempt(
        const std::string& operationId,
        const std::string& operationType,
        int attempt,
        const std::string& reason,
        std::chrono::milliseconds delayMs) override;
    
    void logRetryResult(
        const std::string& operationId,
        bool success,
        const std::string& details) override;
    
    error_reporting::OperationStatsData getRetryStats(const std::string& operationType) override;
    void resetRetryStats() override;

private:
    // Helper method to convert between severity enum types
    static EditorException::Severity convertSeverity(error_reporting::Severity severity);
    static QueueOverflowPolicy convertOverflowPolicy(error_reporting::QueueOverflowPolicy policy);
    static error_reporting::QueueOverflowPolicy convertOverflowPolicy(QueueOverflowPolicy policy);
    
    // Custom log destinations adapter
    class LogDestinationAdapter : public LogDestination {
    public:
        explicit LogDestinationAdapter(std::unique_ptr<error_reporting::ILogDestination> destination);
        ~LogDestinationAdapter() override;
        
        void write(EditorException::Severity severity, const std::string& message) override;
        void flush() override;
        
    private:
        std::unique_ptr<error_reporting::ILogDestination> destination_;
    };
    
    // Track custom destinations
    std::vector<std::shared_ptr<LogDestinationAdapter>> customDestinations_;
    std::mutex destinationsMutex_;
    
    // Local retry stats tracking
    RetryStatsTracker retryStats_;
};

#endif // EDITOR_ERROR_REPORTER_H 