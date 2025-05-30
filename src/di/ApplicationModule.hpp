#pragma once

#include <memory>
#include "Injector.hpp"
#include "TextBufferFactory.hpp"
#include "CommandManagerFactory.hpp"
#include "EditorFactory.hpp"
#include "../interfaces/ITextBuffer.hpp"
#include "../interfaces/ICommandManager.hpp"
#include "../interfaces/IEditor.hpp"
#include "../AppDebugLog.h"

/**
 * @class ApplicationModule
 * @brief Configures the dependency injection container for the application
 * 
 * This module registers all the application's components with the DI container,
 * ensuring they are properly created and wired together.
 */
class ApplicationModule {
public:
    /**
     * @brief Configure the given DI container with application components
     * 
     * @param injector The DI container to configure
     */
    static void configure(di::Injector& injector) {
        LOG_DEBUG("Configuring ApplicationModule");
        
        // Register TextBuffer
        injector.registerFactory<ITextBuffer>(
            [](di::Injector& inj) {
                return TextBufferFactory::create(inj);
            },
            di::Lifetime::Transient
        );
        
        // Register CommandManager
        injector.registerFactory<ICommandManager>(
            [](di::Injector& inj) {
                return CommandManagerFactory::create(inj);
            },
            di::Lifetime::Transient
        );
        
        // Register Editor
        injector.registerFactory<IEditor>(
            [](di::Injector& inj) {
                return EditorFactory::create(inj);
            },
            di::Lifetime::Transient
        );
        
        LOG_DEBUG("ApplicationModule configured successfully");
    }
}; 