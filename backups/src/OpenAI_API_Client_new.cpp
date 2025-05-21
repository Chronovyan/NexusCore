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
#include <sstream>

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

    ApiResponse callChatCompletionEndpoint(
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
        
        // Generate a unique operation ID for this API call
        std::string operationId = "api_call_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::string operationType = "OpenAI_API";
        
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
                
                // Log retry attempt with the new API
                ErrorReporter::logRetryAttempt(
                    operationId,
                    operationType,
                    retryCount,
                    retryReason,
                    finalBackoff
                );
                
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
            
            // Record retry completion with the new API
            ErrorReporter::logRetryResult(
                operationId,
                response.success,
                "Completed after " + std::to_string(retryCount) + 
                " retries. " + (response.success ? "Succeeded" : "Failed") +
                (!response.error_message.empty() ? ": " + response.error_message : "")
            );
            
            // Record retry statistics in our internal tracker
            retryStats_.recordRetryAttempt(retryReason, response.success, retryCount);
            
            // If we have a high number of retries, log detailed statistics
            if (retryCount >= retryPolicy_.maxRetries / 2) {
                if (ErrorReporter::debugLoggingEnabled) {
                    // Get retry stats for this operation type
                    OperationStatsData opStats = ErrorReporter::getRetryStats(operationType);
                    
                    std::stringstream ss;
                    ss << "API Client retry statistics for " << operationType << ":\n"
                       << "  Total retry count: " << opStats.totalRetryCount << "\n"
                       << "  Successful retries: " << opStats.successfulRetryCount << "\n"
                       << "  Total delay: " << opStats.totalRetryDelay.count() << "ms\n"
                       << "\nInternal statistics:\n" << retryStats_.getReport();
                    
                    ErrorReporter::logDebug(ss.str());
                }
            }
        }
        
        return response;
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
    
    void resetRetryStatistics() {
        retryStats_.reset();
        ErrorReporter::resetRetryStats();
        ErrorReporter::logDebug("OpenAI API client retry statistics have been reset");
    }

    RetryStatistics::Stats getRetryStatistics() const {
        return retryStats_.getStats();
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
OpenAI_API_Client::OpenAI_API_Client(const std::string& apiKey) : pImpl(std::make_unique<OpenAIClientImpl>(apiKey)) {
}

// Destructor implementation
OpenAI_API_Client::~OpenAI_API_Client() = default;

// Move constructor implementation
OpenAI_API_Client::OpenAI_API_Client(OpenAI_API_Client&&) noexcept = default;

// Move assignment operator implementation
OpenAI_API_Client& OpenAI_API_Client::operator=(OpenAI_API_Client&&) noexcept = default;

// Implementation of interface methods
ApiResponse OpenAI_API_Client::sendChatCompletionRequest(
    const std::vector<ApiChatMessage>& messages,
    const std::vector<ApiToolDefinition>& tools,
    const std::string& model,
    float temperature,
    int32_t max_tokens
) {
    // Forward to implementation with all parameters
    return pImpl->callChatCompletionEndpoint(
        messages, tools, model, temperature, max_tokens
    );
}

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

RetryStatistics::Stats OpenAI_API_Client::getRetryStatistics() const {
    return pImpl->getRetryStatistics();
}

void OpenAI_API_Client::resetRetryStatistics() {
    pImpl->resetRetryStatistics();
}

} // namespace ai_editor 