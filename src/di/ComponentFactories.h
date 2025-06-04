#pragma once

#include "CommandRegistryFactory.h"
#include "UIExtensionRegistryFactory.h"
#include "EventRegistryFactory.h"
#include "SyntaxHighlightingRegistryFactory.h"
#include "WorkspaceExtensionFactory.h"
#include "TextBufferComponentFactory.h"

/**
 * @brief Utility class for registering all component factories in the DI system
 */
class ComponentFactories {
public:
    /**
     * @brief Register all component factories
     * 
     * This method registers all component factories with the given injector.
     * 
     * @param injector The DI injector to register components with
     */
    static void registerAll(di::Injector& injector) {
        CommandRegistryFactory::registerComponents(injector);
        UIExtensionRegistryFactory::registerComponents(injector);
        EventRegistryFactory::registerComponents(injector);
        SyntaxHighlightingRegistryFactory::registerComponents(injector);
        WorkspaceExtensionFactory::registerComponents(injector);
        TextBufferComponentFactory::registerComponents(injector);
    }
}; 