#include "gtest/gtest.h"
#include "../src/EditorError.h"
#include "../src/SyntaxHighlightingManager.h"
#include <fstream>
#include <thread>
#include <cstdio>
#include <filesystem>
#include <random>

// Helper function to read a log file
std::string readLogFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ""; // File doesn't exist or couldn't be opened
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to generate a random ID for tests
std::string generateRandomId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(0, 0xFFFF);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8);
    ss << distrib(gen) << "-" << distrib(gen) << "-" << distrib(gen);
    return ss.str();
}

// Helper to clean up log files before/after tests
void cleanupLogFiles(const std::string& pattern = "test_*.log") {
    std::filesystem::path logsDir = "logs";
    if (!std::filesystem::exists(logsDir)) {
        return;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(logsDir)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.substr(0, 5) == "test_") {
                std::filesystem::remove(entry.path());
            }
        }
    }
}

class ErrorReporterTest : public ::testing::Test {
protected:
    bool originalDisableLogging;
    
    void SetUp() override {
        // Save original logging setting
        originalDisableLogging = DISABLE_ALL_LOGGING_FOR_TESTS;
        
        // Reset error reporter before each test
        ErrorReporter::clearLogDestinations();
        ErrorReporter::initializeDefaultLogging();
        ErrorReporter::setSeverityThreshold(EditorException::Severity::Debug);
        ErrorReporter::resetRetryStats();
        
        // Ensure logs directory exists
        std::filesystem::create_directories("logs");
        
        // Clean up any existing test log files
        cleanupLogFiles();
    }
    
    void TearDown() override {
        // Reset error reporter after each test
        ErrorReporter::clearLogDestinations();
        ErrorReporter::initializeDefaultLogging();
        
        // Restore original logging setting
        DISABLE_ALL_LOGGING_FOR_TESTS = originalDisableLogging;
        
        // Clean up test log files
        cleanupLogFiles();
    }
};

// Test that console logging works without crashing
TEST_F(ErrorReporterTest, ConsoleLoggingDoesNotCrash) {
    // With default console logging
    EXPECT_NO_THROW({
        ErrorReporter::logDebug("Test debug message");
        ErrorReporter::logWarning("Test warning message");
        ErrorReporter::logError("Test error message");
        ErrorReporter::logException(EditorException("Test exception"));
        ErrorReporter::logUnknownException("Test context");
    });
}

// Test that file logging works
TEST_F(ErrorReporterTest, FileLoggingWritesToFile) {
    // Save original settings so we can restore them
    bool originalDebugLoggingEnabled = ErrorReporter::debugLoggingEnabled;
    EditorException::Severity originalSeverityThreshold = ErrorReporter::severityThreshold;
    bool originalDisableAllLogging = DISABLE_ALL_LOGGING_FOR_TESTS;
    bool originalSuppressAllWarnings = ErrorReporter::suppressAllWarnings;
    
    // Enable debug logging for this test
    ErrorReporter::debugLoggingEnabled = true;
    ErrorReporter::setSeverityThreshold(EditorException::Severity::Debug);
    DISABLE_ALL_LOGGING_FOR_TESTS = false; // Critical: Ensure logging is enabled
    ErrorReporter::suppressAllWarnings = false; // Critical: Allow warnings
    
    // Set up file logging
    std::string logFile = "logs/test_file_logging.log";
    ErrorReporter::enableFileLogging(logFile, false); // Don't append
    
    // Log some messages
    ErrorReporter::logDebug("Debug message to file");
    ErrorReporter::logWarning("Warning message to file");
    ErrorReporter::logError("Error message to file");
    
    // Flush to ensure it's written
    ErrorReporter::flushLogs();
    
    // Read the log file
    std::string logContent = readLogFile(logFile);
    
    // Output log content to help debug
    std::cout << "Log file contents (" << logFile << "):" << std::endl;
    std::cout << "---" << std::endl;
    std::cout << logContent << std::endl;
    std::cout << "---" << std::endl;
    
    // Check that log file contains our messages
    EXPECT_TRUE(logContent.find("Debug message to file") != std::string::npos);
    EXPECT_TRUE(logContent.find("Warning message to file") != std::string::npos);
    EXPECT_TRUE(logContent.find("Error message to file") != std::string::npos);
    
    // Restore original settings
    ErrorReporter::debugLoggingEnabled = originalDebugLoggingEnabled;
    ErrorReporter::setSeverityThreshold(originalSeverityThreshold);
    DISABLE_ALL_LOGGING_FOR_TESTS = originalDisableAllLogging;
    ErrorReporter::suppressAllWarnings = originalSuppressAllWarnings;
}

