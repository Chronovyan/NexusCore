#ifndef RETRY_STATS_H
#define RETRY_STATS_H

#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include <memory>
#include <map>

/**
 * @class RetryEvent
 * @brief Records details about a single retry attempt
 */
struct RetryEvent {
    std::string operationId;          // Unique identifier for the operation
    std::string operationType;        // Type of operation (e.g., "API Call", "File Access")
    int attemptNumber = 0;            // Which attempt this is (1, 2, 3, etc.)
    std::string errorReason;          // Why the previous attempt failed
    std::chrono::system_clock::time_point timestamp;  // When this retry occurred
    std::chrono::milliseconds delay{0};  // Delay before this retry
    bool successful = false;          // Whether this retry succeeded
    
    RetryEvent(const std::string& id, const std::string& type, int attempt,
               const std::string& reason, std::chrono::milliseconds retryDelay)
        : operationId(id), operationType(type), attemptNumber(attempt),
          errorReason(reason), timestamp(std::chrono::system_clock::now()),
          delay(retryDelay) {}
          
    // Default constructor for convenient map usage
    RetryEvent() = default;
};

/**
 * @struct OperationStatsData
 * @brief Non-mutex protected data for operation statistics
 */
struct OperationStatsData {
    int totalRetryCount = 0;
    int successfulRetryCount = 0;
    std::chrono::milliseconds totalRetryDelay{0};
    std::map<std::string, int> retriesByReason;
    std::vector<RetryEvent> retryEvents;
};

/**
 * @class OperationStats
 * @brief Tracks statistics for a specific operation type
 */
class OperationStats {
public:
    void recordRetry(const RetryEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        retryEvents_.push_back(event);
        
        // Update aggregate stats
        totalRetryCount_++;
        if (event.successful) {
            successfulRetryCount_++;
        }
        
        totalRetryDelay_ += event.delay;
        
        // Record by error reason
        retriesByReason_[event.errorReason]++;
    }
    
    // Getters for statistics
    int getTotalRetryCount() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return totalRetryCount_; 
    }
    
    int getSuccessfulRetryCount() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return successfulRetryCount_; 
    }
    
    std::chrono::milliseconds getTotalRetryDelay() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return totalRetryDelay_; 
    }
    
    std::map<std::string, int> getRetriesByReason() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return retriesByReason_;
    }
    
    // Get full event history (for detailed analysis)
    std::vector<RetryEvent> getRetryEvents() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return retryEvents_;
    }
    
    // Get a copy of all stats data at once for thread-safe access
    OperationStatsData getStatsData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        OperationStatsData data;
        data.totalRetryCount = totalRetryCount_;
        data.successfulRetryCount = successfulRetryCount_;
        data.totalRetryDelay = totalRetryDelay_;
        data.retriesByReason = retriesByReason_;
        data.retryEvents = retryEvents_;
        return data;
    }
    
private:
    std::vector<RetryEvent> retryEvents_;
    int totalRetryCount_ = 0;
    int successfulRetryCount_ = 0;
    std::chrono::milliseconds totalRetryDelay_{0};
    std::map<std::string, int> retriesByReason_;
    
    mutable std::mutex mutex_;
};

/**
 * @class RetryStats
 * @brief Global tracker for retry statistics across the application
 */
class RetryStats {
public:
    static RetryStats& getInstance() {
        static RetryStats instance;
        return instance;
    }
    
    /**
     * @brief Record a retry attempt
     * 
     * @param event The retry event to record
     */
    void recordRetry(const RetryEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        operationStats_[event.operationType].recordRetry(event);
        
        // Track by operation ID for collecting sequences
        operationRetries_[event.operationId].push_back(event);
    }
    
    /**
     * @brief Get stats data for a specific operation type
     * 
     * @param operationType The type of operation
     * @return The statistics data for this operation type
     */
    OperationStatsData getOperationStatsData(const std::string& operationType) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = operationStats_.find(operationType);
        if (it != operationStats_.end()) {
            return it->second.getStatsData();
        }
        return OperationStatsData();
    }
    
    /**
     * @brief Get all retry events for a specific operation ID
     * 
     * @param operationId The unique ID of the operation
     * @return Vector of retry events for this operation
     */
    std::vector<RetryEvent> getOperationRetries(const std::string& operationId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = operationRetries_.find(operationId);
        if (it != operationRetries_.end()) {
            return it->second;
        }
        return std::vector<RetryEvent>();
    }
    
    /**
     * @brief Get a map of operation types to their statistics data
     * 
     * @return Map of operation type to its statistics data
     */
    std::map<std::string, OperationStatsData> getAllOperationStatsData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::map<std::string, OperationStatsData> result;
        for (const auto& [type, stats] : operationStats_) {
            result[type] = stats.getStatsData();
        }
        return result;
    }
    
    /**
     * @brief Clear all statistics
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        operationStats_.clear();
        operationRetries_.clear();
    }
    
private:
    RetryStats() = default;
    ~RetryStats() = default;
    
    // Prevent copying
    RetryStats(const RetryStats&) = delete;
    RetryStats& operator=(const RetryStats&) = delete;
    
    std::map<std::string, OperationStats> operationStats_;
    std::map<std::string, std::vector<RetryEvent>> operationRetries_;
    
    mutable std::mutex mutex_;
};

#endif // RETRY_STATS_H 