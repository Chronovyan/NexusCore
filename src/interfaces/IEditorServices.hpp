#pragma once

#include <memory>

// Forward include all interfaces
#include "ITextBuffer.hpp"
#include "ICommandManager.hpp"
#include "IWorkspaceManager.hpp"
#include "ISyntaxHighlightingManager.hpp"
#include "IErrorReporter.hpp"
#include "IEditorCoreThreadPool.hpp"
#include "IDiffEngine.hpp"
#include "IMergeEngine.hpp"

// Forward declare plugin-related interfaces
namespace di { class Injector; }
class ICommandRegistry;
class IUIExtensionRegistry;
namespace ai_editor { class ISyntaxHighlightingRegistry; }
class IEventRegistry;
class IWorkspaceExtension;

/**
 * @interface IEditorServices
 * @brief Central service registry for editor components
 * 
 * This interface provides access to all core services of the editor.
 * It acts as a central point for accessing the various components
 * via dependency injection.
 */
class IEditorServices {
public:
    /**
     * @brief Virtual destructor for proper cleanup in derived classes
     */
    virtual ~IEditorServices() = default;
    
    /**
     * @brief Get the text buffer service
     * 
     * @return Shared pointer to the text buffer interface
     */
    virtual std::shared_ptr<ITextBuffer> getTextBuffer() const = 0;
    
    /**
     * @brief Get the command manager service
     * 
     * @return Shared pointer to the command manager interface
     */
    virtual std::shared_ptr<ICommandManager> getCommandManager() const = 0;
    
    /**
     * @brief Get the workspace manager service
     * 
     * @return Shared pointer to the workspace manager interface
     */
    virtual std::shared_ptr<IWorkspaceManager> getWorkspaceManager() const = 0;
    
    /**
     * @brief Get the syntax highlighting manager service
     * 
     * @return Shared pointer to the syntax highlighting manager interface
     */
    virtual std::shared_ptr<ISyntaxHighlightingManager> getSyntaxHighlightingManager() const = 0;
    
    /**
     * @brief Get the error reporter service
     * 
     * @return Shared pointer to the error reporter interface
     */
    virtual std::shared_ptr<IErrorReporter> getErrorReporter() const = 0;
    
    /**
     * @brief Get the DI container
     * 
     * @return Reference to the dependency injection container
     */
    virtual di::Injector& getInjector() = 0;
    
    /**
     * @brief Get the command registry service
     * 
     * This service allows registering custom commands.
     * 
     * @return Shared pointer to the command registry interface
     */
    virtual std::shared_ptr<ICommandRegistry> getCommandRegistry() const = 0;
    
    /**
     * @brief Get the UI extension registry service
     * 
     * This service allows adding custom UI elements like menu items and toolbars.
     * 
     * @return Shared pointer to the UI extension registry interface
     */
    virtual std::shared_ptr<IUIExtensionRegistry> getUIExtensionRegistry() const = 0;
    
    /**
     * @brief Get the syntax highlighting registry service
     * 
     * This service allows registering custom syntax highlighters.
     * 
     * @return Shared pointer to the syntax highlighting registry interface
     */
    virtual std::shared_ptr<ai_editor::ISyntaxHighlightingRegistry> getSyntaxHighlightingRegistry() const = 0;
    
    /**
     * @brief Get the event registry service
     * 
     * This service allows subscribing to editor events.
     * 
     * @return Shared pointer to the event registry interface
     */
    virtual std::shared_ptr<IEventRegistry> getEventRegistry() const = 0;
    
    /**
     * @brief Get the workspace extension service
     * 
     * This service allows adding custom file type handlers and workspace scanners.
     * 
     * @return Shared pointer to the workspace extension interface
     */
    virtual std::shared_ptr<IWorkspaceExtension> getWorkspaceExtension() const = 0;
    
    /**
     * @brief Get the editor core thread pool service
     * 
     * This service manages worker threads for editor core components.
     * 
     * @return Shared pointer to the editor core thread pool interface
     */
    virtual std::shared_ptr<IEditorCoreThreadPool> getEditorCoreThreadPool() const = 0;
    
    /**
     * @brief Get the diff engine service
     * 
     * This service provides functionality for computing differences between texts.
     * 
     * @return Shared pointer to the diff engine interface
     */
    virtual std::shared_ptr<IDiffEngine> getDiffEngine() const = 0;
    
    /**
     * @brief Get the merge engine service
     * 
     * This service provides functionality for merging different versions of text.
     * 
     * @return Shared pointer to the merge engine interface
     */
    virtual std::shared_ptr<IMergeEngine> getMergeEngine() const = 0;
}; 