// Test that multiple destinations receive logs
TEST_F(ErrorReporterTest, MultiDestinationLogging) {
    // Save original settings so we can restore them
    bool originalDisableAllLogging = DISABLE_ALL_LOGGING_FOR_TESTS;
    bool originalSuppressAllWarnings = ErrorReporter::suppressAllWarnings;
    
    // Enable required logging for this test
    DISABLE_ALL_LOGGING_FOR_TESTS = false; // Critical: Ensure logging is enabled
    ErrorReporter::suppressAllWarnings = false; // Critical: Allow warnings
    
    // Set up multiple file destinations
    std::string logFile1 = "logs/test_multi_dest1.log";
    std::string logFile2 = "logs/test_multi_dest2.log";
    
    ErrorReporter::enableFileLogging(logFile1, false);
    ErrorReporter::enableFileLogging(logFile2, false);
    
    // Log a message
    ErrorReporter::logError("Error message to multiple destinations");
    
    // Flush to ensure it's written
    ErrorReporter::flushLogs();
    
    // Read both log files
    std::string logContent1 = readLogFile(logFile1);
    std::string logContent2 = readLogFile(logFile2);
    
    // Check that both log files contain our message
    EXPECT_TRUE(logContent1.find("Error message to multiple destinations") != std::string::npos);
    EXPECT_TRUE(logContent2.find("Error message to multiple destinations") != std::string::npos);
    
    // Restore original settings
    DISABLE_ALL_LOGGING_FOR_TESTS = originalDisableAllLogging;
    ErrorReporter::suppressAllWarnings = originalSuppressAllWarnings;
}

// Test that severity filters work
TEST_F(ErrorReporterTest, SeverityFilters) {
    // Save original settings so we can restore them
    bool originalDebugLoggingEnabled = ErrorReporter::debugLoggingEnabled;
    EditorException::Severity originalSeverityThreshold = ErrorReporter::severityThreshold;
    bool originalDisableAllLogging = DISABLE_ALL_LOGGING_FOR_TESTS;
    bool originalSuppressAllWarnings = ErrorReporter::suppressAllWarnings;
    
    // Enable required logging for this test
    DISABLE_ALL_LOGGING_FOR_TESTS = false; // Critical: Ensure logging is enabled
    ErrorReporter::suppressAllWarnings = false; // Critical: Allow warnings
    
    // Set up file logging
    std::string logFile = "logs/test_severity_filters.log";
    ErrorReporter::enableFileLogging(logFile, false);
    
    // Set severity threshold to Warning
    ErrorReporter::setSeverityThreshold(EditorException::Severity::Warning);
    
    // Log messages of different severities
    ErrorReporter::logDebug("Debug message should be filtered");
    ErrorReporter::logWarning("Warning message should be logged");
    ErrorReporter::logError("Error message should be logged");
    
    // Flush to ensure it's written
    ErrorReporter::flushLogs();
    
    // Read the log file
    std::string logContent = readLogFile(logFile);
    
    // Output log content to help debug
    std::cout << "Log file contents (" << logFile << "):" << std::endl;
    std::cout << "---" << std::endl;
    std::cout << logContent << std::endl;
    std::cout << "---" << std::endl;
    
    // Check that only appropriate messages are present
    EXPECT_TRUE(logContent.find("Debug message should be filtered") == std::string::npos);
    EXPECT_TRUE(logContent.find("Warning message should be logged") != std::string::npos);
    EXPECT_TRUE(logContent.find("Error message should be logged") != std::string::npos);
    
    // Restore original settings
    ErrorReporter::debugLoggingEnabled = originalDebugLoggingEnabled;
    ErrorReporter::setSeverityThreshold(originalSeverityThreshold);
    DISABLE_ALL_LOGGING_FOR_TESTS = originalDisableAllLogging;
    ErrorReporter::suppressAllWarnings = originalSuppressAllWarnings;
}

