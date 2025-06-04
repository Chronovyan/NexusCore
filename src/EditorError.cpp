#include "EditorError.h"
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm> // Add algorithm header for std::sort

#ifdef _WIN32
#include <direct.h>  // For _getcwd on Windows
#endif

// Define static members of ErrorReporter
bool ErrorReporter::debugLoggingEnabled = false; // Disable debug logging by default for performance
bool ErrorReporter::suppressAllWarnings = false;
EditorException::Severity ErrorReporter::severityThreshold = EditorException::Severity::Debug; // Lower threshold for tests
std::vector<std::unique_ptr<LogDestination, ErrorReporter::LogDestDeleter>> ErrorReporter::destinations_;
std::mutex ErrorReporter::destinationsMutex_;
std::map<std::string, RetryEvent> ErrorReporter::pendingRetries_;
std::mutex ErrorReporter::retryMutex_;

// Define asynchronous logging static members
std::queue<QueuedLogMessage> ErrorReporter::logQueue_;
std::mutex ErrorReporter::queueMutex_;
std::condition_variable ErrorReporter::queueCondition_;
std::condition_variable ErrorReporter::queueNotFullCondition_;
std::thread ErrorReporter::workerThread_;
std::atomic<bool> ErrorReporter::shutdownWorker_(false);
bool ErrorReporter::asyncLoggingEnabled_ = false;
bool ErrorReporter::workerThreadRunning_ = false;

// Define bounded queue configuration and metrics
size_t ErrorReporter::maxQueueSize_ = std::numeric_limits<size_t>::max(); // Default to unbounded queue
QueueOverflowPolicy ErrorReporter::queueOverflowPolicy_ = QueueOverflowPolicy::DropOldest;
std::atomic<size_t> ErrorReporter::queueOverflowCount_(0);
std::atomic<size_t> ErrorReporter::queueHighWaterMark_(0);

// Reference to the global test flag
extern bool DISABLE_ALL_LOGGING_FOR_TESTS;

//------ ConsoleLogDestination Implementation ------

void ConsoleLogDestination::write(EditorException::Severity severity, const std::string& message) {
    // Use appropriate output stream based on severity
    if (severity == EditorException::Severity::Debug) {
        std::cout << message << std::endl;
    } else {
        std::cerr << message << std::endl;
    }
}

void ConsoleLogDestination::flush() {
    std::cout.flush();
    std::cerr.flush();
}

//------ FileLogDestination Implementation ------

FileLogDestination::FileLogDestination(const Config& config) 
    : config_(config), currentSize_(0) {
    std::cout << "FileLogDestination constructor: " << config.filePath << std::endl;
    
    // Create the directory if it doesn't exist
    try {
        std::filesystem::path logPath(config_.filePath);
        std::filesystem::path logDir = logPath.parent_path();
        
        if (!logDir.empty() && !std::filesystem::exists(logDir)) {
            std::filesystem::create_directories(logDir);
            std::cout << "Created log directory: " << logDir.string() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating log directory: " << e.what() << std::endl;
    }
    
    // Initialize date stamp for rotation
    currentDateStamp_ = getDateStamp();
    
    // Open the file
    openFile();
}

FileLogDestination::~FileLogDestination() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
        logFile_.close();
    }
}

void FileLogDestination::write([[maybe_unused]] EditorException::Severity severity, const std::string& message) {
    // Only produce debug output when explicitly enabled
    if (ErrorReporter::debugLoggingEnabled) {
        std::cout << "FileLogDestination::write called: " << message << std::endl;
    }
    
    // Format the message with timestamp
    std::string timestamp = getTimestamp();
    std::string formattedMessage = "[" + timestamp + "] " + message;
    
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    try {
        // Check if we need to rotate before writing
        checkRotation();
        
        // If file isn't open, try to open it
        if (!logFile_.is_open()) {
            openFile();
            if (!logFile_.is_open()) {
                std::cerr << "Failed to open log file in write operation." << std::endl;
                return; // Couldn't open file
            }
        }
        
        // Write to file
        logFile_ << formattedMessage << std::endl;
        
        // Update current size with new message
        currentSize_ += formattedMessage.size() + 1; // +1 for newline
        
        if (ErrorReporter::debugLoggingEnabled) {
            std::cout << "Written to file: " << formattedMessage << " (Size: " << currentSize_ << ")" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing to log file: " << e.what() << std::endl;
    }
}

void FileLogDestination::flush() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
        std::cout << "FileLogDestination::flush called" << std::endl;
    }
}

