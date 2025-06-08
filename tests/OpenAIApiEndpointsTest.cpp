#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "OpenAI_API_Client.h"
#include "IOpenAI_API_Client.h"
#include "OpenAI_API_Client_types.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

// Add NOMINMAX to prevent Windows min/max macros
#define NOMINMAX

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace ai_editor {
namespace test {

// Helper function to read API key from .env file
std::string readApiKeyFromEnvFile() {
    // Try .env file in the project root directory first (two levels up from build/tests)
    std::ifstream envFile("../../.env");
    
    // If can't open, try current directory
    if (!envFile.is_open()) {
        std::cout << "No .env in root directory, trying current directory..." << std::endl;
        envFile.open(".env");
    }
    
    // If still can't open, try parent directory (for build subdirectories)
    if (!envFile.is_open()) {
        std::cout << "No .env in current directory, trying parent directory..." << std::endl;
        envFile.open("../.env");
    }
    
    std::string line;
    std::string apiKey;
    
    std::cout << "Attempting to read .env file..." << std::endl;
    
    if (envFile.is_open()) {
        std::cout << ".env file opened successfully" << std::endl;
        while (std::getline(envFile, line)) {
            // Print the raw bytes of the line for debugging
            std::cout << "Read line: ";
            for (size_t i = 0; i < (std::min)(line.size(), size_t(20)); ++i) {
                std::cout << "0x" << std::hex << (int)(unsigned char)line[i] << " ";
            }
            std::cout << std::dec << std::endl;
            
            // Skip BOM or other non-printable characters at the start
            size_t start = 0;
            while (start < line.size() && (line[start] == '\xEF' || line[start] == '\xBB' || line[start] == '\xBF' || static_cast<unsigned char>(line[start]) < 32)) {
                std::cout << "Skipping byte: 0x" << std::hex << (int)(unsigned char)line[start] << std::dec << std::endl;
                start++;
            }
            
            // Clean the line of any BOM or special characters
            std::string cleanLine = line.substr(start);
            
            std::cout << "Cleaned line: " << cleanLine.substr(0, (std::min)(cleanLine.size(), size_t(20))) << "..." << std::endl;
            
            // Look for OPENAI_API_KEY=sk-...
            if (cleanLine.find("OPENAI_API_KEY=") == 0) {
                apiKey = cleanLine.substr(std::string("OPENAI_API_KEY=").length());
                std::cout << "Found API key starting with: " << (apiKey.size() >= 5 ? apiKey.substr(0, 5) : apiKey) << std::endl;
                if (apiKey.size() >= 10) {
                    std::cout << "API key ending with: " << apiKey.substr(apiKey.size() - 5) << std::endl;
                }
                break;
            }
        }
        envFile.close();
    } else {
        std::cout << "Failed to open .env file. Current directory: " << std::endl;
        // Try to print current directory
        #ifdef _WIN32
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        std::cout << buffer << std::endl;
        #else
        char buffer[PATH_MAX];
        if (getcwd(buffer, PATH_MAX) != NULL) {
            std::cout << buffer << std::endl;
        }
        #endif
    }
    
    if (apiKey.empty()) {
        std::cout << "No API key found in .env file" << std::endl;
    } else {
        std::cout << "API key from .env file: " << apiKey.substr(0, 5) << "..." << apiKey.substr(apiKey.length() - 5) << std::endl;
    }
    
    return apiKey;
}

class OpenAIApiEndpointsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // First try to read from .env file
        apiKeyStr = readApiKeyFromEnvFile();
        if (!apiKeyStr.empty()) {
            apiKeySource = ".env file";
            std::cout << "Using API key from .env file" << std::endl;
            std::cout << "API key prefix: " << apiKeyStr.substr(0, 5) << std::endl;
            return;
        }

        // If not found in .env file, try environment variable
        #ifdef _MSC_VER
        // Windows-specific safe environment variable handling
        char* apiKey = nullptr;
        size_t len = 0;
        errno_t err = _dupenv_s(&apiKey, &len, "OPENAI_API_KEY");
        
        if (err == 0 && apiKey != nullptr) {
            apiKeyStr = std::string(apiKey);
            apiKeySource = "environment variable";
            std::cout << "Using API key from environment variable" << std::endl;
            std::cout << "Full API key from env: " << apiKeyStr << std::endl;
            free(apiKey);
        }
        #else
        // Standard getenv for other platforms
        const char* apiKey = std::getenv("OPENAI_API_KEY");
        if (apiKey) {
            apiKeyStr = std::string(apiKey);
            apiKeySource = "environment variable";
            std::cout << "Using API key from environment variable" << std::endl;
            std::cout << "Full API key from env: " << apiKeyStr << std::endl;
        }
        #endif

        // Skip tests if API key not available
        if (apiKeyStr.empty()) {
            GTEST_SKIP() << "Skipping test because OPENAI_API_KEY is not set in environment or .env file";
        }
        
        if (!apiKeyStr.empty()) {
            std::cout << "API key prefix: " << apiKeyStr.substr(0, 5) << std::endl;
        }
    }

    std::string apiKeyStr;
    std::string apiKeySource = "unknown";
};

