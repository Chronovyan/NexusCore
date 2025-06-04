#include "OpenAIProvider.h"
#include "OpenAI_API_Client.h"
#include "EditorErrorReporter.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace ai_editor {

// Constructor with existing OpenAI API client
OpenAIProvider::OpenAIProvider(std::shared_ptr<IOpenAI_API_Client> apiClient)
    : apiClient_(std::move(apiClient))
    , currentModelId_("gpt-4o")  // Default model
    , initialized_(false)
    , templateManager_(std::make_shared<PromptTemplateManager>())
    , currentTemplate_(nullptr)
{
    if (!apiClient_) {
        throw std::invalid_argument("OpenAIProvider: API client cannot be null");
    }
}

// Constructor that creates a new OpenAI API client
OpenAIProvider::OpenAIProvider(const std::string& apiKey, const std::optional<std::string>& organizationId)
    : currentModelId_("gpt-4o")  // Default model
    , initialized_(false)
    , templateManager_(std::make_shared<PromptTemplateManager>())
    , currentTemplate_(nullptr)
{
    try {
        // Create a new OpenAI API client with the provided key
        apiClient_ = std::make_shared<OpenAI_API_Client>(apiKey, organizationId.value_or(""));
    } catch (const std::exception& e) {
        throw std::runtime_error("OpenAIProvider: Failed to create API client: " + std::string(e.what()));
    }
}

bool OpenAIProvider::initialize(const ProviderOptions& options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        options_ = options;
        
        // Configure the API client based on provider options
        ApiRetryPolicy retryPolicy;
        retryPolicy.maxRetries = options.maxRetries;
        retryPolicy.initialBackoff = std::chrono::milliseconds(options.retryDelayMs);
        apiClient_->setRetryPolicy(retryPolicy);
        apiClient_->enableRetries(options.maxRetries > 0);
        
        // Set the default model if specified in options
        if (options.additionalOptions.count("model") > 0) {
            currentModelId_ = options.additionalOptions.at("model");
        }
        
        // Verify API client connection by listing models
        auto modelListResponse = apiClient_->listModels();
        if (!modelListResponse.success) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "Failed to initialize: " + modelListResponse.error,
                "Check API key and network connection"
            );
            return false;
        }
        
        // Cache model information
        fetchModelCapabilities();
        
        // Set template based on options or select best one for current model
        if (options.additionalOptions.count("templateId") > 0) {
            setCurrentTemplate(options.additionalOptions.at("templateId"));
        } else {
            selectBestTemplateForModel();
        }
        
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Exception during initialization: " + std::string(e.what()),
            "Check API client configuration"
        );
        initialized_ = false;
        return false;
    } catch (...) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Unknown exception during initialization",
            "Check API client configuration"
        );
        initialized_ = false;
        return false;
    }
}

bool OpenAIProvider::isInitialized() const
{
    return initialized_;
}

std::string OpenAIProvider::getProviderName() const
{
    return "OpenAI";
}

std::vector<ModelInfo> OpenAIProvider::listAvailableModels()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot list models: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    try {
        // Request the list of models from the API
        auto modelListResponse = apiClient_->listModels();
        if (!modelListResponse.success) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "Failed to list models: " + modelListResponse.error,
                "Check API connection and permissions"
            );
            return {};
        }
        
        // Convert API model info to ModelInfo structs
        std::vector<ModelInfo> models;
        models.reserve(modelListResponse.models.size());
        
        for (const auto& apiModel : modelListResponse.models) {
            // Skip non-chat models (focus on models suitable for completion)
            if (apiModel.id.find("gpt") == std::string::npos &&
                apiModel.id.find("text-") == std::string::npos) {
                continue;
            }
            
            // Get or create model info
            ModelInfo& modelInfo = modelInfoCache_[apiModel.id];
            
            // Update model info if not already cached
            if (modelInfo.id.empty()) {
                modelInfo.id = apiModel.id;
                modelInfo.name = apiModel.id; // Use ID as name if not available
                modelInfo.provider = "OpenAI";
                modelInfo.isLocal = false; // OpenAI models are never local
                
                // Retrieve detailed model info for capabilities
                auto detailedInfo = apiClient_->retrieveModel(apiModel.id);
                if (detailedInfo.success) {
                    modelInfo.version = detailedInfo.modelInfo.version;
                    modelInfo.contextWindowSize = detailedInfo.modelInfo.context_window;
                    modelInfo.capabilities = parseModelCapabilities(detailedInfo.modelInfo);
                    
                    // Store additional info
                    modelInfo.additionalInfo["owner"] = detailedInfo.modelInfo.owner;
                    modelInfo.additionalInfo["created"] = std::to_string(detailedInfo.modelInfo.created);
                }
            }
            
            models.push_back(modelInfo);
        }
        
        return models;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Exception listing models: " + std::string(e.what()),
            "Check API connection"
        );
        return {};
    }
}

