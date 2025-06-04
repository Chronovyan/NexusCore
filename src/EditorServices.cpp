#include "EditorServices.h"
#include "LoggingCompatibility.h"
#include "AppDebugLog.h"
#include "interfaces/plugins/ICommandRegistry.hpp"
#include "interfaces/plugins/IUIExtensionRegistry.hpp"
#include "interfaces/plugins/ISyntaxHighlightingRegistry.hpp"
#include "interfaces/plugins/IEventRegistry.hpp"
#include "interfaces/plugins/IWorkspaceExtension.hpp"
#include "interfaces/IEditorCoreThreadPool.hpp"
#include "interfaces/IDiffEngine.hpp"
#include "interfaces/IMergeEngine.hpp"

namespace ai_editor {

EditorServices::EditorServices(
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
    std::shared_ptr<IDiffEngine> diffEngine,
    std::shared_ptr<IMergeEngine> mergeEngine,
    di::Injector& injector)
: textBuffer_(std::move(textBuffer)),
  commandManager_(std::move(commandManager)),
  workspaceManager_(std::move(workspaceManager)),
  syntaxHighlightingManager_(std::move(syntaxHighlightingManager)),
  errorReporter_(std::move(errorReporter)),
  commandRegistry_(std::move(commandRegistry)),
  uiExtensionRegistry_(std::move(uiExtensionRegistry)),
  syntaxHighlightingRegistry_(std::move(syntaxHighlightingRegistry)),
  eventRegistry_(std::move(eventRegistry)),
  workspaceExtension_(std::move(workspaceExtension)),
  editorCoreThreadPool_(std::move(editorCoreThreadPool)),
  diffEngine_(std::move(diffEngine)),
  mergeEngine_(std::move(mergeEngine)),
  injector_(injector) {
    
    LOG_DEBUG("EditorServices initialized with all required dependencies");
    
    // Validate that all dependencies are non-null
    if (!textBuffer_) {
        LOG_ERROR("EditorServices initialized with null TextBuffer");
        throw std::invalid_argument("TextBuffer cannot be null");
    }
    
    if (!commandManager_) {
        LOG_ERROR("EditorServices initialized with null CommandManager");
        throw std::invalid_argument("CommandManager cannot be null");
    }
    
    if (!workspaceManager_) {
        LOG_ERROR("EditorServices initialized with null WorkspaceManager");
        throw std::invalid_argument("WorkspaceManager cannot be null");
    }
    
    if (!syntaxHighlightingManager_) {
        LOG_ERROR("EditorServices initialized with null SyntaxHighlightingManager");
        throw std::invalid_argument("SyntaxHighlightingManager cannot be null");
    }
    
    if (!errorReporter_) {
        LOG_ERROR("EditorServices initialized with null ErrorReporter");
        throw std::invalid_argument("ErrorReporter cannot be null");
    }
    
    if (!commandRegistry_) {
        LOG_ERROR("EditorServices initialized with null CommandRegistry");
        throw std::invalid_argument("CommandRegistry cannot be null");
    }
    
    if (!uiExtensionRegistry_) {
        LOG_ERROR("EditorServices initialized with null UIExtensionRegistry");
        throw std::invalid_argument("UIExtensionRegistry cannot be null");
    }
    
    if (!syntaxHighlightingRegistry_) {
        LOG_ERROR("EditorServices initialized with null SyntaxHighlightingRegistry");
        throw std::invalid_argument("SyntaxHighlightingRegistry cannot be null");
    }
    
    if (!eventRegistry_) {
        LOG_ERROR("EditorServices initialized with null EventRegistry");
        throw std::invalid_argument("EventRegistry cannot be null");
    }
    
    if (!workspaceExtension_) {
        LOG_ERROR("EditorServices initialized with null WorkspaceExtension");
        throw std::invalid_argument("WorkspaceExtension cannot be null");
    }
    
    if (!editorCoreThreadPool_) {
        LOG_ERROR("EditorServices initialized with null EditorCoreThreadPool");
        throw std::invalid_argument("EditorCoreThreadPool cannot be null");
    }
    
    if (!diffEngine_) {
        LOG_ERROR("EditorServices initialized with null DiffEngine");
        throw std::invalid_argument("DiffEngine cannot be null");
    }
    
    if (!mergeEngine_) {
        LOG_ERROR("EditorServices initialized with null MergeEngine");
        throw std::invalid_argument("MergeEngine cannot be null");
    }
}

std::shared_ptr<ITextBuffer> EditorServices::getTextBuffer() const {
    return textBuffer_;
}

std::shared_ptr<ICommandManager> EditorServices::getCommandManager() const {
    return commandManager_;
}

std::shared_ptr<IWorkspaceManager> EditorServices::getWorkspaceManager() const {
    return workspaceManager_;
}

std::shared_ptr<ISyntaxHighlightingManager> EditorServices::getSyntaxHighlightingManager() const {
    return syntaxHighlightingManager_;
}

std::shared_ptr<IErrorReporter> EditorServices::getErrorReporter() const {
    return errorReporter_;
}

std::shared_ptr<ICommandRegistry> EditorServices::getCommandRegistry() const {
    return commandRegistry_;
}

di::Injector& EditorServices::getInjector() {
    return injector_;
}

std::shared_ptr<IUIExtensionRegistry> EditorServices::getUIExtensionRegistry() const {
    return uiExtensionRegistry_;
}

std::shared_ptr<ISyntaxHighlightingRegistry> EditorServices::getSyntaxHighlightingRegistry() const {
    return syntaxHighlightingRegistry_;
}

std::shared_ptr<IEventRegistry> EditorServices::getEventRegistry() const {
    return eventRegistry_;
}

std::shared_ptr<IWorkspaceExtension> EditorServices::getWorkspaceExtension() const {
    return workspaceExtension_;
}

std::shared_ptr<IEditorCoreThreadPool> EditorServices::getEditorCoreThreadPool() const {
    return editorCoreThreadPool_;
}

std::shared_ptr<IDiffEngine> EditorServices::getDiffEngine() const {
    return diffEngine_;
}

std::shared_ptr<IMergeEngine> EditorServices::getMergeEngine() const {
    return mergeEngine_;
}

} // namespace ai_editor 