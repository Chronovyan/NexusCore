#pragma once

#include "interfaces/IAIProvider.hpp"
#include "PromptTemplate.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <atomic>

namespace ai_editor {

/**
 * @class AIManager
 * @brief Manages multiple AI providers and provides a unified interface to them.
 * 
 * This class is responsible for creating, initializing, and managing AI providers.
 * It provides a way to register provider creators, create and initialize providers,
 * and set the active provider.
 */
class AIManager {
public:
    /**
     * @brief Callback function type for model change notifications.
     */
    using ModelChangeCallback = std::function<void(const ModelInfo&)>;
    
    /**
     * @brief Callback function type for provider change notifications.
     */
    using ProviderChangeCallback = std::function<void(const std::string&)>;
    
    /**
     * @brief Callback function type for template change notifications.
     */
    using TemplateChangeCallback = std::function<void(const std::string&)>;
    
    /**
     * @brief Function type for creating provider instances.
     */
    using ProviderCreatorFunc = std::function<std::shared_ptr<IAIProvider>(
        const std::map<std::string, std::string>&)>;
    
    /**
     * @brief Constructs a new AIManager instance.
     * 
     * Registers built-in provider types.
     */
    AIManager();
    
    /**
     * @brief Destroys the AIManager instance.
     * 
     * Cleans up all providers.
     */
    ~AIManager();
    
    /**
     * @brief Registers a provider type with a creator function.
     * 
     * @param type The provider type identifier.
     * @param creator Function to create provider instances.
     */
    void registerProvider(const std::string& type, ProviderCreatorFunc creator);
    
    /**
     * @brief Creates a provider of the specified type.
     * 
     * @param type The provider type identifier.
     * @param options Options to pass to the provider creator.
     * @return std::shared_ptr<IAIProvider> The created provider, or nullptr on error.
     */
    std::shared_ptr<IAIProvider> createProvider(
        const std::string& type,
        const std::map<std::string, std::string>& options);
    
    /**
     * @brief Initializes a provider of the specified type.
     * 
     * Creates the provider if it doesn't exist, then initializes it.
     * 
     * @param type The provider type identifier.
     * @param options Options to pass to the provider initializer.
     * @return true if initialization was successful, false otherwise.
     */
    bool initializeProvider(
        const std::string& type,
        const std::map<std::string, std::string>& options);
    
    /**
     * @brief Gets a comma-separated list of available provider types.
     * 
     * @return std::string The available provider types.
     */
    std::string getAvailableProviderTypes() const;
    
    /**
     * @brief Gets a list of available provider types.
     * 
     * @return std::vector<std::string> The available provider types.
     */
    std::vector<std::string> getAvailableProviderTypesList() const;
    
    /**
     * @brief Gets a list of initialized provider types.
     * 
     * @return std::vector<std::string> The initialized provider types.
     */
    std::vector<std::string> getInitializedProviderTypesList() const;
    
    /**
     * @brief Sets the active provider.
     * 
     * @param type The provider type identifier.
     * @return true if the provider was set as active, false otherwise.
     */
    bool setActiveProvider(const std::string& type);
    
    /**
     * @brief Gets the active provider.
     * 
     * @return std::shared_ptr<IAIProvider> The active provider, or nullptr if none.
     */
    std::shared_ptr<IAIProvider> getActiveProvider() const;
    
    /**
     * @brief Gets the active provider type.
     * 
     * @return std::string The active provider type, or empty string if none.
     */
    std::string getActiveProviderType() const;
    
    /**
     * @brief Adds a callback for provider change events.
     * 
     * @param callback The callback function.
     * @return int The callback ID.
     */
    void addProviderChangeCallback(ProviderChangeCallback callback);
    
    /**
     * @brief Adds a callback for model change events.
     * 
     * @param callback The callback function.
     * @return int The callback ID.
     */
    int addModelChangeCallback(ModelChangeCallback callback);
    
    /**
     * @brief Removes a model change callback.
     * 
     * @param callbackId The callback ID to remove.
     */
    void removeModelChangeCallback(int callbackId);
    
    /**
     * @brief Adds a callback for template change events.
     * 
     * @param callback The callback function.
     * @return int The callback ID.
     */
    int addTemplateChangeCallback(TemplateChangeCallback callback);
    
    /**
     * @brief Removes a template change callback.
     * 
     * @param callbackId The callback ID to remove.
     */
    void removeTemplateChangeCallback(int callbackId);
    
    /**
     * @brief Helper method to initialize a local LLama provider.
     * 
     * @param modelPath Path to the model file.
     * @return true if initialization was successful, false otherwise.
     */
    bool initializeLocalLlamaProvider(const std::string& modelPath);
    
    /**
     * @brief List available models from the active provider
     * 
     * @return std::vector<ModelInfo> List of available models
     */
    std::vector<ModelInfo> listAvailableModels();
    
    /**
     * @brief List available models from a specific provider
     * 
     * @param providerType The provider type to list models from
     * @return std::vector<ModelInfo> List of available models
     */
    std::vector<ModelInfo> listAvailableModels(const std::string& providerType);
    
    /**
     * @brief Get information about the current model
     * 
     * @return ModelInfo Information about the current model
     */
    ModelInfo getCurrentModelInfo() const;
    
    /**
     * @brief Set the current model
     * 
     * @param modelId The ID of the model to use
     * @return bool True if successful
     */
    bool setCurrentModel(const std::string& modelId);
    
