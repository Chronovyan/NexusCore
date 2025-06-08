#include "EditorError.h"
#include "RetryStats.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <cstring> // For strerror

// Initialize static members
bool DISABLE_ALL_LOGGING_FOR_TESTS = false;

// Initialize static members
bool ErrorReporter::debugLoggingEnabled = true;
bool ErrorReporter::suppressAllWarnings = false;
EditorException::Severity ErrorReporter::severityThreshold = EditorException::Severity::Debug;

// Initialize log destinations
std::vector<std::unique_ptr<LogDestination, ErrorReporter::LogDestDeleter>> ErrorReporter::destinations_;
std::mutex ErrorReporter::destinationsMutex_;

// Initialize retry tracking
std::map<std::string, RetryEvent> ErrorReporter::pendingRetries_;
std::mutex ErrorReporter::retryMutex_;

// Initialize async logging components
std::queue<QueuedLogMessage> ErrorReporter::logQueue_;
std::mutex ErrorReporter::queueMutex_;
std::condition_variable ErrorReporter::queueCondition_;
std::condition_variable ErrorReporter::queueNotFullCondition_;
std::thread ErrorReporter::workerThread_;
std::atomic<bool> ErrorReporter::shutdownWorker_(false);
bool ErrorReporter::asyncLoggingEnabled_ = false;
bool ErrorReporter::workerThreadRunning_ = false;

// Initialize queue configuration and metrics
size_t ErrorReporter::maxQueueSize_ = 10000;
QueueOverflowPolicy ErrorReporter::queueOverflowPolicy_ = QueueOverflowPolicy::DropOldest;
std::atomic<size_t> ErrorReporter::queueOverflowCount_(0);
std::atomic<size_t> ErrorReporter::queueHighWaterMark_(0);

// Implementation of LogDestination methods
void ConsoleLogDestination::write(EditorException::Severity severity, const std::string& message) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS) return;
    
    if (severity >= EditorException::Severity::Warning) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

void ConsoleLogDestination::flush() {
    std::cout.flush();
    std::cerr.flush();
}

// Implementation of FileLogDestination methods
FileLogDestination::FileLogDestination(const Config& config) 
    : config_(config), currentSize_(0) {
    openFile();
}

FileLogDestination::~FileLogDestination() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void FileLogDestination::write(EditorException::Severity severity, const std::string& message) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS) return;
    
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    // Check if we need to rotate the log file
    checkRotation();
    
    if (logFile_.is_open()) {
        logFile_ << message << std::endl;
        currentSize_ += message.size() + 1; // +1 for newline
    }
}

void FileLogDestination::flush() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
    }
}

void FileLogDestination::checkRotation() {
    std::string currentDate = getDateStamp();
    
    // Check if we need to rotate based on date
    if (config_.rotationType == RotationType::Daily || 
        config_.rotationType == RotationType::Weekly) {
        if (currentDate != currentDateStamp_) {
            rotateFile();
            return;
        }
    }
    
    // Check if we need to rotate based on size
    if (config_.rotationType == RotationType::Size && 
        currentSize_ >= config_.maxSizeBytes) {
        rotateFile();
    }
}

