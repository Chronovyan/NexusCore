#include <gtest/gtest.h>
#include "../src/AIAgentOrchestrator.h"
#include "../src/MockOpenAI_API_Client.h"
#include "../src/WorkspaceManager.h"
#include "../src/UIModel.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace ai_editor;
using json = nlohmann::json;

// Test fixture for AIAgentOrchestrator tests
class AIAgentOrchestratorTest : public ::testing::Test {
protected:
    MockOpenAI_API_Client mockApiClient;
    UIModel uiModel;
    std::string testWorkspacePath;
    WorkspaceManager* workspaceManager;
    AIAgentOrchestrator* orchestrator;

    void SetUp() override {
        // Create a temporary test workspace directory
        testWorkspacePath = "test_workspace";
        std::filesystem::create_directory(testWorkspacePath);
        
        workspaceManager = new WorkspaceManager(testWorkspacePath);
        orchestrator = new AIAgentOrchestrator(mockApiClient, uiModel, *workspaceManager);
    }

    void TearDown() override {
        delete orchestrator;
        delete workspaceManager;
        
        // Clean up the test workspace directory and any files
        std::filesystem::remove_all(testWorkspacePath);
    }
    
    // Helper method to simulate an AI response with a write_file_content tool call
    ApiResponse createWriteFileContentToolResponse(
        const std::string& filename,
        const std::string& content,
        const std::string& explanation = "This is a test file.",
        const std::string& action_type = "create"
    ) {
        ApiToolCall toolCall;
        toolCall.id = "call_123";
        toolCall.function.name = "write_file_content";
        
        // Create arguments for the tool call
        json args = {
            {"filename", filename},
            {"content", content},
            {"description", explanation}
        };
        
        // Only add action_type if it's specified (to test default value handling)
        if (!action_type.empty()) {
            args["action_type"] = action_type;
        }
        
        toolCall.function.arguments = args.dump();
        
        // Create the API response
        ApiResponse response;
        response.success = true;
        response.content = "Here's the generated content for " + filename;
        response.tool_calls.push_back(toolCall);
        
        return response;
    }
    
    // Helper method to simulate an AI response with a tool result and request for next file
    ApiResponse createNextFileToolResponse(
        const std::string& nextFilename,
        const std::string& nextContent,
        const std::string& nextExplanation = "This is the next file."
    ) {
        ApiToolCall toolCall;
        toolCall.id = "call_456";
        toolCall.function.name = "write_file_content";
        
        // Create arguments for the tool call
        json args = {
            {"filename", nextFilename},
            {"content", nextContent},
            {"description", nextExplanation}
        };
        
        toolCall.function.arguments = args.dump();
        
        // Create the API response
        ApiResponse response;
        response.success = true;
        response.content = "Here's the generated content for the next file: " + nextFilename;
        response.tool_calls.push_back(toolCall);
        
        return response;
    }
    
    // Helper method to simulate an AI response for compilation
    ApiResponse createCompilationToolResponse() {
        ApiToolCall toolCall;
        toolCall.id = "call_789";
        toolCall.function.name = "execute_system_command";
        
        // Create arguments for the tool call
        json args = {
            {"command", "cmake . && make"},
            {"command_type", "compile"},
            {"explanation", "This command will build the project using CMake and Make."}
        };
        
        toolCall.function.arguments = args.dump();
        
        // Create the API response
        ApiResponse response;
        response.success = true;
        response.content = "Here are the compilation commands for the project.";
        response.tool_calls.push_back(toolCall);
        
        return response;
    }
};

