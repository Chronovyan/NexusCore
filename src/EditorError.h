#ifndef EDITOR_ERROR_H
#define EDITOR_ERROR_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "RetryStats.h"  // Include the retry stats header

// Global flag for disabling logging during tests
extern bool DISABLE_ALL_LOGGING_FOR_TESTS;

// Forward declarations
class TextBuffer;
class SyntaxHighlighter;

// Base class for all editor-specific exceptions
class EditorException : public std::runtime_error {
public:
    enum class Severity {
        Debug,      // Verbose debugging information
        Warning,    // Non-fatal but noteworthy
        Error,      // Operation failed but editor can continue
        Critical    // Serious error that may require termination or recovery
    };

    EditorException(const std::string& message, Severity severity = Severity::Error)
        : std::runtime_error(message), severity_(severity) {}

    // Get the severity level of the exception
    Severity getSeverity() const { return severity_; }

    // Get a string representation of the severity
    std::string getSeverityString() const {
        switch (severity_) {
            case Severity::Debug: return "Debug";
            case Severity::Warning: return "Warning";
            case Severity::Error: return "Error";
            case Severity::Critical: return "Critical Error";
            default: return "Unknown";
        }
    }

    // Get a formatted error message with severity
    std::string getFormattedMessage() const {
        return getSeverityString() + ": " + what();
    }

private:
    Severity severity_;
};

// Specialized exceptions for different error categories
class TextBufferException : public EditorException {
public:
    TextBufferException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("TextBuffer: " + message, severity) {}
};

class CommandException : public EditorException {
public:
    CommandException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("Command: " + message, severity) {}
};

class SyntaxHighlightingException : public EditorException {
public:
    SyntaxHighlightingException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("Syntax Highlighting: " + message, severity) {}
};

class FileOperationException : public EditorException {
public:
    FileOperationException(const std::string& message, Severity severity = Severity::Error)
        : EditorException("File Operation: " + message, severity) {}
};

/**
 * @enum QueueOverflowPolicy
 * @brief Defines strategies for handling queue overflow in asynchronous logging
 */
enum class QueueOverflowPolicy {
    DropOldest,    // Remove oldest message when queue is full (FIFO overflow)
    DropNewest,    // Reject new messages when queue is full (preserve history)
    BlockProducer, // Block calling thread until space is available (back pressure)
    WarnOnly       // Log warnings but allow queue to grow beyond limit
};

/**
 * @struct AsyncQueueStats
 * @brief Contains statistics about the asynchronous logging queue
 */
struct AsyncQueueStats {
    size_t currentQueueSize;       // Current number of messages in queue
    size_t maxQueueSizeConfigured; // Maximum queue size configured
    size_t highWaterMark;          // Maximum queue size ever reached
    size_t overflowCount;          // Number of messages dropped or rejected due to overflow
    QueueOverflowPolicy policy;    // Current overflow policy in use
};

/**
 * @class LogDestination
 * @brief Abstract interface for log output destinations
 * 
 * This interface defines the contract for all log destination implementations,
 * allowing logs to be sent to various outputs (console, file, etc.)
 */
class LogDestination {
public:
    virtual ~LogDestination() = default;
    
    /**
     * @brief Write a log message to this destination
     * 
     * @param severity The severity level of the message
     * @param message The log message to write
     */
    virtual void write(EditorException::Severity severity, const std::string& message) = 0;
    
    /**
     * @brief Flush any buffered log data to ensure it's persisted
     */
    virtual void flush() = 0;
};

/**
 * @class ConsoleLogDestination
 * @brief Implementation of LogDestination that writes to the console
 * 
 * This class writes log messages to the console, with different severity
 * levels going to stdout (debug) or stderr (warnings, errors).
 */
class ConsoleLogDestination : public LogDestination {
public:
    ConsoleLogDestination() = default;
    ~ConsoleLogDestination() override = default;
    
    void write(EditorException::Severity severity, const std::string& message) override;
    void flush() override;
};

/**
 * @class FileLogDestination
 * @brief Implementation of LogDestination that writes to a file
 * 
 * This class writes log messages to a file, with support for log rotation
 * based on file size, time periods, or both.
 */
class FileLogDestination : public LogDestination {
public:
    /**
     * @brief Enumeration of log rotation strategies
     */
    enum class RotationType {
        None,    // No rotation, single log file
        Size,    // Rotate when file reaches maxSize
        Daily,   // Create a new file each day
        Weekly   // Create a new file each week
    };