void FileLogDestination::checkRotation() {
    bool needRotation = false;
    
    // Check size-based rotation
    if (config_.rotationType == RotationType::Size && 
        currentSize_ >= config_.maxSizeBytes) {
        needRotation = true;
    }
    
    // Check time-based rotation
    std::string currentDate = getDateStamp();
    if ((config_.rotationType == RotationType::Daily ||
         config_.rotationType == RotationType::Weekly) && 
        currentDate != currentDateStamp_) {
        
        // For weekly rotation, only rotate on Monday (day 1)
        if (config_.rotationType != RotationType::Weekly || 
            std::chrono::system_clock::now().time_since_epoch().count() % 7 == 1) {
            needRotation = true;
            currentDateStamp_ = currentDate;
        }
    }
    
    // Perform rotation if needed
    if (needRotation) {
        std::cout << "Rotation needed, rotating log file..." << std::endl;
        rotateFile();
    }
}

void FileLogDestination::rotateFile() {
    // Close current file if open
    if (logFile_.is_open()) {
        logFile_.close();
    }
    
    try {
        // Generate rotated filename with detailed timestamp
        std::filesystem::path originalPath(config_.filePath);
        std::string extension = originalPath.extension().string();
        std::string stem = originalPath.stem().string();
        std::string parentPath = originalPath.parent_path().string();
        
        if (!parentPath.empty()) {
            parentPath += "/";
        }
        
        // Use detailed timestamp (YYYYMMDD-HHMMSS-mmm)
        std::string baseTimestamp = getDetailedTimestamp();
        std::string rotatedName = parentPath + stem + "-" + baseTimestamp + extension;
        
        // Check for filename collision and append counter if needed
        int counter = 1;
        while (std::filesystem::exists(rotatedName) && counter < 1000) {
            // Append counter to handle collision
            rotatedName = parentPath + stem + "-" + baseTimestamp + "-" + std::to_string(counter) + extension;
            counter++;
        }
        
        // Rename current log file to timestamped version
        if (std::filesystem::exists(config_.filePath)) {
            std::cout << "Renaming " << config_.filePath << " to " << rotatedName << std::endl;
            std::filesystem::rename(config_.filePath, rotatedName);
        }
        
        // Clean up old log files if we have too many
        if (config_.maxFileCount > 0) {
            // Get all log files with the same stem
            std::vector<std::filesystem::path> logFiles;
            
            for (const auto& entry : std::filesystem::directory_iterator(originalPath.parent_path())) {
                if (entry.is_regular_file() && 
                    entry.path().filename().string().find(stem) == 0 &&
                    entry.path().filename().string() != originalPath.filename().string()) {
                    logFiles.push_back(entry.path());
                }
            }
            
            // If we have too many, sort by name (which includes date) and delete oldest
            if (logFiles.size() > static_cast<size_t>(config_.maxFileCount - 1)) {
                std::sort(logFiles.begin(), logFiles.end());
                
                // Delete oldest files beyond our limit
                for (size_t i = 0; i < logFiles.size() - (config_.maxFileCount - 1); ++i) {
                    std::cout << "Removing old log file: " << logFiles[i].string() << std::endl;
                    std::filesystem::remove(logFiles[i]);
                }
            }
        }
        
    } catch (const std::exception&) {
        std::cerr << "Error rotating log file" << std::endl;
    }
    
    // Open new log file and reset size
    openFile();
    currentSize_ = 0;
}

