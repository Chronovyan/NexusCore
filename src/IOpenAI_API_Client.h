#ifndef IOPENAI_API_CLIENT_H
#define IOPENAI_API_CLIENT_H

#include "OpenAI_API_Client_types.h"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <mutex>
#include <map>

namespace ai_editor {

/**
 * @struct ApiRetryPolicy
 * @brief Configuration for API request retry behavior
 * 
 * This structure defines the parameters for automatic retry of failed API requests,
 * including the maximum number of retries and the backoff strategy.
 */
struct ApiRetryPolicy {
    // Maximum number of retry attempts
    int maxRetries = 3;
    
    // Initial backoff duration before first retry
    std::chrono::milliseconds initialBackoff = std::chrono::milliseconds(1000);
    
    // Maximum backoff duration for any retry
    std::chrono::milliseconds maxBackoff = std::chrono::milliseconds(30000);
    
    // Backoff multiplier (exponential backoff factor)
    double backoffFactor = 2.0;
    
    // Jitter factor to add randomness to backoff (-jitterFactor to +jitterFactor)
    double jitterFactor = 0.1;
    
    // Whether to retry on rate limit errors (HTTP 429)
    bool retryOnRateLimit = true;
    
    // Whether to retry on server errors (HTTP 5xx)
    bool retryOnServerErrors = true;
    
    // Whether to retry on network errors
    bool retryOnNetworkErrors = true;
};

// Structure to track retry statistics for monitoring and diagnostics
struct RetryStatistics {
    // Total number of requests that required at least one retry
    std::atomic<size_t> totalRequestsWithRetries{0};
    
    // Total number of retries performed across all requests
    std::atomic<size_t> totalRetryAttempts{0};
    
    // Number of requests that succeeded after retries
    std::atomic<size_t> successfulRetriedRequests{0};
    
    // Number of requests that failed even after all retries
    std::atomic<size_t> failedAfterRetries{0};
    
    // Map of retry reasons to counts
    std::map<std::string, size_t> retryReasonCounts;
    mutable std::mutex retryReasonMutex;
    
    // Record retry statistics for a completed request
    void recordRetryAttempt(const std::string& reason, bool ultimateSuccess, int attempts) {
        if (attempts > 0) {
            totalRequestsWithRetries++;
            totalRetryAttempts += attempts;
            
            if (ultimateSuccess) {
                successfulRetriedRequests++;
            } else {
                failedAfterRetries++;
            }
            
            // Update retry reason counts
            std::lock_guard<std::mutex> lock(retryReasonMutex);
            retryReasonCounts[reason]++;
        }
    }
    
    // Get a formatted report of retry statistics
    std::string getReport() const {
        std::string report = "Retry Statistics:\n";
        report += "  Total requests with retries: " + std::to_string(totalRequestsWithRetries) + "\n";
        report += "  Total retry attempts: " + std::to_string(totalRetryAttempts) + "\n";
        report += "  Successful after retries: " + std::to_string(successfulRetriedRequests) + "\n";
        report += "  Failed after retries: " + std::to_string(failedAfterRetries) + "\n";
        
        // Add retry reason breakdown
        {
            std::lock_guard<std::mutex> lock(retryReasonMutex);
            report += "  Retry reasons:\n";
            for (const auto& [reason, count] : retryReasonCounts) {
                report += "    " + reason + ": " + std::to_string(count) + "\n";
            }
        }
        
        return report;
    }
    
    // Reset all statistics
    void reset() {
        totalRequestsWithRetries = 0;
        totalRetryAttempts = 0;
        successfulRetriedRequests = 0;
        failedAfterRetries = 0;
        
        std::lock_guard<std::mutex> lock(retryReasonMutex);
        retryReasonCounts.clear();
    }
    
