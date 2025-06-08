#include "AIAgentOrchestrator.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include "EditorErrorReporter.h"
#include "LoggingCompatibility.h"

namespace ai_editor {

AIAgentOrchestrator::AIAgentOrchestrator(AIManager& aiManager, UIModel& uiModel, WorkspaceManager& workspaceManager, std::shared_ptr<CodeContextProvider> codeContextProvider)
    : aiManager_(aiManager)
    , uiModel_(uiModel)
    , workspaceManager_(workspaceManager)
    , state_(State::IDLE)
    , codeContextProvider_(codeContextProvider)
    , currentLine_(0)
    , currentColumn_(0)
    , contextAwarePromptsEnabled_(false)
{
    // Initialize with a system message
    messages_.push_back(Message(Message::Role::SYSTEM, 
        "You are an AI-powered coding assistant that helps generate code based on user requests. "
        "You can help create new projects, implement features, debug issues, and provide guidance. "
        "Always provide helpful, informative, and accurate responses."));

    // Initialize default context options
    ContextOptions options;
    options.includeDefinitions = true;
    options.includeReferences = true;
    options.includeRelationships = true;
    options.maxSymbols = 10;
    options.maxRelatedFiles = 5;
    options.maxSnippets = 15;
    options.minRelevanceScore = 0.3f;
    options.maxTokens = 2000;
    options.symbolScopeDepth = 2;
    
    if (codeContextProvider_) {
        codeContextProvider_->setContextOptions(options);
    }
}

AIAgentOrchestrator::~AIAgentOrchestrator() {
    // Clean up resources if needed
}

void AIAgentOrchestrator::setCodeContextProvider(std::shared_ptr<CodeContextProvider> contextProvider) {
    codeContextProvider_ = contextProvider;
    
    // Transfer current options to the new provider
    if (codeContextProvider_) {
        codeContextProvider_->setContextOptions(getContextOptions());
    }
}

void AIAgentOrchestrator::enableContextAwarePrompts(bool enable) {
    contextAwarePromptsEnabled_ = enable;
}

void AIAgentOrchestrator::updateEditingContext(const std::string& filePath, int line, int column, 
                                             const std::string& selectedText, 
                                             const std::vector<std::string>& visibleFiles) {
    currentFilePath_ = filePath;
    currentLine_ = line;
    currentColumn_ = column;
    currentSelectedText_ = selectedText;
    currentVisibleFiles_ = visibleFiles;
}

// Enhanced context options
void AIAgentOrchestrator::setContextOptions(const ContextOptions& options) {
    if (codeContextProvider_) {
        codeContextProvider_->setContextOptions(options);
    }
}

ContextOptions AIAgentOrchestrator::getContextOptions() const {
    if (codeContextProvider_) {
        return codeContextProvider_->getContextOptions();
    }
    return ContextOptions(); // Return default if no provider
}

void AIAgentOrchestrator::setMaxTokens(int maxTokens) {
    ContextOptions options = getContextOptions();
    options.maxTokens = maxTokens;
    setContextOptions(options);
}

void AIAgentOrchestrator::setMinRelevanceScore(float minScore) {
    ContextOptions options = getContextOptions();
    options.minRelevanceScore = minScore;
    setContextOptions(options);
}

void AIAgentOrchestrator::setMaxRelatedSymbols(int maxSymbols) {
    ContextOptions options = getContextOptions();
    options.maxSymbols = maxSymbols;
    setContextOptions(options);
}

void AIAgentOrchestrator::setMaxRelatedFiles(int maxFiles) {
    ContextOptions options = getContextOptions();
    options.maxRelatedFiles = maxFiles;
    setContextOptions(options);
}

void AIAgentOrchestrator::setMaxCodeSnippets(int maxSnippets) {
    ContextOptions options = getContextOptions();
    options.maxSnippets = maxSnippets;
    setContextOptions(options);
}

void AIAgentOrchestrator::setScopeDepth(int depth) {
    ContextOptions options = getContextOptions();
    options.symbolScopeDepth = depth;
    setContextOptions(options);
}

void AIAgentOrchestrator::setIncludeDefinitions(bool include) {
    ContextOptions options = getContextOptions();
    options.includeDefinitions = include;
    setContextOptions(options);
}

void AIAgentOrchestrator::setIncludeReferences(bool include) {
    ContextOptions options = getContextOptions();
    options.includeReferences = include;
    setContextOptions(options);
}

void AIAgentOrchestrator::setIncludeRelationships(bool include) {
    ContextOptions options = getContextOptions();
    options.includeRelationships = include;
    setContextOptions(options);
}

// Custom relevance scorers
void AIAgentOrchestrator::registerSymbolRelevanceScorer(const std::string& name, ai_editor::SymbolRelevanceScorer scorer) {
    if (codeContextProvider_) {
        codeContextProvider_->registerSymbolRelevanceScorer(name, scorer);
    } else {
        EditorErrorReporter::reportError("Cannot register symbol relevance scorer: CodeContextProvider not set");
    }
}

void AIAgentOrchestrator::registerFileRelevanceScorer(const std::string& name, ai_editor::FileRelevanceScorer scorer) {
    if (codeContextProvider_) {
        codeContextProvider_->registerFileRelevanceScorer(name, scorer);
    } else {
        EditorErrorReporter::reportError("Cannot register file relevance scorer: CodeContextProvider not set");
    }
}

void AIAgentOrchestrator::handleSubmitUserPrompt(const std::string& userInput)
{
    try {
        // If we're in ERROR state, reset first
        if (state_ == State::AI_ERROR) {
            uiModel_.AddSystemMessage("Recovering from previous error state before processing new prompt.");
            resetState();
        }
        
        // Update the UI model with the user's message
        uiModel_.AddUserMessage(userInput);
        
        // Update the global status to indicate AI processing
        uiModel_.currentGlobalStatus = "AI is processing your request...";
        uiModel_.aiIsProcessing = true;
        
        // Set the orchestrator state
        state_ = State::AWAITING_AI_RESPONSE;
        
        // Enrich the prompt with context if enabled
        std::string enrichedPrompt = enrichPromptWithContext(userInput);
        
        // Add the user message to the conversation history
        messages_.push_back(Message(Message::Role::USER, enrichedPrompt));
        
        // Define tools that the AI can use
        std::vector<ToolDefinition> tools;
        
        // Add write_file tool
        ToolDefinition writeFileTool;
        writeFileTool.name = "write_file";
        writeFileTool.description = "Write content to a file in the workspace";
        writeFileTool.schema = R"({
            "type": "object",
            "properties": {
                "filename": {
                    "type": "string",
                    "description": "The name of the file to write"
                },
                "content": {
                    "type": "string",
                    "description": "The content to write to the file"
                }
            },
            "required": ["filename", "content"]
        })";
        tools.push_back(writeFileTool);
        