void FileLogDestination::rotateFile() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
    
    // Rename existing log files
    if (config_.maxFileCount > 0) {
        try {
            std::filesystem::path logPath(config_.filePath);
            std::string logDir = logPath.parent_path().string();
            std::string logBaseName = logPath.stem().string();
            std::string logExt = logPath.extension().string();
            
            // Delete the oldest log file if we've reached the maximum count
            std::string oldestLog = logDir + "/" + logBaseName + "." + 
                                  std::to_string(config_.maxFileCount - 1) + logExt;
            if (std::filesystem::exists(oldestLog)) {
                std::filesystem::remove(oldestLog);
            }
            
            // Shift all other log files
            for (int i = config_.maxFileCount - 2; i >= 0; --i) {
                std::string oldName = (i == 0) ? 
                    config_.filePath : 
                    logDir + "/" + logBaseName + "." + std::to_string(i) + logExt;
                    
                std::string newName = logDir + "/" + logBaseName + "." + 
                                    std::to_string(i + 1) + logExt;
                
                if (std::filesystem::exists(oldName)) {
                    std::filesystem::rename(oldName, newName);
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "Error rotating log files: " << ex.what() << std::endl;
        }
    }
    
    // Open a new log file
    openFile();
}

void FileLogDestination::openFile() {
    try {
        // Create the directory if it doesn't exist
        std::filesystem::path logPath(config_.filePath);
        std::filesystem::create_directories(logPath.parent_path());
        
        // Open the log file
        logFile_.open(config_.filePath, 
                     std::ios::out | 
                     (config_.appendMode ? std::ios::app : std::ios::trunc));
        
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << config_.filePath << std::endl;
            return;
        }
        
        // Get the current file size for appending
        if (config_.appendMode) {
            logFile_.seekp(0, std::ios::end);
            currentSize_ = logFile_.tellp();
        } else {
            currentSize_ = 0;
        }
        
        // Update the current date stamp
        currentDateStamp_ = getDateStamp();
        
        // Write a header
        if (currentSize_ == 0) {
            logFile_ << "=== Log started at " << getDetailedTimestamp() << " ===" << std::endl;
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "Error opening log file: " << ex.what() << std::endl;
    }
}

std::string FileLogDestination::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string FileLogDestination::getDateStamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string FileLogDestination::getDetailedTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm;
    localtime_s(&tm, &time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' 
        << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// Implementation of ErrorReporter methods
void ErrorReporter::addLogDestination(std::unique_ptr<LogDestination> destination) {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    destinations_.emplace_back(destination.release(), [](LogDestination* p) { delete p; });
}

void ErrorReporter::addLogDestination(LogDestination* destination) {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    destinations_.emplace_back(destination, [](LogDestination*) {});
}

void ErrorReporter::clearLogDestinations() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    destinations_.clear();
}

void ErrorReporter::initializeDefaultLogging() {
    clearLogDestinations();
    addLogDestination(std::make_unique<ConsoleLogDestination>());
}

void ErrorReporter::enableFileLogging(
    const std::string& filePath, 
    bool append,
    FileLogDestination::RotationType rotationType,
    size_t maxSizeBytes,
    int maxFileCount) {
    
    FileLogDestination::Config config;
    config.filePath = filePath;
    config.appendMode = append;
    config.rotationType = rotationType;
    config.maxSizeBytes = maxSizeBytes;
    config.maxFileCount = maxFileCount;
    
    addLogDestination(std::make_unique<FileLogDestination>(config));
}

void ErrorReporter::enableAsyncLogging(bool enable) {
    if (enable && !asyncLoggingEnabled_) {
        asyncLoggingEnabled_ = true;
        if (!workerThreadRunning_) {
            initializeAsyncLogging();
        }
    } else if (!enable && asyncLoggingEnabled_) {
        shutdownAsyncLogging();
    }
}

void ErrorReporter::initializeAsyncLogging() {
    if (workerThreadRunning_) {
        return;
    }
    
    shutdownWorker_ = false;
    workerThread_ = std::thread(workerThreadFunction);
    workerThreadRunning_ = true;
}

void ErrorReporter::shutdownAsyncLogging() {
    if (!asyncLoggingEnabled_ || !workerThreadRunning_) {
        return;
    }
    
    // Signal the worker thread to exit
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        shutdownWorker_ = true;
    }
    
    // Notify the worker thread
    queueCondition_.notify_one();
    
    // Wait for the worker thread to finish
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    workerThreadRunning_ = false;
    asyncLoggingEnabled_ = false;
    
    // Process any remaining messages
    processRemainingMessages();
}

void ErrorReporter::logException(const EditorException& ex) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS) return;
    
    std::string severityStr = getSeverityString(ex.getSeverity());
    std::string message = getDetailedTimestamp() + " [" + severityStr + "] " + ex.what();
    
    if (asyncLoggingEnabled_) {
        enqueueMessage(ex.getSeverity(), message);
    } else {
        writeToDestinations(ex.getSeverity(), message);
    }
}

void ErrorReporter::logDebug(const std::string& message) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS) return;
    
    std::string formattedMessage = getDetailedTimestamp() + " [DEBUG] " + message;
    
    if (asyncLoggingEnabled_) {
        enqueueMessage(EditorException::Severity::Debug, formattedMessage);
    } else {
        writeToDestinations(EditorException::Severity::Debug, formattedMessage);
    }
}