void FileLogDestination::openFile() {
    std::ios_base::openmode mode = std::ios::out;
    if (config_.appendMode) {
        mode |= std::ios::app;
    }
    
    std::cout << "Opening log file: " << config_.filePath << " (append: " << config_.appendMode << ")" << std::endl;
    
    logFile_.open(config_.filePath, mode);
    
    if (logFile_.is_open()) {
        std::cout << "Log file opened successfully" << std::endl;
        if (!config_.appendMode) {
            // For non-append mode, write header
            std::string header = "[" + getTimestamp() + "] === Log Started ===";
            logFile_ << header << std::endl;
            logFile_.flush();
            currentSize_ = header.size() + 1; // +1 for newline
            std::cout << "Wrote header: " << header << std::endl;
        } else if (config_.appendMode) {
            // Get current size for append mode
            logFile_.seekp(0, std::ios::end);
            currentSize_ = static_cast<size_t>(logFile_.tellp());
            std::cout << "Opened in append mode, current size: " << currentSize_ << std::endl;
        }
    } else {
        std::cerr << "FileLogDestination::openFile - Failed to open log file!" << std::endl;
        // Try to diagnose the issue
        std::error_code ec;
        std::filesystem::path p(config_.filePath);
        std::filesystem::path parent = p.parent_path();
        
        std::cerr << "Parent directory: " << parent.string() << std::endl;
        std::cerr << "Parent exists: " << std::filesystem::exists(parent, ec) << std::endl;
        if (ec) {
            std::cerr << "Error checking parent: " << ec.message() << std::endl;
        }
        
        // Try to check permissions
        std::cerr << "Current working directory: ";
#ifdef _WIN32
        char cwd[1024];
        if (_getcwd(cwd, sizeof(cwd)) != NULL) {
            std::cerr << cwd << std::endl;
        } else {
            std::cerr << "Unknown (_getcwd error)" << std::endl;
        }
#else
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            std::cerr << cwd << std::endl;
        } else {
            std::cerr << "Unknown (getcwd error)" << std::endl;
        }
#endif
    }
}

std::string FileLogDestination::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string FileLogDestination::getDateStamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    ss << std::put_time(&tm_now, "%Y%m%d");
    return ss.str();
}

// Add a new helper method for detailed timestamp after the getDateStamp() method
std::string FileLogDestination::getDetailedTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // Get milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    // Format: YYYYMMDD-HHMMSS-mmm
    ss << std::put_time(&tm_now, "%Y%m%d-%H%M%S") << "-" 
       << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

//------ ErrorReporter Implementation ------

void ErrorReporter::addLogDestination(std::unique_ptr<LogDestination> destination) {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    std::cout << "Adding log destination" << std::endl;
    
    // Convert to our vector's unique_ptr type with the default deleter
    std::unique_ptr<LogDestination, LogDestDeleter> dest(
        destination.release(),
        [](LogDestination* p) { delete p; }
    );
    
    destinations_.push_back(std::move(dest));
}

void ErrorReporter::addLogDestination(LogDestination* destination) {
    // This version doesn't take ownership of the pointer
    // It is the caller's responsibility to ensure the pointer remains valid
    // This is typically used when a shared_ptr owns the object elsewhere
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    std::cout << "Adding log destination (raw pointer)" << std::endl;
    
    // Create a unique_ptr with a no-op deleter
    std::unique_ptr<LogDestination, LogDestDeleter> ptr(
        destination,
        [](LogDestination*) { /* No-op deleter */ }
    );
    
    // Add it to the destinations
    destinations_.push_back(std::move(ptr));
}

void ErrorReporter::clearLogDestinations() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    std::cout << "Clearing log destinations" << std::endl;
    destinations_.clear();
}

void ErrorReporter::initializeDefaultLogging() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    
    // Clear existing destinations
    destinations_.clear();
    
    // Add console destination
    std::cout << "Adding default console destination" << std::endl;
    auto console = new ConsoleLogDestination();
    destinations_.push_back(std::unique_ptr<LogDestination, LogDestDeleter>(
        console, 
        [](LogDestination* p) { delete p; }
    ));
}

void ErrorReporter::enableFileLogging(
    const std::string& filePath, 
    bool append,
    FileLogDestination::RotationType rotationType,
    size_t maxSizeBytes,
    int maxFileCount
) {
    FileLogDestination::Config config;
    config.filePath = filePath;
    config.appendMode = append;
    config.rotationType = rotationType;
    config.maxSizeBytes = maxSizeBytes;
    config.maxFileCount = maxFileCount;
    
    // Create file destination with the LogDestDeleter
    auto fileDestRaw = new FileLogDestination(config);
    addLogDestination(fileDestRaw);
}

