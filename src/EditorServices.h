#pragma once

#include "interfaces/IEditorServices.hpp"
#include <memory>

// Forward declarations
namespace di { class Injector; }
class ITextBuffer;
class ICommandManager;
class IWorkspaceManager;
class ISyntaxHighlightingManager;
class IErrorReporter;
class ICommandRegistry;
class IUIExtensionRegistry;
class ISyntaxHighlightingRegistry;
class IEventRegistry;
class IWorkspaceExtension;
class IEditorCoreThreadPool;

namespace ai_editor {

/**
 * @class EditorServices
 * @brief Concrete implementation of the IEditorServices interface
 * 
 * This class serves as a facade for all core editor services, providing
 * a single point of access to various components needed by the editor.
 * It implements the IEditorServices interface by storing and delegating
 * to the actual service implementations.
 */
class EditorServices : public IEditorServices {
public:
    /**
     * @brief Constructor that takes all required service dependencies
     * 
     * @param textBuffer The text buffer service
     * @param commandManager The command manager service
     * @param workspaceManager The workspace manager service
     * @param syntaxHighlightingManager The syntax highlighting manager service
     * @param errorReporter The error reporter service
     * @param commandRegistry The command registry service
     * @param uiExtensionRegistry The UI extension registry service
     * @param syntaxHighlightingRegistry The syntax highlighting registry service
     * @param eventRegistry The event registry service
     * @param workspaceExtension The workspace extension service
     * @param editorCoreThreadPool The editor core thread pool service
     * @param injector The dependency injection container
     */
    EditorServices(
        std::shared_ptr<ITextBuffer> textBuffer,
        std::shared_ptr<ICommandManager> commandManager,
        std::shared_ptr<IWorkspaceManager> workspaceManager,
        std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager,
        std::shared_ptr<IErrorReporter> errorReporter,
        std::shared_ptr<ICommandRegistry> commandRegistry,
        std::shared_ptr<IUIExtensionRegistry> uiExtensionRegistry,
        std::shared_ptr<ISyntaxHighlightingRegistry> syntaxHighlightingRegistry,
        std::shared_ptr<IEventRegistry> eventRegistry,
        std::shared_ptr<IWorkspaceExtension> workspaceExtension,
        std::shared_ptr<IEditorCoreThreadPool> editorCoreThreadPool,
        di::Injector& injector
    );

    /**
     * @brief Destructor
     */
    ~EditorServices() override = default;

    // IEditorServices interface implementation
    
    /**
     * @brief Get the text buffer service
     * 
     * @return Shared pointer to the text buffer interface
     */
    std::shared_ptr<ITextBuffer> getTextBuffer() const override;
    
    /**
     * @brief Get the command manager service
     * 
     * @return Shared pointer to the command manager interface
     */
    std::shared_ptr<ICommandManager> getCommandManager() const override;
    
    /**
     * @brief Get the workspace manager service
     * 
     * @return Shared pointer to the workspace manager interface
     */
    std::shared_ptr<IWorkspaceManager> getWorkspaceManager() const override;
    
    /**
     * @brief Get the syntax highlighting manager service
     * 
     * @return Shared pointer to the syntax highlighting manager interface
     */
    std::shared_ptr<ISyntaxHighlightingManager> getSyntaxHighlightingManager() const override;
    
    /**
     * @brief Get the error reporter service
     * 
     * @return Shared pointer to the error reporter interface
     */
    std::shared_ptr<IErrorReporter> getErrorReporter() const override;

    /**
     * @brief Get the command registry service
     * 
     * @return Shared pointer to the command registry interface
     */
    std::shared_ptr<ICommandRegistry> getCommandRegistry() const override;
    
    /**
     * @brief Get the DI container
     * 
     * @return Reference to the dependency injection container
     */
    di::Injector& getInjector() override;
    
    /**
     * @brief Get the UI extension registry service
     * 
     * @return Shared pointer to the UI extension registry interface
     */
    std::shared_ptr<IUIExtensionRegistry> getUIExtensionRegistry() const override;
    
    /**
     * @brief Get the syntax highlighting registry service
     * 
     * @return Shared pointer to the syntax highlighting registry interface
     */
    std::shared_ptr<ISyntaxHighlightingRegistry> getSyntaxHighlightingRegistry() const override;
    
    /**
     * @brief Get the event registry service
     * 
     * @return Shared pointer to the event registry interface
     */
    std::shared_ptr<IEventRegistry> getEventRegistry() const override;
    
    /**
     * @brief Get the workspace extension service
     * 
     * @return Shared pointer to the workspace extension interface
     */
    std::shared_ptr<IWorkspaceExtension> getWorkspaceExtension() const override;
    
    /**
     * @brief Get the editor core thread pool service
     * 
     * @return Shared pointer to the editor core thread pool interface
     */
    std::shared_ptr<IEditorCoreThreadPool> getEditorCoreThreadPool() const override;

private:
    // Store the service instances
    std::shared_ptr<ITextBuffer> textBuffer_;
    std::shared_ptr<ICommandManager> commandManager_;
    std::shared_ptr<IWorkspaceManager> workspaceManager_;
    std::shared_ptr<ISyntaxHighlightingManager> syntaxHighlightingManager_;
    std::shared_ptr<IErrorReporter> errorReporter_;
    std::shared_ptr<ICommandRegistry> commandRegistry_;
    std::shared_ptr<IUIExtensionRegistry> uiExtensionRegistry_;
    std::shared_ptr<ISyntaxHighlightingRegistry> syntaxHighlightingRegistry_;
    std::shared_ptr<IEventRegistry> eventRegistry_;
    std::shared_ptr<IWorkspaceExtension> workspaceExtension_;
    std::shared_ptr<IEditorCoreThreadPool> editorCoreThreadPool_;
    di::Injector& injector_;  // Reference to the dependency injection container
};

} // namespace ai_editor 