// Test case for processing a write_file_content tool call with more files to generate
TEST_F(AIAgentOrchestratorTest, ProcessWriteFileContentWithMoreFiles) {
    // Set up the orchestrator state to be generating code files
    orchestrator->handleSubmitUserPrompt("Create a simple C++ project with a main.cpp file and CMakeLists.txt");
    
    // Add planned files to UIModel to simulate a received plan
    uiModel.projectFiles.clear();
    uiModel.projectFiles.emplace_back("main.cpp", ProjectFile::StatusToString(ProjectFile::Status::PLANNED));
    uiModel.projectFiles.emplace_back("CMakeLists.txt", ProjectFile::StatusToString(ProjectFile::Status::PLANNED));
    
    // Inject plan data into the orchestrator by simulating a plan response
    json planJson = {
        {"files", json::array({
            {{"name", "main.cpp"}, {"description", "Main entry point for the application."}},
            {{"name", "CMakeLists.txt"}, {"description", "Build configuration file."}}
        })}
    };
    
    ApiToolCall planToolCall;
    planToolCall.id = "plan_call_123";
    planToolCall.function.name = "propose_plan";
    planToolCall.function.arguments = planJson.dump();
    
    ApiResponse planResponse;
    planResponse.success = true;
    planResponse.content = "Here's my plan for your C++ project.";
    planResponse.tool_calls.push_back(planToolCall);
    
    mockApiClient.primeResponse(planResponse);
    
    // Now prime the mock API client with a response for the main.cpp file
    ApiResponse mainFileResponse = createWriteFileContentToolResponse(
        "main.cpp",
        "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}",
        "Main entry point for the application."
    );
    mockApiClient.primeResponse(mainFileResponse);
    
    // Process the plan to transition to the AWAITING_USER_APPROVAL_OF_PREVIEW state
    orchestrator->handleSubmitUserFeedback("The plan looks good.");
    
    // Process user approval to start generating files
    orchestrator->handleSubmitUserApprovalOfPreview("I approve this preview.");
    
    // Prime the mock API client with a response for the next file request (CMakeLists.txt)
    ApiResponse cmakeFileResponse = createNextFileToolResponse(
        "CMakeLists.txt",
        "cmake_minimum_required(VERSION 3.10)\nproject(SimpleProject)\n\nadd_executable(SimpleProject main.cpp)",
        "Build configuration file for CMake."
    );
    mockApiClient.primeResponse(cmakeFileResponse);
    
    // Verify that the main.cpp file was written to disk
    EXPECT_TRUE(std::filesystem::exists(testWorkspacePath + "/main.cpp"));
    
    // Verify that the UI model was updated correctly for main.cpp
    bool mainCppFound = false;
    for (const auto& file : uiModel.projectFiles) {
        if (file.filename == "main.cpp") {
            EXPECT_EQ(file.status, ProjectFile::StatusToString(ProjectFile::Status::GENERATED));
            mainCppFound = true;
            break;
        }
    }
    EXPECT_TRUE(mainCppFound);
    
    // Verify that messages were sent to the OpenAI API client with the correct tool results for main.cpp
    EXPECT_GE(mockApiClient.last_sent_messages_.size(), 2);
    
    // Check if at least one of the messages is a tool result for main.cpp
    bool foundMainCppToolResult = false;
    for (const auto& message : mockApiClient.last_sent_messages_) {
        if (message.role == "tool") {
            try {
                json toolResult = json::parse(message.content);
                if (toolResult.contains("filename") && toolResult["filename"] == "main.cpp") {
                    EXPECT_TRUE(toolResult["success"].get<bool>());
                    foundMainCppToolResult = true;
                    break;
                }
            } catch (...) {
                // Not a valid JSON or doesn't contain the expected fields
                continue;
            }
        }
    }
    EXPECT_TRUE(foundMainCppToolResult);
    
    // Check if at least one of the messages is requesting the CMakeLists.txt file
    bool requestingCMakeLists = false;
    for (const auto& message : mockApiClient.last_sent_messages_) {
        if (message.role == "user" && message.content.find("CMakeLists.txt") != std::string::npos) {
            requestingCMakeLists = true;
            break;
        }
    }
    EXPECT_TRUE(requestingCMakeLists);
    
    // Prime the mock API client with a compilation response (for after CMakeLists.txt is generated)
    ApiResponse compileResponse = createCompilationToolResponse();
    mockApiClient.primeResponse(compileResponse);
    
    // Check if CMakeLists.txt was written to disk after processing
    EXPECT_TRUE(std::filesystem::exists(testWorkspacePath + "/CMakeLists.txt"));
    
    // Verify that the UI model was updated correctly for CMakeLists.txt
    bool cmakeListsFound = false;
    for (const auto& file : uiModel.projectFiles) {
        if (file.filename == "CMakeLists.txt") {
            EXPECT_EQ(file.status, ProjectFile::StatusToString(ProjectFile::Status::GENERATED));
            cmakeListsFound = true;
            break;
        }
    }
    EXPECT_TRUE(cmakeListsFound);
    
    // Verify that compilation commands were requested after the last file was generated
    bool requestingCompilation = false;
    for (const auto& message : mockApiClient.last_sent_messages_) {
        if (message.role == "user" && message.content.find("compile") != std::string::npos) {
            requestingCompilation = true;
            break;
        }
    }
    EXPECT_TRUE(requestingCompilation);
    
    // Verify orchestrator's final state
    EXPECT_EQ(orchestrator->getCurrentState(), AIAgentOrchestrator::OrchestratorState::AWAITING_AI_COMPILE_COMMANDS);
}