    // Make RetryStatistics movable but not copyable
    RetryStatistics() = default;
    RetryStatistics(RetryStatistics&& other) noexcept 
        : totalRequestsWithRetries(other.totalRequestsWithRetries.load()),
          totalRetryAttempts(other.totalRetryAttempts.load()),
          successfulRetriedRequests(other.successfulRetriedRequests.load()),
          failedAfterRetries(other.failedAfterRetries.load()) {
        std::lock_guard<std::mutex> lock(other.retryReasonMutex);
        retryReasonCounts = std::move(other.retryReasonCounts);
    }
    
    RetryStatistics& operator=(RetryStatistics&& other) noexcept {
        if (this != &other) {
            totalRequestsWithRetries = other.totalRequestsWithRetries.load();
            totalRetryAttempts = other.totalRetryAttempts.load();
            successfulRetriedRequests = other.successfulRetriedRequests.load();
            failedAfterRetries = other.failedAfterRetries.load();
            
            std::lock_guard<std::mutex> lock1(retryReasonMutex);
            std::lock_guard<std::mutex> lock2(other.retryReasonMutex);
            retryReasonCounts = std::move(other.retryReasonCounts);
        }
        return *this;
    }
    
    // Delete copy constructor and assignment operator
    RetryStatistics(const RetryStatistics&) = delete;
    RetryStatistics& operator=(const RetryStatistics&) = delete;
};

/**
 * @class IOpenAI_API_Client
 * @brief Interface for OpenAI API client implementations
 * 
 * This interface defines the contract for classes that provide OpenAI API functionality,
 * allowing for real implementations and mocks for testing.
 */
class IOpenAI_API_Client {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IOpenAI_API_Client() = default;
    
    /**
     * @brief Send a chat completion request to the OpenAI API
     * 
     * @param messages Vector of chat messages to send
     * @param tools Optional vector of tool definitions the model can use
     * @param model The model to use (default: "gpt-4o")
     * @param temperature Optional temperature value for randomness (default: 0.7)
     * @param max_tokens Optional maximum tokens to generate (default: 2000)
     * @return ApiResponse containing the result or error
     */
    virtual ApiResponse sendChatCompletionRequest(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools = {},
        const std::string& model = "gpt-4o",
        float temperature = 0.7f,
        int32_t max_tokens = 2000
    ) = 0;
    
    /**
     * @brief List available models from the OpenAI API
     * 
     * @return ApiModelListResponse containing the list of available models or error
     */
    virtual ApiModelListResponse listModels() = 0;
    
    /**
     * @brief Retrieve details for a specific model from the OpenAI API
     * 
     * @param model_id The ID of the model to retrieve
     * @return ApiModelInfo containing model information or error
     */
    virtual ApiModelInfo retrieveModel(const std::string& model_id) = 0;
    
    /**
     * @brief Create embeddings for the provided input using the OpenAI API
     * 
     * @param request The embedding request parameters
     * @return ApiEmbeddingResponse containing the embeddings or error
     */
    virtual ApiEmbeddingResponse createEmbedding(const ApiEmbeddingRequest& request) = 0;
    
    /**
     * @brief Set the retry policy for API requests
     * 
     * @param policy The retry policy configuration to use
     */
    virtual void setRetryPolicy(const ApiRetryPolicy& policy) = 0;
    
    /**
     * @brief Get the current retry policy
     * 
     * @return The current retry policy configuration
     */
    virtual ApiRetryPolicy getRetryPolicy() const = 0;
    
    /**
     * @brief Enable or disable automatic retries
     * 
     * @param enable Whether to enable automatic retries
     */
    virtual void enableRetries(bool enable) = 0;
    
    /**
     * @brief Check if automatic retries are enabled
     * 
     * @return True if automatic retries are enabled, false otherwise
     */
    virtual bool isRetryEnabled() const = 0;
    
    /**
     * @brief Get retry statistics
     * 
     * @return The current retry statistics
     */
    virtual const RetryStatistics& getRetryStatistics() const = 0;
    
    /**
     * @brief Reset retry statistics
     */
    virtual void resetRetryStatistics() = 0;
};

} // namespace ai_editor

#endif // IOPENAI_API_CLIENT_H 