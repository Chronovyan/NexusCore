#ifndef OPENAI_API_CLIENT_H
#define OPENAI_API_CLIENT_H

#include "IOpenAI_API_Client.h"
#include "OpenAI_API_Client_types.h"
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
    
private:
    // Implementation details hidden for PIMPL pattern
    std::unique_ptr<OpenAIClientImpl> pImpl;
};

} // namespace ai_editor

#endif // OPENAI_API_CLIENT_H 