// Test logging exceptions
TEST_F(ErrorReporterTest, ExceptionLogging) {
    // Save original settings so we can restore them
    bool originalDisableAllLogging = DISABLE_ALL_LOGGING_FOR_TESTS;
    bool originalSuppressAllWarnings = ErrorReporter::suppressAllWarnings;
    
    // Enable required logging for this test
    DISABLE_ALL_LOGGING_FOR_TESTS = false; // Critical: Ensure logging is enabled
    ErrorReporter::suppressAllWarnings = false; // Critical: Allow warnings
    
    // Set up file logging
    std::string logFile = "logs/test_exception_logging.log";
    ErrorReporter::enableFileLogging(logFile, false);
    
    // Log various exceptions
    try {
        throw EditorException("Test editor exception", EditorException::Severity::Error);
    } catch (const EditorException& e) {
        ErrorReporter::logException(e);
    }
    
    try {
        throw TextBufferException("Test buffer exception");
    } catch (const EditorException& e) {
        ErrorReporter::logException(e);
    }
    
    try {
        throw CommandException("Test command exception", EditorException::Severity::Critical);
    } catch (const EditorException& e) {
        ErrorReporter::logException(e);
    }
    
    // Log unknown exception
    ErrorReporter::logUnknownException("test context");
    
    // Flush to ensure it's written
    ErrorReporter::flushLogs();
    
    // Read the log file
    std::string logContent = readLogFile(logFile);
    
    // Check that all exceptions were logged
    EXPECT_TRUE(logContent.find("Test editor exception") != std::string::npos);
    EXPECT_TRUE(logContent.find("TextBuffer: Test buffer exception") != std::string::npos);
    EXPECT_TRUE(logContent.find("Command: Test command exception") != std::string::npos);
    EXPECT_TRUE(logContent.find("Unknown exception in test context") != std::string::npos);
    
    // Restore original settings
    DISABLE_ALL_LOGGING_FOR_TESTS = originalDisableAllLogging;
    ErrorReporter::suppressAllWarnings = originalSuppressAllWarnings;
}

// Test retry logging
TEST_F(ErrorReporterTest, RetryLogging) {
    // Set up file logging
    std::string logFile = "logs/test_retry_logging.log";
    ErrorReporter::enableFileLogging(logFile, false);
    
    std::string operationId = generateRandomId();
    std::string operationType = "API_Call";
    
    // Log a retry attempt
    ErrorReporter::logRetryAttempt(
        operationId,
        operationType,
        1,  // First attempt
        "ConnectionError",
        std::chrono::milliseconds(500)
    );
    
    // Log a successful result
    ErrorReporter::logRetryResult(
        operationId,
        true,  // Success
        "Connected after retry"
    );
    
    // Log another retry attempt
    std::string operationId2 = generateRandomId();
    ErrorReporter::logRetryAttempt(
        operationId2,
        operationType,
        2,  // Second attempt
        "ServerError",
        std::chrono::milliseconds(1000)
    );
    
    // Log an unsuccessful result
    ErrorReporter::logRetryResult(
        operationId2,
        false,  // Failure
        "Server still unavailable"
    );
    
    // Flush to ensure it's written
    ErrorReporter::flushLogs();
    
    // Read the log file
    std::string logContent = readLogFile(logFile);
    
    // Check retry-related log entries
    EXPECT_TRUE(logContent.find("Retry attempt #1") != std::string::npos);
    EXPECT_TRUE(logContent.find("ConnectionError") != std::string::npos);
    EXPECT_TRUE(logContent.find("Connected after retry") != std::string::npos);
    EXPECT_TRUE(logContent.find("Retry attempt #2") != std::string::npos);
    EXPECT_TRUE(logContent.find("ServerError") != std::string::npos);
    EXPECT_TRUE(logContent.find("Server still unavailable") != std::string::npos);
}