// Test basic chat completion endpoint connectivity
TEST_F(OpenAIApiEndpointsTest, ChatCompletionEndpointConnects) {
    // Create the OpenAI API client
    OpenAI_API_Client client(apiKeyStr);
    
    // Create a simple request
    std::vector<ApiChatMessage> messages;
    messages.push_back(ApiChatMessage("system", "You are a helpful assistant."));
    messages.push_back(ApiChatMessage("user", "Reply with exactly the text 'API_TEST_SUCCESS'"));
    
    // Send request to OpenAI
    ApiResponse response = client.sendChatCompletionRequest(
        messages,
        {}, // Empty tools vector
        "gpt-3.5-turbo", // Using a cheaper model for testing
        0.0f,  // Zero temperature for deterministic response
        50     // Small max tokens for efficiency
    );
    
    // Check if the request was successful
    if (!response.success) {
        std::cerr << "API request failed with error: " << response.error_message << std::endl;
        
        // Check for specific types of errors
        if (response.error_message.find("401") != std::string::npos) {
            FAIL() << "Authentication error: Invalid API key. Please check your OPENAI_API_KEY in environment variable or .env file.";
        } else if (response.error_message.find("429") != std::string::npos) {
            FAIL() << "Rate limit exceeded: The API key has reached its request limit or quota.";
        } else if (response.error_message.find("Connection") != std::string::npos) {
            FAIL() << "Connection error: Could not connect to OpenAI API. Please check your internet connection.";
        }
    }
    
    // Verify success
    ASSERT_TRUE(response.success) << "API request failed: " << response.error_message;
    
    // Verify we got a non-empty response
    EXPECT_FALSE(response.content.empty()) << "API returned an empty response";
    
    // Verify the content contains our expected string (exact matching might be unreliable)
    EXPECT_THAT(response.content, ::testing::HasSubstr("API_TEST_SUCCESS"));
}

// Test json response parsing functionality
TEST_F(OpenAIApiEndpointsTest, JsonResponseParsingWorks) {
    // Create the OpenAI API client
    OpenAI_API_Client client(apiKeyStr);
    
    // Create a simple request
    std::vector<ApiChatMessage> messages;
    messages.push_back(ApiChatMessage("system", "You are a helpful assistant."));
    messages.push_back(ApiChatMessage("user", "Reply with exactly the text 'API_TEST_SUCCESS'"));
    
    // Send request to OpenAI
    ApiResponse response = client.sendChatCompletionRequest(
        messages,
        {}, // Empty tools vector
        "gpt-3.5-turbo",
        0.0f,
        50
    );
    
    // Verify success
    ASSERT_TRUE(response.success) << "API request failed: " << response.error_message;
    
    // Verify we received a raw JSON response
    EXPECT_FALSE(response.raw_json_response.empty()) << "No raw JSON response received";
    
    // Verify the JSON is well-formed (contains expected fields)
    EXPECT_THAT(response.raw_json_response, ::testing::AllOf(
        ::testing::HasSubstr("\"choices\""), 
        ::testing::HasSubstr("\"message\""),
        ::testing::HasSubstr("\"content\"")
    ));
}

// Test tool calls functionality
TEST_F(OpenAIApiEndpointsTest, ToolCallsEndpointWorks) {
    // Create the OpenAI API client
    OpenAI_API_Client client(apiKeyStr);
    
    // Create tool definition
    std::vector<ApiToolDefinition> tools;
    ApiToolDefinition echoTool("echo", "Echoes back the input provided");
    
    // Add parameters to the echo tool
    ApiFunctionParameter textParam;
    textParam.name = "text";
    textParam.type = "string";
    textParam.description = "The text to echo back";
    textParam.required = true;
    
    echoTool.function.parameters.push_back(textParam);
    tools.push_back(echoTool);
    
    // Create messages that will trigger the tool
    std::vector<ApiChatMessage> messages;
    messages.push_back(ApiChatMessage("system", "You are a helpful assistant that uses tools when appropriate."));
    messages.push_back(ApiChatMessage("user", "Please use the echo tool to echo back 'TEST_TOOL_CALL'"));
    
    // Send request to OpenAI
    ApiResponse response = client.sendChatCompletionRequest(
        messages,
        tools,
        "gpt-4o", // Need a model that supports tools
        0.0f,     // Zero temperature
        50        // Small max tokens
    );
    
    // Verify success
    ASSERT_TRUE(response.success) << "API request with tools failed: " << response.error_message;
    
    // Verify tool calls were made
    EXPECT_FALSE(response.tool_calls.empty()) << "No tool calls were made";
    
    if (!response.tool_calls.empty()) {
        const auto& toolCall = response.tool_calls[0];
        EXPECT_EQ(toolCall.function.name, "echo");
        EXPECT_THAT(toolCall.function.arguments, ::testing::HasSubstr("TEST_TOOL_CALL"));
    }
}

} // namespace test
} // namespace ai_editor

// Run the tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 