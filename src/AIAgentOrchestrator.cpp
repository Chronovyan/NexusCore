#include "AIAgentOrchestrator.h"
#include <iostream>
#include <nlohmann/json.hpp>

namespace ai_editor {

AIAgentOrchestrator::AIAgentOrchestrator(IOpenAI_API_Client& apiClient, UIModel& uiModel, WorkspaceManager& workspaceManager)
    : apiClient_(apiClient)
    , uiModel_(uiModel)
    , workspaceManager_(workspaceManager)
    , orchestratorState_(OrchestratorState::IDLE)
    , lastToolCallId_("")
    , lastToolName_("")
{
    // Initialize the orchestrator
}

void AIAgentOrchestrator::handleSubmitUserPrompt(const std::string& userInput)
{
    // Update the UI model with the user's message
    uiModel_.AddUserMessage(userInput);
    
    // Update the global status to indicate AI processing
    uiModel_.currentGlobalStatus = "AI is processing your request...";
    uiModel_.aiIsProcessing = true;
    
    // Set the orchestrator state
    orchestratorState_ = OrchestratorState::AWAITING_AI_PLAN;
    
    // Clear previous conversation history for a new top-level request
    conversationHistory_.clear();
    
    // Add the system message to the conversation history
    conversationHistory_.emplace_back(
        "system",
        systemMessage_
    );
    
    // Create the contextualized user message
    std::string contextualizedUserInput = 
        "Project Name: Greeter\n"
        "Language: C++\n\n"
        "User Request: " + userInput + "\n\n"
        "Please use the propose_plan tool to outline a structured approach to this project.";
    
    // Add the user message to the conversation history
    conversationHistory_.emplace_back(
        "user",
        contextualizedUserInput
    );
    
    // Define the tools that the AI can use for this call
    std::vector<ApiToolDefinition> tools;
    
    // Add the propose_plan tool
    ApiToolDefinition proposePlanTool("propose_plan", 
        "Propose a structured plan for implementing the requested project. "
        "Include files to create, their purpose, and a step-by-step approach.");
    
    proposePlanTool.function.parameters.push_back({
        "project_name", "string", "Name of the project being created", true
    });
    
    proposePlanTool.function.parameters.push_back({
        "language", "string", "Programming language for the project", true
    });
    
    // Create a parameter for files with proper items definition
    ApiFunctionParameter filesParam;
    filesParam.name = "files";
    filesParam.type = "array";
    filesParam.description = "List of files to be created with their descriptions";
    filesParam.required = true;
    filesParam.items_type = "object";
    filesParam.items_properties = {
        {"filename", "string", "Name of the file to create", true},
        {"description", "string", "Purpose and contents of the file", true}
    };
    proposePlanTool.function.parameters.push_back(filesParam);
    
    // Create a parameter for steps with proper items definition
    ApiFunctionParameter stepsParam;
    stepsParam.name = "steps";
    stepsParam.type = "array";
    stepsParam.description = "Ordered steps to implement the project";
    stepsParam.required = true;
    stepsParam.items_type = "object";
    stepsParam.items_properties = {
        {"step_number", "integer", "The sequence number of this step", true},
        {"description", "string", "What to do in this step", true}
    };
    proposePlanTool.function.parameters.push_back(stepsParam);
    
    proposePlanTool.function.parameters.push_back({
        "description", "string", "Brief description of the project's purpose and functionality", true
    });
    
    tools.push_back(proposePlanTool);
    
    // Add the ask_user_for_clarification tool
    ApiToolDefinition askClarificationTool("ask_user_for_clarification",
        "Ask the user for clarification about their request when requirements are unclear.");
    
    // Create a parameter for questions with proper items definition
    ApiFunctionParameter questionsParam;
    questionsParam.name = "questions";
    questionsParam.type = "array";
    questionsParam.description = "List of specific questions for the user";
    questionsParam.required = true;
    questionsParam.items_type = "string";
    askClarificationTool.function.parameters.push_back(questionsParam);
    
    askClarificationTool.function.parameters.push_back({
        "context", "string", "Explanation of why clarification is needed", true
    });
    
    tools.push_back(askClarificationTool);
    
    // Send the request to the OpenAI API
    ApiResponse response = apiClient_.sendChatCompletionRequest(
        conversationHistory_,
        tools,
        "gpt-4o",  // Use GPT-4o model
        0.7f,      // Temperature
        2000       // Max tokens
    );
    
    // Process the API response
    if (response.success) {
        // Show a success message with a preview of the raw response
        std::string truncatedResponse = response.raw_json_response;
        if (truncatedResponse.length() > 200) {
            truncatedResponse = truncatedResponse.substr(0, 200) + "...";
        }
        
        // Log the raw response for debugging
        std::cout << "Raw API Response: " << response.raw_json_response << std::endl;
        
        // Display any non-tool message content from the AI first
        if (!response.content.empty()) {
            uiModel_.AddAIMessage(response.content);
        }
        
        // Check for tool calls
        if (!response.tool_calls.empty()) {
            // Process the first tool call (in a more complex application, we might handle multiple tool calls)
            const ApiToolCall& firstToolCall = response.tool_calls[0];
            
            // Store the tool call ID for future reference
            lastToolCallId_ = firstToolCall.id;
            lastToolName_ = firstToolCall.function.name;
            
            // Determine which tool was called
            if (firstToolCall.function.name == "propose_plan") {
                // Process the propose_plan tool call
                if (processProposePlanToolCall(firstToolCall)) {
                    // Update the orchestrator state
                    orchestratorState_ = OrchestratorState::AWAITING_USER_FEEDBACK_ON_PLAN;
                    uiModel_.currentGlobalStatus = "AI has proposed a plan. Please review and respond.";
                } else {
                    // Error processing the tool call
                    uiModel_.AddSystemMessage("Error processing the propose_plan tool call.");
                    orchestratorState_ = OrchestratorState::ERROR_STATE;
                    uiModel_.currentGlobalStatus = "Error processing AI response.";
                }
            } else if (firstToolCall.function.name == "ask_user_for_clarification") {
                // Process the ask_user_for_clarification tool call
                if (processAskForClarificationToolCall(firstToolCall)) {
                    // Update the orchestrator state
                    orchestratorState_ = OrchestratorState::AWAITING_USER_CLARIFICATION_BEFORE_PLAN;
                    uiModel_.currentGlobalStatus = "AI needs clarification. Please answer the questions.";
                } else {
                    // Error processing the tool call
                    uiModel_.AddSystemMessage("Error processing the ask_user_for_clarification tool call.");
                    orchestratorState_ = OrchestratorState::ERROR_STATE;
                    uiModel_.currentGlobalStatus = "Error processing AI response.";
                }
            } else {
                // Unknown tool call
                uiModel_.AddSystemMessage("AI called an unknown tool: " + firstToolCall.function.name);
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Unexpected AI response.";
            }
        } else {
            // No tool calls, just content
            if (!response.content.empty()) {
                // We've already displayed the content above, so we just need to update the state
                orchestratorState_ = OrchestratorState::PLAN_RECEIVED_AWAITING_PARSE;
                uiModel_.currentGlobalStatus = "AI responded with text. Please review.";
                
                // Since the AI didn't use the tools as expected, we might want to provide guidance to the user
                uiModel_.AddSystemMessage("The AI didn't provide a structured plan. You may want to ask it to use the propose_plan tool explicitly.");
            } else {
                // No content or tool calls
                uiModel_.AddSystemMessage("AI response contained no content or tool calls.");
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Empty AI response.";
            }
        }
        
        // Add a system message with the raw response for debugging (shortened)
        uiModel_.AddSystemMessage("Received response from AI. Raw: " + truncatedResponse);
    } else {
        // Show an error message
        uiModel_.AddSystemMessage("Error communicating with AI: " + response.error_message);
        
        // Update the global status
        uiModel_.currentGlobalStatus = "Error from AI.";
        
        // Update the orchestrator state
        orchestratorState_ = OrchestratorState::ERROR_STATE;
    }
    
    // Reset the AI processing flag
    uiModel_.aiIsProcessing = false;
}

void AIAgentOrchestrator::handleSubmitUserFeedback(const std::string& userFeedbackText)
{
    // Ensure we're in an appropriate state
    if (orchestratorState_ != OrchestratorState::AWAITING_USER_FEEDBACK_ON_PLAN && 
        orchestratorState_ != OrchestratorState::AWAITING_USER_CLARIFICATION_BEFORE_PLAN &&
        orchestratorState_ != OrchestratorState::AWAITING_USER_CLARIFICATION) {
        uiModel_.AddSystemMessage("Error: Cannot submit feedback in the current state.");
        return;
    }
    
    // Update the UI model with the user's message
    uiModel_.AddUserMessage(userFeedbackText);
    
    // Update the global status to indicate AI processing
    uiModel_.currentGlobalStatus = "Processing your feedback and requesting application preview from AI...";
    uiModel_.aiIsProcessing = true;
    
    // Set the orchestrator state
    orchestratorState_ = OrchestratorState::AWAITING_AI_ABSTRACT_PREVIEW;
    
    // Prepare the messages for the API call
    // First, copy the existing conversation history (including system message and initial user request)
    std::vector<ApiChatMessage> messages = conversationHistory_;
    
    // Add the AI's response as an assistant message (if it's not already there)
    // This usually contains both the content and the tool call
    // We can skip this if we've already added it to the conversation history
    
    // Add a tool message to respond to the AI's tool call
    if (!lastToolCallId_.empty() && !lastToolName_.empty()) {
        // Create a JSON response for the tool message
        nlohmann::json toolResponse = {
            {"user_feedback_received", true},
            {"user_response_summary", "User has provided feedback/answers. Ready for next step."}
        };
        
        // Add the tool message to tell the AI the result of its tool call
        messages.emplace_back(
            "tool",
            toolResponse.dump(),
            lastToolName_
        );
        
        // Set the tool_call_id (this is a special field in the API)
        messages.back().tool_call_id = lastToolCallId_;
    }
    
    // Add the user feedback message with instructions to provide an abstract preview
    std::string nextPromptToAI = 
        "User's response to your previous plan/questions: \n\"" + 
        userFeedbackText + 
        "\"\n\nBased on this, please now provide an abstract preview " +
        "of the application's functionality and user interaction. " +
        "Use the 'provide_abstract_preview' tool.";
    
    messages.emplace_back(
        "user",
        nextPromptToAI
    );
    
    // Define the tools that the AI can use for this call
    std::vector<ApiToolDefinition> tools;
    
    // Add the provide_abstract_preview tool
    ApiToolDefinition abstractPreviewTool("provide_abstract_preview", 
        "Provide an abstract preview of the application highlighting key functionality and interfaces.");
    
    abstractPreviewTool.function.parameters.push_back({
        "overview", "string", "A high-level description of what the application does", true
    });
    
    abstractPreviewTool.function.parameters.push_back({
        "core_functionality", "array", "List of key features and capabilities", true
    });
    
    abstractPreviewTool.function.parameters.push_back({
        "user_interaction", "string", "Description of how users will interact with the application", true
    });
    
    abstractPreviewTool.function.parameters.push_back({
        "technical_highlights", "array", "Notable technical aspects of the implementation", true
    });
    
    abstractPreviewTool.function.parameters.push_back({
        "next_planned_file_to_generate", "string", "The next file that will be generated when the user approves", true
    });
    
    tools.push_back(abstractPreviewTool);
    
    // Add the ask_user_for_clarification tool as a fallback
    ApiToolDefinition askClarificationTool("ask_user_for_clarification",
        "Ask the user for clarification about their request when requirements are unclear.");
    
    // Create a parameter for questions with proper items definition
    ApiFunctionParameter questionsParam;
    questionsParam.name = "questions";
    questionsParam.type = "array";
    questionsParam.description = "List of specific questions for the user";
    questionsParam.required = true;
    questionsParam.items_type = "string";
    askClarificationTool.function.parameters.push_back(questionsParam);
    
    askClarificationTool.function.parameters.push_back({
        "context", "string", "Explanation of why clarification is needed", true
    });
    
    tools.push_back(askClarificationTool);
    
    // Send the request to the OpenAI API
    ApiResponse response = apiClient_.sendChatCompletionRequest(
        messages,
        tools,
        "gpt-4o",  // Use GPT-4o model
        0.7f,      // Temperature
        2000       // Max tokens
    );
    
    // Process the API response
    if (response.success) {
        // Show a success message with a preview of the raw response
        std::string truncatedResponse = response.raw_json_response;
        if (truncatedResponse.length() > 200) {
            truncatedResponse = truncatedResponse.substr(0, 200) + "...";
        }
        
        // Log the raw response for debugging
        std::cout << "Raw API Response (Abstract Preview): " << response.raw_json_response << std::endl;
        
        // Display any non-tool message content from the AI first
        if (!response.content.empty()) {
            uiModel_.AddAIMessage(response.content);
        }
        
        // Check for tool calls
        if (!response.tool_calls.empty()) {
            // Process the first tool call
            const ApiToolCall& firstToolCall = response.tool_calls[0];
            
            // Store the tool call ID for future reference
            lastToolCallId_ = firstToolCall.id;
            lastToolName_ = firstToolCall.function.name;
            
            // Determine which tool was called
            if (firstToolCall.function.name == "provide_abstract_preview") {
                // Process the abstract preview tool call
                if (processProvideAbstractPreviewToolCall(firstToolCall)) {
                    // Update the orchestrator state - now waiting for user approval
                    orchestratorState_ = OrchestratorState::AWAITING_USER_APPROVAL_OF_PREVIEW;
                    uiModel_.currentGlobalStatus = "AI has provided an application preview. Please review and type 'approve preview' or 'yes' to start coding " + nextPlannedFileToGenerate_ + ", or provide feedback.";
                } else {
                    // Error processing the tool call
                    uiModel_.AddSystemMessage("Error processing the provide_abstract_preview tool call.");
                    orchestratorState_ = OrchestratorState::ERROR_STATE;
                    uiModel_.currentGlobalStatus = "Error processing AI response.";
                }
            } else if (firstToolCall.function.name == "ask_user_for_clarification") {
                // Process the ask_user_for_clarification tool call
                if (processAskForClarificationToolCall(firstToolCall)) {
                    // Update the orchestrator state
                    orchestratorState_ = OrchestratorState::AWAITING_USER_CLARIFICATION;
                    uiModel_.currentGlobalStatus = "AI needs clarification before providing a preview. Please answer the questions.";
                } else {
                    // Error processing the tool call
                    uiModel_.AddSystemMessage("Error processing the ask_user_for_clarification tool call.");
                    orchestratorState_ = OrchestratorState::ERROR_STATE;
                    uiModel_.currentGlobalStatus = "Error processing AI response.";
                }
            } else {
                // Unknown tool call
                uiModel_.AddSystemMessage("AI called an unknown tool: " + firstToolCall.function.name);
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Unexpected AI response.";
            }
        } else {
            // No tool calls, just content
            if (!response.content.empty()) {
                // We've already displayed the content above, so we just need to update the state
                uiModel_.currentGlobalStatus = "AI provided a response without using tools. Please review.";
                uiModel_.AddSystemMessage("The AI didn't use the expected tool. You may want to ask it to use the provide_abstract_preview tool explicitly.");
            } else {
                // No content or tool calls
                uiModel_.AddSystemMessage("AI response contained no content or tool calls.");
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Empty AI response.";
            }
        }
        
        // Keep track of the conversation for future interactions
        // Add the most recent messages to the conversation history
        conversationHistory_ = messages;
    } else {
        // Show an error message
        uiModel_.AddSystemMessage("Error communicating with AI: " + response.error_message);
        
        // Update the global status
        uiModel_.currentGlobalStatus = "Error from AI.";
        
        // Update the orchestrator state
        orchestratorState_ = OrchestratorState::ERROR_STATE;
    }
    
    // Reset the AI processing flag
    uiModel_.aiIsProcessing = false;
}

void AIAgentOrchestrator::handleSubmitUserApprovalOfPreview(const std::string& userApprovalText)
{
    // Ensure we're in the correct state
    if (orchestratorState_ != OrchestratorState::AWAITING_USER_APPROVAL_OF_PREVIEW) {
        uiModel_.AddSystemMessage("Error: Cannot process approval in the current state.");
        return;
    }
    
    // Update the UI model with the user's approval message
    uiModel_.AddUserMessage(userApprovalText);
    
    // Add AI acknowledgment message
    uiModel_.AddAIMessage("Great! Starting code generation for " + nextPlannedFileToGenerate_ + "...");
    
    // Update UI status
    uiModel_.currentGlobalStatus = "AI is generating " + nextPlannedFileToGenerate_ + "...";
    uiModel_.aiIsProcessing = true;
    
    // Update the project file status in the UI model
    for (auto& file : uiModel_.projectFiles) {
        if (file.filename == nextPlannedFileToGenerate_) {
            file.status = ProjectFile::StatusToString(ProjectFile::Status::GENERATING);
            break;
        }
    }
    
    // Set the orchestrator state
    orchestratorState_ = OrchestratorState::GENERATING_CODE_FILES;
    
    // Prepare the messages for the API call
    // First, copy the existing conversation history
    std::vector<ApiChatMessage> messages = conversationHistory_;
    
    // Add a tool message to respond to the AI's previous tool call (provide_abstract_preview)
    if (!lastToolCallId_.empty() && !lastToolName_.empty()) {
        // Create a JSON response for the tool message
        nlohmann::json toolResponse = {
            {"user_approved", true},
            {"user_feedback", "User approved the preview: " + userApprovalText}
        };
        
        // Add the tool message to tell the AI the result of its tool call
        messages.emplace_back(
            "tool",
            toolResponse.dump(),
            lastToolName_
        );
        
        // Set the tool_call_id (this is a special field in the API)
        messages.back().tool_call_id = lastToolCallId_;
    }
    
    // Add a user message instructing the AI to generate the file content
    std::string promptForFileGen = 
        std::string("The abstract preview was approved. ") +
        std::string("Please now generate the full content for the file: '") + nextPlannedFileToGenerate_ + "' " +
        "as outlined in your initial plan. Use the 'write_file_content' tool " +
        "and provide an explanation of what the generated code/script does.";
    
    messages.emplace_back(
        "user",
        promptForFileGen
    );
    
    // Define the tools that the AI can use for this call
    std::vector<ApiToolDefinition> tools;
    
    // Add the write_file_content tool
    ApiToolDefinition writeFileContentTool("write_file_content", 
        "Write content for a specified file in the project.");
    
    writeFileContentTool.function.parameters.push_back({
        "filename", "string", "Name of the file to create or modify", true
    });
    
    writeFileContentTool.function.parameters.push_back({
        "content", "string", "Content to write to the file", true
    });
    
    writeFileContentTool.function.parameters.push_back({
        "description", "string", "Brief description of the file's purpose and functionality", true
    });
    
    tools.push_back(writeFileContentTool);
    
    // Add the ask_user_for_clarification tool as a fallback
    ApiToolDefinition askClarificationTool("ask_user_for_clarification",
        "Ask the user for clarification about their request when requirements are unclear.");
    
    // Create a parameter for questions with proper items definition
    ApiFunctionParameter questionsParam;
    questionsParam.name = "questions";
    questionsParam.type = "array";
    questionsParam.description = "List of specific questions for the user";
    questionsParam.required = true;
    questionsParam.items_type = "string";
    askClarificationTool.function.parameters.push_back(questionsParam);
    
    askClarificationTool.function.parameters.push_back({
        "context", "string", "Explanation of why clarification is needed", true
    });
    
    tools.push_back(askClarificationTool);
    
    // Send the request to the OpenAI API
    ApiResponse response = apiClient_.sendChatCompletionRequest(
        messages,
        tools,
        "gpt-4o",  // Use GPT-4o model
        0.7f,      // Temperature
        2000       // Max tokens
    );
    
    // Process the API response
    if (response.success) {
        // Show a success message with a preview of the raw response
        std::string truncatedResponse = response.raw_json_response;
        if (truncatedResponse.length() > 200) {
            truncatedResponse = truncatedResponse.substr(0, 200) + "...";
        }
        
        // Log the raw response for debugging
        std::cout << "Raw API Response (File Content Generation): " << response.raw_json_response << std::endl;
        
        // Display any non-tool message content from the AI first
        if (!response.content.empty()) {
            uiModel_.AddAIMessage(response.content);
        }
        
        // Check for tool calls
        if (!response.tool_calls.empty()) {
            // Process the first tool call
            const ApiToolCall& firstToolCall = response.tool_calls[0];
            
            // Store the tool call ID for future reference
            lastToolCallId_ = firstToolCall.id;
            lastToolName_ = firstToolCall.function.name;
            
            // Determine which tool was called
            if (firstToolCall.function.name == "write_file_content") {
                // Process the write_file_content tool call
                bool processSuccess = processWriteFileContentToolCall(firstToolCall);
                
                // Request the next file or compilation commands
                requestNextFileOrCompilation(
                    firstToolCall.id,
                    firstToolCall.function.name,
                    processSuccess,
                    processSuccess ? nextPlannedFileToGenerate_ : "unknown",
                    processSuccess ? "" : "Failed to write file content"
                );
            } else if (firstToolCall.function.name == "ask_user_for_clarification") {
                // Process the ask_user_for_clarification tool call
                if (processAskForClarificationToolCall(firstToolCall)) {
                    // Update the orchestrator state
                    orchestratorState_ = OrchestratorState::AWAITING_USER_CLARIFICATION;
                    uiModel_.currentGlobalStatus = "AI needs clarification while generating file. Please answer the questions.";
                } else {
                    // Error processing the tool call
                    uiModel_.AddSystemMessage("Error processing the ask_user_for_clarification tool call.");
                    orchestratorState_ = OrchestratorState::ERROR_STATE;
                    uiModel_.currentGlobalStatus = "Error processing AI response.";
                }
            } else {
                // Unknown tool call
                uiModel_.AddSystemMessage("AI called an unknown tool: " + firstToolCall.function.name);
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Unexpected AI response.";
            }
        } else {
            // No tool calls, just content
            if (!response.content.empty()) {
                // We've already displayed the content above, so we just need to update the state
                uiModel_.currentGlobalStatus = "AI provided a response without using tools. Please review.";
                uiModel_.AddSystemMessage("The AI didn't use the expected tool. You may want to ask it to use the write_file_content tool explicitly.");
            } else {
                // No content or tool calls
                uiModel_.AddSystemMessage("AI response contained no content or tool calls.");
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Empty AI response.";
            }
        }
        
        // Keep track of the conversation for future interactions
        // Add the most recent messages to the conversation history
        conversationHistory_ = messages;
    } else {
        // Show an error message
        uiModel_.AddSystemMessage("Error communicating with AI: " + response.error_message);
        
        // Update the global status
        uiModel_.currentGlobalStatus = "Error from AI.";
        
        // Update the orchestrator state
        orchestratorState_ = OrchestratorState::ERROR_STATE;
    }
    
    // Reset the AI processing flag if needed
    if (orchestratorState_ != OrchestratorState::GENERATING_CODE_FILES) {
        uiModel_.aiIsProcessing = false;
    }
}

bool AIAgentOrchestrator::processProposePlanToolCall(const ApiToolCall& toolCall)
{
    try {
        // Parse the arguments JSON
        nlohmann::json argsJson = nlohmann::json::parse(toolCall.function.arguments);
        
        // Extract the project information
        std::string projectName = argsJson["project_name"];
        std::string language = argsJson["language"];
        std::string description = argsJson["description"];
        
        // Add the AI's plan to the chat history
        uiModel_.AddAIMessage("I've analyzed your request and created a plan for the " + projectName + " project:");
        
        // Add the project description
        uiModel_.AddAIMessage("**Project Description**: " + description);
        
        // Clear any existing project files to start fresh with the new plan
        uiModel_.projectFiles.clear();
        
        // Process files
        if (argsJson.contains("files") && argsJson["files"].is_array()) {
            uiModel_.AddAIMessage("**Files to create**:");
            
            for (const auto& file : argsJson["files"]) {
                std::string fileName = file["filename"];
                std::string fileDesc = file["description"];
                
                // Add to UI model's project files
                uiModel_.AddProjectFile(fileName, ProjectFile::Status::PLANNED, fileDesc);
                
                // Add to chat message
                uiModel_.AddAIMessage("- " + fileName + ": " + fileDesc);
            }
        }
        
        // Process steps
        if (argsJson.contains("steps") && argsJson["steps"].is_array()) {
            uiModel_.AddAIMessage("**Implementation Steps**:");
            
            int stepNumber = 1;
            for (const auto& step : argsJson["steps"]) {
                std::string stepDesc = step["description"];
                uiModel_.AddAIMessage(std::to_string(stepNumber) + ". " + stepDesc);
                stepNumber++;
            }
        }
        
        // Add instructions for the user
        uiModel_.AddAIMessage("Does this plan look good? Would you like to proceed with this approach or would you like to make any adjustments?");
        
        // Save the plan for future reference
        lastPlanJson_ = argsJson;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing propose_plan tool call: " << e.what() << std::endl;
        uiModel_.AddSystemMessage("Error parsing plan: " + std::string(e.what()));
        return false;
    }
}

bool AIAgentOrchestrator::processAskForClarificationToolCall(const ApiToolCall& toolCall)
{
    try {
        // Parse the arguments JSON
        nlohmann::json argsJson = nlohmann::json::parse(toolCall.function.arguments);
        
        // Extract the clarification context
        std::string context = argsJson["context"];
        
        // Add the AI's clarification request to the chat history
        uiModel_.AddAIMessage("I need some more information before I can create a plan:");
        uiModel_.AddAIMessage(context);
        
        // Process questions
        if (argsJson.contains("questions") && argsJson["questions"].is_array()) {
            for (const auto& question : argsJson["questions"]) {
                std::string questionText = question;
                uiModel_.AddAIMessage("- " + questionText);
            }
        }
        
        // Add instructions for the user
        uiModel_.AddAIMessage("Please provide the requested information so I can better understand your requirements.");
        
        // Save the questions for future reference
        lastClarificationJson_ = argsJson;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing ask_user_for_clarification tool call: " << e.what() << std::endl;
        uiModel_.AddSystemMessage("Error parsing clarification request: " + std::string(e.what()));
        return false;
    }
}

bool AIAgentOrchestrator::processProvideAbstractPreviewToolCall(const ApiToolCall& toolCall)
{
    try {
        // Parse the arguments JSON
        nlohmann::json argsJson = nlohmann::json::parse(toolCall.function.arguments);
        
        // Extract the overview information
        std::string overview = argsJson["overview"];
        std::string userInteraction = argsJson["user_interaction"];
        
        // Get the next file to generate
        if (argsJson.contains("next_planned_file_to_generate")) {
            nextPlannedFileToGenerate_ = argsJson["next_planned_file_to_generate"];
        } else {
            // Default if not provided
            nextPlannedFileToGenerate_ = "CMakeLists.txt";
        }
        
        // Add the AI's preview to the chat history
        uiModel_.AddAIMessage("**Application Preview**");
        uiModel_.AddAIMessage("**Overview**: " + overview);
        
        // Process core functionality
        if (argsJson.contains("core_functionality") && argsJson["core_functionality"].is_array()) {
            uiModel_.AddAIMessage("**Core Functionality**:");
            
            for (const auto& feature : argsJson["core_functionality"]) {
                std::string featureDesc = feature;
                uiModel_.AddAIMessage("- " + featureDesc);
            }
        }
        
        // Add user interaction section
        uiModel_.AddAIMessage("**User Interaction**: " + userInteraction);
        
        // Process technical highlights
        if (argsJson.contains("technical_highlights") && argsJson["technical_highlights"].is_array()) {
            uiModel_.AddAIMessage("**Technical Highlights**:");
            
            for (const auto& highlight : argsJson["technical_highlights"]) {
                std::string highlightDesc = highlight;
                uiModel_.AddAIMessage("- " + highlightDesc);
            }
        }
        
        // Add next steps for the user
        uiModel_.AddAIMessage("I'm ready to generate the code files for this application. The first file I'll create will be **" + 
                             nextPlannedFileToGenerate_ + "**.");
        uiModel_.AddAIMessage("Please type 'approve preview' or 'yes' to proceed, or provide feedback if you'd like any changes to the design.");
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing provide_abstract_preview tool call: " << e.what() << std::endl;
        uiModel_.AddSystemMessage("Error parsing abstract preview: " + std::string(e.what()));
        return false;
    }
}

bool AIAgentOrchestrator::processWriteFileContentToolCall(const ApiToolCall& toolCall)
{
    try {
        // Parse the arguments JSON
        nlohmann::json argsJson = nlohmann::json::parse(toolCall.function.arguments);
        
        // Extract the file information
        std::string filename = argsJson["filename"];
        std::string content = argsJson["content"];
        std::string explanation = argsJson.value("description", "");
        std::string actionType = argsJson.value("action_type", "create"); // Default to "create" if not specified
        
        // Try to write the file to disk
        bool writeSuccess = workspaceManager_.writeFile(filename, content);
        
        if (writeSuccess) {
            // Update the UI model
            uiModel_.AddAIMessage("I've generated the contents for **" + filename + "**:");
            
            // Add a short preview of the file content, limiting to first few lines
            std::string previewContent = content;
            size_t maxPreviewLength = 300;
            if (previewContent.length() > maxPreviewLength) {
                size_t cutoffPos = previewContent.find('\n', maxPreviewLength);
                if (cutoffPos == std::string::npos) {
                    cutoffPos = maxPreviewLength;
                }
                previewContent = previewContent.substr(0, cutoffPos) + "\n... (file continues)";
            }
            
            uiModel_.AddAIMessage("```\n" + previewContent + "\n```");
            
            // Add the file explanation if provided
            if (!explanation.empty()) {
                uiModel_.AddAIMessage("**File Description**: " + explanation);
            }
            
            // Update the project file status in the UI model
            bool fileFound = false;
            for (auto& file : uiModel_.projectFiles) {
                if (file.filename == filename) {
                    file.status = ProjectFile::StatusToString(ProjectFile::Status::GENERATED);
                    fileFound = true;
                    break;
                }
            }
            
            // If this is a new file not in the original plan, add it to the project files
            if (!fileFound) {
                uiModel_.AddProjectFile(filename, ProjectFile::Status::GENERATED, explanation);
            }
            
            // Add the file to our list of generated files
            generatedFiles_.push_back(filename);
            
            // Update the global status
            uiModel_.currentGlobalStatus = filename + " generated successfully. Preparing next step...";
            
            // Add a system message
            uiModel_.AddSystemMessage("Editor: File " + filename + " saved successfully.");
            
            return true;
        } else {
            // File write failed
            uiModel_.AddSystemMessage("Editor Error: Failed to save " + filename);
            
            // Update the project file status in the UI model
            for (auto& file : uiModel_.projectFiles) {
                if (file.filename == filename) {
                    file.status = ProjectFile::StatusToString(ProjectFile::Status::ERROR);
                    break;
                }
            }
            
            // Update the global status
            uiModel_.currentGlobalStatus = "Error saving file " + filename;
            
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing write_file_content tool call: " << e.what() << std::endl;
        uiModel_.AddSystemMessage("Error parsing file content: " + std::string(e.what()));
        return false;
    }
}

std::string AIAgentOrchestrator::determineNextFileToGenerate() const
{
    // If we have the plan JSON, check the files array
    if (lastPlanJson_.contains("files") && lastPlanJson_["files"].is_array()) {
        for (const auto& file : lastPlanJson_["files"]) {
            std::string filename = file["filename"];
            
            // Check if this file has already been generated
            bool alreadyGenerated = false;
            for (const auto& generatedFile : generatedFiles_) {
                if (generatedFile == filename) {
                    alreadyGenerated = true;
                    break;
                }
            }
            
            // If this file hasn't been generated yet, return it
            if (!alreadyGenerated) {
                return filename;
            }
        }
    }
    
    // If no files in the plan or all files have been generated, return empty string
    return "";
}

void AIAgentOrchestrator::requestNextFileOrCompilation(
    const std::string& previousToolCallId, 
    const std::string& previousToolName, 
    bool successResult, 
    const std::string& filename, 
    const std::string& errorMessage)
{
    // Construct messages to send to the API
    std::vector<ApiChatMessage> messages = conversationHistory_;
    
    // Add a tool message to respond to the previous tool call
    if (!previousToolCallId.empty() && !previousToolName.empty()) {
        // Create a JSON response for the tool message
        nlohmann::json toolResponse;
        if (successResult) {
            toolResponse = {
                {"success", true},
                {"filename", filename},
                {"message", "File written successfully by editor."}
            };
        } else {
            toolResponse = {
                {"success", false},
                {"filename", filename},
                {"error", errorMessage.empty() ? "Editor failed to write file." : errorMessage}
            };
        }
        
        // Add the tool message to tell the AI the result of its tool call
        messages.emplace_back(
            "tool",
            toolResponse.dump(),
            previousToolName
        );
        
        // Set the tool_call_id (this is a special field in the API)
        messages.back().tool_call_id = previousToolCallId;
    }
    
    // Determine if there are more files to generate
    std::string nextFile = determineNextFileToGenerate();
    
    if (!nextFile.empty()) {
        // There are more files to generate
        std::string promptForNextFile = 
            "File " + filename + " was " + (successResult ? "saved successfully" : "not saved due to an error") + 
            ". Please now generate the content for '" + nextFile + "' " +
            "using the 'write_file_content' tool and provide an explanation.";
        
        messages.emplace_back(
            "user",
            promptForNextFile
        );
        
        // Update the global status
        uiModel_.currentGlobalStatus = "Requesting AI to generate " + nextFile + "...";
        uiModel_.aiIsProcessing = true;
        
        // Set next file to generate
        nextPlannedFileToGenerate_ = nextFile;
        
        // Update the project file status in the UI model
        for (auto& file : uiModel_.projectFiles) {
            if (file.filename == nextFile) {
                file.status = ProjectFile::StatusToString(ProjectFile::Status::GENERATING);
                break;
            }
        }
    } else {
        // All files have been generated, request compilation commands
        std::string promptForCompile = 
            std::string("All planned files have been generated and saved successfully. ") +
            std::string("Please now provide the necessary shell command(s) to compile the entire project ") +
            "using the build script (e.g., `CMakeLists.txt`) you defined. Call the `execute_system_command` tool with `command_type: 'compile'`.";
        
        messages.emplace_back(
            "user",
            promptForCompile
        );
        
        // Update the global status
        uiModel_.currentGlobalStatus = "All files generated. Requesting AI for compilation instructions...";
        uiModel_.aiIsProcessing = true;
        
        // Update orchestrator state
        orchestratorState_ = OrchestratorState::AWAITING_AI_COMPILE_COMMANDS;
    }
    
    // Define the tools that the AI can use for this call
    std::vector<ApiToolDefinition> tools;
    
    if (!nextFile.empty()) {
        // If generating another file, provide the write_file_content tool
        ApiToolDefinition writeFileContentTool("write_file_content", 
            "Write content for a specified file in the project.");
        
        writeFileContentTool.function.parameters.push_back({
            "filename", "string", "Name of the file to create or modify", true
        });
        
        writeFileContentTool.function.parameters.push_back({
            "content", "string", "Content to write to the file", true
        });
        
        writeFileContentTool.function.parameters.push_back({
            "description", "string", "Brief description of the file's purpose and functionality", true
        });
        
        writeFileContentTool.function.parameters.push_back({
            "action_type", "string", "Whether to 'create' a new file or 'update' an existing one", false
        });
        
        tools.push_back(writeFileContentTool);
    } else {
        // If requesting compilation, provide the execute_system_command tool
        ApiToolDefinition executeCmdTool("execute_system_command", 
            "Execute a system command to compile, test, or run the generated code.");
        
        executeCmdTool.function.parameters.push_back({
            "command", "string", "The command to execute", true
        });
        
        executeCmdTool.function.parameters.push_back({
            "command_type", "string", "Type of command: 'compile', 'test', or 'run'", true
        });
        
        executeCmdTool.function.parameters.push_back({
            "explanation", "string", "Explanation of what this command does", true
        });
        
        tools.push_back(executeCmdTool);
    }
    
    // Add the ask_user_for_clarification tool as a fallback
    ApiToolDefinition askClarificationTool("ask_user_for_clarification",
        "Ask the user for clarification about their request when requirements are unclear.");
    
    // Create a parameter for questions with proper items definition
    ApiFunctionParameter questionsParam;
    questionsParam.name = "questions";
    questionsParam.type = "array";
    questionsParam.description = "List of specific questions for the user";
    questionsParam.required = true;
    questionsParam.items_type = "string";
    askClarificationTool.function.parameters.push_back(questionsParam);
    
    askClarificationTool.function.parameters.push_back({
        "context", "string", "Explanation of why clarification is needed", true
    });
    
    tools.push_back(askClarificationTool);
    
    // Send the request to the OpenAI API
    ApiResponse response = apiClient_.sendChatCompletionRequest(
        messages,
        tools,
        "gpt-4o",  // Use GPT-4o model
        0.7f,      // Temperature
        2000       // Max tokens
    );
    
    // Process the API response
    if (response.success) {
        // Keep track of the conversation for future interactions
        conversationHistory_ = messages;
        
        // Display any non-tool message content from the AI first
        if (!response.content.empty()) {
            uiModel_.AddAIMessage(response.content);
        }
        
        // Check for tool calls
        if (!response.tool_calls.empty()) {
            // Process the first tool call
            const ApiToolCall& firstToolCall = response.tool_calls[0];
            
            // Store the tool call ID for future reference
            lastToolCallId_ = firstToolCall.id;
            lastToolName_ = firstToolCall.function.name;
            
            // Determine which tool was called
            if (firstToolCall.function.name == "write_file_content") {
                // Process the write_file_content tool call
                bool processSuccess = processWriteFileContentToolCall(firstToolCall);
                
                // Request the next file or compilation commands
                requestNextFileOrCompilation(
                    firstToolCall.id,
                    firstToolCall.function.name,
                    processSuccess,
                    processSuccess ? nextPlannedFileToGenerate_ : "unknown",
                    processSuccess ? "" : "Failed to write file content"
                );
            } else if (firstToolCall.function.name == "execute_system_command") {
                // TODO: Implement processing of execute_system_command tool call
                // For this task, we're focusing on the write_file_content flow
                // This would be handled in a similar way as processWriteFileContentToolCall
                
                // Update the orchestrator state
                orchestratorState_ = OrchestratorState::COMPILATION_IN_PROGRESS;
                uiModel_.currentGlobalStatus = "Executing compilation command...";
                
                // For now, just acknowledge the command
                uiModel_.AddSystemMessage("Received compilation command from AI.");
            } else if (firstToolCall.function.name == "ask_user_for_clarification") {
                // Process the ask_user_for_clarification tool call
                if (processAskForClarificationToolCall(firstToolCall)) {
                    // Update the orchestrator state
                    orchestratorState_ = OrchestratorState::AWAITING_USER_CLARIFICATION;
                    uiModel_.currentGlobalStatus = "AI needs clarification. Please answer the questions.";
                } else {
                    // Error processing the tool call
                    uiModel_.AddSystemMessage("Error processing the ask_user_for_clarification tool call.");
                    orchestratorState_ = OrchestratorState::ERROR_STATE;
                    uiModel_.currentGlobalStatus = "Error processing AI response.";
                }
            } else {
                // Unknown tool call
                uiModel_.AddSystemMessage("AI called an unknown tool: " + firstToolCall.function.name);
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Unexpected AI response.";
            }
        } else {
            // No tool calls, just content
            if (!response.content.empty()) {
                // We've already displayed the content above, so we just need to update the state
                uiModel_.currentGlobalStatus = "AI provided a response without using tools. Please review.";
                
                // Provide guidance based on what we were expecting
                if (!nextFile.empty()) {
                    uiModel_.AddSystemMessage("The AI didn't use the expected write_file_content tool. You may want to ask it to use this tool explicitly.");
                } else {
                    uiModel_.AddSystemMessage("The AI didn't use the expected execute_system_command tool. You may want to ask it to use this tool explicitly.");
                }
            } else {
                // No content or tool calls
                uiModel_.AddSystemMessage("AI response contained no content or tool calls.");
                orchestratorState_ = OrchestratorState::ERROR_STATE;
                uiModel_.currentGlobalStatus = "Empty AI response.";
            }
        }
    } else {
        // Show an error message
        uiModel_.AddSystemMessage("Error communicating with AI: " + response.error_message);
        
        // Update the global status
        uiModel_.currentGlobalStatus = "Error from AI.";
        
        // Update the orchestrator state
        orchestratorState_ = OrchestratorState::ERROR_STATE;
        
        // Reset the AI processing flag
        uiModel_.aiIsProcessing = false;
    }
}

} // namespace ai_editor 