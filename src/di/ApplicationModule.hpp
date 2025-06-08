#pragma once

#include "Injector.hpp"
#include "CoreModule.hpp"
#include "EditorFactory.hpp"
#include "../TextBuffer.h"
#include "../CommandManager.h"
#include "../SyntaxHighlightingManager.h"
#include "../interfaces/ITextBuffer.hpp"
#include "../interfaces/ICommandManager.hpp"
#include "../interfaces/ISyntaxHighlightingManager.hpp"
#include "../interfaces/IEditor.hpp"
#include <iostream>

namespace di {

/**
 * @brief Configures the dependency injection container for the application
 * 
 * This module registers all the application services with the DI container.
 * It builds on top of the CoreModule, which registers essential services.
 */
class ApplicationModule {
public:
    /**
     * @brief Configure the application services
     * 
     * This method registers all application-specific services with the injector.
     * It should be called after the CoreModule is configured.
     * 
     * @param injector The injector to configure
     */
    static void configure(Injector& injector) {
        std::cout << "Configuring ApplicationModule..." << std::endl;
        
        // Register ITextBuffer implementation
        injector.registerFactory<ITextBuffer>([]() {
            std::cout << "Creating new TextBuffer" << std::endl;
            return std::make_shared<TextBuffer>();
        });
        
        // Register ICommandManager implementation
        injector.registerFactory<ICommandManager>([]() {
            std::cout << "Creating new CommandManager" << std::endl;
            return std::make_shared<CommandManager>();
        });
        
        // Register ISyntaxHighlightingManager implementation
        injector.registerFactory<ISyntaxHighlightingManager>([]() {
            std::cout << "Creating new SyntaxHighlightingManager" << std::endl;
            return std::make_shared<SyntaxHighlightingManager>();
        });
        
        // Register IEditor implementation
        injector.registerFactory<IEditor>([](Injector& inj) {
            std::cout << "Creating new Editor via factory" << std::endl;
            return EditorFactory::createEditor(inj);
        });
        
        std::cout << "ApplicationModule configured successfully" << std::endl;
    }
};

} // namespace di 