#pragma once

#include "../Editor.h"
#include "../interfaces/IEditor.hpp"
#include "../interfaces/ITextBuffer.hpp"
#include "../interfaces/ICommandManager.hpp"
#include "../interfaces/ISyntaxHighlightingManager.hpp"
#include "../interfaces/IDiffEngine.hpp"
#include "../interfaces/IMergeEngine.hpp"
#include "Injector.hpp"

namespace di {

/**
 * @brief Factory for creating Editor instances
 * 
 * This factory provides a method to create properly configured Editor instances
 * with all dependencies injected from the DI container.
 */
class EditorFactory {
public:
    /**
     * @brief Create a new Editor instance
     * 
     * This factory method creates a new Editor instance with all dependencies
     * resolved from the DI container.
     * 
     * @param injector The DI container to resolve dependencies from
     * @return A shared pointer to the created Editor instance
     */
    static std::shared_ptr<IEditor> createEditor(Injector& injector) {
        // Resolve dependencies
        auto textBuffer = injector.get<ITextBuffer>();
        auto commandManager = injector.get<ICommandManager>();
        auto syntaxHighlightingManager = injector.get<ISyntaxHighlightingManager>();
        auto diffEngine = injector.get<IDiffEngine>();
        auto mergeEngine = injector.get<IMergeEngine>();
        
        // Create and return the Editor instance
        return std::make_shared<Editor>(
            textBuffer,
            commandManager,
            syntaxHighlightingManager,
            diffEngine,
            mergeEngine
        );
    }
};

} // namespace di 