void ErrorReporter::enableAsyncLogging(bool enable) {
    if (enable == asyncLoggingEnabled_) {
        return; // Already in the requested state
    }
    
    if (enable) {
        // Initialize the worker thread first, then set the flag
        initializeAsyncLogging();
        asyncLoggingEnabled_ = true;
    } else {
        // Disable async logging - first set flag, then shutdown worker
        asyncLoggingEnabled_ = false;
        shutdownAsyncLogging();
    }
}

void ErrorReporter::initializeAsyncLogging() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // Don't start thread if already running
    if (workerThreadRunning_) {
        return;
    }
    
    // Reset shutdown flag
    shutdownWorker_ = false;
    
    try {
        // Start worker thread
        workerThread_ = std::thread(workerThreadFunction);
        workerThreadRunning_ = true;
    }
    catch (const std::exception&) {
        // Fall back to synchronous logging
        workerThreadRunning_ = false;
        asyncLoggingEnabled_ = false;
    }
}

void ErrorReporter::shutdownAsyncLogging() {
    // Signal worker thread to stop
    shutdownWorker_ = true;
    
    // First, check if the worker thread is running
    bool isRunning = false;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        isRunning = workerThreadRunning_;
        if (!isRunning) {
            return; // Thread not running
        }
    }
    
    // Wake up worker thread
    queueCondition_.notify_one();
    
    // Wait for worker thread to complete
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        workerThreadRunning_ = false;
    }
    
    // Process any remaining messages directly
    processRemainingMessages();
}

void ErrorReporter::workerThreadFunction() {
    // Fast path for processing messages
    std::vector<QueuedLogMessage> batch;
    batch.reserve(100); // Pre-reserve space for larger batch processing
    
    while (true) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // If queue is empty, wait for work or shutdown signal
            if (logQueue_.empty()) {
                queueCondition_.wait_for(lock, std::chrono::milliseconds(20), [&]() {
                    return !logQueue_.empty() || shutdownWorker_;
                });
            }
            
            if (shutdownWorker_ && logQueue_.empty()) {
                break;  // Exit loop if shutdown and queue is empty
            }
            
            // Get current queue size to determine if we need to signal later
            size_t originalQueueSize = logQueue_.size();
            bool queueWasFull = originalQueueSize >= maxQueueSize_;
            
            // Quickly extract all messages from the queue under lock
            // This minimizes lock contention
            if (!logQueue_.empty()) {
                while (!logQueue_.empty() && batch.size() < 100) {
                    batch.push_back(std::move(logQueue_.front()));
                    logQueue_.pop();
                }
            }
            
            // If queue was full but now has space, notify waiting producers
            // Only relevant for BlockProducer policy
            if (queueWasFull && queueOverflowPolicy_ == QueueOverflowPolicy::BlockProducer && 
                logQueue_.size() < maxQueueSize_) {
                // Signal that there's now space in the queue
                queueNotFullCondition_.notify_all();
            }
        }  // Release lock before processing
        
        // Process batch of messages outside the lock
        if (!batch.empty()) {
            // Lock once for the batch
            std::lock_guard<std::mutex> destLock(destinationsMutex_);
            
            for (const auto& message : batch) {
                // Write to all destinations (without flushing each time)
                for (auto& destination : destinations_) {
                    destination->write(message.severity, message.formattedMessage);
                }
            }
            
            // Flush once after processing the batch
            for (auto& destination : destinations_) {
                destination->flush();
            }
            
            batch.clear();
        }
    }
}

void ErrorReporter::processRemainingMessages() {
    std::queue<QueuedLogMessage> remainingMessages;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        remainingMessages.swap(logQueue_);  // Efficiently get all messages
    }
    
    if (debugLoggingEnabled) {
        std::cout << "Processing remaining messages..." << std::endl;
    }
    
    int count = 0;
    
    // Quick check to avoid locking if there are no messages
    if (!remainingMessages.empty()) {
        std::lock_guard<std::mutex> destLock(destinationsMutex_);
        
        while (!remainingMessages.empty()) {
            auto& msg = remainingMessages.front();
            
            if (debugLoggingEnabled) {
                std::cout << "Processing remaining message: " << msg.formattedMessage << std::endl;
            }
            
            // Process the message directly
            for (auto& destination : destinations_) {
                destination->write(msg.severity, msg.formattedMessage);
            }
            
            remainingMessages.pop();
            count++;
        }
        
        // Flush once after processing all messages
        for (auto& destination : destinations_) {
            destination->flush();
        }
    }
    
    if (debugLoggingEnabled) {
        if (count > 0) {
            std::cout << "Processed " << count << " remaining messages." << std::endl;
        } else {
            std::cout << "No remaining messages to process." << std::endl;
        }
    }
}

