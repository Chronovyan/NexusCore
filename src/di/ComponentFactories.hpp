#pragma once

#include <memory>

#include "DIFramework.hpp"
#include "ServiceCollection.hpp"
#include "../interfaces/IEditor.hpp"
#include "../interfaces/ITextBuffer.hpp"
#include "../interfaces/ISyntaxHighlightingManager.hpp"
#include "../interfaces/ICommandManager.hpp"
#include "../interfaces/IApplication.hpp"
#include "../interfaces/IErrorReporter.hpp"
#include "../interfaces/IWorkspaceManager.hpp"
#include "../interfaces/IEditorServices.hpp"
#include "../interfaces/IEditorCoreThreadPool.hpp"
#include "../interfaces/plugins/ICommandRegistry.hpp"
#include "../interfaces/plugins/IUIExtensionRegistry.hpp"
#include "../interfaces/plugins/ISyntaxHighlightingRegistry.hpp"
#include "../interfaces/plugins/IEventRegistry.hpp"
#include "../interfaces/plugins/IWorkspaceExtension.hpp"
#include "../Editor.h"
#include "../TextBuffer.h"
#include "../ThreadSafeTextBuffer.h"
#include "../SyntaxHighlightingManager.h"
#include "../CommandManager.h"
#include "../Application.h"
#include "../WorkspaceManager.h"
#include "../EditorErrorReporter.h"
#include "../EditorServices.h"
#include "../EditorCoreThreadPool.h"
#include "../AppDebugLog.h"
#include "../plugins/PluginManager.hpp"
#include "../plugins/CommandRegistry.hpp"
#include "../plugins/UIExtensionRegistry.hpp"
#include "../plugins/SyntaxHighlightingRegistry.hpp"
#include "../plugins/EventRegistry.hpp"
#include "../plugins/WorkspaceExtension.hpp"
#include "factories/PluginManagerFactory.hpp"
#include "factories/CommandRegistryFactory.hpp"
#include "factories/UIExtensionRegistryFactory.hpp"
#include "factories/SyntaxHighlightingRegistryFactory.hpp"
#include "factories/EventRegistryFactory.hpp"
#include "factories/WorkspaceExtensionFactory.hpp"
#include "../interfaces/IDiffEngine.hpp"
#include "../interfaces/IMergeEngine.hpp"
#include "../diff/DiffMergeFactory.h"

namespace di {

/**
 * @class ComponentFactories
 * @brief Factory classes for all major components in the application
 * 
 * This class provides factory methods for creating all major components in the application.
 * It is used by the DIFramework to register components with the dependency injection container.
 */
class ComponentFactories {
public:
    /**
     * @brief Register all component factories with a service collection
     * 
     * This method registers all component factories with the provided service collection.
     * It is used to initialize the dependency injection container with all required services.
     * 
     * @param services The service collection to register factories with
     */
    static void registerAll(ServiceCollection& services) {
        // Register core services
        registerTextBuffer(services);
        registerSyntaxHighlightingManager(services);
        registerCommandManager(services);
        registerErrorReporter(services);
        registerWorkspaceManager(services);
        registerEditorCoreThreadPool(services);
        
        // Register plugin registry interfaces
        registerCommandRegistry(services);
        registerUIExtensionRegistry(services);
        registerSyntaxHighlightingRegistry(services);
        registerEventRegistry(services);
        registerWorkspaceExtension(services);
        
        registerEditorServices(services);

        // Register the editor
        registerEditor(services);
        
        // Register the plugin manager
        registerPluginManager(services);
        
        // Register the application
        registerApplication(services);
    }
    
    /**
     * @brief Register the text buffer factory
     * 
     * @param services The service collection to register with
     */
    static void registerTextBuffer(ServiceCollection& services) {
        // Register the standard text buffer
        services.addSingleton<ITextBuffer>([](){ 
            auto buffer = std::make_shared<TextBuffer>();
            buffer->addLine(""); // Initialize with an empty line
            return buffer;
        });
        
        // Register the thread-safe text buffer as a separate service
        services.addSingleton<ThreadSafeTextBuffer>([](std::shared_ptr<DIFramework> provider){ 
            auto buffer = provider->get<ITextBuffer>();
            return std::make_shared<ThreadSafeTextBuffer>(buffer);
        });
    }
    
    /**
     * @brief Register the syntax highlighting manager factory
     * 
     * @param services The service collection to register with
     */
    static void registerSyntaxHighlightingManager(ServiceCollection& services) {
        services.addSingleton<ISyntaxHighlightingManager>([](){ 
            return std::make_shared<SyntaxHighlightingManager>();
        });
    }
    
    /**
     * @brief Register the command manager factory
     * 
     * @param services The service collection to register with
     */
    static void registerCommandManager(ServiceCollection& services) {
        services.addSingleton<ICommandManager>([](){ 
            return std::make_shared<CommandManager>();
        });
    }
    
    /**
     * @brief Register the error reporter factory
     * 
     * @param services The service collection to register with
     */
    static void registerErrorReporter(ServiceCollection& services) {
        services.addSingleton<IErrorReporter>([](){ 
            return std::make_shared<EditorErrorReporter>();
        });
    }
    
    /**
     * @brief Register the workspace manager factory
     * 
     * @param services The service collection to register with
     */
    static void registerWorkspaceManager(ServiceCollection& services) {
        services.addSingleton<IWorkspaceManager>([](){ 
            return std::make_shared<WorkspaceManager>();
        });
    }
    
