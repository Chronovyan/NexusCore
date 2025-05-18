#include "OpenAI_API_Client.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace ai_editor {

// PIMPL implementation
class OpenAIClientImpl {
public:
    explicit OpenAIClientImpl(const std::string& apiKey) 
        : apiKey_(apiKey), 
          apiUrl_("https://api.openai.com/v1/chat/completions"),
          retryEnabled_(true),
          retryPolicy_() { // Default retry policy
        headers_ = cpr::Header{
            {"Content-Type", "application/json"},
            {"Authorization", "Bearer " + apiKey_}
        };
    }

    ApiResponse sendRequest(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools,
        const std::string& model,
        float temperature,
        int32_t max_tokens
    ) {
        ApiResponse response;
        
        // Prepare the JSON request body outside the retry loop
        json requestBody;
        try {
            requestBody = prepareRequestBody(messages, tools, model, temperature, max_tokens);
        } catch (const std::exception& e) {
            response.success = false;
            response.error_message = "Exception preparing request: " + std::string(e.what());
            return response;
        }
        
        std::string requestBodyStr = requestBody.dump();
        
        #ifdef _DEBUG
        std::cout << "Request: " << requestBodyStr << std::endl;
        #endif
        
        // Initialize retry counters and flags
        int retryCount = 0;
        bool shouldRetry = false;
        std::string retryReason;
        
        // Create random engine for jitter
        std::random_device rd;
        std::mt19937 gen(rd());
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
                
                // Apply jitter
                double jitter = 1.0 + jitterDist(gen);
                auto backoffWithJitter = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::duration<double, std::milli>(baseBackoff.count() * jitter));
                
                // Cap at max backoff
                auto finalBackoff = (backoffWithJitter > retryPolicy_.maxBackoff) ? retryPolicy_.maxBackoff : backoffWithJitter;
                
                std::cout << "Retry " << retryCount << " for " << retryReason << ". Backing off for " 
                          << finalBackoff.count() << "ms" << std::endl;
                
