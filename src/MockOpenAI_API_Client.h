#ifndef MOCK_OPENAI_API_CLIENT_H
#define MOCK_OPENAI_API_CLIENT_H

#include "IOpenAI_API_Client.h"
#include "OpenAI_API_Client_types.h"
#include <queue>
#include <string>
#include <functional>
#include <chrono>
#include <map>
#include <algorithm>

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
        retryStats_(),
        modelListSuccessResponse_(true),
        modelListErrorStatusCode_(500),
        modelInfoSuccessResponse_(true),
        embeddingSuccessResponse_(true),
        embeddingErrorStatusCode_(500) {}
    
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
     * @brief List available models
     * 
     * @return ApiModelListResponse containing model information or error
     */
    ApiModelListResponse listModels() override {
        // Store that this method was called
        listModels_called_ = true;
        
        // If we have a model list response in the queue, return it
        if (!model_list_response_queue_.empty()) {
            ApiModelListResponse response = model_list_response_queue_.front();
            model_list_response_queue_.pop();
            return response;
        }
        
        // Otherwise, return default or pre-configured response
        if (modelListSuccessResponse_) {
            return successModelListResponse_;
        } else {
            ApiModelListResponse errorResponse;
            errorResponse.success = false;
            errorResponse.error_message = modelListErrorMessage_;
            errorResponse.raw_json_response = "{\"error\":{\"message\":\"" + modelListErrorMessage_ + 
                                            "\",\"code\":" + std::to_string(modelListErrorStatusCode_) + "}}";
            return errorResponse;
        }
    }
    
    /**
     * @brief Retrieve details for a specific model
     * 
     * @param model_id The ID of the model to retrieve
     * @return ApiModelInfo containing model information or error
     */
    ApiModelInfo retrieveModel(const std::string& model_id) override {
        // Store that this method was called with this model_id
        retrieveModel_called_ = true;
        last_retrieved_model_id_ = model_id;
        
        // If we have a specific response for this model_id, return it
        if (model_responses_.find(model_id) != model_responses_.end()) {
            return model_responses_[model_id];
        }
        
        // Otherwise return the default response
        if (modelInfoSuccessResponse_) {
            ApiModelInfo model;
            model.id = model_id;
            model.object = "model";
            model.created = "1234567890";
            model.owned_by = "organization-owner";
            return model;
        } else {
            // In a real implementation, we would throw an exception or return an error object
            // For this mock, we'll return an empty model with an id that indicates an error
            ApiModelInfo errorModel;
            errorModel.id = "error:" + modelInfoErrorMessage_;
            return errorModel;
        }
    }
    
    /**
     * @brief Create embeddings for the provided input
     * 
     * @param request The embedding request parameters
     * @return ApiEmbeddingResponse containing the embeddings or error
     */
    ApiEmbeddingResponse createEmbedding(const ApiEmbeddingRequest& request) override {
        // Store that this method was called with these parameters
        createEmbedding_called_ = true;
        last_embedding_request_ = request;
        
        // If we have an embedding response in the queue, return it
        if (!embedding_response_queue_.empty()) {
            ApiEmbeddingResponse response = embedding_response_queue_.front();
            embedding_response_queue_.pop();
            return response;
        }
        
        // Otherwise, return default or pre-configured response
        if (embeddingSuccessResponse_) {
            return successEmbeddingResponse_;
        } else {
            ApiEmbeddingResponse errorResponse;
            errorResponse.success = false;
            errorResponse.error_message = embeddingErrorMessage_;
            errorResponse.raw_json_response = "{\"error\":{\"message\":\"" + embeddingErrorMessage_ + 
                                            "\",\"code\":" + std::to_string(embeddingErrorStatusCode_) + "}}";
            return errorResponse;
        }
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
    
    // Model listing helper methods
    
    /**
     * @brief Add a model list response to the queue
     */
    void primeModelListResponse(const ApiModelListResponse& response) {
        model_list_response_queue_.push(response);
    }
    
    /**
     * @brief Set a successful model list response
     */
    void setSuccessModelListResponse(const std::vector<ApiModelInfo>& models) {
        successModelListResponse_.success = true;
        successModelListResponse_.models = models;
        successModelListResponse_.error_message = "";
        // Create a basic JSON representation
        std::string json = "{\"object\":\"list\",\"data\":[";
        for (size_t i = 0; i < models.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"id\":\"" + models[i].id + "\",\"object\":\"model\"}";
        }
        json += "]}";
        successModelListResponse_.raw_json_response = json;
        modelListSuccessResponse_ = true;
    }
    
    /**
     * @brief Set an error model list response
     */
    void setErrorModelListResponse(const std::string& errorMessage, int statusCode = 500) {
        modelListErrorMessage_ = errorMessage;
        modelListErrorStatusCode_ = statusCode;
        modelListSuccessResponse_ = false;
    }
    
    // Model info helper methods
    
    /**
     * @brief Configure response for a specific model ID
     */
    void setModelResponse(const std::string& model_id, const ApiModelInfo& model) {
        model_responses_[model_id] = model;
    }
    
    /**
     * @brief Set default success or error response for model info
     */
    void setModelInfoSuccessResponse(bool success) {
        modelInfoSuccessResponse_ = success;
    }
    
    /**
     * @brief Set error message for model info failures
     */
    void setModelInfoErrorMessage(const std::string& message) {
        modelInfoErrorMessage_ = message;
    }
    
    // Embedding helper methods
    
    /**
     * @brief Add an embedding response to the queue
     */
    void primeEmbeddingResponse(const ApiEmbeddingResponse& response) {
        embedding_response_queue_.push(response);
    }
    
    /**
     * @brief Set a successful embedding response
     */
    void setSuccessEmbeddingResponse(const std::vector<std::vector<float>>& embeddings, 
                                    const std::string& model = "text-embedding-ada-002") {
        successEmbeddingResponse_.success = true;
        successEmbeddingResponse_.model = model;
        successEmbeddingResponse_.object = "list";
        successEmbeddingResponse_.error_message = "";
        
        // Create data objects for each embedding
        successEmbeddingResponse_.data.clear();
        for (size_t i = 0; i < embeddings.size(); ++i) {
            ApiEmbeddingData data;
            data.embedding = embeddings[i];
            data.index = static_cast<int>(i);
            data.object = "embedding";
            successEmbeddingResponse_.data.push_back(data);
        }
        
        // Set usage stats
        successEmbeddingResponse_.usage_prompt_tokens = 8;
        successEmbeddingResponse_.usage_total_tokens = 8;
        
        // Create a simplified JSON representation
        std::string json = "{\"object\":\"list\",\"data\":[";
        for (size_t i = 0; i < embeddings.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"object\":\"embedding\",\"index\":" + std::to_string(i) + ",\"embedding\":[";
            // Add first few embedding values for brevity
            for (size_t j = 0; j < (std::min)(size_t(3), embeddings[i].size()); ++j) {
                if (j > 0) json += ",";
                json += std::to_string(embeddings[i][j]);
            }
            if (embeddings[i].size() > 3) {
                json += ",...";
            }
            json += "]}";
        }
        json += "],\"model\":\"" + model + "\",\"usage\":{\"prompt_tokens\":8,\"total_tokens\":8}}";
        successEmbeddingResponse_.raw_json_response = json;
        
        embeddingSuccessResponse_ = true;
    }
    
    /**
     * @brief Set an error embedding response
     */
    void setErrorEmbeddingResponse(const std::string& errorMessage, int statusCode = 500) {
        embeddingErrorMessage_ = errorMessage;
        embeddingErrorStatusCode_ = statusCode;
        embeddingSuccessResponse_ = false;
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
    
    // Variables to track which new methods were called
    bool listModels_called_ = false;
    bool retrieveModel_called_ = false;
    bool createEmbedding_called_ = false;
    
    // Variables to store parameters of last calls
    std::string last_retrieved_model_id_;
    ApiEmbeddingRequest last_embedding_request_;
    
private:
    // Queue of responses to return
    std::queue<ApiResponse> response_queue_;
    
    // Retry-related members
    bool retryEnabled_;
    ApiRetryPolicy retryPolicy_;
    RetryStatistics retryStats_;
    
    // New private members for additional endpoints
    std::queue<ApiModelListResponse> model_list_response_queue_;
    std::queue<ApiEmbeddingResponse> embedding_response_queue_;
    
    bool modelListSuccessResponse_;
    std::string modelListErrorMessage_;
    int modelListErrorStatusCode_;
    ApiModelListResponse successModelListResponse_;
    
    bool modelInfoSuccessResponse_;
    std::string modelInfoErrorMessage_;
    std::map<std::string, ApiModelInfo> model_responses_;
    
    bool embeddingSuccessResponse_;
    std::string embeddingErrorMessage_;
    int embeddingErrorStatusCode_;
    ApiEmbeddingResponse successEmbeddingResponse_;
};

} // namespace ai_editor

#endif // MOCK_OPENAI_API_CLIENT_H 