void ErrorReporter::logError(const std::string& message) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS) return;
    
    std::string formattedMessage = getDetailedTimestamp() + " [ERROR] " + message;
    
    if (asyncLoggingEnabled_) {
        enqueueMessage(EditorException::Severity::EDITOR_ERROR, formattedMessage);
    } else {
        writeToDestinations(EditorException::Severity::EDITOR_ERROR, formattedMessage);
    }
}

void ErrorReporter::logWarning(const std::string& message) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS || suppressAllWarnings) return;
    
    std::string formattedMessage = getDetailedTimestamp() + " [WARNING] " + message;
    
    if (asyncLoggingEnabled_) {
        enqueueMessage(EditorException::Severity::Warning, formattedMessage);
    } else {
        writeToDestinations(EditorException::Severity::Warning, formattedMessage);
    }
}

void ErrorReporter::logUnknownException(const std::string& context) {
    if (DISABLE_ALL_LOGGING_FOR_TESTS) return;
    
    std::string message = getDetailedTimestamp() + " [CRITICAL] Unknown exception in " + context;
    
    if (asyncLoggingEnabled_) {
        enqueueMessage(EditorException::Severity::Critical, message);
    } else {
        writeToDestinations(EditorException::Severity::Critical, message);
    }
}

void ErrorReporter::logRetryAttempt(
    const std::string& operationId,
    const std::string& operationType,
    int attempt,
    const std::string& reason,
    std::chrono::milliseconds delayMs) {
    
    std::lock_guard<std::mutex> lock(retryMutex_);
    
    RetryEvent& event = pendingRetries_[operationId];
    event.operationId = operationId;
    event.operationType = operationType;
    event.attemptNumber = attempt;
    event.errorReason = reason;
    event.timestamp = std::chrono::system_clock::now();
    event.delay = delayMs;
    
    std::string message = "Retry attempt #" + std::to_string(attempt) + 
                         " for " + operationType + 
                         " (ID: " + operationId + "): " + reason +
                         ", delay: " + std::to_string(delayMs.count()) + "ms";
    
    logDebug(message);
}

void ErrorReporter::logRetryResult(
    const std::string& operationId,
    bool success,
    const std::string& details) {
    
    std::lock_guard<std::mutex> lock(retryMutex_);
    
    auto it = pendingRetries_.find(operationId);
    if (it == pendingRetries_.end()) {
        logWarning("logRetryResult called for unknown operation ID: " + operationId);
        return;
    }
    
    RetryEvent& event = it->second;
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - event.timestamp);
    
    // Update retry statistics
    // Record the attempt in the retry stats
    RetryStats& retryStats = RetryStats::getInstance();
    RetryEvent retryEvent = event; // Make a copy
    retryEvent.successful = success;
    retryEvent.errorReason = details;
    retryStats.recordRetry(retryEvent);
    
    std::string message = "Retry " + std::string(success ? "succeeded" : "failed") +
                         " for " + event.operationType +
                         " (ID: " + operationId + ") after " +
                         std::to_string(duration.count()) + "ms";
    
    if (!details.empty()) {
        message += ": " + details;
    }
    
    if (success) {
        logDebug(message);
    } else {
        logError(message);
    }
    
    // Remove the completed retry event
    pendingRetries_.erase(it);
}

OperationStatsData ErrorReporter::getRetryStats(const std::string& operationType) {
    return RetryStats::getInstance().getOperationStatsData(operationType);
}

void ErrorReporter::resetRetryStats() {
    RetryStats::getInstance().reset();
}

void ErrorReporter::setSeverityThreshold(EditorException::Severity threshold) {
    severityThreshold = threshold;
}

void ErrorReporter::flushLogs() {
    if (asyncLoggingEnabled_) {
        // If async logging is enabled, process all queued messages
        processRemainingMessages();
    }
    
    // Flush all destinations
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    for (auto& dest : destinations_) {
        if (dest) {
            dest->flush();
        }
    }
}

