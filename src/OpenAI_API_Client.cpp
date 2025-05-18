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
class OpenAI_API_ClientImpl {
public:
    OpenAI_API_ClientImpl() :
        apiKey_(""),
        apiBase_("https://api.openai.com"),
        apiVersion_("v1"),
        defaultModel_("gpt-3.5-turbo"),
        defaultTimeout_(30000),
        retryEnabled_(true),
        retryPolicy_(), // Default retry policy
        retryStats_() {} // Initialize retry statistics

    ApiResponse callChatCompletionEndpoint(const ApiChatRequest& request) {
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
    
    RetryStatistics getRetryStatistics() const {
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
OpenAI_API_Client::OpenAI_API_Client() : pImpl(std::make_unique<OpenAI_API_ClientImpl>()) {}

// Destructor implementation
OpenAI_API_Client::~OpenAI_API_Client() = default;

// Move constructor implementation
OpenAI_API_Client::OpenAI_API_Client(OpenAI_API_Client&&) noexcept = default;

// Move assignment operator implementation
OpenAI_API_Client& OpenAI_API_Client::operator=(OpenAI_API_Client&&) noexcept = default;

// Forward API calls to implementation
ApiResponse OpenAI_API_Client::callChatCompletionEndpoint(const ApiChatRequest& request) {
    return pImpl->callChatCompletionEndpoint(request);
}

// Forward configuration methods to implementation
void OpenAI_API_Client::setApiKey(const std::string& apiKey) {
    pImpl->setApiKey(apiKey);
}

void OpenAI_API_Client::setApiBase(const std::string& baseUrl) {
    pImpl->setApiBase(baseUrl);
}

void OpenAI_API_Client::setApiVersion(const std::string& version) {
    pImpl->setApiVersion(version);
}

void OpenAI_API_Client::setDefaultModel(const std::string& model) {
    pImpl->setDefaultModel(model);
}

void OpenAI_API_Client::setDefaultTimeout(int timeout) {
    pImpl->setDefaultTimeout(timeout);
}

// Forward retry methods to implementation
void OpenAI_API_Client::setRetryPolicy(const ApiRetryPolicy& policy) {
    pImpl->setRetryPolicy(policy);
}

ApiRetryPolicy OpenAI_API_Client::getRetryPolicy() const {
    return pImpl->getRetryPolicy();
}

void OpenAI_API_Client::setRetryEnabled(bool enable) {
    pImpl->setRetryEnabled(enable);
}

bool OpenAI_API_Client::isRetryEnabled() const {
    return pImpl->isRetryEnabled();
}

// Implement the retry statistics methods
RetryStatistics OpenAI_API_Client::getRetryStatistics() const {
    return pImpl->getRetryStatistics();
}

void OpenAI_API_Client::resetRetryStatistics() {
    pImpl->resetRetryStatistics();
}

} // namespace ai_editor 