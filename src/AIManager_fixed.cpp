#include "AIManager.h"
#include "EditorErrorReporter.h"
#include "OpenAIProvider.h"
#include "LlamaProvider.h"

#include <algorithm>
#include <stdexcept>
#include <filesystem>

namespace ai_editor {

AIManager::AIManager()
    : nextCallbackId_(0), activeProvider_(nullptr),
      templateManager_(std::make_shared<PromptTemplateManager>())
{
    // Register built-in providers
    registerProvider("openai", [this](const std::map<std::string, std::string>& options) {
        ProviderOptions providerOpts;
        for (const auto& [key, value] : options) {
            providerOpts.additionalOptions[key] = value;
        }
        return std::make_shared<OpenAIProvider>(providerOpts);
    });
    
    registerProvider("llama", [this](const std::map<std::string, std::string>& options) {
        ProviderOptions providerOpts;
        for (const auto& [key, value] : options) {
            providerOpts.additionalOptions[key] = value;
        }
        return std::make_shared<LlamaProvider>(providerOpts);
    });
}

AIManager::~AIManager()
{
    // Clean up providers
    for (auto& [_, provider] : providers_) {
        try {
            if (provider) {
                provider->shutdown();
            }
        } catch (const std::exception& e) {
            EditorErrorReporter::reportError(
                "AIManager",
                "Exception during provider cleanup: " + std::string(e.what()) + ". Ignoring and continuing shutdown"
            );
        }
    }
    
    providers_.clear();
    activeProvider_ = nullptr;
}

void AIManager::registerProvider(const std::string& type, ProviderCreatorFunc creator)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Convert provider type to lowercase for case-insensitive comparison
        std::string providerTypeLower = type;
        std::transform(providerTypeLower.begin(), providerTypeLower.end(), providerTypeLower.begin(), 
                       [](unsigned char c) { return std::tolower(c); });
        
        // Store the creator function
        providerCreators_[providerTypeLower] = creator;
        
        // Log the registration
        std::cout << "Registered provider: " << providerTypeLower << std::endl;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Failed to register provider " + type + ": " + e.what()
        );
    }
}

std::string AIManager::getAvailableProviderTypes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string result;
    bool first = true;
    
    for (const auto& [type, _] : providerCreators_) {
        if (!first) {
            result += ", ";
        } else {
            first = false;
        }
        result += type;
    }
    
    return result;
}

bool AIManager::setActiveProvider(const std::string& providerType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Convert provider type to lowercase for case-insensitive comparison
    std::string providerTypeLower = providerType;
    std::transform(providerTypeLower.begin(), providerTypeLower.end(), providerTypeLower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check if the provider is registered
    if (providers_.find(providerTypeLower) == providers_.end()) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Provider not registered: " + providerType + " - Register the provider first"
        );
        return false;
    }
    
    // Set the active provider
    activeProviderType_ = providerTypeLower;
    activeProvider_ = providers_[providerTypeLower];
    
    // Notify listeners
    notifyProviderChange(providerTypeLower);
    
    return true;
}

std::shared_ptr<IAIProvider> AIManager::getActiveProvider() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return activeProvider_;
}

std::string AIManager::getActiveProviderType() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return activeProviderType_;
}

bool AIManager::isProviderInitialized(const std::string& providerType) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string providerTypeLower = providerType;
    std::transform(providerTypeLower.begin(), providerTypeLower.end(), providerTypeLower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    auto it = providers_.find(providerTypeLower);
    return it != providers_.end() && it->second != nullptr;
}

std::shared_ptr<IAIProvider> AIManager::getProvider(const std::string& providerType) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string providerTypeLower = providerType;
    std::transform(providerTypeLower.begin(), providerTypeLower.end(), providerTypeLower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    auto it = providers_.find(providerTypeLower);
    if (it == providers_.end()) {
        return nullptr;
    }
    
    return it->second;
}

std::vector<ModelInfo> AIManager::listAvailableModels() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "No active provider. Set an active provider first"
        );
        return {};
    }
    
    return provider->listAvailableModels();
}

std::vector<ModelInfo> AIManager::listAvailableModels(const std::string& providerType) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getProvider(providerType);
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Provider not registered: " + providerType + ". Register the provider first"
        );
        return {};
    }
    
    return provider->listAvailableModels();
}

ModelInfo AIManager::getCurrentModelInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "No active provider. Set an active provider first"
        );
        return {};
    }
    
    return provider->getCurrentModelInfo();
}

bool AIManager::setCurrentModel(const std::string& modelId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "No active provider. Set an active provider first"
        );
        return false;
    }
    
    bool result = provider->setCurrentModel(modelId);
    if (result) {
        ModelInfo modelInfo;
        modelInfo.modelId = modelId;
        modelInfo.providerType = activeProviderType_;
        notifyModelChange(modelInfo);
    }
    
    return result;
}

std::vector<float> AIManager::generateEmbedding(const std::string& text, const std::optional<std::string>& modelId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "No active provider. Set an active provider first"
        );
        return {};
    }
    
    return provider->generateEmbedding(text, modelId);
}