std::string ErrorReporter::getSeverityString(EditorException::Severity severity) {
    switch (severity) {
        case EditorException::Severity::Debug:    return "DEBUG";
        case EditorException::Severity::Warning:  return "WARNING";
        case EditorException::Severity::EDITOR_ERROR: return "ERROR";
        case EditorException::Severity::Critical: return "CRITICAL";
        default:                            return "UNKNOWN";
    }
}

std::string ErrorReporter::getDetailedTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' 
        << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void ErrorReporter::configureAsyncQueue(
    size_t maxQueueSize,
    QueueOverflowPolicy overflowPolicy) {
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    maxQueueSize_ = maxQueueSize;
    queueOverflowPolicy_ = overflowPolicy;
}

AsyncQueueStats ErrorReporter::getAsyncQueueStats() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    AsyncQueueStats stats;
    stats.currentQueueSize = logQueue_.size();
    stats.maxQueueSizeConfigured = maxQueueSize_;
    stats.highWaterMark = queueHighWaterMark_;
    stats.overflowCount = queueOverflowCount_;
    stats.policy = queueOverflowPolicy_;
    
    return stats;
}

void ErrorReporter::workerThreadFunction() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        // Wait for messages or shutdown signal
        queueCondition_.wait(lock, [] {
            return !logQueue_.empty() || shutdownWorker_;
        });
        
        // Check if we should exit
        if (shutdownWorker_ && logQueue_.empty()) {
            break;
        }
        
        // Process messages in batches for efficiency
        std::queue<QueuedLogMessage> batch;
        batch.swap(logQueue_);
        
        // Notify any waiting threads that the queue has space
        if (queueOverflowPolicy_ == QueueOverflowPolicy::BlockProducer) {
            queueNotFullCondition_.notify_all();
        }
        
        // Release the lock while processing the batch
        lock.unlock();
        
        // Process all messages in the batch
        while (!batch.empty()) {
            const auto& msg = batch.front();
            writeToDestinations(msg.severity, msg.formattedMessage);
            batch.pop();
        }
    }
}

void ErrorReporter::processRemainingMessages() {
    std::queue<QueuedLogMessage> batch;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        batch.swap(logQueue_);
    }
    
    while (!batch.empty()) {
        const auto& msg = batch.front();
        writeToDestinations(msg.severity, msg.formattedMessage);
        batch.pop();
    }
}

void ErrorReporter::enqueueMessage(EditorException::Severity severity, const std::string& message) {
    // Skip if below severity threshold
    if (severity < severityThreshold) {
        return;
    }
    
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    // Check if the queue is full
    if (maxQueueSize_ > 0 && logQueue_.size() >= maxQueueSize_) {
        queueOverflowCount_++;
        
        switch (queueOverflowPolicy_) {
            case QueueOverflowPolicy::WarnOnly:
                // Allow queue to grow beyond max size, just log a warning
                if (logQueue_.size() >= maxQueueSize_) {
                    std::cerr << "Warning: Log queue size (" << logQueue_.size() 
                              << ") exceeds maximum configured size (" 
                              << maxQueueSize_ << ")" << std::endl;
                }
                break;
            case QueueOverflowPolicy::DropNewest:
                // Drop the new message
                return;
                
            case QueueOverflowPolicy::DropOldest:
                // Remove the oldest message
                logQueue_.pop();
                break;
                
            case QueueOverflowPolicy::BlockProducer:
                // Wait for space to become available
                queueNotFullCondition_.wait(lock, [&] {
                    return logQueue_.size() < maxQueueSize_;
                });
                break;
        }
    }
    
    // Add the new message
    logQueue_.emplace(severity, message);
    
    // Update high water mark
    if (logQueue_.size() > queueHighWaterMark_) {
        queueHighWaterMark_ = logQueue_.size();
    }
    
    // Notify the worker thread
    queueCondition_.notify_one();
}

void ErrorReporter::writeToDestinations(EditorException::Severity severity, const std::string& message) {
    if (severity < severityThreshold) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    
    for (auto& dest : destinations_) {
        if (dest) {
            try {
                dest->write(severity, message);
            } catch (const std::exception& ex) {
                // Log the error to stderr as a last resort
                std::cerr << "Error writing to log destination: " << ex.what() << std::endl;
            }
        }
    }
}
