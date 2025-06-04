#include "interfaces/IAIProvider.hpp"
#include "EditorErrorReporter.h"

#include <mutex>
#include <unordered_map>

namespace ai_editor {

// Map to store registered provider factory functions
static std::unordered_map<std::string, std::function<std::unique_ptr<IAIProvider>(const ProviderOptions&)>> providerFactories;

// Mutex for thread-safe operations on the factory map
static std::mutex factoryMutex;

std::unique_ptr<IAIProvider> AIProviderFactory::createProvider(
    const std::string& providerType,
    const ProviderOptions& options)
{
    std::lock_guard<std::mutex> lock(factoryMutex);
    
    // Convert provider type to lowercase for case-insensitive lookup
    std::string providerTypeLower = providerType;
    std::transform(providerTypeLower.begin(), providerTypeLower.end(), providerTypeLower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    auto it = providerFactories.find(providerTypeLower);
    if (it == providerFactories.end()) {
        EditorErrorReporter::reportError(
            "AIProviderFactory",
            "Unknown AI provider type: " + providerType,
            "Available provider types: " + [&]() {
                std::string types;
                for (const auto& [type, _] : providerFactories) {
                    if (!types.empty()) types += ", ";
                    types += type;
                }
                return types.empty() ? "none" : types;
            }()
        );
        return nullptr;
    }
    
    try {
        // Create the provider instance using the registered factory function
        auto provider = it->second(options);
        
        // Initialize the provider
        if (provider && !provider->initialize(options)) {
            EditorErrorReporter::reportError(
                "AIProviderFactory",
                "Failed to initialize " + providerType + " provider",
                "Check provider options and system configuration"
            );
            return nullptr;
        }
        
        return provider;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "AIProviderFactory",
            "Exception creating " + providerType + " provider: " + e.what(),
            "Check provider implementation and options"
        );
        return nullptr;
    } catch (...) {
        EditorErrorReporter::reportError(
            "AIProviderFactory",
            "Unknown exception creating " + providerType + " provider",
            "Check provider implementation and options"
        );
        return nullptr;
    }
}

void AIProviderFactory::registerProviderType(
    const std::string& providerType,
    std::function<std::unique_ptr<IAIProvider>(const ProviderOptions&)> factoryFn)
{
    if (!factoryFn) {
        EditorErrorReporter::reportError(
            "AIProviderFactory",
            "Cannot register null factory function for provider type: " + providerType,
            "Provide a valid factory function"
        );
        return;
    }
    
    std::lock_guard<std::mutex> lock(factoryMutex);
    
    // Convert provider type to lowercase for case-insensitive lookup
    std::string providerTypeLower = providerType;
    std::transform(providerTypeLower.begin(), providerTypeLower.end(), providerTypeLower.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check if this provider type is already registered
    if (providerFactories.find(providerTypeLower) != providerFactories.end()) {
        EditorErrorReporter::reportWarning(
            "AIProviderFactory",
            "Overriding existing registration for provider type: " + providerType,
            "This may cause unexpected behavior if the provider is already in use"
        );
    }
    
    // Register the factory function
    providerFactories[providerTypeLower] = std::move(factoryFn);
}

std::vector<std::string> AIProviderFactory::getRegisteredProviderTypes()
{
    std::lock_guard<std::mutex> lock(factoryMutex);
    
    std::vector<std::string> types;
    types.reserve(providerFactories.size());
    
    for (const auto& [type, _] : providerFactories) {
        types.push_back(type);
    }
    
    return types;
}

} // namespace ai_editor 