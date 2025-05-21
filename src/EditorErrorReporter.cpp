#include "EditorErrorReporter.h"
#include "EditorError.h"
#include <iostream>

// Constructor
EditorErrorReporter::EditorErrorReporter() {
    // Initialize with default retry stats tracking
    retryStats_.resetAllStats();
}

// Destructor - flush logs on shutdown
EditorErrorReporter::~EditorErrorReporter() {
    try {
        flushLogs();
    } catch (...) {
        // Prevent exceptions from escaping destructor
        std::cerr << "Error flushing logs during EditorErrorReporter destruction" << std::endl;
    }
}

// LogDestinationAdapter implementation
EditorErrorReporter::LogDestinationAdapter::LogDestinationAdapter(
    std::unique_ptr<error_reporting::ILogDestination> destination)
    : destination_(std::move(destination)) {
}

EditorErrorReporter::LogDestinationAdapter::~LogDestinationAdapter() = default;

void EditorErrorReporter::LogDestinationAdapter::write(
    EditorException::Severity severity, const std::string& message) {
    // Convert severity to destination's expected enum type
    auto destSeverity = static_cast<error_reporting::Severity>(severity);
    destination_->write(destSeverity, message);
}

void EditorErrorReporter::LogDestinationAdapter::flush() {
    destination_->flush();
}

// Convert between severity enum types
EditorException::Severity EditorErrorReporter::convertSeverity(error_reporting::Severity severity) {
    switch (severity) {
        case error_reporting::Severity::Debug:
            return EditorException::Severity::Debug;
        case error_reporting::Severity::Warning:
            return EditorException::Severity::Warning;
        case error_reporting::Severity::Error:
            return EditorException::Severity::Error;
        case error_reporting::Severity::Critical:
            return EditorException::Severity::Critical;
        default:
            return EditorException::Severity::Error;
    }
}

QueueOverflowPolicy EditorErrorReporter::convertOverflowPolicy(error_reporting::QueueOverflowPolicy policy) {
    switch (policy) {
        case error_reporting::QueueOverflowPolicy::DropOldest:
            return QueueOverflowPolicy::DropOldest;
        case error_reporting::QueueOverflowPolicy::DropNewest:
            return QueueOverflowPolicy::DropNewest;
        case error_reporting::QueueOverflowPolicy::BlockProducer:
            return QueueOverflowPolicy::BlockProducer;
        case error_reporting::QueueOverflowPolicy::WarnOnly:
            return QueueOverflowPolicy::WarnOnly;
        default:
            return QueueOverflowPolicy::DropOldest;
    }
}

error_reporting::QueueOverflowPolicy EditorErrorReporter::convertOverflowPolicy(QueueOverflowPolicy policy) {
    switch (policy) {
        case QueueOverflowPolicy::DropOldest:
            return error_reporting::QueueOverflowPolicy::DropOldest;
        case QueueOverflowPolicy::DropNewest:
            return error_reporting::QueueOverflowPolicy::DropNewest;
        case QueueOverflowPolicy::BlockProducer:
            return error_reporting::QueueOverflowPolicy::BlockProducer;
        case QueueOverflowPolicy::WarnOnly:
            return error_reporting::QueueOverflowPolicy::WarnOnly;
        default:
            return error_reporting::QueueOverflowPolicy::DropOldest;
    }
}

// Interface implementation
void EditorErrorReporter::addLogDestination(std::unique_ptr<error_reporting::ILogDestination> destination) {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    auto adapter = std::make_shared<LogDestinationAdapter>(std::move(destination));
    customDestinations_.push_back(adapter);
    ErrorReporter::addLogDestination(adapter);
}

void EditorErrorReporter::clearLogDestinations() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    ErrorReporter::clearLogDestinations();
    customDestinations_.clear();
}

void EditorErrorReporter::initializeDefaultLogging() {
    ErrorReporter::initializeDefaultLogging();
}

