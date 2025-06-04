#ifndef RETRY_STATS_TRACKER_H
#define RETRY_STATS_TRACKER_H

#include <string>
#include <map>
#include <mutex>

/**
 * @struct SimpleRetryStats
 * @brief Simplified statistics for retry operations
 */
struct SimpleRetryStats {
    size_t totalAttempts = 0;       ///< Total retry attempts
    size_t successfulAttempts = 0;  ///< Number of successful attempts
    size_t failedAttempts = 0;      ///< Number of failed attempts
    double averageRetryCount = 0.0; ///< Average number of retries per operation
};

/**
 * @class RetryStatsTracker
 * @brief Thread-safe tracker for retry statistics
 * 
 * This class provides a simplified way to track and query retry statistics
 * for different operation types.
 */
class RetryStatsTracker {
public:
    RetryStatsTracker() = default;
    ~RetryStatsTracker() = default;
    
    /**
     * @brief Record a retry attempt for an operation
     * 
     * @param operationType The type of operation being retried
     */
    void recordRetryAttempt(const std::string& operationType) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& stats = statsMap_[operationType];
        stats.totalAttempts++;
        
        // Recalculate average
        if (stats.totalAttempts > 0) {
            double opCount = static_cast<double>(
                stats.successfulAttempts + stats.failedAttempts);
            if (opCount > 0) {
                stats.averageRetryCount = static_cast<double>(stats.totalAttempts) / opCount;
            }
        }
    }
    
    /**
     * @brief Record the success or failure of an operation
     * 
     * @param operationType The type of operation
     * @param success Whether the operation was successful
     */
    void recordOperationResult(const std::string& operationType, bool success) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& stats = statsMap_[operationType];
        
        if (success) {
            stats.successfulAttempts++;
        } else {
            stats.failedAttempts++;
        }
        
        // Recalculate average
        if (stats.totalAttempts > 0) {
            double opCount = static_cast<double>(
                stats.successfulAttempts + stats.failedAttempts);
            if (opCount > 0) {
                stats.averageRetryCount = static_cast<double>(stats.totalAttempts) / opCount;
            }
        }
    }
    
    /**
     * @brief Get statistics for an operation type
     * 
     * @param operationType The type of operation
     * @return Statistics for the operation
     */
    SimpleRetryStats getStats(const std::string& operationType) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = statsMap_.find(operationType);
        if (it != statsMap_.end()) {
            return it->second;
        }
        return SimpleRetryStats{};
    }
    
    /**
     * @brief Reset all statistics
     */
    void resetAllStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        statsMap_.clear();
    }
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, SimpleRetryStats> statsMap_;
};

#endif // RETRY_STATS_TRACKER_H 