void ErrorReporter::enqueueMessage(EditorException::Severity severity, const std::string& message) {
    // Fast path - avoid locking if async logging not enabled
    if (!asyncLoggingEnabled_) {
        // Fall back to synchronous logging if async not enabled
        std::lock_guard<std::mutex> lock(destinationsMutex_);
        for (auto& destination : destinations_) {
            destination->write(severity, message);
            destination->flush();
        }
        return;
    }
    
    // Fast path - avoid locking if worker thread not running
    if (!workerThreadRunning_) {
        // Fall back to synchronous logging if worker thread not running
        std::lock_guard<std::mutex> lock(destinationsMutex_);
        for (auto& destination : destinations_) {
            destination->write(severity, message);
            destination->flush();
        }
        return;
    }
    
    // Add message to queue with bounded queue handling
    bool messageQueued = false;
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        // Update high water mark if queue size is larger than previous max
        size_t currentSize = logQueue_.size();
        size_t currentHighWaterMark = queueHighWaterMark_.load();
        if (currentSize > currentHighWaterMark) {
            queueHighWaterMark_.store(currentSize);
        }
        
        // Check if queue is at max capacity
        if (currentSize >= maxQueueSize_) {
            // Handle according to policy
            switch (queueOverflowPolicy_) {
                case QueueOverflowPolicy::DropOldest:
                    // Remove oldest message to make room
                    if (debugLoggingEnabled) {
                        std::cout << "Queue overflow (DropOldest): Dropping oldest message" << std::endl;
                    }
                    logQueue_.pop();
                    queueOverflowCount_++;
                    
                    // Now add the new message
                    logQueue_.emplace(severity, message);
                    messageQueued = true;
                    break;
                    
                case QueueOverflowPolicy::DropNewest:
                    // Just drop the new message
                    if (debugLoggingEnabled) {
                        std::cout << "Queue overflow (DropNewest): Dropping new message" << std::endl;
                    }
                    queueOverflowCount_++;
                    messageQueued = false;
                    break;
                    
                case QueueOverflowPolicy::BlockProducer:
                    // Wait until queue has space
                    if (debugLoggingEnabled) {
                        std::cout << "Queue full: Blocking producer until space available" << std::endl;
                    }
                    
                    // Wait for space with a timeout to prevent potential deadlocks
                    while (logQueue_.size() >= maxQueueSize_ && !shutdownWorker_) {
                        queueNotFullCondition_.wait_for(lock, 
                                              std::chrono::milliseconds(100), 
                                              [&]() { 
                                                  return logQueue_.size() < maxQueueSize_ || shutdownWorker_; 
                                              });
                    }
                    
                    // Check if we're shutting down; if so, don't queue the message
                    if (shutdownWorker_) {
                        messageQueued = false;
                    } else {
                        // Now add the message since there's space
                        logQueue_.emplace(severity, message);
                        messageQueued = true;
                    }
                    break;
                    
                case QueueOverflowPolicy::WarnOnly:
                    // Log a warning (but not through the queue to avoid recursion)
                    if (logQueue_.size() % 1000 == 0) { // Only log periodically
                        std::cerr << "WARNING: Async log queue exceeding configured maximum size: " 
                                 << logQueue_.size() << "/" << maxQueueSize_ << std::endl;
                    }
                    // Add the message anyway (allow unbounded growth)
                    logQueue_.emplace(severity, message);
                    messageQueued = true;
                    break;
                    
                default:
                    // Unknown policy, default to unbounded behavior
                    logQueue_.emplace(severity, message);
                    messageQueued = true;
                    break;
            }
        } else {
            // Queue not full, add message normally
            logQueue_.emplace(severity, message);
            messageQueued = true;
        }
    }
    
    // Notify worker thread if message was queued
    if (messageQueued) {
        // Only notify worker if this is the first message or if there are many messages
        // This reduces context switching overhead
        static thread_local int localCounter = 0;
        if (++localCounter % 20 == 0) {
            queueCondition_.notify_one();
        }
    }
}

