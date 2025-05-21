#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <vector>

// Forward declarations for types used in the interface
namespace error_reporting {
    
/**
 * @enum Severity
 * @brief Defines severity levels for logging
 */
enum class Severity {
    Debug,      ///< Debug information
    Warning,    ///< Warnings that don't prevent operation
    Error,      ///< Errors that may affect operation
    Critical    ///< Critical errors that prevent operation
};

/**
 * @enum QueueOverflowPolicy
 * @brief Defines how to handle queue overflow in async logging
 */
enum class QueueOverflowPolicy {
    DropOldest,    ///< Remove oldest messages when queue is full
    DropNewest,    ///< Discard new messages when queue is full
    BlockProducer, ///< Block the producer thread until space is available
    WarnOnly       ///< Allow unbounded queue growth but report warnings
};

/**
 * @struct AsyncQueueStats
 * @brief Statistics about the async logging queue
 */
struct AsyncQueueStats {
    size_t currentQueueSize;       ///< Current number of messages in queue
    size_t maxQueueSizeConfigured; ///< Maximum queue size configured
    size_t highWaterMark;          ///< Maximum queue size ever reached
    size_t overflowCount;          ///< Number of messages dropped due to overflow
    QueueOverflowPolicy policy;    ///< Current overflow policy
};

/**
 * @struct OperationStatsData
 * @brief Statistics about retry operations
 */
struct OperationStatsData {
    size_t totalAttempts;       ///< Total retry attempts
    size_t successfulAttempts;  ///< Number of successful attempts
    size_t failedAttempts;      ///< Number of failed attempts
    double averageRetryCount;   ///< Average number of retries per operation
};

/**
 * @interface ILogDestination
 * @brief Interface for log output destinations
 */
class ILogDestination {
public:
    virtual ~ILogDestination() = default;
    
    /**
     * @brief Write a message to this destination
     * 
     * @param severity The severity level of the message
     * @param message The message to write
     */
    virtual void write(Severity severity, const std::string& message) = 0;
    
    /**
     * @brief Flush any buffered log messages
     */
    virtual void flush() = 0;
};

} // namespace error_reporting

/**
 * @interface IErrorReporter
 * @brief Interface for error logging and reporting
 * 
 * Defines the contract for components that handle logging and error reporting.
 */
class IErrorReporter {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IErrorReporter() = default;
    
    /**
     * @brief Add a new log destination
     * 
     * @param destination Unique pointer to the destination to add
     */
    virtual void addLogDestination(std::unique_ptr<error_reporting::ILogDestination> destination) = 0;
    
    /**
     * @brief Remove all log destinations
     */
    virtual void clearLogDestinations() = 0;
    
    /**
     * @brief Initialize default logging (console only)
     */
    virtual void initializeDefaultLogging() = 0;
    
    /**
     * @brief Enable file logging
     * 
     * @param filePath Path to the log file
     * @param append Whether to append to existing file
     * @param rotationType Type of rotation (size/date)
     * @param maxSizeBytes Maximum file size before rotation
     * @param maxFileCount Maximum number of rotated files
     */
    virtual void enableFileLogging(
        const std::string& filePath, 
        bool append,
        int rotationType,
        size_t maxSizeBytes,
        int maxFileCount) = 0;
    
    /**
     * @brief Enable or disable asynchronous logging
     * 
     * @param enable True to enable, false to disable
     */
    virtual void enableAsyncLogging(bool enable) = 0;
    
    /**
     * @brief Log a debug message
     * 
     * @param message The message to log
     */
    virtual void logDebug(const std::string& message) = 0;
    
    /**
     * @brief Log an error message
     * 
     * @param message The message to log
     */
    virtual void logError(const std::string& message) = 0;
    
    /**
     * @brief Log a warning message
     * 
     * @param message The message to log
     */
    virtual void logWarning(const std::string& message) = 0;
    
    /**
     * @brief Log an unknown exception
     * 
     * @param context Context information about where the exception occurred
     */
    virtual void logUnknownException(const std::string& context) = 0;
    
    /**
     * @brief Configure the asynchronous logging queue
     * 
     * @param maxQueueSize Maximum number of messages in the queue
     * @param overflowPolicy Policy to use when the queue is full
     */
    virtual void configureAsyncQueue(
        size_t maxQueueSize,
        error_reporting::QueueOverflowPolicy overflowPolicy) = 0;
    
    /**
     * @brief Get statistics about the async logging queue
     * 
     * @return Statistics structure
     */
    virtual error_reporting::AsyncQueueStats getAsyncQueueStats() = 0;
    
    /**
     * @brief Set the severity threshold for logging
     * 
     * @param threshold Minimum severity level to log
     */
    virtual void setSeverityThreshold(error_reporting::Severity threshold) = 0;
    
    /**
     * @brief Flush all log destinations
     */
    virtual void flushLogs() = 0;
    
    /**
     * @brief Log the start of a retry operation
     * 
     * @param operationId Unique identifier for this operation
     * @param operationType Type of operation being retried
     * @param attempt Attempt number (starting from 1)
     * @param reason Reason for the retry
     * @param delayMs Delay before the retry in milliseconds
     */
    virtual void logRetryAttempt(
        const std::string& operationId,
        const std::string& operationType,
        int attempt,
        const std::string& reason,
        std::chrono::milliseconds delayMs) = 0;
    
    /**
     * @brief Log the result of a retry attempt
     * 
     * @param operationId ID matching the previous logRetryAttempt call
     * @param success Whether the retry was successful
     * @param details Additional details about the result
     */
    virtual void logRetryResult(
        const std::string& operationId,
        bool success,
        const std::string& details) = 0;
    
    /**
     * @brief Get statistics for a specific operation type
     * 
     * @param operationType The type of operation
     * @return Statistics for the operation
     */
    virtual error_reporting::OperationStatsData getRetryStats(const std::string& operationType) = 0;
    
    /**
     * @brief Reset all retry statistics
     */
    virtual void resetRetryStats() = 0;
}; 