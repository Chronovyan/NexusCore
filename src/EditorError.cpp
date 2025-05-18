#include "EditorError.h"
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <direct.h>  // For _getcwd on Windows
#endif

// Define static members of ErrorReporter
bool ErrorReporter::debugLoggingEnabled = false;
bool ErrorReporter::suppressAllWarnings = false;
EditorException::Severity ErrorReporter::severityThreshold = EditorException::Severity::Warning;
std::vector<std::unique_ptr<LogDestination>> ErrorReporter::destinations_;
std::mutex ErrorReporter::destinationsMutex_;
std::map<std::string, RetryEvent> ErrorReporter::pendingRetries_;
std::mutex ErrorReporter::retryMutex_;

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
    // Create the directory if it doesn't exist
    try {
        std::filesystem::path logPath(config_.filePath);
        std::filesystem::path logDir = logPath.parent_path();
        
        if (!logDir.empty() && !std::filesystem::exists(logDir)) {
            std::filesystem::create_directories(logDir);
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

void FileLogDestination::write(EditorException::Severity severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    // Check if we need to rotate before writing
    checkRotation();
    
    // If file isn't open, try to open it
    if (!logFile_.is_open()) {
        openFile();
        if (!logFile_.is_open()) {
            return; // Couldn't open file
        }
    }
    
    // Format: [YYYY-MM-DD HH:MM:SS] [SEVERITY] Message
    std::string timestampedMessage = "[" + getTimestamp() + "] " + message;
    
    // Write to file
    logFile_ << timestampedMessage << std::endl;
    
    // Update current size
    currentSize_ += timestampedMessage.size() + 1; // +1 for newline
}

void FileLogDestination::flush() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
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
                    std::filesystem::remove(logFiles[i]);
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error rotating log file: " << e.what() << std::endl;
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
    
    logFile_.open(config_.filePath, mode);
    
    if (logFile_.is_open()) {
        if (!config_.appendMode) {
            // For non-append mode, write header
            logFile_ << "[" << getTimestamp() << "] === Log Started ===" << std::endl;
            currentSize_ += 50; // Approximate size of header
        } else if (config_.appendMode) {
            // Get current size for append mode
            logFile_.seekp(0, std::ios::end);
            currentSize_ = static_cast<size_t>(logFile_.tellp());
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
    destinations_.push_back(std::move(destination));
}

void ErrorReporter::clearLogDestinations() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    destinations_.clear();
}

void ErrorReporter::initializeDefaultLogging() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    
    // Clear existing destinations
    destinations_.clear();
    
    // Add console destination
    destinations_.push_back(std::make_unique<ConsoleLogDestination>());
}

void ErrorReporter::enableFileLogging(
    const std::string& filePath, 
    bool append,
    FileLogDestination::RotationType rotationType,
    size_t maxSizeBytes,
    int maxFileCount) {
    
    // Create file log configuration
    FileLogDestination::Config config;
    config.filePath = filePath;
    config.appendMode = append;
    config.rotationType = rotationType;
    config.maxSizeBytes = maxSizeBytes;
    config.maxFileCount = maxFileCount;
    
    // Add file destination
    addLogDestination(std::make_unique<FileLogDestination>(config));
}

void ErrorReporter::logException(const EditorException& ex) {
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
    // Skip debug messages if disabled
    if (!debugLoggingEnabled) {
        return;
    }
    
    if (EditorException::Severity::Debug < severityThreshold) {
        return;
    }
    
    writeToDestinations(EditorException::Severity::Debug, "Debug: " + message);
}

void ErrorReporter::logError(const std::string& message) {
    // Errors are always logged (no filtering)
    writeToDestinations(EditorException::Severity::Error, "Error: " + message);
}

void ErrorReporter::logWarning(const std::string& message) {
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
    logError("Unknown exception in " + context);
}

void ErrorReporter::setSeverityThreshold(EditorException::Severity threshold) {
    severityThreshold = threshold;
}

void ErrorReporter::flushLogs() {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    for (auto& destination : destinations_) {
        destination->flush();
    }
}

void ErrorReporter::writeToDestinations(EditorException::Severity severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(destinationsMutex_);
    
    // If we have no destinations, create default console destination
    if (destinations_.empty()) {
        destinations_.push_back(std::make_unique<ConsoleLogDestination>());
    }
    
    // Write to all destinations
    for (auto& destination : destinations_) {
        destination->write(severity, message);
    }
}

std::string ErrorReporter::getSeverityString(EditorException::Severity severity) {
    switch (severity) {
        case EditorException::Severity::Debug: return "Debug";
        case EditorException::Severity::Warning: return "Warning";
        case EditorException::Severity::Error: return "Error";
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