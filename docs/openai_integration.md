# OpenAI API Integration

This document explains how the AI-First TextEditor integrates with the OpenAI API.

## Overview

The application uses OpenAI's API to power its AI features, particularly code generation and project planning. The integration consists of:

1. Authentication and API communication
2. Message formatting and parsing
3. Tool definition and usage
4. Response handling and error management

## Authentication

Authentication is handled via API key:

```cpp
// In OpenAI_API_Client.cpp
std::string OpenAI_API_Client::getApiKey() {
    // Try to get from environment variables first
    char* apiKey = std::getenv("OPENAI_API_KEY");
    if (apiKey != nullptr) {
        return std::string(apiKey);
    }
    
    // Otherwise try to read from .env file
    // ...
}
```

The API key can be provided in two ways:
1. Environment variable: `OPENAI_API_KEY`
2. `.env` file in the executable directory

## API Endpoints

The application primarily uses these endpoints:

### Chat Completions API

```cpp
// In OpenAI_API_Client.cpp
ApiResponse OpenAI_API_Client::createChatCompletion(
    const std::vector<ApiChatMessage>& messages,
    const std::vector<ApiToolDefinition>& tools
) {
    // Construct request URL
    std::string url = baseUrl_ + "/chat/completions";
    
    // Create request body
    nlohmann::json requestBody = {
        {"model", model_},
        {"messages", nlohmann::json::array()},
        {"temperature", temperature_}
    };
    
    // Add messages to request
    for (const auto& message : messages) {
        // ...
    }
    
    // Add tools if provided
    if (!tools.empty()) {
        // ...
    }
    
    // Send request
    cpr::Response response = cpr::Post(
        cpr::Url{url},
        cpr::Header{{"Authorization", "Bearer " + getApiKey()}},
        cpr::Body{requestBody.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );
    
    // Parse response
    // ...
}
```

## Message Format

Messages follow the OpenAI Chat Completions format:

```cpp
struct ApiChatMessage {
    std::string role;  // "system", "user", "assistant", or "tool"
    std::string content;
    std::string name;  // Optional
    std::string tool_call_id;  // For tool responses
};
```

## Tool Definitions

The application defines several tools that the AI can use:

```cpp
// Example tool definition
ApiToolDefinition writeFileContentTool("write_file_content",
    "Write content to a file in the project workspace");

// Add parameters
writeFileContentTool.function.parameters.push_back({
    "filename", "string", "The name of the file to write"
});
writeFileContentTool.function.parameters.push_back({
    "content", "string", "The content to write to the file"
});
```

### Available Tools

1. `propose_plan`: Generate a project plan
2. `write_file_content`: Create or update a file
3. `execute_system_command`: Run compilation commands

## Error Handling

The API client handles various error scenarios:

```cpp
// Error handling for HTTP errors
if (response.status_code != 200) {
    ApiResponse errorResponse;
    errorResponse.success = false;
    
    try {
        auto jsonResponse = nlohmann::json::parse(response.text);
        if (jsonResponse.contains("error")) {
            auto error = jsonResponse["error"];
            if (error.contains("message")) {
                errorResponse.error_message = error["message"];
            }
            if (error.contains("type")) {
                errorResponse.error_type = error["type"];
            }
        }
    } catch (...) {
        errorResponse.error_message = "Failed to parse error response";
    }
    
    return errorResponse;
}
```

## Request Flow

The typical AI interaction flow:

1. User submits a prompt
2. AIAgentOrchestrator creates a conversation history
3. OpenAI_API_Client sends the request
4. Response is processed by AIAgentOrchestrator
5. If the response contains tool calls, they're executed
6. Results are displayed to the user

## Configuration

API settings are configurable:

```cpp
// Example configuration
OpenAI_API_Client client;
client.setModel("gpt-4o");  // Use GPT-4o
client.setTemperature(0.7); // Slightly creative
client.setMaxTokens(4000);  // Longer responses
```

## Testing API Integration

For testing the API integration without making actual API calls:

```cpp
// Create a mock client
MockOpenAI_API_Client mockClient;

// Create and prime a response
ApiResponse mockResponse;
mockResponse.success = true;
mockResponse.content = "Test content";
mockClient.primeResponse(mockResponse);

// Use in your component
AIAgentOrchestrator orchestrator(mockClient, uiModel, workspaceManager);
```

## Response Examples

### Plan Response

```json
{
  "files": [
    {
      "name": "main.cpp",
      "description": "Main entry point for the application"
    },
    {
      "name": "CMakeLists.txt",
      "description": "Build configuration file"
    }
  ],
  "explanation": "This is a simple C++ project with a main file and build configuration."
}
```

### Write File Response

```json
{
  "filename": "main.cpp",
  "content": "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}",
  "description": "Main entry point for the application"
}
``` 