// Regular log methods implementation
void ErrorReporter::logException(const EditorException& ex) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    // Skip based on severity filters
    if (suppressAllWarnings && ex.getSeverity() <= EditorException::Severity::Warning) {
        return;
    }
    
    if (ex.getSeverity() < severityThreshold) {
        return;
    }
    
    // Format message with severity and log it
    writeToDestinations(ex.getSeverity(), "[" + getSeverityString(ex.getSeverity()) + "] " + ex.what());
}

void ErrorReporter::logDebug(const std::string& message) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    // If debug messages are below the threshold, don't log them
    if (EditorException::Severity::Debug < severityThreshold) {
        return;
    }
    
    // Format as: [timestamp] Debug: message
    // For consistency with exception formatting
    std::string formattedMessage = "[" + getDetailedTimestamp() + "] Debug: " + message;
    
    // Use either async or sync logging based on current settings
    if (asyncLoggingEnabled_ && workerThreadRunning_) {
        enqueueMessage(EditorException::Severity::Debug, formattedMessage);
    } else {
        writeToDestinations(EditorException::Severity::Debug, formattedMessage);
    }
}

void ErrorReporter::logError(const std::string& message) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    // Errors are always logged (no filtering)
    writeToDestinations(EditorException::Severity::EDITOR_ERROR, "Error: " + message);
}

void ErrorReporter::logWarning(const std::string& message) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    // Skip warnings if suppressed
    if (suppressAllWarnings) {
        return;
    }
    
    if (EditorException::Severity::Warning < severityThreshold) {
        return;
    }
    
    // Special handling for tests on Windows
    #ifdef _WIN32
    if (suppressAllWarnings) {
        return;
    }
    #endif
    
    writeToDestinations(EditorException::Severity::Warning, "Warning: " + message);
}

void ErrorReporter::logUnknownException(const std::string& context) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    logError("Unknown exception in " + context);
}

void ErrorReporter::setSeverityThreshold(EditorException::Severity threshold) {
    severityThreshold = threshold;
}

void ErrorReporter::flushLogs() {
    if (asyncLoggingEnabled_ && workerThreadRunning_) {
        if (debugLoggingEnabled) {
            std::cout << "Flushing async queue..." << std::endl;
        }
        
        // First notify worker thread to process any queued messages
        queueCondition_.notify_one();
        
        // Small delay to allow the worker thread to process messages
        // Increased slightly to ensure processing completes
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Process any remaining messages directly
        processRemainingMessages();
    }
    
    // Lock and flush all destinations
    {
        std::lock_guard<std::mutex> lock(destinationsMutex_);
        if (debugLoggingEnabled) {
            std::cout << "Flushing " << destinations_.size() << " destinations" << std::endl;
        }
        
        for (auto& destination : destinations_) {
            destination->flush();
        }
    }
}

void ErrorReporter::writeToDestinations(EditorException::Severity severity, const std::string& message) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    // Skip messages below the severity threshold
    if (severity < severityThreshold) {
        return;
    }
    
    // Skip warning messages if suppressed
    if (suppressAllWarnings && severity == EditorException::Severity::Warning) {
        return;
    }
    
    // If async logging is enabled and worker thread is running, queue the message
    if (asyncLoggingEnabled_ && workerThreadRunning_) {
        // Fast path for async logging - directly enqueue without function call overhead
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            logQueue_.emplace(severity, message);
        }
        
        // Notify worker only occasionally to reduce overhead
        static thread_local int notifyCounter = 0;
        if (++notifyCounter % 20 == 0) {
            queueCondition_.notify_one();
        }
        
        return;
    }
    
    // Empty destinations check - avoid locking
    if (destinations_.empty()) {
        return;
    }
    
    // Optimized synchronous logging path
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    
    // Write and flush in a single pass
    for (auto& destination : destinations_) {
        destination->write(severity, message);
    }
    
    for (auto& destination : destinations_) {
        destination->flush();
    }
}

