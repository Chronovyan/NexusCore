#ifndef LLAMA_PROVIDER_H
#define LLAMA_PROVIDER_H

#include "interfaces/IAIProvider.hpp"
#include "PromptTemplate.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <thread>
#include <atomic>

namespace ai_editor {

/**
 * @class LlamaProvider
 * @brief Implementation of IAIProvider for local LLama models
 * 
 * This class provides an implementation of IAIProvider for local LLama models,
 * allowing the application to use locally-running models instead of cloud APIs.
 */
class LlamaProvider : public IAIProvider {
public:
    /**
     * @brief Constructor
     * 
     * @param modelPath Path to the model file or directory
     */
    explicit LlamaProvider(const std::string& modelPath);
    
    /**
     * @brief Destructor
     */
    ~LlamaProvider();
    
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
     * @return std::string The name of the provider ("LLama")
     */
    std::string getProviderName() const override;
    
    /**
     * @brief List available models from the local LLama installation
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
     * @brief Send a completion request to the local LLama model
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
     * @brief Scan for available models in the model directory
     */
    void scanAvailableModels();
    
    /**
     * @brief Load a model
     * 
     * @param modelId The ID of the model to load
     * @return bool True if successful, false otherwise
     */
    bool loadModel(const std::string& modelId);
    
    /**
     * @brief Unload the current model
     */
    void unloadModel();
    
    /**
     * @brief Convert chat messages to a prompt string
     * 
     * @param messages The messages to convert
     * @return std::string The prompt string
     */
    std::string convertMessagesToPrompt(const std::vector<Message>& messages) const;
    
    /**
     * @brief Parse tool calls from the model output
     * 
     * @param output The model output to parse
     * @return std::vector<ToolCall> The parsed tool calls
     */
    std::vector<ToolCall> parseToolCallsFromOutput(const std::string& output) const;

    /**
     * @brief Generate a simulated response based on prompt content
     * 
     * @param prompt The prompt string
     * @param messages The original messages
     * @return std::string The generated response
     */
    std::string generateSimulatedResponse(const std::string& prompt, const std::vector<Message>& messages) const;
    
    /**
     * @brief Determine if a tool call should be generated based on prompt content
     * 
     * @param prompt The prompt string
     * @return bool True if a tool call should be generated
     */
    bool shouldGenerateToolCall(const std::string& prompt) const;
    
    /**
     * @brief Extract tool calls from response content
     * 
     * @param content The response content
     * @param tools Available tool definitions
     * @return std::pair<std::string, std::vector<ToolCall>> The modified content and extracted tool calls
     */
    std::pair<std::string, std::vector<ToolCall>> extractToolCalls(
        const std::string& content,
        const std::vector<ToolDefinition>& tools) const;
    
    /**
     * @brief Count tokens in a text string
     * 
     * @param text The text to count tokens in
     * @return int The approximate token count
     */
    int countTokens(const std::string& text) const;
    
    /**
     * @brief Select the best template for the current model
     * 
     * This will check available templates and choose the most appropriate one
     * for the current model.
     * 
     * @return bool True if a template was selected, false otherwise
     */
    bool selectBestTemplateForModel();

    // Path to the model directory
    std::string modelPath_;
    
    // Current model ID
    std::string currentModelId_;
    
    // Provider options
    ProviderOptions options_;
    
    // Cached model information
    std::map<std::string, ModelInfo> modelInfoCache_;
    
    // Model context (placeholder for the actual LLama model context)
    void* modelContext_;
    
    // Mutex for thread-safe operations
    mutable std::mutex mutex_;
    
    // Initialization flag
    bool initialized_;
    
    // Background worker thread for inference
    std::thread workerThread_;
    
    // Flag to signal worker thread to stop
    std::atomic<bool> stopWorker_;
    
    // Prompt template manager
    std::shared_ptr<PromptTemplateManager> templateManager_;
    
    // Current template
    std::shared_ptr<PromptTemplate> currentTemplate_;
};

/**
 * @brief Factory function for creating LlamaProvider instances
 * 
 * @param options Provider configuration options
 * @return std::unique_ptr<IAIProvider> The created provider
 */
std::unique_ptr<IAIProvider> createLlamaProvider(const ProviderOptions& options);

/**
 * @brief Register the LlamaProvider with the AIProviderFactory
 * 
 * This function should be called during application initialization to register
 * the LlamaProvider type with the factory.
 */
void registerLlamaProvider();

} // namespace ai_editor

#endif // LLAMA_PROVIDER_H 