// Test case for processing the last file and requesting compilation
TEST_F(AIAgentOrchestratorTest, ProcessLastFileAndRequestCompilation) {
    // Setup the orchestrator state to be generating code files
    orchestrator->handleSubmitUserPrompt("Create a minimal C++ project with only a main.cpp file");
    
    // Set up the UIModel with only one planned file
    uiModel.projectFiles.clear();
    uiModel.projectFiles.emplace_back("main.cpp", ProjectFile::StatusToString(ProjectFile::Status::PLANNED));
    
    // Inject plan data into the orchestrator by simulating a plan response
    json planJson = {
        {"files", json::array({
            {{"name", "main.cpp"}, {"description", "Simple main.cpp file for a minimal C++ project."}}
        })}
    };
    
    ApiToolCall planToolCall;
    planToolCall.id = "plan_call_456";
    planToolCall.function.name = "propose_plan";
    planToolCall.function.arguments = planJson.dump();
    
    ApiResponse planResponse;
    planResponse.success = true;
    planResponse.content = "Here's my plan for your minimal C++ project.";
    planResponse.tool_calls.push_back(planToolCall);
    
    mockApiClient.primeResponse(planResponse);
    
    // Process the plan
    orchestrator->handleSubmitUserFeedback("The plan looks good.");
    
    // Prime the mock API client with a response for the abstract preview
    ApiToolCall previewToolCall;
    previewToolCall.id = "preview_call_789";
    previewToolCall.function.name = "provide_abstract_preview";
    previewToolCall.function.arguments = json({
        {"files", json::array({
            {{"name", "main.cpp"}, {"description", "A simple Hello World application."}}
        })},
        {"explanation", "This project will create a simple C++ application that prints Hello World."}
    }).dump();
    
    ApiResponse previewResponse;
    previewResponse.success = true;
    previewResponse.content = "Here's a preview of what I'll generate.";
    previewResponse.tool_calls.push_back(previewToolCall);
    
    mockApiClient.primeResponse(previewResponse);
    
    // Prime the mock API client with a response for the main.cpp file
    ApiResponse mainFileResponse = createWriteFileContentToolResponse(
        "main.cpp",
        "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}",
        "Simple main.cpp file for a minimal C++ project."
    );
    mockApiClient.primeResponse(mainFileResponse);
    
    // Prime the mock API client with a compilation response
    ApiResponse compileResponse = createCompilationToolResponse();
    mockApiClient.primeResponse(compileResponse);
    
    // Process user approval of the preview to generate files
    orchestrator->handleSubmitUserApprovalOfPreview("approve preview");
    
    // Verify that the file was written to disk
    EXPECT_TRUE(std::filesystem::exists(testWorkspacePath + "/main.cpp"));
    
    // Verify that the UI model was updated correctly
    bool mainCppFound = false;
    for (const auto& file : uiModel.projectFiles) {
        if (file.filename == "main.cpp") {
            EXPECT_EQ(file.status, ProjectFile::StatusToString(ProjectFile::Status::GENERATED));
            mainCppFound = true;
            break;
        }
    }
    EXPECT_TRUE(mainCppFound);
    
    // Verify that messages were sent to the OpenAI API client with the correct tool results
    EXPECT_GE(mockApiClient.last_sent_messages_.size(), 2);
    
    // Check that a tool result for main.cpp was sent
    bool foundMainCppToolResult = false;
    for (const auto& message : mockApiClient.last_sent_messages_) {
        if (message.role == "tool") {
            try {
                json toolResult = json::parse(message.content);
                if (toolResult.contains("filename") && toolResult["filename"] == "main.cpp") {
                    EXPECT_TRUE(toolResult["success"].get<bool>());
                    foundMainCppToolResult = true;
                    break;
                }
            } catch (...) {
                // Not a valid JSON or doesn't contain the expected fields
                continue;
            }
        }
    }
    EXPECT_TRUE(foundMainCppToolResult);
    
    // Check that the orchestrator requested compilation
    bool requestingCompilation = false;
    for (const auto& message : mockApiClient.last_sent_messages_) {
        if (message.role == "user" && message.content.find("compile") != std::string::npos) {
            requestingCompilation = true;
            break;
        }
    }
    EXPECT_TRUE(requestingCompilation);
    
    // Verify the orchestrator state changed to AWAITING_AI_COMPILE_COMMANDS
    EXPECT_EQ(orchestrator->getCurrentState(), AIAgentOrchestrator::OrchestratorState::AWAITING_AI_COMPILE_COMMANDS);
}

