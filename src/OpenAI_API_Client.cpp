#include "OpenAI_API_Client.h"
#include <cpr/session.h>
#include <cpr/response.h>
#include <cpr/payload.h>
#include <cpr/api.h>
#include <cpr/cprtypes.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <chrono>
#include "OpenAI_API_Client_types.h"
#include "EditorError.h"

using json = nlohmann::json;

namespace ai_editor {

// Implementation class
class OpenAIClientImpl {
public:
    OpenAIClientImpl(const std::string& apiKey) :
        apiKey_(apiKey),
        apiBase_("https://api.openai.com"),
        apiVersion_("v1"),
        defaultModel_("gpt-3.5-turbo"),
        defaultTimeout_(30000),
        retryEnabled_(true),
        retryPolicy_(), // Default retry policy
        retryStats_() {} // Initialize retry statistics

    // List available models from OpenAI API
    ApiModelListResponse listModels() {
        ApiModelListResponse response;
        response.success = false;
        response.error_message = "Method not fully implemented yet";
        return response;
    }
    
    // Retrieve details for a specific model
    ApiModelInfo retrieveModel([[maybe_unused]] const std::string& model_id) {
        ApiModelInfo modelInfo;
        modelInfo.id = "error:not_implemented";
        return modelInfo;
    }
    
    // Create embeddings for the provided input
    ApiEmbeddingResponse createEmbedding([[maybe_unused]] const ApiEmbeddingRequest& request) {
        ApiEmbeddingResponse response;
        response.success = false;
        response.error_message = "Embedding functionality not fully implemented yet";
        return response;
    }

    ApiResponse sendChatCompletionRequest(
        [[maybe_unused]] const std::vector<ApiChatMessage>& messages,
        [[maybe_unused]] const std::vector<ApiToolDefinition>& tools,
        [[maybe_unused]] const std::string& model,
        [[maybe_unused]] float temperature,
        [[maybe_unused]] int32_t max_tokens
    ) {
        // Create a simple response for now
        ApiResponse response;
        response.success = false;
        response.error_message = "Chat completion functionality not fully implemented yet";
        
        return response;
    }

    void setApiKey(const std::string& apiKey) {
        apiKey_ = apiKey;
    }
    
    void setApiBase(const std::string& baseUrl) {
        apiBase_ = baseUrl;
    }
    
    void setApiVersion(const std::string& version) {
        apiVersion_ = version;
    }
    
    void setDefaultModel(const std::string& model) {
        defaultModel_ = model;
    }
    
    void setDefaultTimeout(int timeout) {
        defaultTimeout_ = timeout;
    }
    
    void setRetryPolicy(const ApiRetryPolicy& policy) {
        retryPolicy_ = policy;
    }
    
    ApiRetryPolicy getRetryPolicy() const {
        return retryPolicy_;
    }
    
    void setRetryEnabled(bool enable) {
        retryEnabled_ = enable;
    }
    
    bool isRetryEnabled() const {
        return retryEnabled_;
    }
    
    const RetryStatistics& getRetryStatistics() const {
        return retryStats_;
    }
    
    void resetRetryStatistics() {
        retryStats_.reset();
        ErrorReporter::logDebug("Retry statistics have been reset");
    }

private:
    // Member variables
    std::string apiKey_;
    std::string apiBase_;
    std::string apiVersion_;
    std::string defaultModel_;
    int defaultTimeout_;
    bool retryEnabled_;
    ApiRetryPolicy retryPolicy_;
    RetryStatistics retryStats_;
    std::random_device rd_;
};

// Constructor implementation
OpenAI_API_Client::OpenAI_API_Client(const std::string& apiKey) : pImpl(std::make_unique<OpenAIClientImpl>(apiKey)) {}

// Destructor implementation
OpenAI_API_Client::~OpenAI_API_Client() = default;

// Move constructor implementation
OpenAI_API_Client::OpenAI_API_Client(OpenAI_API_Client&&) noexcept = default;

// Move assignment operator implementation
OpenAI_API_Client& OpenAI_API_Client::operator=(OpenAI_API_Client&&) noexcept = default;

// Implementation of sendChatCompletionRequest
ApiResponse OpenAI_API_Client::sendChatCompletionRequest(
    const std::vector<ApiChatMessage>& messages,
    const std::vector<ApiToolDefinition>& tools,
    const std::string& model,
    float temperature,
    int32_t max_tokens
) {
    return pImpl->sendChatCompletionRequest(messages, tools, model, temperature, max_tokens);
}

// Forward retry methods to implementation
void OpenAI_API_Client::setRetryPolicy(const ApiRetryPolicy& policy) {
    pImpl->setRetryPolicy(policy);
}

ApiRetryPolicy OpenAI_API_Client::getRetryPolicy() const {
    return pImpl->getRetryPolicy();
}

void OpenAI_API_Client::enableRetries(bool enable) {
    pImpl->setRetryEnabled(enable);
}

bool OpenAI_API_Client::isRetryEnabled() const {
    return pImpl->isRetryEnabled();
}

// Implement the missing API methods
ApiModelListResponse OpenAI_API_Client::listModels() {
    return pImpl->listModels();
}

ApiModelInfo OpenAI_API_Client::retrieveModel(const std::string& model_id) {
    return pImpl->retrieveModel(model_id);
}

ApiEmbeddingResponse OpenAI_API_Client::createEmbedding(const ApiEmbeddingRequest& request) {
    return pImpl->createEmbedding(request);
}

// Implement the retry statistics methods
const RetryStatistics& OpenAI_API_Client::getRetryStatistics() const {
    return pImpl->getRetryStatistics();
}

void OpenAI_API_Client::resetRetryStatistics() {
    pImpl->resetRetryStatistics();
}

// Utility method for backward compatibility
nlohmann::json OpenAI_API_Client::generateChatRequestBodyWithSystemAndUserMessage(
    const std::string& systemMessage,
    const std::string& userPrompt,
    const std::string& model
) {
    json request;
    request["model"] = model;
    request["messages"] = json::array();
    
    if (!systemMessage.empty()) {
        request["messages"].push_back({
            {"role", "system"},
            {"content", systemMessage}
        });
    }
    
    request["messages"].push_back({
        {"role", "user"},
        {"content", userPrompt}
    });
    
    return request;
}

} // namespace ai_editor 