ModelInfo OpenAIProvider::getCurrentModelInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot get current model info: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    // Check if we have cached info for the current model
    auto it = modelInfoCache_.find(currentModelId_);
    if (it != modelInfoCache_.end()) {
        return it->second;
    }
    
    // If not cached, get model info from the API
    try {
        auto modelInfo = apiClient_->retrieveModel(currentModelId_);
        if (!modelInfo.success) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "Failed to get current model info: " + modelInfo.error,
                "Check if the model ID is valid"
            );
            return {};
        }
        
        // Create and cache the model info
        ModelInfo info;
        info.id = currentModelId_;
        info.name = currentModelId_; // Use ID as name if not available
        info.provider = "OpenAI";
        info.version = modelInfo.modelInfo.version;
        info.isLocal = false;
        info.contextWindowSize = modelInfo.modelInfo.context_window;
        info.capabilities = parseModelCapabilities(modelInfo.modelInfo);
        
        // Store additional info
        info.additionalInfo["owner"] = modelInfo.modelInfo.owner;
        info.additionalInfo["created"] = std::to_string(modelInfo.modelInfo.created);
        
        modelInfoCache_[currentModelId_] = info;
        return info;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Exception getting current model info: " + std::string(e.what()),
            "Check API connection"
        );
        return {};
    }
}

bool OpenAIProvider::setCurrentModel(const std::string& modelId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot set model: Provider not initialized",
            "Call initialize() first"
        );
        return false;
    }
    
    try {
        // Verify the model exists by retrieving its info
        auto modelInfo = apiClient_->retrieveModel(modelId);
        if (!modelInfo.success) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "Failed to set model: " + modelInfo.error,
                "Check if the model ID is valid"
            );
            return false;
        }
        
        // Update the current model
        currentModelId_ = modelId;
        
        // Cache the model info if not already cached
        if (modelInfoCache_.find(modelId) == modelInfoCache_.end()) {
            ModelInfo info;
            info.id = modelId;
            info.name = modelId; // Use ID as name if not available
            info.provider = "OpenAI";
            info.version = modelInfo.modelInfo.version;
            info.isLocal = false;
            info.contextWindowSize = modelInfo.modelInfo.context_window;
            info.capabilities = parseModelCapabilities(modelInfo.modelInfo);
            
            // Store additional info
            info.additionalInfo["owner"] = modelInfo.modelInfo.owner;
            info.additionalInfo["created"] = std::to_string(modelInfo.modelInfo.created);
            
            modelInfoCache_[modelId] = info;
        }
        
        // Select the best template for the new model
        selectBestTemplateForModel();
        
        return true;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Exception setting model: " + std::string(e.what()),
            "Check API connection"
        );
        return false;
    }
}

