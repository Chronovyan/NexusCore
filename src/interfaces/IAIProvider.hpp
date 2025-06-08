#ifndef IAIPROVIDER_HPP
#define IAIPROVIDER_HPP

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include <functional>

namespace ai_editor {

// Forward declaration for circular reference
class PromptTemplate;

/**
 * @struct ModelInfo
 * @brief Information about an AI model
 */
struct ModelInfo {
    std::string id;                 // Unique identifier for the model
    std::string name;               // Human-readable name
    std::string provider;           // Provider name (e.g., "OpenAI", "LLama")
    std::string version;            // Model version
    std::map<std::string, std::string> capabilities; // Map of capabilities and their support level
    bool isLocal;                   // Whether the model runs locally or via API
    size_t contextWindowSize;       // Maximum context window size in tokens
    std::map<std::string, std::string> additionalInfo; // Additional provider-specific information
};

/**
 * @struct Message
 * @brief Represents a message in a conversation
 */
struct Message {
    enum class Role {
        SYSTEM,
        USER,
        ASSISTANT,
        TOOL,
        FUNCTION  // For backward compatibility
    };
    
    Role role;
    std::string content;
    std::optional<std::string> name;  // Optional name for function/tool roles
    
    // Constructor for standard messages
    Message(Role role, const std::string& content)
        : role(role), content(content), name(std::nullopt) {}
    
    // Constructor for function/tool messages
    Message(Role role, const std::string& content, const std::string& name)
        : role(role), content(content), name(name) {}
};

/**
 * @struct ToolDefinition
 * @brief Defines a tool that can be used by the AI model
 */
struct ToolDefinition {
    std::string name;
    std::string description;
    std::string schema;  // JSON schema for the tool parameters
};

/**
 * @struct ToolCall
 * @brief Represents a tool call made by the AI
 */
struct ToolCall {
    std::string id;
    std::string name;
    std::string arguments;  // JSON string of arguments
};

/**
 * @struct CompletionResponse
 * @brief Response from a completion request
 */
struct CompletionResponse {
    enum class Status {
        SUCCESS,
        API_ERROR
    };
    
    Status status;
    std::string content;  // Text response if successful
    std::vector<ToolCall> toolCalls;  // Tool calls if any
    std::string errorMessage;  // Error message if status is ERROR
    std::map<std::string, std::string> metadata;  // Additional response metadata
};

/**
 * @struct ProviderOptions
 * @brief Options for configuring the AI provider
 */
struct ProviderOptions {
    // Network settings
    int timeoutSeconds = 30;
    int maxRetries = 3;
    int retryDelayMs = 1000;
    
    // Model settings
    float temperature = 0.7f;
    int maxTokens = 2000;
    
    // Template settings
    std::string templateId; // ID of the prompt template to use
    
    // Provider-specific settings
    std::map<std::string, std::string> additionalOptions;
};

/**
 * @class IAIProvider
 * @brief Interface for AI provider implementations
 * 
 * This interface defines the contract for classes that provide AI functionality,
 * allowing the application to work with different AI models interchangeably.
 */
class IAIProvider {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IAIProvider() = default;
    
    /**
     * @brief Initialize the provider
     * 
     * @param options Configuration options for the provider
     * @return bool True if initialization succeeded, false otherwise
     */
    virtual bool initialize(const ProviderOptions& options) = 0;
    
    /**
     * @brief Check if the provider is initialized
     * 
     * @return bool True if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief Get the provider name
     * 
     * @return std::string The name of the provider (e.g., "OpenAI", "LLama")
     */
    virtual std::string getProviderName() const = 0;
    
    /**
     * @brief List available models from this provider
     * 
     * @return std::vector<ModelInfo> List of available models
     */
    virtual std::vector<ModelInfo> listAvailableModels() = 0;
    
    /**
     * @brief Get information about the current model
     * 
     * @return ModelInfo Information about the current model
     */
    virtual ModelInfo getCurrentModelInfo() const = 0;
    
    /**
     * @brief Set the current model
     * 
     * @param modelId The ID of the model to use
     * @return bool True if successful, false otherwise
     */
    virtual bool setCurrentModel(const std::string& modelId) = 0;
    
    /**
     * @brief Send a completion request to the AI
     * 
     * @param messages Vector of messages in the conversation
     * @param tools Optional vector of tool definitions the model can use
     * @return CompletionResponse containing the result or error
     */
    virtual CompletionResponse sendCompletionRequest(
        const std::vector<Message>& messages,
        const std::vector<ToolDefinition>& tools = {}
    ) = 0;
    
    /**
     * @brief Generate embeddings for the provided input
     * 
     * @param input The text to generate embeddings for
     * @param modelId Optional specific model to use for embeddings
     * @return std::vector<float> The embedding vector or empty vector on error
     */
    virtual std::vector<float> generateEmbedding(
        const std::string& input,
        const std::optional<std::string>& modelId = std::nullopt
    ) = 0;
    
    /**
     * @brief Get the provider options
     * 
     * @return ProviderOptions The current provider options
     */
    virtual ProviderOptions getOptions() const = 0;
    
    /**
     * @brief Set provider options
     * 
     * @param options The options to set
     */
    virtual void setOptions(const ProviderOptions& options) = 0;
    
    /**
     * @brief Check if a specific capability is supported
     * 
     * @param capability The capability to check (e.g., "tools", "embeddings")
     * @return bool True if supported, false otherwise
     */
    virtual bool supportsCapability(const std::string& capability) const = 0;
    
    /**
     * @brief Get the current prompt template
     * 
     * @return std::shared_ptr<PromptTemplate> The current template, or nullptr if none
     */
    virtual std::shared_ptr<PromptTemplate> getCurrentTemplate() const = 0;
    
    /**
     * @brief Set the current prompt template
     * 
     * @param templateId The ID of the template to use
     * @return bool True if successful, false otherwise
     */
    virtual bool setCurrentTemplate(const std::string& templateId) = 0;
    
    /**
     * @brief Get available templates for the current model
     * 
     * @return std::vector<std::string> List of template IDs compatible with the current model
     */
    virtual std::vector<std::string> getAvailableTemplates() const = 0;
};

/**
 * @class AIProviderFactory
 * @brief Factory for creating AI provider instances
 */
class AIProviderFactory {
public:
    /**
     * @brief Create an AI provider instance
     * 
     * @param providerType The type of provider to create (e.g., "openai", "llama")
     * @param options Provider configuration options
     * @return std::unique_ptr<IAIProvider> The created provider or nullptr on error
     */
    static std::unique_ptr<IAIProvider> createProvider(
        const std::string& providerType,
        const ProviderOptions& options = {}
    );
    
    /**
     * @brief Register a provider type with a factory function
     * 
     * @param providerType The type of provider to register
     * @param factoryFn The factory function to create instances of this provider
     */
    static void registerProviderType(
        const std::string& providerType,
        std::function<std::unique_ptr<IAIProvider>(const ProviderOptions&)> factoryFn
    );
    
    /**
     * @brief Get a list of registered provider types
     * 
     * @return std::vector<std::string> List of registered provider types
     */
    static std::vector<std::string> getRegisteredProviderTypes();
};

} // namespace ai_editor

#endif // IAIPROVIDER_HPP 