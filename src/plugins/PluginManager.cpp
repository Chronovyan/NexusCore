#include "PluginManager.hpp"
#include "../interfaces/plugins/PluginAPI.hpp"
#include "../AppDebugLog.h"
#include <filesystem>
#include <algorithm>
#include <stdexcept>

#ifdef _WIN32
    // Windows-specific includes
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
#else
    // Unix-specific includes
    #include <dlfcn.h>
    #include <dirent.h>
    #define PATH_SEPARATOR "/"
#endif

PluginManager::PluginManager(std::shared_ptr<IEditorServices> editorServices) 
    : editorServices_(std::move(editorServices)) {
    if (!editorServices_) {
        LOG_ERROR("PluginManager initialized with null IEditorServices");
        throw std::invalid_argument("IEditorServices cannot be null");
    }
    LOG_INFO("PluginManager initialized");
}

PluginManager::~PluginManager() {
    try {
        unloadAllPlugins();
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during PluginManager destruction: " + std::string(e.what()));
    }
}

size_t PluginManager::loadPlugins(const std::string& pluginsDirectory) {
    LOG_INFO("Searching for plugins in directory: " + pluginsDirectory);
    
    // Check if directory exists
    if (!std::filesystem::exists(pluginsDirectory) || !std::filesystem::is_directory(pluginsDirectory)) {
        LOG_ERROR("Plugin directory does not exist or is not a directory: " + pluginsDirectory);
        return 0;
    }
    
    size_t loadedCount = 0;
    
    try {
        // Iterate through all files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(pluginsDirectory)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
            // Check for platform-specific library extensions
            #ifdef _WIN32
            if (extension == ".dll") {
            #else
            if (extension == ".so" || extension == ".dylib") {
            #endif
                std::string pluginPath = entry.path().string();
                if (loadPlugin(pluginPath)) {
                    loadedCount++;
                    LOG_INFO("Successfully loaded plugin from: " + pluginPath);
                } else {
                    LOG_WARNING("Failed to load plugin from: " + pluginPath);
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while loading plugins: " + std::string(e.what()));
    }
    
    LOG_INFO("Loaded " + std::to_string(loadedCount) + " plugins from directory: " + pluginsDirectory);
    return loadedCount;
}

bool PluginManager::loadPlugin(const std::string& pluginPath) {
    LOG_INFO("Attempting to load plugin from: " + pluginPath);
    
    // Check if file exists
    if (!std::filesystem::exists(pluginPath) || !std::filesystem::is_regular_file(pluginPath)) {
        LOG_ERROR("Plugin file does not exist or is not a regular file: " + pluginPath);
        return false;
    }

    try {
        // Load the library
        LibraryHandle handle = loadLibrary(pluginPath);
        if (!handle.handle) {
            LOG_ERROR("Failed to load library: " + pluginPath);
            return false;
        }
        
        // Create the plugin instance
        auto plugin = createPluginInstance(handle);
        if (!plugin) {
            LOG_ERROR("Failed to create plugin instance from: " + pluginPath);
            unloadLibrary(handle);
            return false;
        }
        
        // Get plugin ID and check if it's already loaded
        std::string pluginId = plugin->getName();
        if (pluginId.empty()) {
            LOG_ERROR("Plugin has empty name: " + pluginPath);
            unloadLibrary(handle);
            return false;
        }
        
        if (isPluginLoaded(pluginId)) {
            LOG_WARNING("Plugin with ID '" + pluginId + "' is already loaded. Unloading duplicate from: " + pluginPath);
            unloadLibrary(handle);
            return false;
        }
        
        // Initialize the plugin
        LOG_INFO("Initializing plugin: " + pluginId);
        if (!plugin->initialize(editorServices_)) {
            LOG_ERROR("Failed to initialize plugin: " + pluginId);
            unloadLibrary(handle);
            return false;
        }
        
        // Store the plugin and library handle
        plugins_[pluginId] = plugin;
        libraryHandles_[pluginId] = handle;
        
        LOG_INFO("Successfully loaded and initialized plugin: " + pluginId + " (version: " + plugin->getVersion() + ")");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while loading plugin from " + pluginPath + ": " + std::string(e.what()));
        return false;
    }
}

bool PluginManager::unloadPlugin(const std::string& pluginId) {
    LOG_INFO("Attempting to unload plugin: " + pluginId);
    
    // Check if plugin exists
    auto pluginIt = plugins_.find(pluginId);
    if (pluginIt == plugins_.end()) {
        LOG_WARNING("Plugin not found for unloading: " + pluginId);
        return false;
    }
    
    try {
        // Get the plugin and handle
        auto plugin = pluginIt->second;
        auto handleIt = libraryHandles_.find(pluginId);
        
        if (handleIt == libraryHandles_.end()) {
            LOG_ERROR("Library handle not found for plugin: " + pluginId);
            return false;
        }
        
        // Shut down the plugin
        LOG_INFO("Shutting down plugin: " + pluginId);
        plugin->shutdown();
        
        // Remove the plugin from the map
        plugins_.erase(pluginIt);
        
        // Unload the library
        bool unloadResult = unloadLibrary(handleIt->second);
        libraryHandles_.erase(handleIt);
        
        if (!unloadResult) {
            LOG_WARNING("Failed to unload library for plugin: " + pluginId);
            return false;
        }
        
        LOG_INFO("Successfully unloaded plugin: " + pluginId);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while unloading plugin " + pluginId + ": " + std::string(e.what()));
        return false;
    }
}

size_t PluginManager::unloadAllPlugins() {
    LOG_INFO("Unloading all plugins");
    
    size_t unloadedCount = 0;
    // Create a copy of the plugin IDs to avoid iterator invalidation
    std::vector<std::string> pluginIds;
    for (const auto& pair : plugins_) {
        pluginIds.push_back(pair.first);
    }
    
    for (const auto& pluginId : pluginIds) {
        if (unloadPlugin(pluginId)) {
            unloadedCount++;
        }
    }
    
    LOG_INFO("Unloaded " + std::to_string(unloadedCount) + " plugins");
    return unloadedCount;
}

std::shared_ptr<IPlugin> PluginManager::getPlugin(const std::string& pluginId) const {
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<IPlugin>> PluginManager::getLoadedPlugins() const {
    std::vector<std::shared_ptr<IPlugin>> result;
    for (const auto& pair : plugins_) {
        result.push_back(pair.second);
    }
    return result;
}

bool PluginManager::isPluginLoaded(const std::string& pluginId) const {
    return plugins_.find(pluginId) != plugins_.end();
}

size_t PluginManager::getPluginCount() const {
    return plugins_.size();
}

// Platform-specific implementations

PluginManager::LibraryHandle PluginManager::loadLibrary(const std::string& libraryPath) {
    LibraryHandle handle;
    handle.path = libraryPath;
    
    LOG_DEBUG("Loading library: " + libraryPath);
    
#ifdef _WIN32
    // Windows implementation
    handle.handle = (void*)LoadLibraryA(libraryPath.c_str());
    if (!handle.handle) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to load library. Error code: " + std::to_string(error));
    }
#else
    // Unix implementation
    handle.handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!handle.handle) {
        const char* error = dlerror();
        LOG_ERROR("Failed to load library: " + std::string(error ? error : "Unknown error"));
    }
#endif
    
    return handle;
}

bool PluginManager::unloadLibrary(const LibraryHandle& handle) {
    if (!handle.handle) {
        LOG_WARNING("Attempted to unload null library handle");
        return false;
    }
    
    LOG_DEBUG("Unloading library: " + handle.path);
    
    bool result = false;
#ifdef _WIN32
    // Windows implementation
    result = FreeLibrary((HMODULE)handle.handle) != 0;
    if (!result) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to unload library. Error code: " + std::to_string(error));
    }
#else
    // Unix implementation
    result = dlclose(handle.handle) == 0;
    if (!result) {
        const char* error = dlerror();
        LOG_ERROR("Failed to unload library: " + std::string(error ? error : "Unknown error"));
    }
#endif
    
    return result;
}

std::shared_ptr<IPlugin> PluginManager::createPluginInstance(const LibraryHandle& handle) {
    if (!handle.handle) {
        LOG_ERROR("Attempted to create plugin instance from null library handle");
        return nullptr;
    }
    
    LOG_DEBUG("Creating plugin instance from library: " + handle.path);
    
    CreatePluginFunc createFunc = nullptr;
    
#ifdef _WIN32
    // Windows implementation
    createFunc = (CreatePluginFunc)GetProcAddress((HMODULE)handle.handle, PLUGIN_CREATE_FUNCTION_NAME);
    if (!createFunc) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to get plugin creation function. Error code: " + std::to_string(error));
        return nullptr;
    }
#else
    // Unix implementation
    createFunc = (CreatePluginFunc)dlsym(handle.handle, PLUGIN_CREATE_FUNCTION_NAME);
    if (!createFunc) {
        const char* error = dlerror();
        LOG_ERROR("Failed to get plugin creation function: " + std::string(error ? error : "Unknown error"));
        return nullptr;
    }
#endif
    
    try {
        // Call the creation function
        auto plugin = createFunc();
        if (!plugin) {
            LOG_ERROR("Plugin creation function returned null");
            return nullptr;
        }
        
        return plugin;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in plugin creation function: " + std::string(e.what()));
        return nullptr;
    }
} 