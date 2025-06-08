#ifndef OPENAI_API_CLIENT_H
#define OPENAI_API_CLIENT_H

#include "IOpenAI_API_Client.h"
#include "OpenAI_API_Client_types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>

namespace ai_editor {

// Forward declaration of internal implementation
class OpenAIClientImpl;

/**
 * OpenAI API Client for making chat completion requests
 * 
 * This class handles communication with the OpenAI API for chat completions.
 * It supports sending messages with optional tool definitions and receiving
 * responses containing text or tool calls.
 */
class OpenAI_API_Client : public IOpenAI_API_Client {
public:
    // Constructor
    explicit OpenAI_API_Client(const std::string& apiKey);
    
    // Destructor
    ~OpenAI_API_Client() override;
    
    // Disable copy
    OpenAI_API_Client(const OpenAI_API_Client&) = delete;
    OpenAI_API_Client& operator=(const OpenAI_API_Client&) = delete;
    
    // Enable move
    OpenAI_API_Client(OpenAI_API_Client&&) noexcept;
    OpenAI_API_Client& operator=(OpenAI_API_Client&&) noexcept;
    
    /**
     * Send a chat completion request to the OpenAI API
     * 
     * @param messages Vector of chat messages to send
     * @param tools Optional vector of tool definitions the model can use
     * @param model The model to use (default: "gpt-4o")
     * @param temperature Optional temperature value for randomness (default: 0.7)
     * @param max_tokens Optional maximum tokens to generate (default: 2000)
     * @return ApiResponse containing the result or error
     */
    ApiResponse sendChatCompletionRequest(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools = {},
        const std::string& model = "gpt-4o",
        float temperature = 0.7f,
        int32_t max_tokens = 2000
    ) override;
    
    /**
     * @brief Set the retry policy for API requests
     * 
     * @param policy The retry policy configuration to use
     */
    void setRetryPolicy(const ApiRetryPolicy& policy) override;
    
    /**
     * @brief Get the current retry policy
     * 
     * @return The current retry policy configuration
     */
    ApiRetryPolicy getRetryPolicy() const override;
    
    /**
     * @brief Enable or disable automatic retries
     * 
     * @param enable Whether to enable automatic retries
     */
    void enableRetries(bool enable) override;
    
    /**
     * @brief Check if automatic retries are enabled
     * 
     * @return True if automatic retries are enabled, false otherwise
     */
    bool isRetryEnabled() const override;
    
    /**
     * @brief List available models from the OpenAI API
     * 
     * @return ApiModelListResponse containing the list of available models or error
     */
    ApiModelListResponse listModels() override;
    
    /**
     * @brief Retrieve details for a specific model from the OpenAI API
     * 
     * @param model_id The ID of the model to retrieve
     * @return ApiModelInfo containing model information or error
     */
    ApiModelInfo retrieveModel(const std::string& model_id) override;
    
    /**
     * @brief Create embeddings for the provided input using the OpenAI API
     * 
     * @param request The embedding request parameters
     * @return ApiEmbeddingResponse containing the embeddings or error
     */
    ApiEmbeddingResponse createEmbedding(const ApiEmbeddingRequest& request) override;
    
    // Retry statistics methods
    const RetryStatistics& getRetryStatistics() const override;
    void resetRetryStatistics() override;
    
    // Generate headers and signature
    static nlohmann::json generateChatRequestBodyWithSystemAndUserMessage(
        const std::string& systemMessage,
        const std::string& userPrompt,
        const std::string& model = "gpt-3.5-turbo"
    );
    
private:
    // Implementation details hidden for PIMPL pattern
    std::unique_ptr<OpenAIClientImpl> pImpl;
};

} // namespace ai_editor

#endif // OPENAI_API_CLIENT_H 