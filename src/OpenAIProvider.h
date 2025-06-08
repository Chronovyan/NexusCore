#ifndef OPENAI_PROVIDER_H
#define OPENAI_PROVIDER_H

#include "interfaces/IAIProvider.hpp"
#include "IOpenAI_API_Client.h"
#include "PromptTemplate.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace ai_editor {

/**
 * @class OpenAIProvider
 * @brief Implementation of IAIProvider for OpenAI models
 * 
 * This class adapts the existing OpenAI_API_Client to the new IAIProvider interface,
 * allowing it to work within the multi-model framework.
 */
class OpenAIProvider : public IAIProvider {
public:
    /**
     * @brief Constructor with an existing OpenAI API client
     * 
     * @param apiClient The OpenAI API client to use
     */
    explicit OpenAIProvider(std::shared_ptr<IOpenAI_API_Client> apiClient);
    
    /**
     * @brief Constructor that creates a new OpenAI API client
     * 
     * @param apiKey The OpenAI API key to use
     * @param organizationId Optional organization ID
     */
    explicit OpenAIProvider(const std::string& apiKey, const std::optional<std::string>& organizationId = std::nullopt);
    
    /**
     * @brief Initialize the provider
     * 
     * @param options Configuration options for the provider
     * @return bool True if initialization succeeded, false otherwise
     */
    bool initialize(const ProviderOptions& options) override;
    
    /**
     * @brief Check if the provider is initialized
     * 
     * @return bool True if initialized, false otherwise
     */
    bool isInitialized() const override;
    
    /**
     * @brief Get the provider name
     * 
     * @return std::string The name of the provider ("OpenAI")
     */
    std::string getProviderName() const override;
    
    /**
     * @brief List available models from OpenAI
     * 
     * @return std::vector<ModelInfo> List of available models
     */
    std::vector<ModelInfo> listAvailableModels() override;
    
    /**
     * @brief Get information about the current model
     * 
     * @return ModelInfo Information about the current model
     */
    ModelInfo getCurrentModelInfo() const override;
    
    /**
     * @brief Set the current model
     * 
     * @param modelId The ID of the model to use
     * @return bool True if successful, false otherwise
     */
    bool setCurrentModel(const std::string& modelId) override;
    
    /**
     * @brief Send a completion request to OpenAI
     * 
     * @param messages Vector of messages in the conversation
     * @param tools Optional vector of tool definitions the model can use
     * @return CompletionResponse containing the result or error
     */
    CompletionResponse sendCompletionRequest(
        const std::vector<Message>& messages,
        const std::vector<ToolDefinition>& tools = {}
    ) override;
    
    /**
     * @brief Generate embeddings for the provided input
     * 
     * @param input The text to generate embeddings for
     * @param modelId Optional specific model to use for embeddings
     * @return std::vector<float> The embedding vector or empty vector on error
     */
    std::vector<float> generateEmbedding(
        const std::string& input,
        const std::optional<std::string>& modelId = std::nullopt
    ) override;
    
    /**
     * @brief Get the provider options
     * 
     * @return ProviderOptions The current provider options
     */
    ProviderOptions getOptions() const override;
    
    /**
     * @brief Set provider options
     * 
     * @param options The options to set
     */
    void setOptions(const ProviderOptions& options) override;
    
    /**
     * @brief Check if a specific capability is supported
     * 
     * @param capability The capability to check (e.g., "tools", "embeddings")
     * @return bool True if supported, false otherwise
     */
    bool supportsCapability(const std::string& capability) const override;
    
    /**
     * @brief Get the current prompt template
     * 
     * @return std::shared_ptr<PromptTemplate> The current template, or nullptr if none
     */
    std::shared_ptr<PromptTemplate> getCurrentTemplate() const override;
    
    /**
     * @brief Set the current prompt template
     * 
     * @param templateId The ID of the template to use
     * @return bool True if successful, false otherwise
     */
    bool setCurrentTemplate(const std::string& templateId) override;
    
    /**
     * @brief Get available templates for the current model
     * 
     * @return std::vector<std::string> List of template IDs compatible with the current model
     */
    std::vector<std::string> getAvailableTemplates() const override;

private:
    /**
     * @brief Convert IAIProvider Message to OpenAI ApiChatMessage
     * 
     * @param message The Message to convert
     * @return ApiChatMessage The converted message
     */
    ApiChatMessage convertToApiChatMessage(const Message& message) const;
    
    /**
     * @brief Convert IAIProvider ToolDefinition to OpenAI ApiToolDefinition
     * 
     * @param toolDef The ToolDefinition to convert
     * @return ApiToolDefinition The converted tool definition
     */
    ApiToolDefinition convertToApiToolDefinition(const ToolDefinition& toolDef) const;
    
    /**
     * @brief Convert OpenAI ApiToolCall to IAIProvider ToolCall
     * 
     * @param apiToolCall The ApiToolCall to convert
     * @return ToolCall The converted tool call
     */
    ToolCall convertFromApiToolCall(const ApiToolCall& apiToolCall) const;
    
    /**
     * @brief Fetch and cache model capabilities
     */
    void fetchModelCapabilities();
    
    /**
     * @brief Parse model capabilities from API response
     * 
     * @param modelInfo The model info from the API
     * @return std::map<std::string, std::string> Map of capabilities
     */
    std::map<std::string, std::string> parseModelCapabilities(const ApiModelInfo& modelInfo) const;
    
    /**
     * @brief Select the best template for the current model
     * 
     * This will check available templates and choose the most appropriate one
     * for the current model.
     * 
     * @return bool True if a template was selected, false otherwise
     */
    bool selectBestTemplateForModel();

    // OpenAI API client
    std::shared_ptr<IOpenAI_API_Client> apiClient_;
    
    // Current model ID
    std::string currentModelId_;
    
    // Provider options
    ProviderOptions options_;
    
    // Cached model information
    mutable std::map<std::string, ModelInfo> modelInfoCache_;
    
    // Mutex for thread-safe operations
    mutable std::mutex mutex_;
    
    // Initialization flag
    bool initialized_;
    
    // Prompt template manager
    std::shared_ptr<PromptTemplateManager> templateManager_;
    
    // Current template
    std::shared_ptr<PromptTemplate> currentTemplate_;
};

/**
 * @brief Factory function for creating OpenAIProvider instances
 * 
 * @param options Provider configuration options
 * @return std::unique_ptr<IAIProvider> The created provider
 */
std::unique_ptr<IAIProvider> createOpenAIProvider(const ProviderOptions& options);

/**
 * @brief Register the OpenAIProvider with the AIProviderFactory
 * 
 * This function should be called during application initialization to register
 * the OpenAIProvider type with the factory.
 */
void registerOpenAIProvider();

} // namespace ai_editor

#endif // OPENAI_PROVIDER_H 