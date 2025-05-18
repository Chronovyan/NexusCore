#include "gtest/gtest.h"
#include "../src/EditorError.h"
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
    void SetUp() override {
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
    
    // Check that log file contains our messages
    EXPECT_TRUE(logContent.find("Debug message to file") != std::string::npos);
    EXPECT_TRUE(logContent.find("Warning message to file") != std::string::npos);
    EXPECT_TRUE(logContent.find("Error message to file") != std::string::npos);
}

// Test that multiple destinations receive logs
TEST_F(ErrorReporterTest, MultiDestinationLogging) {
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
}

// Test that severity filters work
TEST_F(ErrorReporterTest, SeverityFilters) {
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
    
    // Check that only appropriate messages are present
    EXPECT_TRUE(logContent.find("Debug message should be filtered") == std::string::npos);
    EXPECT_TRUE(logContent.find("Warning message should be logged") != std::string::npos);
    EXPECT_TRUE(logContent.find("Error message should be logged") != std::string::npos);
}

// Test logging exceptions
TEST_F(ErrorReporterTest, ExceptionLogging) {
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
    EXPECT_TRUE(logContent.find("Delay: 500ms") != std::string::npos);
    EXPECT_TRUE(logContent.find("Retry succeeded") != std::string::npos);
    EXPECT_TRUE(logContent.find("Connected after retry") != std::string::npos);
    EXPECT_TRUE(logContent.find("Retry attempt #2") != std::string::npos);
    EXPECT_TRUE(logContent.find("ServerError") != std::string::npos);
    EXPECT_TRUE(logContent.find("Delay: 1000ms") != std::string::npos);
    EXPECT_TRUE(logContent.find("Retry failed") != std::string::npos);
    EXPECT_TRUE(logContent.find("Server still unavailable") != std::string::npos);
    
    // Check that stats are being recorded
    OperationStatsData stats = ErrorReporter::getRetryStats(operationType);
    EXPECT_EQ(2, stats.totalRetryCount);
    EXPECT_EQ(1, stats.successfulRetryCount);
    
    // Get retries by reason
    auto retryReasons = stats.retriesByReason;
    EXPECT_EQ(1, retryReasons["ConnectionError"]);
    EXPECT_EQ(1, retryReasons["ServerError"]);
}

// Test that retry stats can be reset
TEST_F(ErrorReporterTest, RetryStatsReset) {
    // Record some retry events
    std::string operationId = generateRandomId();
    std::string operationType = "Database";
    
    // Log a retry attempt and result
    ErrorReporter::logRetryAttempt(
        operationId,
        operationType,
        1,
        "ConnectionTimeout",
        std::chrono::milliseconds(200)
    );
    
    ErrorReporter::logRetryResult(
        operationId,
        true
    );
    
    // Verify stats exist
    OperationStatsData statsBefore = ErrorReporter::getRetryStats(operationType);
    EXPECT_EQ(1, statsBefore.totalRetryCount);
    
    // Reset stats
    ErrorReporter::resetRetryStats();
    
    // Verify stats are cleared
    OperationStatsData statsAfter = ErrorReporter::getRetryStats(operationType);
    EXPECT_EQ(0, statsAfter.totalRetryCount);
}

// Test log rotation - disabled by default due to dependency on file size
TEST_F(ErrorReporterTest, DISABLED_LogRotation) {
    // Set up file logging with small max size
    std::string logFile = "logs/test_rotation.log";
    constexpr size_t smallMaxSize = 1024; // 1KB
    
    // Configure small file size for faster rotation
    ErrorReporter::enableFileLogging(
        logFile,                                    // Path
        false,                                      // Don't append
        FileLogDestination::RotationType::Size,     // Rotate by size
        smallMaxSize,                               // Small max size (1KB)
        3                                           // Keep 3 files
    );
    
    // Generate log entries until rotation occurs
    // This is a somewhat brute force approach
    std::string largeMessage(100, 'X'); // 100 character message
    
    for (int i = 0; i < 20; i++) {
        ErrorReporter::logError("Test rotation message " + std::to_string(i) + ": " + largeMessage);
        ErrorReporter::flushLogs();
    }
    
    // Check that rotation occurred (original file still exists but is small)
    std::filesystem::path originalPath(logFile);
    EXPECT_TRUE(std::filesystem::exists(originalPath));
    
    // Check for rotated files
    bool foundRotatedFile = false;
    for (const auto& entry : std::filesystem::directory_iterator(originalPath.parent_path())) {
        std::string filename = entry.path().filename().string();
        if (filename.find("test_rotation-") != std::string::npos) {
            foundRotatedFile = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundRotatedFile);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 