CompletionResponse OpenAIProvider::sendCompletionRequest(
    const std::vector<Message>& messages,
    const std::vector<ToolDefinition>& tools)
{
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot send completion request: Provider not initialized",
            "Call initialize() first"
        );
        return {CompletionResponse::Status::API_ERROR, "", {}, "Provider not initialized", {}};
    }
    
    try {
        std::vector<ApiChatMessage> apiMessages;
        apiMessages.reserve(messages.size());
        
        // Apply message formatting if a template is available
        if (currentTemplate_) {
            for (const auto& message : messages) {
                ApiChatMessage apiMessage = convertToApiChatMessage(message);
                
                // Format the message content based on its role
                switch (message.role) {
                    case Message::Role::SYSTEM:
                        apiMessage.content = currentTemplate_->formatSystemMessage(message.content);
                        break;
                    case Message::Role::USER:
                        apiMessage.content = currentTemplate_->formatUserMessage(message.content);
                        break;
                    case Message::Role::ASSISTANT:
                        apiMessage.content = currentTemplate_->formatAssistantMessage(message.content);
                        break;
                    case Message::Role::TOOL:
                    case Message::Role::FUNCTION:
                        apiMessage.content = currentTemplate_->formatToolMessage(message.content);
                        break;
                    default:
                        // Use as is for unknown roles
                        break;
                }
                
                apiMessages.push_back(apiMessage);
            }
        } else {
            // No template available, use messages as-is
            for (const auto& message : messages) {
                apiMessages.push_back(convertToApiChatMessage(message));
            }
        }
        
        // Convert tools to API format
        std::vector<ApiToolDefinition> apiTools;
        apiTools.reserve(tools.size());
        for (const auto& tool : tools) {
            apiTools.push_back(convertToApiToolDefinition(tool));
        }
        
        // Send the request to the API
        auto response = apiClient_->sendChatCompletionRequest(
            apiMessages,
            apiTools,
            currentModelId_,
            options_.temperature,
            options_.maxTokens
        );
        
        // Convert the API response to CompletionResponse
        CompletionResponse result;
        
        if (response.success) {
            result.status = CompletionResponse::Status::SUCCESS;
            result.content = response.message.content;
            
            // Convert tool calls
            for (const auto& apiToolCall : response.message.tool_calls) {
                result.toolCalls.push_back(convertFromApiToolCall(apiToolCall));
            }
            
            // Add metadata
            result.metadata["model"] = response.model;
            result.metadata["usage.prompt_tokens"] = std::to_string(response.usage.prompt_tokens);
            result.metadata["usage.completion_tokens"] = std::to_string(response.usage.completion_tokens);
            result.metadata["usage.total_tokens"] = std::to_string(response.usage.total_tokens);
        } else {
            result.status = CompletionResponse::Status::API_ERROR;
            result.errorMessage = response.error;
            
            // Add error metadata if available
            if (!response.error_type.empty()) {
                result.metadata["error_type"] = response.error_type;
            }
            if (response.error_code != 0) {
                result.metadata["error_code"] = std::to_string(response.error_code);
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Exception sending completion request: " + std::string(e.what()),
            "Check API connection"
        );
        return {CompletionResponse::Status::API_ERROR, "", {}, "Exception: " + std::string(e.what()), {}};
    }
}

std::vector<float> OpenAIProvider::generateEmbedding(
    const std::string& input,
    const std::optional<std::string>& modelId)
{
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot generate embedding: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    try {
        // Prepare the embedding request
        ApiEmbeddingRequest request;
        request.input = input;
        request.model = modelId.value_or("text-embedding-3-small"); // Default to text-embedding-3-small if not specified
        
        // Send the request to the API
        auto response = apiClient_->createEmbedding(request);
        
        if (!response.success) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "Failed to generate embedding: " + response.error,
                "Check API connection and model ID"
            );
            return {};
        }
        
        // Extract the embedding data
        if (response.data.empty() || response.data[0].embedding.empty()) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "Empty embedding returned",
                "Check input text and model compatibility"
            );
            return {};
        }
        
        return response.data[0].embedding;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Exception generating embedding: " + std::string(e.what()),
            "Check API connection"
        );
        return {};
    }
}

ProviderOptions OpenAIProvider::getOptions() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

void OpenAIProvider::setOptions(const ProviderOptions& options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    options_ = options;
    
    // Apply relevant options to the API client
    ApiRetryPolicy retryPolicy;
    retryPolicy.maxRetries = options.maxRetries;
    retryPolicy.initialBackoff = std::chrono::milliseconds(options.retryDelayMs);
    apiClient_->setRetryPolicy(retryPolicy);
    apiClient_->enableRetries(options.maxRetries > 0);
    
    // Set the model if specified
    if (options.additionalOptions.count("model") > 0) {
        currentModelId_ = options.additionalOptions.at("model");
        selectBestTemplateForModel();
    }
    
    // Set template if specified
    if (options.additionalOptions.count("templateId") > 0) {
        setCurrentTemplate(options.additionalOptions.at("templateId"));
    }
}

