#ifndef OPENAI_API_CLIENT_TYPES_H
#define OPENAI_API_CLIENT_TYPES_H

#include <string>
#include <vector>
#include <optional>

namespace ai_editor {

// Represents a single message in the chat history
struct ApiChatMessage {
    std::string role;  // "system", "user", "assistant", or "tool"
    std::string content;
    std::optional<std::string> name;  // For tool roles
    std::optional<std::string> tool_call_id;  // For tool response messages
    
    // Constructor for basic message
    ApiChatMessage(const std::string& role, const std::string& content)
        : role(role), content(content) {}
    
    // Constructor with name
    ApiChatMessage(const std::string& role, const std::string& content, const std::string& name)
        : role(role), content(content), name(name) {}
};

// Represents a function parameter schema
struct ApiFunctionParameter {
    std::string name;
    std::string type;  // e.g., "string", "integer", "boolean"
    std::string description;
    bool required = false;
    
    // For array type parameters
    std::string items_type;  // The type of items in the array (e.g., "object", "string")
    
    // For object type items in an array
    struct Property {
        std::string name;
        std::string type;
        std::string description;
        bool required;
    };
    std::vector<Property> items_properties;  // Properties of object items
    
    // Constructor for simple parameters
    ApiFunctionParameter() = default;
    
    // Constructor for basic parameters
    ApiFunctionParameter(const std::string& name, const std::string& type, const std::string& description, bool required)
        : name(name), type(type), description(description), required(required) {}
};

// Represents a tool definition the AI can call
struct ApiToolDefinition {
    std::string type = "function";
    
    struct FunctionDefinition {
        std::string name;
        std::string description;
        std::vector<ApiFunctionParameter> parameters;
    };
    
    FunctionDefinition function;
    
    // Constructor
    ApiToolDefinition(const std::string& name, const std::string& description)
        : type("function") {
        function.name = name;
        function.description = description;
    }
};

// Represents a tool call in the response
struct ApiToolCall {
    std::string id;
    std::string type;  // Usually "function"
    
    struct Function {
        std::string name;
        std::string arguments;  // JSON string of arguments
    };
    
    Function function;
};

// Represents the API response
struct ApiResponse {
    bool success = false;
    std::string raw_json_response;
    std::string error_message;
    std::string content;  // The text content from the response if present
    std::vector<ApiToolCall> tool_calls;  // Any tool calls from the response
};

// Represents information about an OpenAI model
struct ApiModelInfo {
    std::string id;
    std::string object;
    std::string created;
    std::string owned_by;
    std::vector<std::string> permissions;
    std::string root;
    std::string parent;
};

// Represents a response from the list models endpoint
struct ApiModelListResponse {
    bool success;
    std::string error_message;
    std::vector<ApiModelInfo> models;
    std::string raw_json_response;
};

// Represents a request to create embeddings
struct ApiEmbeddingRequest {
    std::string input;
    std::string model;
    std::string user;  // Optional user identifier
};

// Represents a single embedding data point
struct ApiEmbeddingData {
    std::vector<float> embedding;
    int index;
    std::string object;
};

// Represents a response from the create embeddings endpoint
struct ApiEmbeddingResponse {
    bool success;
    std::string error_message;
    std::string raw_json_response;
    std::string model;
    std::string object;
    std::vector<ApiEmbeddingData> data;
    int usage_prompt_tokens;
    int usage_total_tokens;
};

} // namespace ai_editor

#endif // OPENAI_API_CLIENT_TYPES_H 