    /**
     * @brief Configuration for file logging
     */
    struct Config {
        std::string filePath;          // Path to log file
        bool appendMode;               // Append to existing file?
        RotationType rotationType;     // Rotation strategy
        size_t maxSizeBytes;           // Max file size before rotation
        int maxFileCount;              // Keep this many old logs
        
        // Constructor with default values
        Config() 
            : filePath("logs/editor.log"),
              appendMode(true),
              rotationType(RotationType::Size),
              maxSizeBytes(10 * 1024 * 1024),
              maxFileCount(5) {}
    };

    /**
     * @brief Construct a file log destination
     * 
     * @param config Configuration for the file logging
     */
    explicit FileLogDestination(const Config& config = Config());
    
    /**
     * @brief Destructor - ensures log file is properly closed
     */
    ~FileLogDestination() override;
    
    void write(EditorException::Severity severity, const std::string& message) override;
    void flush() override;

private:
    Config config_;
    std::ofstream logFile_;
    std::mutex fileMutex_;
    size_t currentSize_ = 0;
    std::string currentDateStamp_;
    
    // Helper methods for log file management
    void checkRotation();
    void rotateFile();
    void openFile();
    std::string getTimestamp();
    std::string getDateStamp();
    std::string getDetailedTimestamp();
};

/**
 * @struct QueuedLogMessage
 * @brief Container for log messages in the asynchronous queue
 * 
 * This structure holds all the necessary information for a log message
 * that has been queued for asynchronous processing.
 */
struct QueuedLogMessage {
    EditorException::Severity severity;
    std::string formattedMessage;
    
    // Default constructor
    QueuedLogMessage() 
        : severity(EditorException::Severity::Debug), formattedMessage("") {}
    
    QueuedLogMessage(EditorException::Severity sev, std::string msg)
        : severity(sev), formattedMessage(std::move(msg)) {}
};

/**
 * @class ErrorReporter
 * @brief Enhanced error logging and reporting utility
 * 
 * This class provides a flexible logging system with support for multiple
 * output destinations, configurable log levels, file logging, and retry tracking.
 * It also supports asynchronous logging for improved performance.
 */
class ErrorReporter {
public:
    // Static flags
    static bool debugLoggingEnabled;
    static bool suppressAllWarnings;
    static EditorException::Severity severityThreshold;
    
    /**
     * @brief Add a new log destination
     * 
     * @param destination Unique pointer to the destination to add
     */
    static void addLogDestination(std::unique_ptr<LogDestination> destination);
    
    /**
     * @brief Remove all log destinations
     */
    static void clearLogDestinations();
    
    /**
     * @brief Initialize default logging (console only)
     */
    static void initializeDefaultLogging();
    
    /**
     * @brief Convenience method to set up file logging
     * 
     * @param filePath Path to the log file
     * @param append Whether to append to existing file
     * @param rotationType Type of log rotation to use
     * @param maxSizeBytes Maximum file size before rotation
     * @param maxFileCount Maximum number of log files to keep
     */
    static void enableFileLogging(
        const std::string& filePath = "logs/editor.log", 
        bool append = true,
        FileLogDestination::RotationType rotationType = FileLogDestination::RotationType::Size,
        size_t maxSizeBytes = 10 * 1024 * 1024,
        int maxFileCount = 5
    );
    
    /**
     * @brief Enable or disable asynchronous logging
     * 
     * When enabled, log messages are queued and processed in a background thread,
     * improving performance by not blocking the calling thread.
     * 
     * @param enable True to enable async logging, false to disable
     */
    static void enableAsyncLogging(bool enable);
    
    /**
     * @brief Initialize the asynchronous logging system
     * 
     * This starts the worker thread that processes queued log messages.
     * This is called automatically by enableAsyncLogging(true) if needed.
     */
    static void initializeAsyncLogging();
    
    /**
     * @brief Shutdown the asynchronous logging system
     * 
     * This stops the worker thread after flushing any queued messages.
     * This is called automatically when the program exits if async logging is enabled.
     */
    static void shutdownAsyncLogging();
    
    /**
     * @brief Log an exception
     * 
     * @param ex The exception to log
     */
    static void logException(const EditorException& ex);
    
    /**
     * @brief Log a debug message
     * 
     * @param message The message to log
     */
    static void logDebug(const std::string& message);
    
    /**
     * @brief Log an error message
     * 
     * @param message The message to log
     */
    static void logError(const std::string& message);
    
