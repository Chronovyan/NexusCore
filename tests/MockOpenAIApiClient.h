#pragma once

#include "../src/OpenAI_API_Client.h"

// Mock implementation of IOpenAI_API_Client for testing
class MockOpenAIApiClient : public IOpenAI_API_Client {
public:
    // Implement required methods with empty implementations for testing
    ApiResponse sendChatCompletionRequest(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools,
        const std::string& model,
        float temperature,
        int maxTokens
    ) override {
        return {};
    }
    
    ApiEmbeddingResponse generateEmbedding(
        const std::string& text,
        const std::string& model
    ) override {
        return {};
    }
    
    ApiModelListResponse listModels() override {
        return {};
    }
};