    /**
     * @brief Register the editor core thread pool factory
     * 
     * @param services The service collection to register with
     */
    static void registerEditorCoreThreadPool(ServiceCollection& services) {
        services.addSingleton<IEditorCoreThreadPool>([](){ 
            return std::make_shared<EditorCoreThreadPool>();
        });
    }
    
    /**
     * @brief Register the command registry factory
     * 
     * @param services The service collection to register with
     */
    static void registerCommandRegistry(ServiceCollection& services) {
        services.addSingleton<ICommandRegistry>([](){
            return std::make_shared<CommandRegistry>();
        });
    }
    
    /**
     * @brief Register the UI extension registry factory
     * 
     * @param services The service collection to register with
     */
    static void registerUIExtensionRegistry(ServiceCollection& services) {
        services.addSingleton<IUIExtensionRegistry>([](){
            return std::make_shared<UIExtensionRegistry>();
        });
    }
    
    /**
     * @brief Register the syntax highlighting registry factory
     * 
     * @param services The service collection to register with
     */
    static void registerSyntaxHighlightingRegistry(ServiceCollection& services) {
        services.addSingleton<ISyntaxHighlightingRegistry>([](){
            return std::make_shared<SyntaxHighlightingRegistry>();
        });
    }
    
    /**
     * @brief Register the event registry factory
     * 
     * @param services The service collection to register with
     */
    static void registerEventRegistry(ServiceCollection& services) {
        services.addSingleton<IEventRegistry>([](){
            return std::make_shared<EventRegistry>();
        });
    }
    
    /**
     * @brief Register the workspace extension factory
     * 
     * @param services The service collection to register with
     */
    static void registerWorkspaceExtension(ServiceCollection& services) {
        services.addSingleton<IWorkspaceExtension>([](){
            return std::make_shared<WorkspaceExtension>();
        });
    }
    
    /**
     * @brief Register the editor services factory
     * 
     * @param services The service collection to register with
     */
    static void registerEditorServices(ServiceCollection& services) {
        services.addSingleton<IEditorServices>([](std::shared_ptr<DIFramework> provider){ 
            auto textBuffer = provider->get<ITextBuffer>();
            auto commandManager = provider->get<ICommandManager>();
            auto workspaceManager = provider->get<IWorkspaceManager>();
            auto syntaxHighlightingManager = provider->get<ISyntaxHighlightingManager>();
            auto errorReporter = provider->get<IErrorReporter>();
            auto commandRegistry = provider->get<ICommandRegistry>();
            auto uiExtensionRegistry = provider->get<IUIExtensionRegistry>();
            auto syntaxHighlightingRegistry = provider->get<ISyntaxHighlightingRegistry>();
            auto eventRegistry = provider->get<IEventRegistry>();
            auto workspaceExtension = provider->get<IWorkspaceExtension>();
            auto threadPool = provider->get<IEditorCoreThreadPool>();
            auto diffEngine = provider->get<IDiffEngine>();
            auto mergeEngine = provider->get<IMergeEngine>();

            return std::make_shared<EditorServices>(
                textBuffer,
                commandManager,
                workspaceManager,
                syntaxHighlightingManager,
                errorReporter,
                commandRegistry,
                uiExtensionRegistry,
                syntaxHighlightingRegistry,
                eventRegistry,
                workspaceExtension,
                threadPool,
                diffEngine,
                mergeEngine,
                provider->getInjector()
            );
        });
    }
    
    /**
     * @brief Register the editor factory
     * 
     * @param services The service collection to register with
     */
    static void registerEditor(ServiceCollection& services) {
        services.addSingleton<IEditor>([](std::shared_ptr<DIFramework> provider){ 
            auto textBuffer = provider->get<ITextBuffer>();
            auto syntaxHighlighter = provider->get<ISyntaxHighlightingManager>();
            auto commandManager = provider->get<ICommandManager>();
            auto errorReporter = provider->get<IErrorReporter>();
            
            return std::make_shared<Editor>(textBuffer, syntaxHighlighter, commandManager, errorReporter);
        });
    }
    
    /**
     * @brief Register the plugin manager factory
     * 
     * @param services The service collection to register with
     */
    static void registerPluginManager(ServiceCollection& services) {
        services.addSingleton<PluginManager>([](std::shared_ptr<DIFramework> provider){
            auto editorServices = provider->get<IEditorServices>();
            return PluginManagerFactory::createPluginManager(editorServices);
        });
    }
    
    /**
     * @brief Register the application factory
     * 
     * @param services The service collection to register with
     */
    static void registerApplication(ServiceCollection& services) {
        services.addSingleton<IApplication>([](std::shared_ptr<DIFramework> provider){ 
            auto editor = provider->get<IEditor>();
            auto workspaceManager = provider->get<IWorkspaceManager>();
            auto editorServices = provider->get<IEditorServices>();
            // Get the plugin manager to ensure it's initialized before the application starts
            auto pluginManager = provider->get<PluginManager>();
            
            return std::make_shared<Application>(editor, workspaceManager, editorServices);
        });
    }

    /**
     * @brief Register factories for diff and merge components
     * 
     * @param framework The DI framework to register with
     */
    static void registerDiffMergeFactories(std::shared_ptr<DIFramework> framework) {
        // Register IDiffEngine
        framework->registerFactory<IDiffEngine>(
            [](std::shared_ptr<DIFramework> provider) {
                return DiffMergeFactory::createDiffEngine();
            });
        
        // Register IMergeEngine
        framework->registerFactory<IMergeEngine>(
            [](std::shared_ptr<DIFramework> provider) {
                auto diffEngine = provider->get<IDiffEngine>();
                return DiffMergeFactory::createMergeEngine(diffEngine);
            });
    }
};

} // namespace di 