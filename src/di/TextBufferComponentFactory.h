#pragma once

#include "di/Injector.h"
#include "di/TextBufferFactory.h"
#include "interfaces/ITextBuffer.hpp"
#include "TextBufferConfig.h"
#include <memory>

/**
 * @brief Component factory for text buffers in the DI system
 * 
 * This class provides factory methods to register different text buffer implementations
 * with the dependency injection system. It uses TextBufferFactory to create the appropriate
 * buffer based on configuration or runtime conditions.
 */
class TextBufferComponentFactory {
public:
    /**
     * @brief Register text buffer components with the DI system
     * 
     * This method registers the following:
     * 1. TextBufferConfig singleton for configuration
     * 2. Default text buffer provider based on configuration and file size
     * 3. Basic text buffer provider (non-thread-safe)
     * 4. Thread-safe text buffer provider
     * 5. Virtualized text buffer provider (for large files)
     * 6. Thread-safe virtualized text buffer provider
     * 
     * @param injector The DI injector to register components with
     */
    static void registerComponents(di::Injector& injector) {
        // Register the configuration as a singleton
        injector.registerSingleton<TextBufferConfig>();
        
        // Register the default text buffer provider (smart detection based on file size)
        injector.registerFactory<ITextBuffer>([](const di::Injector& inj) {
            // Get configuration from the injector
            auto config = inj.get<TextBufferConfig>();
            return TextBufferFactory::createDefaultTextBuffer("", *config);
        }, di::Lifetime::Transient);

        // Register named providers for specific buffer types
        
        // Basic text buffer (non-thread-safe)
        injector.registerFactory<ITextBuffer>("basic", [](const di::Injector& inj) {
            return TextBufferFactory::createBasicTextBuffer();
        }, di::Lifetime::Transient);
        
        // Thread-safe text buffer
        injector.registerFactory<ITextBuffer>("thread_safe", [](const di::Injector& inj) {
            return TextBufferFactory::createThreadSafeTextBuffer();
        }, di::Lifetime::Transient);
        
        // Virtualized text buffer for large files
        injector.registerFactory<ITextBuffer>("virtualized", [](const di::Injector& inj) {
            auto config = inj.get<TextBufferConfig>();
            return TextBufferFactory::createVirtualizedTextBuffer(
                "", 
                config->defaultPageSize, 
                config->defaultCacheSize);
        }, di::Lifetime::Transient);
        
        // Thread-safe virtualized text buffer
        injector.registerFactory<ITextBuffer>("virtualized_thread_safe", [](const di::Injector& inj) {
            auto config = inj.get<TextBufferConfig>();
            return TextBufferFactory::createThreadSafeVirtualizedTextBuffer(
                "", 
                config->defaultPageSize, 
                config->defaultCacheSize);
        }, di::Lifetime::Transient);
    }
    
    /**
     * @brief Register a text buffer with a specific file
     * 
     * This method can be used to register a text buffer with a specific file.
     * It will use TextBufferFactory to create the appropriate buffer based on file size.
     * 
     * @param injector The DI injector to register the component with
     * @param name The name to register the text buffer under
     * @param filename The path to the file to load
     */
    static void registerFileBuffer(di::Injector& injector, const std::string& name, const std::string& filename) {
        injector.registerFactory<ITextBuffer>(name, [filename](const di::Injector& inj) {
            auto config = inj.get<TextBufferConfig>();
            return TextBufferFactory::createDefaultTextBuffer(filename, *config);
        }, di::Lifetime::Transient);
    }
    
    /**
     * @brief Register a virtualized text buffer with custom parameters
     * 
     * This method can be used to register a virtualized text buffer with custom
     * page size and cache size parameters.
     * 
     * @param injector The DI injector to register the component with
     * @param name The name to register the text buffer under
     * @param filename The path to the file to load (optional)
     * @param pageSize The page size in lines (0 = use config default)
     * @param cacheSize The cache size in pages (0 = use config default)
     * @param threadSafe Whether to use a thread-safe buffer
     */
    static void registerCustomVirtualizedBuffer(
        di::Injector& injector, 
        const std::string& name, 
        const std::string& filename,
        size_t pageSize = 0,
        size_t cacheSize = 0,
        bool threadSafe = true) {
        
        injector.registerFactory<ITextBuffer>(name, [=](const di::Injector& inj) {
            auto config = inj.get<TextBufferConfig>();
            
            // Use config defaults if parameters are 0
            size_t actualPageSize = (pageSize > 0) ? pageSize : config->defaultPageSize;
            size_t actualCacheSize = (cacheSize > 0) ? cacheSize : config->defaultCacheSize;
            
            if (threadSafe) {
                return TextBufferFactory::createThreadSafeVirtualizedTextBuffer(
                    filename, actualPageSize, actualCacheSize);
            } else {
                return TextBufferFactory::createVirtualizedTextBuffer(
                    filename, actualPageSize, actualCacheSize);
            }
        }, di::Lifetime::Transient);
    }
}; 