        // Send the request to the AI provider through AIManager
        CompletionResponse response = aiManager_.sendCompletionRequest(messages_, tools);
        
        // Process the AI response
        processAIResponse(response);
    }
    catch (const std::exception& e) {
        // Handle any exceptions
        state_ = State::AI_ERROR;
        uiModel_.currentGlobalStatus = "Error processing request";
        uiModel_.aiIsProcessing = false;
        
        std::string errorMessage = "Error processing your request: " + std::string(e.what());
        uiModel_.AddSystemMessage(errorMessage);
        
        EditorErrorReporter::reportError(
            "AIAgentOrchestrator",
            "Error processing user prompt: " + std::string(e.what()) + ". Check the AI provider and configuration.",
            3 // ERROR severity level
        );
    }
}

void AIAgentOrchestrator::handleUserFeedback(const std::string& userFeedback)
{
    try {
        if (state_ != State::AWAITING_APPROVAL) {
            uiModel_.AddSystemMessage("Not waiting for approval, treating as new prompt.");
            handleSubmitUserPrompt(userFeedback);
            return;
        }
        
        // Add the user feedback to the UI
        uiModel_.AddUserMessage(userFeedback);
        
        // Add the user feedback to the conversation history
        messages_.push_back(Message(Message::Role::USER, userFeedback));
        
        // Update status
        uiModel_.currentGlobalStatus = "AI is processing your feedback...";
        uiModel_.aiIsProcessing = true;
        
        // Change state
        state_ = State::AWAITING_AI_RESPONSE;
        
        // Define tools for this context
        std::vector<ToolDefinition> tools;
        
        // Send the request to the AI provider
        CompletionResponse response = aiManager_.sendCompletionRequest(messages_, tools);
        
        // Process the AI response
        processAIResponse(response);
        
    } catch (const std::exception& e) {
        uiModel_.AddSystemMessage("Error processing feedback: " + std::string(e.what()));
        uiModel_.currentGlobalStatus = "Error occurred";
        uiModel_.aiIsProcessing = false;
        state_ = State::AI_ERROR;
    }
}

void AIAgentOrchestrator::handleUserFeedbackDuringExecution(const std::string& userFeedback)
{
    try {
        if (state_ != State::EXECUTING_TASK) {
            uiModel_.AddSystemMessage("Not executing task, treating as new prompt.");
            handleSubmitUserPrompt(userFeedback);
            return;
        }
        
        // Add the user feedback to the UI
        uiModel_.AddUserMessage(userFeedback);
        
        // Add the user feedback to the conversation history
        messages_.push_back(Message(Message::Role::USER, userFeedback));
        
        // Update status
        uiModel_.currentGlobalStatus = "AI is processing your feedback...";
        uiModel_.aiIsProcessing = true;
        
        // Change state
        state_ = State::AWAITING_AI_RESPONSE;
        
        // Define tools for this context
        std::vector<ToolDefinition> tools;
        
        // Add write_file tool during execution
        ToolDefinition writeFileTool;
        writeFileTool.name = "write_file";
        writeFileTool.description = "Write content to a file in the workspace";
        writeFileTool.schema = R"({
            "type": "object",
            "properties": {
                "filename": {
                    "type": "string",
                    "description": "The name of the file to write"
                },
                "content": {
                    "type": "string",
                    "description": "The content to write to the file"
                }
            },
            "required": ["filename", "content"]
        })";
        tools.push_back(writeFileTool);
        
        // Send the request to the AI provider
        CompletionResponse response = aiManager_.sendCompletionRequest(messages_, tools);
        
        // Process the AI response
        processAIResponse(response);
        
    } catch (const std::exception& e) {
        uiModel_.AddSystemMessage("Error processing feedback: " + std::string(e.what()));
        uiModel_.currentGlobalStatus = "Error occurred";
        uiModel_.aiIsProcessing = false;
        state_ = State::AI_ERROR;
    }
}

std::string AIAgentOrchestrator::getCurrentProviderType() const
{
    return aiManager_.getActiveProviderType();
}

bool AIAgentOrchestrator::setActiveProvider(const std::string& providerType)
{
    if (aiManager_.setActiveProvider(providerType)) {
        uiModel_.SetCurrentModel(providerType, aiManager_.getCurrentModelInfo().id);
        return true;
    }
    return false;
}

ModelInfo AIAgentOrchestrator::getCurrentModelInfo() const
{
    return aiManager_.getCurrentModelInfo();
}

bool AIAgentOrchestrator::setCurrentModel(const std::string& modelId)
{
    if (aiManager_.setCurrentModel(modelId)) {
        uiModel_.SetCurrentModel(aiManager_.getActiveProviderType(), modelId);
        return true;
    }
    return false;
}

void AIAgentOrchestrator::resetState()
{
    // Reset the state to IDLE
    state_ = State::IDLE;
    
    // Clear conversation history but keep system message
    Message systemMessage = messages_.front();
    messages_.clear();
    messages_.push_back(systemMessage);
    
    // Update UI model
    uiModel_.currentGlobalStatus = "Orchestrator reset - Ready for new task";
    uiModel_.aiIsProcessing = false;
    uiModel_.AddSystemMessage("Error state cleared. You can start a new coding task.");
}

bool AIAgentOrchestrator::configureLocalLlamaProvider(const std::string& modelPath)
{
    // First try to initialize the LLama provider
    if (aiManager_.initializeLocalLlamaProvider(modelPath)) {
        // Set the provider as active
        if (aiManager_.setActiveProvider("llama")) {
            // Get the current model info
            ModelInfo currentModel = aiManager_.getCurrentModelInfo();
            if (!currentModel.id.empty()) {
                // Update the UI model
                uiModel_.SetCurrentModel("llama", currentModel.id);
                uiModel_.AddSystemMessage("Local LLama model configured: " + currentModel.name);
                return true;
            }
        }
    }
    
    uiModel_.AddSystemMessage("Failed to configure local LLama model. Check the model path and try again.");
    return false;
}