// Test case for handling an error during file write
TEST_F(AIAgentOrchestratorTest, HandleFileWriteError) {
    // Create a mock WorkspaceManager that always fails to write files
    class MockFailingWorkspaceManager : public WorkspaceManager {
    public:
        explicit MockFailingWorkspaceManager(const std::string& path) : WorkspaceManager(path) {}
        
        // Override the writeFile method to always fail
        bool writeFile(const std::string& filename, const std::string& content) {
            // Always return false to simulate a write error
            return false;
        }
    };
    
    // Replace the workspace manager with our failing version
    delete workspaceManager;
    delete orchestrator;
    
    workspaceManager = new MockFailingWorkspaceManager(testWorkspacePath);
    orchestrator = new AIAgentOrchestrator(mockApiClient, uiModel, *workspaceManager);
    
    // Setup the orchestrator state to be generating code files
    orchestrator->handleSubmitUserPrompt("Create a simple C++ project with a main.cpp file");
    
    // Set up the UIModel with a planned file
    uiModel.projectFiles.clear();
    uiModel.projectFiles.emplace_back("main.cpp", ProjectFile::StatusToString(ProjectFile::Status::PLANNED));
    
    // Inject plan data into the orchestrator by simulating a plan response
    json planJson = {
        {"files", json::array({
            {{"name", "main.cpp"}, {"description", "Main entry point for the application."}}
        })}
    };
    
    ApiToolCall planToolCall;
    planToolCall.id = "plan_call_789";
    planToolCall.function.name = "propose_plan";
    planToolCall.function.arguments = planJson.dump();
    
    ApiResponse planResponse;
    planResponse.success = true;
    planResponse.content = "Here's my plan for your C++ project.";
    planResponse.tool_calls.push_back(planToolCall);
    
    mockApiClient.primeResponse(planResponse);
    
    // Process the plan
    orchestrator->handleSubmitUserFeedback("The plan looks good.");
    
    // Prime the mock API client with a response for the abstract preview
    ApiToolCall previewToolCall;
    previewToolCall.id = "preview_call_101";
    previewToolCall.function.name = "provide_abstract_preview";
    previewToolCall.function.arguments = json({
        {"files", json::array({
            {{"name", "main.cpp"}, {"description", "A simple C++ application."}}
        })},
        {"explanation", "This project will create a simple C++ application."}
    }).dump();
    
    ApiResponse previewResponse;
    previewResponse.success = true;
    previewResponse.content = "Here's a preview of what I'll generate.";
    previewResponse.tool_calls.push_back(previewToolCall);
    
    mockApiClient.primeResponse(previewResponse);
    
    // Prime the mock API client with a response for the main.cpp file
    ApiResponse mainFileResponse = createWriteFileContentToolResponse(
        "main.cpp",
        "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}",
        "Main entry point for the application."
    );
    mockApiClient.primeResponse(mainFileResponse);
    
    // Prime the mock API client with a fallback response
    // This could be a response to the error report or a suggestion for an alternative file
    mockApiClient.primeResponse(createWriteFileContentToolResponse(
        "README.md",
        "# Simple C++ Project\n\nThis is a simple C++ project.",
        "Project documentation."
    ));
    
    // Process user approval of the preview to generate files
    orchestrator->handleSubmitUserApprovalOfPreview("yes");
    
    // Verify that the UI model was updated correctly to show the error
    bool mainCppFound = false;
    for (const auto& file : uiModel.projectFiles) {
        if (file.filename == "main.cpp") {
            EXPECT_EQ(file.status, ProjectFile::StatusToString(ProjectFile::Status::ERROR));
            mainCppFound = true;
            break;
        }
    }
    EXPECT_TRUE(mainCppFound);
    
    // Verify that a system message was added about the error
    bool errorMessageFound = false;
    for (const auto& message : uiModel.chatHistory) {
        if (message.senderType == ChatMessage::Sender::SYSTEM && 
            message.text.find("Error") != std::string::npos && 
            message.text.find("main.cpp") != std::string::npos) {
            errorMessageFound = true;
            break;
        }
    }
    EXPECT_TRUE(errorMessageFound);
    
    // Verify that the failed tool result was sent to the OpenAI API client
    bool foundFailedToolResult = false;
    for (const auto& message : mockApiClient.last_sent_messages_) {
        if (message.role == "tool") {
            try {
                json toolResult = json::parse(message.content);
                if (toolResult.contains("filename") && 
                    toolResult["filename"] == "main.cpp" && 
                    toolResult.contains("success") && 
                    !toolResult["success"].get<bool>()) {
                    foundFailedToolResult = true;
                    break;
                }
            } catch (...) {
                // Not a valid JSON or doesn't contain the expected fields
                continue;
            }
        }
    }
    EXPECT_TRUE(foundFailedToolResult);
} 