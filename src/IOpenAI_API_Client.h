#ifndef IOPENAI_API_CLIENT_H
#define IOPENAI_API_CLIENT_H

#include "OpenAI_API_Client_types.h"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

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
    
    // Initial backoff delay in milliseconds
    std::chrono::milliseconds initialBackoff = std::chrono::milliseconds(1000);
    
    // Maximum backoff delay in milliseconds
    std::chrono::milliseconds maxBackoff = std::chrono::milliseconds(15000);
    
    // Exponential factor for backoff (e.g., 2.0 means each retry waits twice as long)
    double backoffFactor = 2.0;
    
    // Jitter factor (0.0-1.0) to add randomness to backoff times to prevent thundering herd
    double jitterFactor = 0.2;
    
    // Whether to retry on rate limiting errors (HTTP 429)
    bool retryOnRateLimit = true;
    
    // Whether to retry on server errors (HTTP 500-599)
    bool retryOnServerErrors = true;
    
    // Whether to retry on network/connection errors
    bool retryOnNetworkErrors = true;
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
};

} // namespace ai_editor

#endif // IOPENAI_API_CLIENT_H 