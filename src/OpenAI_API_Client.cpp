#include "OpenAI_API_Client.h"
#include <cpr/cpr.h>
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

    ApiResponse sendChatCompletionRequest(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools,
        const std::string& model,
        float temperature,
        int32_t max_tokens
    ) {
        // Prepare the JSON request body outside the retry loop
        json requestJson;
        
        // ... populate requestJson with the request data ...
        
        // Initialize retry counters and flags
        int retryCount = 0;
        bool shouldRetry = false;
        std::string retryReason;
        ApiResponse response;
        
        // For adding jitter to our retry backoff
        std::mt19937 gen(rd_());
        std::uniform_real_distribution<> jitterDist(-retryPolicy_.jitterFactor, retryPolicy_.jitterFactor);
        
        // Retry loop
        do {
            shouldRetry = false;
            
            // If this is a retry, apply backoff delay
            if (retryCount > 0) {
                // Calculate backoff with exponential factor and jitter
                double backoffMultiplier = std::pow(retryPolicy_.backoffFactor, retryCount - 1);
                auto baseBackoff = std::chrono::duration_cast<std::chrono::milliseconds>(
                    retryPolicy_.initialBackoff * backoffMultiplier);
                
                // Add jitter to prevent thundering herd problem
                double jitter = 1.0 + jitterDist(gen);
                auto backoffWithJitter = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::duration<double, std::milli>(baseBackoff.count() * jitter));
                
                // Cap at max backoff
                auto finalBackoff = (backoffWithJitter > retryPolicy_.maxBackoff) ? retryPolicy_.maxBackoff : backoffWithJitter;
                
                // Log retry information with appropriate severity
                std::string retryMsg = "Retry " + std::to_string(retryCount) + " for " + retryReason + 
                                      ". Backing off for " + std::to_string(finalBackoff.count()) + "ms";
                if (ErrorReporter::debugLoggingEnabled) {
                    ErrorReporter::logDebug(retryMsg);
                } else {
                    ErrorReporter::logWarning(retryMsg);
                }
                
                // Sleep for the calculated backoff period
                std::this_thread::sleep_for(finalBackoff);
            }
            
            // Make the actual API request
            // ... API request code ...
            
            // Process the response
            // ... response processing code ...
            
            // Error handling and retry decision logic
            // ... error handling and retry decision code ...
            
        } while (shouldRetry);
        
        // Update retry statistics and log summary
        if (retryCount > 0) {
            // Update the response with retry info
            response.error_message += " (Retried " + std::to_string(retryCount) + " times)";
            
            // Record retry statistics
            retryStats_.recordRetryAttempt(retryReason, response.success, retryCount);
            
            // Log success/failure after retries
            if (response.success) {
                ErrorReporter::logDebug("API request succeeded after " + 
                                      std::to_string(retryCount) + " retries for " + retryReason);
            } else {
                ErrorReporter::logWarning("API request failed after " + 
                                        std::to_string(retryCount) + " retries for " + retryReason);
            }
            
            // If we have a high number of retries, log detailed statistics
            if (retryCount >= retryPolicy_.maxRetries / 2 && ErrorReporter::debugLoggingEnabled) {
                ErrorReporter::logDebug("Current retry statistics:\n" + retryStats_.getReport());
            }
        }
        
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