    /**
     * @brief Set the current model for a specific provider
     * 
     * @param providerType The provider type
     * @param modelId The ID of the model to use
     * @return bool True if successful
     */
    bool setCurrentModel(const std::string& providerType, const std::string& modelId);
    
    /**
     * @brief Send a completion request to the active provider
     * 
     * @param messages Vector of messages in the conversation
     * @param tools Optional vector of tool definitions the model can use
     * @return CompletionResponse containing the result or error
     */
    CompletionResponse sendCompletionRequest(
        const std::vector<Message>& messages,
        const std::vector<ToolDefinition>& tools = {}
    );
    
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
    );
    
    /**
     * @brief Get the provider options for the active provider
     * 
     * @return ProviderOptions The current provider options
     */
    ProviderOptions getProviderOptions() const;
    
    /**
     * @brief Get the provider options for a specific provider
     * 
     * @param providerType The provider type
     * @return ProviderOptions The current provider options
     */
    ProviderOptions getProviderOptions(const std::string& providerType) const;
    
    /**
     * @brief Set provider options for the active provider
     * 
     * @param options The options to set
     * @return bool True if options were set successfully
     */
    bool setProviderOptions(const ProviderOptions& options);
    
    /**
     * @brief Set provider options for a specific provider
     * 
     * @param providerType The provider type
     * @param options The options to set
     * @return bool True if options were set successfully
     */
    bool setProviderOptions(const std::string& providerType, const ProviderOptions& options);
    
    /**
     * @brief Check if a specific capability is supported by the active provider
     * 
     * @param capability The capability to check (e.g., "tools", "embeddings")
     * @return bool True if supported, false otherwise
     */
    bool supportsCapability(const std::string& capability) const;
    
    /**
     * @brief Check if a specific capability is supported by a specific provider
     * 
     * @param providerType The provider type
     * @param capability The capability to check (e.g., "tools", "embeddings")
     * @return bool True if supported, false otherwise
     */
    bool supportsCapability(const std::string& providerType, const std::string& capability) const;
    
    /**
     * @brief Get the current prompt template for the active provider
     * 
     * @return std::shared_ptr<PromptTemplate> The current template, or nullptr if none
     */
    std::shared_ptr<PromptTemplate> getCurrentTemplate() const;
    
    /**
     * @brief Get the current prompt template for a specific provider
     * 
     * @param providerType The provider type
     * @return std::shared_ptr<PromptTemplate> The current template, or nullptr if none
     */
    std::shared_ptr<PromptTemplate> getCurrentTemplate(const std::string& providerType) const;
    
    /**
     * @brief Set the current prompt template for the active provider
     * 
     * @param templateId The ID of the template to use
     * @return bool True if successful, false otherwise
     */
    bool setCurrentTemplate(const std::string& templateId);
    
    /**
     * @brief Set the current prompt template for a specific provider
     * 
     * @param providerType The provider type
     * @param templateId The ID of the template to use
     * @return bool True if successful, false otherwise
     */
    bool setCurrentTemplate(const std::string& providerType, const std::string& templateId);
    
    /**
     * @brief Get available templates for the active provider's current model
     * 
     * @return std::vector<std::string> List of template IDs compatible with the current model
     */
    std::vector<std::string> getAvailableTemplates() const;
    
    /**
     * @brief Get available templates for a specific provider's current model
     * 
     * @param providerType The provider type
     * @return std::vector<std::string> List of template IDs compatible with the current model
     */
    std::vector<std::string> getAvailableTemplates(const std::string& providerType) const;
    
    /**
     * @brief Get template information by ID
     * 
     * @param templateId The template ID
     * @return PromptTemplateInfo Information about the template, or empty struct if not found
     */
    PromptTemplateInfo getTemplateInfo(const std::string& templateId) const;
    
    /**
     * @brief Get all available template information
     * 
     * @return std::vector<PromptTemplateInfo> Information about all templates
     */
    std::vector<PromptTemplateInfo> getAllTemplateInfo() const;
    
    /**
     * @brief Notify template change callbacks
     * 
     * @param templateId The new template ID
     */
    void notifyTemplateChange(const std::string& templateId);

private:
    /**
     * @brief Get a provider by type
     * 
     * @param providerType The provider type to get
     * @return IAIProvider* The provider or nullptr if not found
     */
    IAIProvider* getProvider(const std::string& providerType) const;
    
    /**
     * @brief Notify model change callbacks
     * 
     * @param modelInfo The new model information
     */
    void notifyModelChange(const ModelInfo& modelInfo);

    // Map of provider types to creator functions
    std::map<std::string, ProviderCreatorFunc> providerCreators_;
    
    // Map of provider types to provider instances
    std::map<std::string, std::shared_ptr<IAIProvider>> providers_;
    
    // The active provider
    std::shared_ptr<IAIProvider> activeProvider_;
    
    // Provider change callbacks
    std::vector<ProviderChangeCallback> providerChangeCallbacks_;
    
    // Model change callbacks
    std::map<int, ModelChangeCallback> modelChangeCallbacks_;
    
    // Template change callbacks
    std::map<int, TemplateChangeCallback> templateChangeCallbacks_;
    
    // Next callback ID
    std::atomic<int> nextCallbackId_;
    
    // Mutex for thread safety
    mutable std::mutex mutex_;
    
    // Shared prompt template manager
    std::shared_ptr<PromptTemplateManager> templateManager_;
};

} // namespace ai_editor 