    /**
     * @brief Log a warning message
     * 
     * @param message The message to log
     */
    static void logWarning(const std::string& message);
    
    /**
     * @brief Log an unknown exception
     * 
     * @param context Context where the exception occurred
     */
    static void logUnknownException(const std::string& context);
    
    /**
     * @brief Log the start of a retry operation
     * 
     * @param operationId Unique identifier for this operation sequence
     * @param operationType Type of operation (e.g., "API Call", "Database", etc.)
     * @param attempt Which attempt number this is (1-based)
     * @param reason Reason for the retry
     * @param delayMs Delay before this retry in milliseconds
     */
    static void logRetryAttempt(
        const std::string& operationId,
        const std::string& operationType,
        int attempt,
        const std::string& reason,
        std::chrono::milliseconds delayMs
    );
    
    /**
     * @brief Log the result of a retry attempt
     * 
     * @param operationId ID matching the previous logRetryAttempt call
     * @param success Whether the retry was successful
     * @param details Additional details about the result
     */
    static void logRetryResult(
        const std::string& operationId,
        bool success,
        const std::string& details = ""
    );
    
    /**
     * @brief Get statistics for a specific operation type
     * 
     * @param operationType The type of operation
     * @return Statistics for the specified operation type
     */
    static OperationStatsData getRetryStats(const std::string& operationType);
    
    /**
     * @brief Reset all retry statistics
     */
    static void resetRetryStats();
    
    /**
     * @brief Set the severity threshold for logging
     * 
     * @param threshold Minimum severity level to log
     */
    static void setSeverityThreshold(EditorException::Severity threshold);
    
    /**
     * @brief Flush all log destinations and async queue if enabled
     */
    static void flushLogs();

    /**
     * @brief Convert severity enum to string
     * 
     * @param severity The severity level
     * @return String representation of the severity
     */
    static std::string getSeverityString(EditorException::Severity severity);
    
    /**
     * @brief Generate a formatted timestamp string for log messages
     * 
     * @return String containing the current timestamp in "YYYY-MM-DD HH:MM:SS" format
     */
    static std::string getDetailedTimestamp();

    /**
     * @brief Configure the asynchronous logging queue behavior
     * 
     * @param maxQueueSize Maximum number of messages allowed in the queue
     * @param overflowPolicy How to handle queue overflow situations
     */
    static void configureAsyncQueue(
        size_t maxQueueSize,
        QueueOverflowPolicy overflowPolicy = QueueOverflowPolicy::DropOldest
    );
    
    /**
     * @brief Get current statistics about the asynchronous logging queue
     * 
     * @return AsyncQueueStats containing queue metrics and configuration
     */
    static AsyncQueueStats getAsyncQueueStats();

private:
    // Collection of log destinations
    static std::vector<std::unique_ptr<LogDestination>> destinations_;
    static std::mutex destinationsMutex_;
    
    // Map of operation IDs to their current retry events
    static std::map<std::string, RetryEvent> pendingRetries_;
    static std::mutex retryMutex_;
    
    // Asynchronous logging components
    static std::queue<QueuedLogMessage> logQueue_;
    static std::mutex queueMutex_;
    static std::condition_variable queueCondition_;
    static std::condition_variable queueNotFullCondition_; // New: signals when queue has space
    static std::thread workerThread_;
    static std::atomic<bool> shutdownWorker_;
    static bool asyncLoggingEnabled_;
    static bool workerThreadRunning_;
    
    // Queue configuration and metrics
    static size_t maxQueueSize_;
    static QueueOverflowPolicy queueOverflowPolicy_;
    static std::atomic<size_t> queueOverflowCount_;
    static std::atomic<size_t> queueHighWaterMark_;
    
    /**
     * @brief Worker thread function for async logging
     * 
     * This function runs in a separate thread and processes queued log messages.
     */
    static void workerThreadFunction();
    
    /**
     * @brief Process all remaining messages in the queue
     * 
     * This is called during shutdown or when flush is requested.
     */
    static void processRemainingMessages();
    
    /**
     * @brief Enqueue a message for async processing
     * 
     * @param severity The severity level of the message
     * @param message The message to log
     */
    static void enqueueMessage(EditorException::Severity severity, const std::string& message);
    
    /**
     * @brief Write a message to all registered destinations
     * 
     * @param severity The severity level of the message
     * @param message The message to write
     */
    static void writeToDestinations(EditorException::Severity severity, const std::string& message);
};

// Static member declarations are kept in the header, but definitions moved to a source file

#endif // EDITOR_ERROR_H 