bool OpenAIProvider::supportsCapability(const std::string& capability) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return false;
    }
    
    // Check if the current model is cached
    auto it = modelInfoCache_.find(currentModelId_);
    if (it == modelInfoCache_.end()) {
        return false;
    }
    
    // Check if the capability is supported
    const auto& capabilities = it->second.capabilities;
    auto capIt = capabilities.find(capability);
    if (capIt == capabilities.end()) {
        return false;
    }
    
    // Check if the capability is enabled
    return capIt->second == "yes" || 
           capIt->second == "true" || 
           capIt->second == "supported" || 
           capIt->second == "enabled";
}

// Helper methods for conversion between API and interface types

ApiChatMessage OpenAIProvider::convertToApiChatMessage(const Message& message) const
{
    ApiChatMessage apiMessage;
    
    // Convert role
    switch (message.role) {
        case Message::Role::SYSTEM:
            apiMessage.role = "system";
            break;
        case Message::Role::USER:
            apiMessage.role = "user";
            break;
        case Message::Role::ASSISTANT:
            apiMessage.role = "assistant";
            break;
        case Message::Role::TOOL:
            apiMessage.role = "tool";
            break;
        case Message::Role::FUNCTION:
            apiMessage.role = "function";
            break;
        default:
            apiMessage.role = "user"; // Default to user for unknown roles
            break;
    }
    
    // Set content and name
    apiMessage.content = message.content;
    if (message.name) {
        apiMessage.name = *message.name;
    }
    
    return apiMessage;
}

ApiToolDefinition OpenAIProvider::convertToApiToolDefinition(const ToolDefinition& toolDef) const
{
    ApiToolDefinition apiToolDef;
    apiToolDef.type = "function"; // OpenAI's API expects "function" for tool type
    apiToolDef.function.name = toolDef.name;
    apiToolDef.function.description = toolDef.description;
    apiToolDef.function.parameters = toolDef.schema;
    
    return apiToolDef;
}

ToolCall OpenAIProvider::convertFromApiToolCall(const ApiToolCall& apiToolCall) const
{
    ToolCall toolCall;
    toolCall.id = apiToolCall.id;
    toolCall.name = apiToolCall.function.name;
    toolCall.arguments = apiToolCall.function.arguments;
    
    return toolCall;
}

void OpenAIProvider::fetchModelCapabilities()
{
    // We'll populate this incrementally as we learn more about each model
    
    // First, get all available models
    auto modelListResponse = apiClient_->listModels();
    if (!modelListResponse.success) {
        return;
    }
    
    // For each relevant model, get detailed capabilities
    for (const auto& apiModel : modelListResponse.models) {
        // Skip non-chat models
        if (apiModel.id.find("gpt") == std::string::npos &&
            apiModel.id.find("text-") == std::string::npos) {
            continue;
        }
        
        auto detailedInfo = apiClient_->retrieveModel(apiModel.id);
        if (!detailedInfo.success) {
            continue;
        }
        
        // Create model info with capabilities
        ModelInfo info;
        info.id = apiModel.id;
        info.name = apiModel.id; // Use ID as name if not available
        info.provider = "OpenAI";
        info.version = detailedInfo.modelInfo.version;
        info.isLocal = false;
        info.contextWindowSize = detailedInfo.modelInfo.context_window;
        info.capabilities = parseModelCapabilities(detailedInfo.modelInfo);
        
        // Store additional info
        info.additionalInfo["owner"] = detailedInfo.modelInfo.owner;
        info.additionalInfo["created"] = std::to_string(detailedInfo.modelInfo.created);
        
        // Cache the model info
        modelInfoCache_[apiModel.id] = info;
    }
}

std::map<std::string, std::string> OpenAIProvider::parseModelCapabilities(const ApiModelInfo& modelInfo) const
{
    std::map<std::string, std::string> capabilities;
    
    // Set basic capabilities based on model ID and properties
    
    // All OpenAI models support basic text completion
    capabilities["text_completion"] = "yes";
    
    // Function/tool calling support
    if (modelInfo.id.find("gpt-4") != std::string::npos ||
        modelInfo.id.find("gpt-3.5-turbo") != std::string::npos) {
        capabilities["tools"] = "yes";
        capabilities["function_calling"] = "yes";
    } else {
        capabilities["tools"] = "no";
        capabilities["function_calling"] = "no";
    }
    
    // Embedding support
    if (modelInfo.id.find("text-embedding") != std::string::npos) {
        capabilities["embeddings"] = "yes";
    } else {
        capabilities["embeddings"] = "no";
    }
    
    // Vision capabilities
    if (modelInfo.id.find("vision") != std::string::npos ||
        modelInfo.id.find("gpt-4o") != std::string::npos ||
        modelInfo.id.find("gpt-4-turbo") != std::string::npos) {
        capabilities["vision"] = "yes";
    } else {
        capabilities["vision"] = "no";
    }
    
    return capabilities;
}