TEST_F(ErrorReporterTest, RetryStatsReset) {
    // Save original settings so we can restore them
    bool originalDisableAllLogging = DISABLE_ALL_LOGGING_FOR_TESTS;
    bool originalSuppressAllWarnings = ErrorReporter::suppressAllWarnings;
    
    // Enable required logging for this test
    DISABLE_ALL_LOGGING_FOR_TESTS = false; // Critical: Ensure logging is enabled
    ErrorReporter::suppressAllWarnings = false; // Critical: Allow warnings
    
    // First add some retry data
    std::string operationId = generateRandomId();
    std::string operationType = "API_Call";
    
    // Log several retry attempts and results
    for (int i = 1; i <= 5; i++) {
        ErrorReporter::logRetryAttempt(
            operationId,
            operationType,
            i,
            "Error" + std::to_string(i),
            std::chrono::milliseconds(100 * i)
        );
        
        // Also log retry results to actually record the events in the global stats
        ErrorReporter::logRetryResult(
            operationId,
            i % 2 == 0, // Alternate success/failure
            "Test result " + std::to_string(i)
        );
        
        // Use a new operation ID for each iteration to avoid overwriting the pending retry
        operationId = generateRandomId();
    }
    
    // Verify stats were recorded
    OperationStatsData stats = ErrorReporter::getRetryStats(operationType);
    EXPECT_GT(stats.totalRetryCount, 0);
    
    // Reset stats
    ErrorReporter::resetRetryStats();
    
    // Verify stats were cleared
    stats = ErrorReporter::getRetryStats(operationType);
    EXPECT_EQ(stats.totalRetryCount, 0);
    EXPECT_EQ(stats.successfulRetryCount, 0);
    EXPECT_EQ(stats.totalRetryDelay.count(), 0);
    EXPECT_TRUE(stats.retriesByReason.empty());
    EXPECT_TRUE(stats.retryEvents.empty());
    
    // Restore original settings
    DISABLE_ALL_LOGGING_FOR_TESTS = originalDisableAllLogging;
    ErrorReporter::suppressAllWarnings = originalSuppressAllWarnings;
}

// Test log rotation (disabled by default due to file timestamps)
TEST_F(ErrorReporterTest, DISABLED_LogRotation) {
    // Create a small max size that will force rotation
    std::string logFile = "logs/test_rotation.log";
    
    // Configure with small max size and rotation
    FileLogDestination::Config config;
    config.filePath = logFile;
    config.appendMode = false;
    config.rotationType = FileLogDestination::RotationType::Size;
    config.maxSizeBytes = 200; // Very small to force rotation
    config.maxFileCount = 3;
    
    // Create custom destination
    auto fileLogger = std::make_unique<FileLogDestination>(config);
    ErrorReporter::addLogDestination(std::move(fileLogger));
    
    // Write more than max_size data
    for (int i = 0; i < 20; i++) {
        ErrorReporter::logDebug("This is log message #" + std::to_string(i) + 
                               " that will eventually cause log rotation.");
    }
    
    // Flush to ensure it's written
    ErrorReporter::flushLogs();
    
    // Check that a rotated file exists
    bool foundRotatedFile = false;
    std::filesystem::path originalPath(logFile);
    
    for (const auto& entry : std::filesystem::directory_iterator(originalPath.parent_path())) {
        std::string filename = entry.path().filename().string();
        if (filename.find("test_rotation-") != std::string::npos) {
            foundRotatedFile = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundRotatedFile);
}

// Only include the main function when this file is compiled as a standalone test
#ifndef RUN_ALL_TESTS_INCLUDE
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif 