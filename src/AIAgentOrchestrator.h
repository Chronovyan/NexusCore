#ifndef AI_AGENT_ORCHESTRATOR_H
#define AI_AGENT_ORCHESTRATOR_H

#include "UIModel.h"
#include "IOpenAI_API_Client.h"
#include "WorkspaceManager.h"
#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace ai_editor {

/**
 * @class AIAgentOrchestrator
 * @brief Orchestrates AI interactions for code generation and project management
 * 
 * This class bridges the UI and OpenAI API, managing the conversation workflow,
 * handling tool calls, and coordinating the AI's responses with user input.
 */
class AIAgentOrchestrator {
public:
    /**
     * @enum OrchestratorState
     * @brief Represents the current state of the orchestrator in the conversation workflow
     */
    enum class OrchestratorState {
        IDLE,
        AWAITING_AI_PLAN,
        PLAN_RECEIVED_AWAITING_PARSE,
        AWAITING_USER_FEEDBACK_ON_PLAN,
        AWAITING_USER_CLARIFICATION_BEFORE_PLAN,
        AWAITING_USER_CLARIFICATION,
        AWAITING_AI_ABSTRACT_PREVIEW,
        AWAITING_USER_APPROVAL_OF_PREVIEW,
        GENERATING_CODE_FILES,
        AWAITING_AI_COMPILE_COMMANDS,
        COMPILATION_IN_PROGRESS,
        TESTING_IN_PROGRESS,
        EXECUTION_IN_PROGRESS,
        ERROR_STATE
    };

    /**
     * @brief Constructor
     * 
     * @param apiClient Reference to an IOpenAI_API_Client interface
     * @param uiModel Reference to the UI model for updates
     * @param workspaceManager Reference to the workspace manager for file operations
     */
    AIAgentOrchestrator(IOpenAI_API_Client& apiClient, UIModel& uiModel, WorkspaceManager& workspaceManager);

    /**
     * @brief Handle a user prompt submission
     * 
     * Takes the user's input, updates the UI model, and sends the prompt to the AI
     * 
     * @param userInput The text entered by the user
     */
    void handleSubmitUserPrompt(const std::string& userInput);
    
    /**
     * @brief Handle user feedback on the AI's plan or clarification questions
     * 
     * Takes the user's feedback, updates the UI model, and requests an abstract preview from the AI
     * 
     * @param userFeedbackText The feedback text entered by the user
     */
    void handleSubmitUserFeedback(const std::string& userFeedbackText);
    
    /**
     * @brief Handle user approval of the AI's abstract preview
     * 
     * Takes the user's approval message, updates the UI model, and triggers the generation of the first file
     * 
     * @param userApprovalText The approval message entered by the user
     */
    void handleSubmitUserApprovalOfPreview(const std::string& userApprovalText);
    
    /**
     * @brief Get the current state of the orchestrator
     * 
     * @return OrchestratorState The current state
     */
    OrchestratorState getCurrentState() const { return orchestratorState_; }

private:
    /**
     * @brief Process a propose_plan tool call from the AI
     * 
     * @param toolCall The propose_plan tool call
     * @return bool True if the tool call was processed successfully
     */
    bool processProposePlanToolCall(const ApiToolCall& toolCall);
    
    /**
     * @brief Process an ask_user_for_clarification tool call from the AI
     * 
     * @param toolCall The ask_user_for_clarification tool call
     * @return bool True if the tool call was processed successfully
     */
    bool processAskForClarificationToolCall(const ApiToolCall& toolCall);

    /**
     * @brief Process a provide_abstract_preview tool call from the AI
     * 
     * @param toolCall The provide_abstract_preview tool call
     * @return bool True if the tool call was processed successfully
     */
    bool processProvideAbstractPreviewToolCall(const ApiToolCall& toolCall);
    
    /**
     * @brief Process a write_file_content tool call from the AI
     * 
     * @param toolCall The write_file_content tool call
     * @return bool True if the tool call was processed successfully
     */
    bool processWriteFileContentToolCall(const ApiToolCall& toolCall);
    
    /**
     * @brief Determine the next file to generate based on the plan
     * 
     * @return std::string The name of the next file to generate, or empty string if all files are generated
     */
    std::string determineNextFileToGenerate() const;
    
    /**
     * @brief Request the generation of the next file from the AI
     * 
     * @param previousToolCallId The ID of the previous tool call
     * @param previousToolName The name of the previous tool call
     * @param successResult Whether the previous file was generated successfully
     * @param filename The name of the file that was generated or attempted
     * @param errorMessage Error message if the file generation failed
     */
    void requestNextFileOrCompilation(const std::string& previousToolCallId, const std::string& previousToolName, 
                                      bool successResult, const std::string& filename, 
                                      const std::string& errorMessage = "");

    // Reference to the OpenAI API client interface
    IOpenAI_API_Client& apiClient_;
    
    // Reference to the UI model for updates
    UIModel& uiModel_;
    
    // Reference to the workspace manager for file operations
    WorkspaceManager& workspaceManager_;
    
    // Conversation history for the AI
    std::vector<ApiChatMessage> conversationHistory_;
    
    // Current state of the orchestrator
    OrchestratorState orchestratorState_;
    
    // Last used tool call ID (for tracking API responses)
    std::string lastToolCallId_;
    
    // Last used tool name (for constructing tool responses)
    std::string lastToolName_;
    
    // Store the last plan JSON for reference
    nlohmann::json lastPlanJson_;
    
    // Store the last clarification questions JSON for reference
    nlohmann::json lastClarificationJson_;
    
    // Store the next file to generate after abstract preview approval
    std::string nextPlannedFileToGenerate_;
    
    // List of files that have been generated
    std::vector<std::string> generatedFiles_;
    
    // System message for AI that defines its role and instructions
    const std::string systemMessage_ = 
        "You are an AI-powered coding assistant that helps generate C++ projects based on user requests. "
        "Your task is to guide the user through a step-by-step process of building a functional C++ application. "
        "Follow these steps when responding to the user:"
        "\n\n1. PLAN: Understand the user's request and outline the project structure, files needed, and approach."
        "\n2. CLARIFY: If the requirements are unclear, ask specific questions to refine the plan."
        "\n3. PREVIEW: Show the user what files will be created and their purpose."
        "\n4. GENERATE: Create the necessary code files with proper structure and comments."
        "\n5. COMPILE: Prepare compilation instructions."
        "\n6. TEST: Suggest ways to test the application."
        "\n7. EXECUTE: Provide commands to run the application."
        "\n\nUse the provided tools to accomplish these tasks. DO NOT simulate tool outputs or invent "
        "file contents that haven't been generated yet. If you're unsure about something, "
        "use the ask_user_for_clarification tool.";
};

} // namespace ai_editor

#endif // AI_AGENT_ORCHESTRATOR_H 