#ifndef IOPENAI_API_CLIENT_H
#define IOPENAI_API_CLIENT_H

#include "OpenAI_API_Client_types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ai_editor {

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
};

} // namespace ai_editor

#endif // IOPENAI_API_CLIENT_H 