void EditorErrorReporter::enableFileLogging(
    const std::string& filePath,
    bool append,
    int rotationType,
    size_t maxSizeBytes,
    int maxFileCount) {
    
    // Convert rotation type to the FileLogDestination::RotationType
    FileLogDestination::RotationType type;
    switch (rotationType) {
        case 0: type = FileLogDestination::RotationType::None; break;
        case 1: type = FileLogDestination::RotationType::Size; break;
        case 2: type = FileLogDestination::RotationType::Daily; break;
        case 3: type = FileLogDestination::RotationType::Weekly; break;
        default: type = FileLogDestination::RotationType::Size; break;
    }
    
    ErrorReporter::enableFileLogging(filePath, append, type, maxSizeBytes, maxFileCount);
}

void EditorErrorReporter::enableAsyncLogging(bool enable) {
    ErrorReporter::enableAsyncLogging(enable);
}

void EditorErrorReporter::logDebug(const std::string& message) {
    ErrorReporter::logDebug(message);
}

void EditorErrorReporter::logError(const std::string& message) {
    ErrorReporter::logError(message);
}

void EditorErrorReporter::logWarning(const std::string& message) {
    ErrorReporter::logWarning(message);
}

void EditorErrorReporter::logUnknownException(const std::string& context) {
    ErrorReporter::logUnknownException(context);
}

void EditorErrorReporter::configureAsyncQueue(
    size_t maxQueueSize,
    error_reporting::QueueOverflowPolicy overflowPolicy) {
    
    QueueOverflowPolicy convertedPolicy = convertOverflowPolicy(overflowPolicy);
    ErrorReporter::configureAsyncQueue(maxQueueSize, convertedPolicy);
}

error_reporting::AsyncQueueStats EditorErrorReporter::getAsyncQueueStats() {
    AsyncQueueStats stats = ErrorReporter::getAsyncQueueStats();
    
    error_reporting::AsyncQueueStats convertedStats;
    convertedStats.currentQueueSize = stats.currentQueueSize;
    convertedStats.maxQueueSizeConfigured = stats.maxQueueSizeConfigured;
    convertedStats.highWaterMark = stats.highWaterMark;
    convertedStats.overflowCount = stats.overflowCount;
    convertedStats.policy = convertOverflowPolicy(stats.policy);
    
    return convertedStats;
}

void EditorErrorReporter::setSeverityThreshold(error_reporting::Severity threshold) {
    EditorException::Severity convertedThreshold = convertSeverity(threshold);
    ErrorReporter::setSeverityThreshold(convertedThreshold);
}

void EditorErrorReporter::flushLogs() {
    ErrorReporter::flushLogs();
}

void EditorErrorReporter::logRetryAttempt(
    const std::string& operationId,
    const std::string& operationType,
    int attempt,
    const std::string& reason,
    std::chrono::milliseconds delayMs) {
    
    ErrorReporter::logRetryAttempt(operationId, operationType, attempt, reason, delayMs);
    
    // Track in our local retry stats
    retryStats_.recordRetryAttempt(operationType);
}

void EditorErrorReporter::logRetryResult(
    const std::string& operationId,
    bool success,
    const std::string& details) {
    
    ErrorReporter::logRetryResult(operationId, success, details);
    
    // We don't have the operation type here, so we can't update retry stats
    // This would need to be enhanced to maintain an operationId->operationType map
}

error_reporting::OperationStatsData EditorErrorReporter::getRetryStats(const std::string& operationType) {
    RetryStats stats = retryStats_.getStats(operationType);
    
    error_reporting::OperationStatsData convertedStats;
    convertedStats.totalAttempts = stats.totalAttempts;
    convertedStats.successfulAttempts = stats.successfulAttempts;
    convertedStats.failedAttempts = stats.failedAttempts;
    convertedStats.averageRetryCount = stats.averageRetryCount;
    
    return convertedStats;
}

void EditorErrorReporter::resetRetryStats() {
    retryStats_.resetAllStats();
} 