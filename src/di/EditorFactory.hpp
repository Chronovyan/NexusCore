#pragma once

#include <memory>
#include "Injector.hpp"
#include "../Editor.h"
#include "../interfaces/IEditor.hpp"
#include "../interfaces/ITextBuffer.hpp"
#include "../interfaces/ICommandManager.hpp"
#include "../interfaces/ISyntaxHighlightingManager.hpp"
#include "../AppDebugLog.h"

/**
 * @class EditorFactory
 * @brief Factory for creating and configuring Editor instances
 * 
 * This factory is responsible for creating Editor instances with the appropriate
 * dependencies and configuration.
 */
class EditorFactory {
public:
    /**
     * @brief Create a new Editor instance with dependencies resolved from the injector
     * 
     * @param injector The dependency injector
     * @return A shared pointer to an Editor instance, as an IEditor
     */
    static std::shared_ptr<IEditor> create(di::Injector& injector) {
        // In the future, Editor will use injected dependencies
        // For now, we create a basic Editor which manages its own dependencies
        
        // Create a new Editor instance
        auto editor = std::make_shared<Editor>();
        
        // Additional configuration could be done here
        
        LOG_DEBUG("Created new Editor instance");
        
        return editor;
    }
    
    /**
     * @brief Create a new Editor instance with explicitly provided dependencies
     * 
     * @param textBuffer The text buffer to use
     * @param commandManager The command manager to use
     * @param syntaxHighlightingManager The syntax highlighting manager to use
     * @return A shared pointer to an Editor instance, as an IEditor
     */
    static std::shared_ptr<IEditor> createWithDependencies(
        std::shared_ptr<ITextBuffer> textBuffer,
        std::shared_ptr<ICommandManager> commandManager,
        std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager
    ) {
        // Note: This factory method is prepared for when Editor is refactored
        // to take its dependencies via constructor injection. For now, it just
        // creates a basic Editor instance.
        
        // Create a new Editor instance
        auto editor = std::make_shared<Editor>();
        
        // In the future, the dependencies would be passed to the constructor
        
        LOG_DEBUG("Created new Editor instance with explicit dependencies");
        
        return editor;
    }
}; 