void AIAgentOrchestrator::processAIResponse(const CompletionResponse& response)
{
    try {
        // Reset AI processing flag
        uiModel_.aiIsProcessing = false;
        
        if (response.status != ai_editor::CompletionResponse::Status::SUCCESS) {
            // Handle error
            uiModel_.AddSystemMessage("Error from AI provider: " + response.errorMessage);
            uiModel_.currentGlobalStatus = "Error from AI provider";
            state_ = State::AI_ERROR;
            return;
        }
        
        // Add the AI's response to the conversation history
        messages_.push_back(Message(Message::Role::ASSISTANT, response.content));
        
        // Add the response to the UI
        uiModel_.AddAIMessage(response.content);
        
        // Check for tool calls
        if (!response.toolCalls.empty()) {
            if (processToolCalls(response.toolCalls)) {
                // Tool calls processed successfully
                state_ = State::EXECUTING_TASK;
                uiModel_.currentGlobalStatus = "AI is executing task";
            } else {
                // Error processing tool calls
                state_ = State::AI_ERROR;
                uiModel_.currentGlobalStatus = "Error processing AI tool calls";
            }
        } else {
            // No tool calls, just a regular response
            state_ = State::AWAITING_APPROVAL;
            uiModel_.currentGlobalStatus = "AI response received. Waiting for your feedback.";
        }
    } catch (const std::exception& e) {
        uiModel_.AddSystemMessage("Error processing AI response: " + std::string(e.what()));
        uiModel_.currentGlobalStatus = "Error processing AI response";
        state_ = State::AI_ERROR;
    }
}

bool AIAgentOrchestrator::processToolCalls(const std::vector<ToolCall>& toolCalls)
{
    bool allSuccessful = true;
    
    for (const auto& toolCall : toolCalls) {
        try {
            // Add the tool call to the conversation history
            messages_.push_back(Message(
                Message::Role::TOOL,
                "Tool execution result",
                toolCall.name
            ));
            
            if (toolCall.name == "write_file") {
                // Parse the arguments as JSON
                nlohmann::json args = nlohmann::json::parse(toolCall.arguments);
                
                // Extract filename and content
                std::string filename = args["filename"];
                std::string content = args["content"];
                
                // Write the file to the workspace
                if (workspaceManager_.writeFile(filename, content)) {
                    uiModel_.AddSystemMessage("File created: " + filename);
                    
                    // Add or update the file in the UI model
                    bool fileExists = false;
                    for (auto& file : uiModel_.projectFiles) {
                        if (file.filename == filename) {
                            file.status = ProjectFile::StatusToString(ProjectFile::Status::GENERATED);
                            fileExists = true;
                            break;
                        }
                    }
                    
                    if (!fileExists) {
                        uiModel_.AddProjectFile(filename, ProjectFile::Status::GENERATED);
                    }
                } else {
                    uiModel_.AddSystemMessage("Failed to create file: " + filename);
                    allSuccessful = false;
                }
            } else {
                uiModel_.AddSystemMessage("Unknown tool called: " + toolCall.name);
                allSuccessful = false;
            }
        } catch (const std::exception& e) {
            uiModel_.AddSystemMessage("Error processing tool call: " + std::string(e.what()));
            allSuccessful = false;
        }
    }
    
    return allSuccessful;
}

std::string AIAgentOrchestrator::enrichPromptWithContext(const std::string& userPrompt)
{
    if (!contextAwarePromptsEnabled_ || !codeContextProvider_ || currentFilePath_.empty()) {
        return userPrompt;
    }
    
    try {
        // Get context with current options
        CodeContext context = codeContextProvider_->getContext(
            currentFilePath_,
            currentLine_,
            currentColumn_,
            currentSelectedText_,
            currentVisibleFiles_,
            getContextOptions()
        );
        
        // Generate contextual prompt
        return codeContextProvider_->generateContextualPrompt(userPrompt, context, getContextOptions());
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError("Failed to enrich prompt with context: " + std::string(e.what()));
        return userPrompt;
    }
}

} // namespace ai_editor 