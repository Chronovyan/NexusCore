#pragma once

#include <memory>
#include "Injector.hpp"
#include "../TextBuffer.h"
#include "../interfaces/ITextBuffer.hpp"
#include "../AppDebugLog.h"

/**
 * @class TextBufferFactory
 * @brief Factory for creating and configuring TextBuffer instances
 * 
 * This factory is responsible for creating TextBuffer instances with the appropriate
 * configuration and returning them as ITextBuffer interface implementations.
 */
class TextBufferFactory {
public:
    /**
     * @brief Create a new TextBuffer instance
     * 
     * @param injector The dependency injector
     * @return A shared pointer to a TextBuffer instance, as an ITextBuffer
     */
    static std::shared_ptr<ITextBuffer> create(di::Injector& injector) {
        // Currently TextBuffer doesn't have dependencies, but if it did,
        // we would resolve them from the injector here
        
        // Create a new TextBuffer instance
        auto textBuffer = std::make_shared<TextBuffer>();
        
        // Initialize with an empty line so the buffer is never completely empty
        textBuffer->addLine("");
        
        LOG_DEBUG("Created new TextBuffer instance");
        
        return textBuffer;
    }
}; 