#ifndef MOCK_OPENAI_API_CLIENT_H
#define MOCK_OPENAI_API_CLIENT_H

#include "IOpenAI_API_Client.h"
#include <queue>
#include <string>

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
     * @brief Constructor
     */
    MockOpenAI_API_Client() : retry_enabled_(true), retry_policy_() {}
    
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
    ) override {
        // Store the request parameters for test inspection
        last_sent_messages_ = messages;
        last_sent_tools_ = tools;
        last_sent_model_ = model;
        last_sent_temperature_ = temperature;
        last_sent_max_tokens_ = max_tokens;
        
        // Return the next response from the queue, or a default error response if empty
        if (response_queue_.empty()) {
            return {
                false,
                "",
                "MockOpenAI_API_Client: No pre-configured response in queue",
                "",
                {}
            };
        }
        
        ApiResponse response = response_queue_.front();
        response_queue_.pop();
        return response;
    }
    
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
        retry_policy_ = policy;
        last_set_retry_policy_ = policy; // Store for test inspection
    }
    
    /**
     * @brief Get the current retry policy
     * 
     * @return The current retry policy configuration
     */
    ApiRetryPolicy getRetryPolicy() const override {
        return retry_policy_;
    }
    
    /**
     * @brief Enable or disable automatic retries
     * 
     * @param enable Whether to enable automatic retries
     */
    void enableRetries(bool enable) override {
        retry_enabled_ = enable;
        last_retry_enabled_value_ = enable; // Store for test inspection
    }
    
    /**
     * @brief Check if automatic retries are enabled
     * 
     * @return True if automatic retries are enabled, false otherwise
     */
    bool isRetryEnabled() const override {
        return retry_enabled_;
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
    
private:
    // Queue of responses to return
    std::queue<ApiResponse> response_queue_;
    
    // Retry-related members
    bool retry_enabled_;
    ApiRetryPolicy retry_policy_;
};

} // namespace ai_editor

#endif // MOCK_OPENAI_API_CLIENT_H 