std::string ErrorReporter::getSeverityString(EditorException::Severity severity) {
    switch (severity) {
        case EditorException::Severity::Debug: return "Debug";
        case EditorException::Severity::Warning: return "Warning";
        case EditorException::Severity::EDITOR_ERROR: return "Error";
        case EditorException::Severity::Critical: return "Critical";
        default: return "Unknown";
    }
}

void ErrorReporter::logRetryAttempt(
    const std::string& operationId,
    const std::string& operationType,
    int attempt,
    const std::string& reason,
    std::chrono::milliseconds delayMs
) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    // Create a new retry event
    RetryEvent event(operationId, operationType, attempt, reason, delayMs);
    
    // Store the pending retry in our map
    {
        std::lock_guard<std::mutex> lock(retryMutex_);
        pendingRetries_[operationId] = event;
    }
    
    // Log the retry attempt
    std::stringstream ss;
    ss << "Retry attempt #" << attempt << " for " << operationType 
       << " (ID: " << operationId << ") - Reason: " << reason
       << " - Delay: " << delayMs.count() << "ms";
    
    // Log as warnings for visibility, but enable filtering if needed
    writeToDestinations(EditorException::Severity::Warning, ss.str());
}

void ErrorReporter::logRetryResult(
    const std::string& operationId,
    bool success,
    const std::string& details
) {
    // Check global test flag first
    if (DISABLE_ALL_LOGGING_FOR_TESTS) {
        return;
    }
    
    RetryEvent event;
    
    // Retrieve and remove the pending retry
    {
        std::lock_guard<std::mutex> lock(retryMutex_);
        auto it = pendingRetries_.find(operationId);
        if (it != pendingRetries_.end()) {
            event = it->second;
            pendingRetries_.erase(it);
        } else {
            // Couldn't find the pending retry - log an error
            logError("Failed to find pending retry with ID: " + operationId);
            return;
        }
    }
    
    // Update the event with the result
    event.successful = success;
    
    // Record in the global statistics
    RetryStats::getInstance().recordRetry(event);
    
    // Log the result
    std::stringstream ss;
    ss << "Retry " << (success ? "succeeded" : "failed") << " for " << event.operationType
       << " (ID: " << operationId << ") - Attempt #" << event.attemptNumber;
    
    if (!details.empty()) {
        ss << " - " << details;
    }
    
    EditorException::Severity severity = success ? 
        EditorException::Severity::Debug : EditorException::Severity::Warning;
    
    writeToDestinations(severity, ss.str());
}

OperationStatsData ErrorReporter::getRetryStats(const std::string& operationType) {
    return RetryStats::getInstance().getOperationStatsData(operationType);
}

void ErrorReporter::resetRetryStats() {
    RetryStats::getInstance().reset();
    logDebug("Retry statistics have been reset");
}

// Add implementation for getDetailedTimestamp method in ErrorReporter
std::string ErrorReporter::getDetailedTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    tm tm_now;
    
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif
    
    ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void ErrorReporter::configureAsyncQueue(size_t maxQueueSize, QueueOverflowPolicy overflowPolicy) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // Log the configuration change
    if (debugLoggingEnabled) {
        std::string policyStr;
        switch (overflowPolicy) {
            case QueueOverflowPolicy::DropOldest: policyStr = "DropOldest"; break;
            case QueueOverflowPolicy::DropNewest: policyStr = "DropNewest"; break;
            case QueueOverflowPolicy::BlockProducer: policyStr = "BlockProducer"; break;
            case QueueOverflowPolicy::WarnOnly: policyStr = "WarnOnly"; break;
            default: policyStr = "Unknown";
        }
        
        std::cout << "Configuring async queue: maxSize=" << maxQueueSize 
                  << ", policy=" << policyStr << std::endl;
    }
    
    maxQueueSize_ = maxQueueSize;
    queueOverflowPolicy_ = overflowPolicy;
    queueOverflowCount_ = 0;  // Reset overflow counter
    queueHighWaterMark_ = logQueue_.size();  // Initialize to current size
}

AsyncQueueStats ErrorReporter::getAsyncQueueStats() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return {
        logQueue_.size(),            // currentQueueSize
        maxQueueSize_,               // maxQueueSizeConfigured
        queueHighWaterMark_.load(),  // highWaterMark
        queueOverflowCount_.load(),  // overflowCount
        queueOverflowPolicy_         // policy
    };
} 