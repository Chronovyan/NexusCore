#include "OpenAI_API_Client.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>
#include <memory>

using json = nlohmann::json;

namespace ai_editor {

// PIMPL implementation
class OpenAIClientImpl {
public:
    explicit OpenAIClientImpl(const std::string& apiKey) 
        : apiKey_(apiKey), apiUrl_("https://api.openai.com/v1/chat/completions") {
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
        
        try {
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
            
            // Make the request
            std::string requestBodyStr = requestBody.dump();
            
            #ifdef _DEBUG
            std::cout << "Request: " << requestBodyStr << std::endl;
            #endif
            
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
            
            if (httpResponse.status_code >= 200 && httpResponse.status_code < 300) {
                // Successful response
                response.success = true;
                
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
            } else {
                // Error response
                response.success = false;
                response.error_message = "HTTP Error " + std::to_string(httpResponse.status_code) + ": " + httpResponse.text;
            }
        } catch (const std::exception& e) {
            response.success = false;
            response.error_message = "Exception: " + std::string(e.what());
        }
        
        return response;
    }

private:
    std::string apiKey_;
    std::string apiUrl_;
    cpr::Header headers_;
};

// Implementation of OpenAI_API_Client using PIMPL

OpenAI_API_Client::OpenAI_API_Client(const std::string& apiKey)
    : pImpl(std::make_unique<OpenAIClientImpl>(apiKey)) {
}

OpenAI_API_Client::~OpenAI_API_Client() = default;

OpenAI_API_Client::OpenAI_API_Client(OpenAI_API_Client&&) noexcept = default;
OpenAI_API_Client& OpenAI_API_Client::operator=(OpenAI_API_Client&&) noexcept = default;

ApiResponse OpenAI_API_Client::sendChatCompletionRequest(
    const std::vector<ApiChatMessage>& messages,
    const std::vector<ApiToolDefinition>& tools,
    const std::string& model,
    float temperature,
    int32_t max_tokens
) {
    return pImpl->sendRequest(messages, tools, model, temperature, max_tokens);
}

} // namespace ai_editor 