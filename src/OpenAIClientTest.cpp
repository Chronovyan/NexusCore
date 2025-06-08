#include "OpenAI_API_Client.h"
#include "MockOpenAI_API_Client.h"
#include "AppDebugLog.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

#ifdef _MSC_VER
#include <cstdlib>
#endif

using namespace ai_editor;

// Helper function to read API key from .env file
std::string readApiKeyFromEnvFile() {
    LOG_DEBUG("Attempting to read .env file...");
    
    // Try .env file in the current directory first
    std::ifstream envFile(".env");
    
    // If can't open, try parent directory
    if (!envFile.is_open()) {
        LOG_DEBUG("No .env in current directory, trying parent directory...");
        envFile.open("../.env");
    }
    
    // If still can't open, try project root directory
    if (!envFile.is_open()) {
        LOG_DEBUG("No .env in parent directory, trying project root...");
        envFile.open("../../.env");
    }
    
    if (!envFile.is_open()) {
        LOG_ERROR("Failed to open .env file");
        return "";
    }
    
    std::string line;
    std::string apiKey;
    
    while (std::getline(envFile, line)) {
        // Look for OPENAI_API_KEY=
        if (line.find("OPENAI_API_KEY=") == 0) {
            apiKey = line.substr(15); // Length of "OPENAI_API_KEY="
            LOG_DEBUG("Found API key in .env file");
            return apiKey;
        }
    }
    
    LOG_ERROR("API key not found in .env file");
    return "";
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    // Initialize debug logging
    LOG_INIT("OpenAIClientTest");
    LOG_DEBUG("Starting OpenAIClientTest");
    
    try {
        // Get API key (for real client, not used for mock)
        std::string apiKeyStr = "test-api-key"; // Placeholder, not needed for mock
        
        // Create a mock OpenAI API client instead of real one
        LOG_DEBUG("Creating mock OpenAI API client");
        MockOpenAI_API_Client client;
        LOG_DEBUG("Mock OpenAI API client created");
        
        // Set up a mock successful response
        LOG_DEBUG("Setting up mock response");
        client.setResponseContent("I'm a helpful assistant and I can provide information, answer questions, and assist with various tasks. How can I help you today?");
        
        // Create a simple request
        LOG_DEBUG("Creating API request");
        std::vector<ApiChatMessage> messages;
        messages.push_back(ApiChatMessage("system", "You are a helpful assistant."));
        messages.push_back(ApiChatMessage("user", "Hello, what can you do for me?"));
        
        std::vector<ApiToolDefinition> tools; // Empty tools vector for now
        
        // Send request to mock OpenAI client
        LOG_DEBUG("Sending request to mock OpenAI API");
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