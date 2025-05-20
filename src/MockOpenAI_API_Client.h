#ifndef MOCK_OPENAI_API_CLIENT_H
#define MOCK_OPENAI_API_CLIENT_H

#include "IOpenAI_API_Client.h"
#include "OpenAI_API_Client_types.h"
#include <queue>
#include <string>
#include <functional>
#include <chrono>

namespace ai_editor {

/**
 * @class MockOpenAI_API_Client
 * @brief Mock implementation of IOpenAI_API_Client for testing
 * 
 * This class allows tests to pre-configure specific API responses without making
 * real network calls, and provides methods to inspect what was sent to the client.
 */
class MockOpenAI_API_Client : public IOpenAI_API_Client {
public:
    /**
     * @brief Enum for simulating different failure types
     */
    enum class FailureType {
        None,            // No failure (success)
        Network,         // Network connectivity issues
        Authentication,  // Authentication errors (401, 403)
        RateLimit,       // Rate limiting (429)
        ServerError,     // Server errors (5xx)
        InvalidRequest,  // Client errors (4xx)
        Timeout,         // Request timeout
        SchemaValidation // Schema validation error
    };
    
    /**
     * @brief Structure to define a failure scenario
     */
    struct FailureScenario {
        FailureType type;
        std::string message;
        
        FailureScenario(FailureType t, const std::string& msg) : type(t), message(msg) {}
    };
    
    /**
     * @brief Constructor
     */
    MockOpenAI_API_Client() : 
        successResponse_(true),
        retryEnabled_(true),
        retryPolicy_(),
        retryStats_() {}
    
    /**
     * @brief Mock implementation of sendChatCompletionRequest
     * 
     * Returns pre-configured responses from a queue and stores the request parameters
     * for later inspection by tests.
     * 
     * @param messages Vector of chat messages to send
     * @param tools Optional vector of tool definitions the model can use
     * @param model The model to use (default: "gpt-4o")
     * @param temperature Optional temperature value for randomness (default: 0.7)
     * @param max_tokens Optional maximum tokens to generate (default: 2000)
     * @return ApiResponse containing the pre-configured result
     */
    ApiResponse sendChatCompletionRequest(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools = {},
        const std::string& model = "gpt-4o",
        float temperature = 0.7f,
        int32_t max_tokens = 2000
    ) override;
    
    /**
     * @brief Add a response to the queue to be returned by the next call
     * 
     * @param response The ApiResponse to return
     */
    void primeResponse(const ApiResponse& response) {
        response_queue_.push(response);
    }
    
    /**
     * @brief Add a JSON response to the queue with success/failure status
     * 
     * @param json_string The raw JSON response string
     * @param success Whether the response is successful (default: true)
     * @param error_message Error message if success is false
     */
    void primeJsonResponse(
        const std::string& json_string,
        bool success = true,
        const std::string& error_message = ""
    ) {
        ApiResponse response;
        response.success = success;
        response.raw_json_response = json_string;
        response.error_message = success ? "" : error_message;
        primeResponse(response);
    }
    
    /**
     * @brief Clear any pending responses in the queue
     */
    void clearQueue() {
        std::queue<ApiResponse> empty;
        std::swap(response_queue_, empty);
    }
    
    /**
     * @brief Set the retry policy for API requests
     * 
     * @param policy The retry policy configuration to use
     */
    void setRetryPolicy(const ApiRetryPolicy& policy) override {
        retryPolicy_ = policy;
        last_set_retry_policy_ = policy; // Store for test inspection
    }
    
    /**
     * @brief Get the current retry policy
     * 
     * @return The current retry policy configuration
     */
    ApiRetryPolicy getRetryPolicy() const override {
        return retryPolicy_;
    }
    
    /**
     * @brief Enable or disable automatic retries
     * 
     * @param enable Whether to enable automatic retries
     */
    void enableRetries(bool enable) override {
        retryEnabled_ = enable;
        last_retry_enabled_value_ = enable; // Store for test inspection
    }
    
    /**
     * @brief Check if automatic retries are enabled
     * 
     * @return True if automatic retries are enabled, false otherwise
     */
    bool isRetryEnabled() const override {
        return retryEnabled_;
    }
    
    /**
     * @brief Set the response content to return
     */
    void setResponseContent(const std::string& content) {
        responseContent_ = content;
        successResponse_ = true;
    }
    
    /**
     * @brief Set an error response to return
     */
    void setErrorResponse(const std::string& errorMessage, int statusCode = 500) {
        errorMessage_ = errorMessage;
        errorStatusCode_ = statusCode;
        successResponse_ = false;
    }
    
    /**
     * @brief Set a custom response handler function
     */
    void setResponseHandler(std::function<ApiResponse(const std::vector<ApiChatMessage>&)> handler) {
        responseHandler_ = handler;
    }
    
    /**
     * @brief Set tool call responses
     */
    void setToolCallResponses(const std::vector<ApiToolCall>& toolCalls) {
        toolCalls_ = toolCalls;
        successResponse_ = true;
    }
    
    /**
     * @brief Implementation of callChatCompletionEndpoint - deprecated, use sendChatCompletionRequest
     */
    ApiResponse callChatCompletionEndpoint(const std::vector<ApiChatMessage>& messages) {
        // For backward compatibility
        return sendChatCompletionRequest(messages);
    }
    
    // Retry statistics methods
    const RetryStatistics& getRetryStatistics() const override {
        return retryStats_;
    }
    
    void resetRetryStatistics() override {
        retryStats_.reset();
    }
    
    // Methods to simulate retry scenarios for testing
    void simulateRetries(int count, const std::string& reason, bool success) {
        simulateRetries_ = true;
        simulatedRetryCount_ = count;
        simulatedRetryReason_ = reason;
        simulatedRetrySuccess_ = success;
        retryCount_ = 0;  // Track the current retry attempt number
        
        // Record the retry statistics immediately for testing purposes
        // This way, they're recorded even if retries aren't actually triggered
        if (retryEnabled_) {
            // Simply call recordRetryAttempt which will update all the necessary stats
            retryStats_.recordRetryAttempt(reason, success, count);
        }
    }
    
    void stopSimulatingRetries() {
        simulateRetries_ = false;
    }
    
    // Stored request parameters for test inspection
    std::vector<ApiChatMessage> last_sent_messages_;
    std::vector<ApiToolDefinition> last_sent_tools_;
    std::string last_sent_model_;
    float last_sent_temperature_ = 0.0f;
    int32_t last_sent_max_tokens_ = 0;
    
    // Store retry-related calls for test inspection
    ApiRetryPolicy last_set_retry_policy_;
    bool last_retry_enabled_value_ = true;
    
    // Sequence of failures to simulate for testing retry behavior
    std::vector<FailureScenario> failureSequence;
    
    // Retry simulation fields
    bool simulateRetries_ = false;
    int retryCount_ = 0;  // Track the current retry attempt number
    int simulatedRetryCount_ = 0;
    std::string simulatedRetryReason_ = "test";
    bool simulatedRetrySuccess_ = true;
    
    std::string responseContent_;
    std::string errorMessage_;
    int errorStatusCode_ = 0;
    bool successResponse_;
    std::vector<ApiToolCall> toolCalls_;
    std::function<ApiResponse(const std::vector<ApiChatMessage>&)> responseHandler_;
    
private:
    // Queue of responses to return
    std::queue<ApiResponse> response_queue_;
    
    // Retry-related members
    bool retryEnabled_;
    ApiRetryPolicy retryPolicy_;
    RetryStatistics retryStats_;
};

} // namespace ai_editor

#endif // MOCK_OPENAI_API_CLIENT_H 