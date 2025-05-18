#include "OpenAI_API_Client.h"
#include "AppDebugLog.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#ifdef _MSC_VER
#include <cstdlib>
#endif

using namespace ai_editor;

int main(int argc, char** argv) {
    // Initialize debug logging
    LOG_INIT("OpenAIClientTest");
    LOG_DEBUG("Starting OpenAIClientTest");
    
    try {
        // Check for API key
        LOG_DEBUG("Checking for API key environment variable");
        
        #ifdef _MSC_VER
        // Windows-specific safe environment variable handling
        char* apiKey = nullptr;
        size_t len = 0;
        errno_t err = _dupenv_s(&apiKey, &len, "OPENAI_API_KEY");
        
        if (err != 0 || apiKey == nullptr) {
            LOG_ERROR("OPENAI_API_KEY environment variable not set");
            std::cerr << "ERROR: OPENAI_API_KEY environment variable not set!" << std::endl;
            std::cerr << "Please set your OpenAI API key as an environment variable." << std::endl;
            return 1;
        }
        
        // Store a copy of the key
        std::string apiKeyStr(apiKey);
        
        // Free the memory allocated by _dupenv_s
        free(apiKey);
        apiKey = nullptr;
        #else
        // Standard getenv for other platforms
        char* apiKey = std::getenv("OPENAI_API_KEY");
        if (!apiKey) {
            LOG_ERROR("OPENAI_API_KEY environment variable not set");
            std::cerr << "ERROR: OPENAI_API_KEY environment variable not set!" << std::endl;
            std::cerr << "Please set your OpenAI API key as an environment variable." << std::endl;
            return 1;
        }
        
        // Store a copy of the key
        std::string apiKeyStr(apiKey);
        #endif
        
        LOG_DEBUG("API key found");
        
        // Create the OpenAI API client
        LOG_DEBUG("Creating OpenAI API client");
        OpenAI_API_Client client(apiKeyStr);
        LOG_DEBUG("OpenAI API client created");
        
        // Create a simple request
        LOG_DEBUG("Creating API request");
        std::vector<ApiChatMessage> messages;
        messages.push_back(ApiChatMessage("system", "You are a helpful assistant."));
        messages.push_back(ApiChatMessage("user", "Hello, what can you do for me?"));
        
        std::vector<ApiToolDefinition> tools; // Empty tools vector for now
        
        // Send request to OpenAI
        LOG_DEBUG("Sending request to OpenAI API");
        ApiResponse response = client.sendChatCompletionRequest(
            messages,
            tools,
            "gpt-3.5-turbo",
            0.7f,
            150
        );
        
        // Log and print result
        if (response.success) {
            LOG_DEBUG("Request successful");
            LOG_DEBUG("Response content: " + response.content);
            std::cout << "Response: " << response.content << std::endl;
            return 0;
        } else {
            LOG_ERROR("Request failed: " + response.error_message);
            std::cerr << "ERROR: " << response.error_message << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception occurred: " + std::string(e.what()));
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        LOG_ERROR("Unknown exception occurred");
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
} 