// Prompt template methods

std::shared_ptr<PromptTemplate> OpenAIProvider::getCurrentTemplate() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return currentTemplate_;
}

bool OpenAIProvider::setCurrentTemplate(const std::string& templateId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot set template: Provider not initialized",
            "Call initialize() first"
        );
        return false;
    }
    
    // Try to get the specified template
    auto template_ = templateManager_->getTemplate(templateId);
    if (!template_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Template not found: " + templateId,
            "Check if the template ID is valid"
        );
        return false;
    }
    
    // Check if the template is compatible with the current model
    if (!template_->isCompatibleWithModel(currentModelId_)) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Template is not compatible with the current model: " + currentModelId_,
            "Select a different template or model"
        );
        return false;
    }
    
    // Check if the template is for the correct provider
    if (!template_->isForProvider("OpenAI")) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Template is not for OpenAI provider: " + templateId,
            "Select a template for OpenAI"
        );
        return false;
    }
    
    // Set the current template
    currentTemplate_ = template_;
    return true;
}

std::vector<std::string> OpenAIProvider::getAvailableTemplates() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot get available templates: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    // Get all templates for this provider
    auto allTemplates = templateManager_->getTemplatesForProvider("OpenAI");
    
    // Filter templates compatible with the current model
    std::vector<std::string> compatibleTemplateIds;
    for (const auto& template_ : allTemplates) {
        if (template_->isCompatibleWithModel(currentModelId_)) {
            compatibleTemplateIds.push_back(template_->getId());
        }
    }
    
    return compatibleTemplateIds;
}

bool OpenAIProvider::selectBestTemplateForModel()
{
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Cannot select template: Provider not initialized",
            "Call initialize() first"
        );
        return false;
    }
    
    // Find the best template for the current model
    auto bestTemplate = templateManager_->findTemplateForModel(currentModelId_, "OpenAI");
    if (!bestTemplate) {
        // If no specific template is found, use the default template for OpenAI
        bestTemplate = templateManager_->getDefaultTemplateForProvider("OpenAI");
        if (!bestTemplate) {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "No default template available for OpenAI",
                "Create a default template or specify one explicitly"
            );
            currentTemplate_ = nullptr;
            return false;
        }
    }
    
    currentTemplate_ = bestTemplate;
    return true;
}

// Factory function for creating OpenAIProvider instances
std::unique_ptr<IAIProvider> createOpenAIProvider(const ProviderOptions& options)
{
    // Extract API key and organization ID from options
    std::string apiKey;
    std::optional<std::string> orgId;
    
    if (options.additionalOptions.count("api_key") > 0) {
        apiKey = options.additionalOptions.at("api_key");
    } else {
        // Try to get from environment variable
        const char* envApiKey = std::getenv("OPENAI_API_KEY");
        if (envApiKey) {
            apiKey = envApiKey;
        } else {
            EditorErrorReporter::reportError(
                "OpenAIProvider",
                "No API key provided",
                "Set 'api_key' in additionalOptions or OPENAI_API_KEY environment variable"
            );
            return nullptr;
        }
    }
    
    if (options.additionalOptions.count("organization_id") > 0) {
        orgId = options.additionalOptions.at("organization_id");
    }
    
    try {
        // Create and initialize the provider
        auto provider = std::make_unique<OpenAIProvider>(apiKey, orgId);
        
        if (!provider->initialize(options)) {
            return nullptr;
        }
        
        return provider;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "OpenAIProvider",
            "Failed to create provider: " + std::string(e.what()),
            "Check API key and options"
        );
        return nullptr;
    }
}

// Register the OpenAIProvider with the AIProviderFactory
void registerOpenAIProvider()
{
    AIProviderFactory::registerProviderType("openai", createOpenAIProvider);
}

} // namespace ai_editor 