ProviderOptions AIManager::getProviderOptions() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "No active provider. Set an active provider first"
        );
        return {};
    }
    
    return provider->getOptions();
}

ProviderOptions AIManager::getProviderOptions(const std::string& providerType) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getProvider(providerType);
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Provider not registered: " + providerType + ". Register the provider first"
        );
        return {};
    }
    
    return provider->getOptions();
}

bool AIManager::setProviderOptions(const std::string& providerType, const ProviderOptions& options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getProvider(providerType);
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Provider not registered: " + providerType + ". Register the provider first"
        );
        return false;
    }
    
    return provider->setOptions(options);
}

std::shared_ptr<PromptTemplateManager> AIManager::getTemplateManager() const
{
    return templateManager_;
}

std::vector<PromptTemplateInfo> AIManager::getAvailableTemplates() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "No active provider. Set an active provider first"
        );
        return {};
    }
    
    return provider->getAvailableTemplates();
}

std::vector<PromptTemplateInfo> AIManager::getAvailableTemplates(const std::string& providerType) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getProvider(providerType);
    if (!provider) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Provider not registered: " + providerType + ". Register the provider first"
        );
        return {};
    }
    
    return provider->getAvailableTemplates();
}

PromptTemplateInfo AIManager::getTemplateInfo(const std::string& templateId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto template_ = templateManager_->getTemplate(templateId);
    if (!template_) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Template not found: " + templateId + ". Check if the template ID is valid"
        );
        return {};
    }
    
    return *template_;
}

bool AIManager::registerTemplate(const PromptTemplateInfo& templateInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        return templateManager_->registerTemplate(templateInfo);
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Failed to register template: " + std::string(e.what())
        );
        return false;
    }
}

bool AIManager::unregisterTemplate(const std::string& templateId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        return templateManager_->unregisterTemplate(templateId);
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "AIManager",
            "Failed to unregister template: " + std::string(e.what())
        );
        return false;
    }
}

int AIManager::addProviderChangeCallback(ProviderChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!callback) {
        return -1; // Invalid callback ID
    }
    
    // Store the callback and return its index as the ID
    int callbackId = static_cast<int>(providerChangeCallbacks_.size());
    providerChangeCallbacks_.push_back(callback);
    return callbackId;
}

void AIManager::removeProviderChangeCallback(int callbackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Since we're using a vector, we need to find the callback with the given ID
    // Note: This is a simplified implementation since we don't store IDs in the vector
    // In a real implementation, you might want to store the callbacks with their IDs
    if (callbackId >= 0 && callbackId < static_cast<int>(providerChangeCallbacks_.size())) {
        providerChangeCallbacks_.erase(providerChangeCallbacks_.begin() + callbackId);
    }
}

int AIManager::addModelChangeCallback(ModelChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!callback) {
        return -1; // Invalid callback ID
    }
    
    int callbackId = nextCallbackId_++;
    modelChangeCallbacks_[callbackId] = callback;
    return callbackId;
}

void AIManager::removeModelChangeCallback(int callbackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    modelChangeCallbacks_.erase(callbackId);
}

int AIManager::addTemplateChangeCallback(TemplateChangeCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!callback) {
        return -1; // Invalid callback ID
    }
    
    int callbackId = nextCallbackId_++;
    templateChangeCallbacks_[callbackId] = callback;
    return callbackId;
}

void AIManager::removeTemplateChangeCallback(int callbackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    templateChangeCallbacks_.erase(callbackId);
}

void AIManager::notifyProviderChange(const std::string& providerType)
{
    std::vector<ProviderChangeCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = providerChangeCallbacks_;
    }
    
    for (const auto& callback : callbacks) {
        try {
            if (callback) {
                callback(providerType);
            }
        } catch (const std::exception& e) {
            EditorErrorReporter::reportError(
                "AIManager",
                "Exception in provider change callback: " + std::string(e.what())
            );
        }
    }
}

void AIManager::notifyModelChange(const ModelInfo& modelInfo)
{
    std::map<int, ModelChangeCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = modelChangeCallbacks_;
    }
    
    for (const auto& [_, callback] : callbacks) {
        try {
            if (callback) {
                callback(modelInfo);
            }
        } catch (const std::exception& e) {
            EditorErrorReporter::reportError(
                "AIManager",
                "Exception in model change callback: " + std::string(e.what())
            );
        }
    }
}

void AIManager::notifyTemplateChange(const std::string& templateId, bool added)
{
    std::map<int, TemplateChangeCallback> callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = templateChangeCallbacks_;
    }
    
    for (const auto& [_, callback] : callbacks) {
        try {
            if (callback) {
                callback(templateId, added);
            }
        } catch (const std::exception& e) {
            EditorErrorReporter::reportError(
                "AIManager",
                "Exception in template change callback: " + std::string(e.what())
            );
        }
    }
}

bool AIManager::supportsCapability(AICapability capability) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto provider = getActiveProvider();
    if (!provider) {
        return false;
    }
    
    return provider->supportsCapability(capability);
}

} // namespace ai_editor