                // Sleep for the calculated duration
                std::this_thread::sleep_for(finalBackoff);
            }
            
            // Make the API request
            cpr::Response httpResponse = cpr::Post(
                cpr::Url{apiUrl_},
                headers_,
                cpr::Body{requestBodyStr},
                cpr::Timeout{30000} // 30 seconds timeout
            );
            
            // Process response
            response.raw_json_response = httpResponse.text;
            
            #ifdef _DEBUG
            std::cout << "Response code: " << httpResponse.status_code << std::endl;
            std::cout << "Response: " << httpResponse.text << std::endl;
            #endif
            
            // Check for transient errors that we might want to retry
            if (httpResponse.status_code >= 200 && httpResponse.status_code < 300) {
                // Successful response
                response.success = true;
                
                try {
                    // Parse JSON response
                    json responseJson = json::parse(httpResponse.text);
                    
                    // Extract content and/or tool calls
                    if (responseJson.contains("choices") && !responseJson["choices"].empty()) {
                        const auto& choice = responseJson["choices"][0];
                        const auto& message = choice["message"];
                        
                        // Extract content if present
                        if (message.contains("content") && !message["content"].is_null()) {
                            response.content = message["content"].get<std::string>();
                        }
                        
                        // Extract tool calls if present
                        if (message.contains("tool_calls") && !message["tool_calls"].is_null()) {
                            for (const auto& toolCall : message["tool_calls"]) {
                                ApiToolCall apiToolCall;
                                apiToolCall.id = toolCall["id"].get<std::string>();
                                apiToolCall.type = toolCall["type"].get<std::string>();
                                
                                if (toolCall.contains("function")) {
                                    apiToolCall.function.name = toolCall["function"]["name"].get<std::string>();
                                    apiToolCall.function.arguments = toolCall["function"]["arguments"].get<std::string>();
                                }
                                
                                response.tool_calls.push_back(apiToolCall);
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    // JSON parsing error - this is not retryable
                    response.success = false;
                    response.error_message = "Error parsing response: " + std::string(e.what());
                }
            } else {
                // Error response - check if it's retryable
                response.success = false;
                response.error_message = "HTTP Error " + std::to_string(httpResponse.status_code) + ": " + httpResponse.text;
                
                if (retryEnabled_) {
                    // Check for rate limiting (429)
                    if (httpResponse.status_code == 429 && retryPolicy_.retryOnRateLimit) {
                        shouldRetry = true;
                        retryReason = "rate limiting";
                        
                        // Check for Retry-After header
                        if (httpResponse.header.find("Retry-After") != httpResponse.header.end()) {
                            try {
                                int retryAfterSecs = std::stoi(httpResponse.header["Retry-After"]);
                                // We'll use the retry-after time from the server on the next iteration
                                std::cout << "Server requested Retry-After: " << retryAfterSecs << " seconds" << std::endl;
                            } catch (...) {
                                // Couldn't parse Retry-After header, will use default backoff
                            }
                        }
                    }
                    // Check for server errors (5xx)
                    else if (httpResponse.status_code >= 500 && httpResponse.status_code < 600 && retryPolicy_.retryOnServerErrors) {
                        shouldRetry = true;
                        retryReason = "server error";
                    }
                }
            }
            
            // Check for network errors which are represented with error codes in cpr
            if (httpResponse.error.code != cpr::ErrorCode::OK && retryEnabled_ && retryPolicy_.retryOnNetworkErrors) {
                response.success = false;
                response.error_message = "Network Error: " + httpResponse.error.message;
                shouldRetry = true;
                retryReason = "network error";
            }
            
            // Only retry if we have remaining attempts
            if (shouldRetry && retryCount >= retryPolicy_.maxRetries) {
                shouldRetry = false;
                std::cout << "Maximum retry attempts reached (" << retryPolicy_.maxRetries << "). Giving up." << std::endl;
            }
            
            // Increment retry counter if we're going to retry
            if (shouldRetry) {
                retryCount++;
            }
            
        } while (shouldRetry);
        
        // Add retry information to the response
        if (retryCount > 0) {
            response.error_message += " (Retried " + std::to_string(retryCount) + " times)";
        }
        
        return response;
    }
    
    void setRetryPolicy(const ApiRetryPolicy& policy) {
        retryPolicy_ = policy;
    }
    
    ApiRetryPolicy getRetryPolicy() const {
        return retryPolicy_;
    }
    
    void enableRetries(bool enable) {
        retryEnabled_ = enable;
    }
    
    bool isRetryEnabled() const {
        return retryEnabled_;
    }

private:
    // Helper method to prepare the JSON request body
    json prepareRequestBody(
        const std::vector<ApiChatMessage>& messages,
        const std::vector<ApiToolDefinition>& tools,
        const std::string& model,
        float temperature,
        int32_t max_tokens
    ) {
        // Convert messages to JSON
        json messagesJson = json::array();
        for (const auto& msg : messages) {
            json messageJson = {
                {"role", msg.role},
                {"content", msg.content}
            };
            
            if (msg.name.has_value()) {
                messageJson["name"] = msg.name.value();
            }
            
            // Handle tool_call_id if present (for responding to tool calls)
            if (msg.tool_call_id.has_value() && !msg.tool_call_id.value().empty()) {
                messageJson["tool_call_id"] = msg.tool_call_id.value();
            }
            
            messagesJson.push_back(messageJson);
        }
        
        // Prepare request body
        json requestBody = {
            {"model", model},
            {"messages", messagesJson},
            {"temperature", temperature},
            {"max_tokens", max_tokens}
        };
        
        // Add tools if provided
        if (!tools.empty()) {
            json toolsJson = json::array();
            
            for (const auto& tool : tools) {
                json parametersJson = {
                    {"type", "object"},
                    {"properties", json::object()}
                };
                
                std::vector<std::string> requiredParams;
                
                for (const auto& param : tool.function.parameters) {
                    parametersJson["properties"][param.name] = {
                        {"type", param.type},
                        {"description", param.description}
                    };
                    
                    // Add items definition for array parameters
                    if (param.type == "array" && !param.items_type.empty()) {
                        json itemsJson;
                        itemsJson["type"] = param.items_type;
                        
                        // If it's an object type with properties
                        if (param.items_type == "object" && !param.items_properties.empty()) {
                            json propertiesJson = json::object();
                            std::vector<std::string> requiredItemProps;
                            
                            for (const auto& prop : param.items_properties) {
                                propertiesJson[prop.name] = {
                                    {"type", prop.type},
                                    {"description", prop.description}
                                };
                                
                                if (prop.required) {
                                    requiredItemProps.push_back(prop.name);
                                }
                            }
                            
                            itemsJson["properties"] = propertiesJson;
                            
                            if (!requiredItemProps.empty()) {
                                itemsJson["required"] = requiredItemProps;
                            }
                        }
                        
                        parametersJson["properties"][param.name]["items"] = itemsJson;
                    }
                    
                    if (param.required) {
                        requiredParams.push_back(param.name);
                    }
                }
                
                if (!requiredParams.empty()) {
                    parametersJson["required"] = requiredParams;
                }
                
                json toolJson = {
                    {"type", tool.type},
                    {"function", {
                        {"name", tool.function.name},
                        {"description", tool.function.description}
                    }}
                };
                
                // Only add parameters if we have any
                if (!tool.function.parameters.empty()) {
                    toolJson["function"]["parameters"] = parametersJson;
                }
                
                toolsJson.push_back(toolJson);
            }
            
            requestBody["tools"] = toolsJson;
            requestBody["tool_choice"] = "auto";
        }
        
        return requestBody;
    }

    std::string apiKey_;
    std::string apiUrl_;
    cpr::Header headers_;
    bool retryEnabled_;
    ApiRetryPolicy retryPolicy_;
};

// Constructor implementation
OpenAI_API_Client::OpenAI_API_Client(const std::string& apiKey)
    : pImpl(std::make_unique<OpenAIClientImpl>(apiKey)) {
}

// Destructor
OpenAI_API_Client::~OpenAI_API_Client() = default;

// Move constructor
OpenAI_API_Client::OpenAI_API_Client(OpenAI_API_Client&&) noexcept = default;

// Move assignment
OpenAI_API_Client& OpenAI_API_Client::operator=(OpenAI_API_Client&&) noexcept = default;

// sendChatCompletionRequest implementation
ApiResponse OpenAI_API_Client::sendChatCompletionRequest(
    const std::vector<ApiChatMessage>& messages,
    const std::vector<ApiToolDefinition>& tools,
    const std::string& model,
    float temperature,
    int32_t max_tokens
) {
    return pImpl->sendRequest(messages, tools, model, temperature, max_tokens);
}

// Set retry policy implementation
void OpenAI_API_Client::setRetryPolicy(const ApiRetryPolicy& policy) {
    pImpl->setRetryPolicy(policy);
}

// Get retry policy implementation
ApiRetryPolicy OpenAI_API_Client::getRetryPolicy() const {
    return pImpl->getRetryPolicy();
}

// Enable/disable retries implementation
void OpenAI_API_Client::enableRetries(bool enable) {
    pImpl->enableRetries(enable);
}

// Check if retries are enabled implementation
bool OpenAI_API_Client::isRetryEnabled() const {
    return pImpl->isRetryEnabled();
}

} // namespace ai_editor 