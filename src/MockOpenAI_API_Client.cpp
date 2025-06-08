#include "MockOpenAI_API_Client.h"
#include "EditorError.h"
#include <chrono>
#include <thread>
#include <sstream>

namespace ai_editor {

ApiResponse MockOpenAI_API_Client::sendChatCompletionRequest(
    const std::vector<ApiChatMessage>& messages,
    const std::vector<ApiToolDefinition>& tools,
    const std::string& model,
    float temperature,
    int32_t max_tokens
) {
    // Store request parameters for inspection
    last_sent_messages_ = messages;
    last_sent_tools_ = tools;
    last_sent_model_ = model;
    last_sent_temperature_ = temperature;
    last_sent_max_tokens_ = max_tokens;
    
    // Process the failure sequence if configured
    if (!failureSequence.empty()) {
        // Enable retry simulation for failure sequence
        simulateRetries_ = true;
        
        FailureScenario scenario = failureSequence.front();
        failureSequence.erase(failureSequence.begin());
        
        ApiResponse response;
        response.success = (scenario.type == FailureType::None);
        response.error_message = scenario.message;
        
        // If it's not a success, update response to reflect the failure type
        if (scenario.type != FailureType::None) {
            // Generate error message based on failure type
            std::string errorType;
            
            switch (scenario.type) {
                case FailureType::Authentication:
                    errorType = "authentication_error";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\",\"code\":401}}";
                    break;
                case FailureType::RateLimit:
                    errorType = "rate_limit_exceeded";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\",\"code\":429}}";
                    break;
                case FailureType::ServerError:
                    errorType = "server_error";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\",\"code\":500}}";
                    break;
                case FailureType::InvalidRequest:
                    errorType = "invalid_request_error";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\",\"code\":400}}";
                    break;
                case FailureType::Timeout:
                    errorType = "timeout_error";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\",\"code\":408}}";
                    break;
                case FailureType::Network:
                    errorType = "network_error";
                    response.raw_json_response = ""; // Network errors don't have JSON
                    break;
                case FailureType::SchemaValidation:
                    errorType = "schema_validation_error";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\",\"code\":422}}";
                    break;
                default:
                    errorType = "unknown_error";
                    response.raw_json_response = "{\"error\":{\"message\":\"" + scenario.message + "\",\"type\":\"" + errorType + "\"}}";
                    break;
            }
            
            // If retry simulation is enabled, record retry attempt and append info
            if (simulateRetries_ && !response.success) {
                retryStats_.recordRetryAttempt(errorType, false, 1);
                retryCount_++;
                
                // Note that retry was attempted in the response
                response.error_message += " (Retry attempt #" + std::to_string(retryCount_) + ")";
                
                // Generate a unique operation ID for this API call
                std::string operationId = "mock_api_call_" + 
                    std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                
                // Log retry attempt
                ErrorReporter::logRetryAttempt(
                    operationId,
                    "MockOpenAI_API",
                    retryCount_,
                    errorType,
                    std::chrono::milliseconds(10) // Mock backoff time
                );
            }
        } else {
            // It's a success response
            response.content = responseContent_;
            response.tool_calls = toolCalls_;
        }
        
        return response;
    }
    
    // If a custom handler is set, use it
    if (responseHandler_) {
        return responseHandler_(messages);
    }
    
    // If we have a response in the queue, return it
    if (!response_queue_.empty()) {
        ApiResponse response = response_queue_.front();
        response_queue_.pop();
        return response;
    }
    
    // Otherwise use the default success/error response
    ApiResponse response;
    if (successResponse_) {
        response.success = true;
        response.content = responseContent_;
        response.tool_calls = toolCalls_;
    } else {
        response.success = false;
        response.error_message = errorMessage_;
        response.raw_json_response = "{\"error\":{\"message\":\"" + errorMessage_ + "\",\"code\":" + std::to_string(errorStatusCode_) + "}}";
        
        // Only log retry attempts for debugging/visibility, but don't record stats again
        // since that's now done in the simulateRetries method
        if (retryEnabled_ && simulateRetries_) {
            // Generate a unique operation ID for this API call
            std::string operationId = "mock_api_call_" + 
                std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            
            // Reset the retry counter to start fresh
            retryCount_ = 0;
            
            // Log the individual retry attempts for observability
            for (int i = 0; i < simulatedRetryCount_; i++) {
                retryCount_++;
                ErrorReporter::logRetryAttempt(
                    operationId,
                    "MockOpenAI_API",
                    retryCount_,
                    simulatedRetryReason_,
                    std::chrono::milliseconds(10) // Mock backoff time
                );
            }
            
            // Log final result
            ErrorReporter::logRetryResult(
                operationId,
                simulatedRetrySuccess_,
                simulatedRetrySuccess_ ? "Succeeded after retries" : "Failed after maximum retries"
            );
        }
    }
    
    return response;
}

} // namespace ai_editor 