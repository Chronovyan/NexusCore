#pragma once

#include "UIModel.h"
#include "AIManager.h"
#include "WorkspaceManager.h"
#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "interfaces/IAIProvider.hpp"
#include "CodeContextProvider.hpp"
#include <functional>

namespace ai_editor {

/**
 * @class AIAgentOrchestrator
 * @brief Orchestrates AI interactions for code generation and project management
 * 
 * This class bridges the UI and AI providers, managing the conversation workflow,
 * handling tool calls, and coordinating the AI's responses with user input.
 */
class AIAgentOrchestrator {
public:
    /**
     * @enum State
     * @brief Represents the current state of the orchestrator in the conversation workflow
     */
    enum class State {
        IDLE,
        AWAITING_AI_RESPONSE,
        AWAITING_APPROVAL,
        EXECUTING_TASK,
        AI_ERROR
    };

    /**
     * @brief Constructor
     * 
     * @param aiManager Reference to an AIManager for accessing AI providers
     * @param uiModel Reference to the UI model for updates
     * @param workspaceManager Reference to the workspace manager for file operations
     * @param codeContextProvider The code context provider (optional)
     */
    AIAgentOrchestrator(AIManager& aiManager, UIModel& uiModel, WorkspaceManager& workspaceManager, std::shared_ptr<CodeContextProvider> codeContextProvider = nullptr);

    /**
     * @brief Destructor
     */
    ~AIAgentOrchestrator();

    /**
     * @brief Handle a user prompt submission
     * 
     * Takes the user's input, updates the UI model, and sends the prompt to the AI
     * 
     * @param userInput The text entered by the user
     */
    void handleSubmitUserPrompt(const std::string& userInput);
    
    /**
     * @brief Handle user feedback on the AI's proposal
     * 
     * Takes the user's feedback, updates the UI model, and sends it to the AI
     * 
     * @param userFeedback The feedback text entered by the user
     */
    void handleUserFeedback(const std::string& userFeedback);
    
    /**
     * @brief Handle user feedback during task execution
     * 
     * Takes the user's feedback during execution, updates the UI model, and sends it to the AI
     * 
     * @param userFeedback The feedback text entered by the user
     */
    void handleUserFeedbackDuringExecution(const std::string& userFeedback);
    
    /**
     * @brief Get the current state of the orchestrator
     * 
     * @return State The current state
     */
    State getState() const { return state_; }
    
    /**
     * @brief Get the current AI provider type
     * 
     * @return std::string The current AI provider type
     */
    std::string getCurrentProviderType() const;
    
    /**
     * @brief Set the active AI provider
     * 
     * @param providerType The type of provider to set as active
     * @return bool True if the provider was set as active
     */
    bool setActiveProvider(const std::string& providerType);
    
    /**
     * @brief Get the current model info
     * 
     * @return ModelInfo Information about the current model
     */
    ModelInfo getCurrentModelInfo() const;
    
    /**
     * @brief Set the current model
     * 
     * @param modelId The ID of the model to use
     * @return bool True if successful
     */
    bool setCurrentModel(const std::string& modelId);

    /**
     * @brief Reset the orchestrator state to IDLE
     * 
     * This method allows recovery from ERROR state by resetting the orchestrator to its
     * initial state, clearing conversation history and other session data, and updating
     * the UI model to reflect the reset state.
     */
    void resetState();

    /**
     * @brief Configure and activate the local LLama provider
     * 
     * This method initializes the LLama provider with the specified model path,
     * sets it as the active provider, and clears the conversation history.
     * 
     * @param modelPath Path to the model file or directory containing model files
     * @return true if configuration was successful, false otherwise
     */
    bool configureLocalLlamaProvider(const std::string& modelPath);

    /**
     * @brief Set the code context provider
     * 
     * @param contextProvider The code context provider
     */
    void setCodeContextProvider(std::shared_ptr<CodeContextProvider> contextProvider);
    
    /**
     * @brief Enable or disable context-aware prompts
     * 
     * @param enabled Whether to enable context-aware prompts
     */
    void setContextAwarePromptsEnabled(bool enabled) {
        contextAwarePromptsEnabled_ = enabled;
    }
    
    /**
     * @brief Check if context-aware prompts are enabled
     * 
     * @return bool Whether context-aware prompts are enabled
     */
    bool areContextAwarePromptsEnabled() const {
        return contextAwarePromptsEnabled_ && codeContextProvider_ != nullptr;
    }
    
    /**
     * @brief Update the current editing context
     * 
     * @param filePath The current file path
     * @param line The current line number
     * @param column The current column number
     * @param selectedText The currently selected text
     * @param visibleFiles List of currently visible files
     */
    void updateEditingContext(
        const std::string& filePath,
        size_t line,
        size_t column,
        const std::string& selectedText = "",
        const std::vector<std::string>& visibleFiles = {});

    // Context management
    void enableContextAwarePrompts(bool enable);
    void updateEditingContext(const std::string& filePath, int line, int column, 
                             const std::string& selectedText, 
                             const std::vector<std::string>& visibleFiles);
    
    // Enhanced context options
    void setContextOptions(const ContextOptions& options);
    ContextOptions getContextOptions() const;
    void setMaxTokens(int maxTokens);
    void setMinRelevanceScore(float minScore);
    void setMaxRelatedSymbols(int maxSymbols);
    void setMaxRelatedFiles(int maxFiles);
    void setMaxCodeSnippets(int maxSnippets);
    void setScopeDepth(int depth);
    void setIncludeDefinitions(bool include);
    void setIncludeReferences(bool include);
    void setIncludeRelationships(bool include);
    
    // Custom relevance scorers
    void registerSymbolRelevanceScorer(const std::string& name, ai_editor::SymbolRelevanceScorer scorer);
    void registerFileRelevanceScorer(const std::string& name, ai_editor::FileRelevanceScorer scorer);
    
    // AI provider management
    void setAIProvider(std::shared_ptr<IAIProvider> aiProvider);

private:
    // Reference to the AIManager for accessing AI providers
    AIManager& aiManager_;
    
    // Reference to the UI model for updates
    UIModel& uiModel_;
    
    // Reference to the workspace manager for file operations
    WorkspaceManager& workspaceManager_;
    
    // Current state of the orchestrator
    State state_ = State::IDLE;
    
    // Conversation history
    std::vector<Message> messages_;
    
    // Process and handle AI responses
    void processAIResponse(const CompletionResponse& response);
    
    // Process tool calls in AI responses
    bool processToolCalls(const std::vector<ToolCall>& toolCalls);

    // Generate contextual prompt
    std::string enrichPromptWithContext(const std::string& userPrompt);

    // Member variables
    std::shared_ptr<CodeContextProvider> codeContextProvider_;
    
    // Current editing context
    std::string currentFilePath_;
    size_t currentLine_ = 0;
    size_t currentColumn_ = 0;
    std::string currentSelectedText_;
    std::vector<std::string> currentVisibleFiles_;
    
    // Configuration
    bool contextAwarePromptsEnabled_ = true;
    
    // Context options
    ContextOptions contextOptions_;
};

} // namespace ai_editor 