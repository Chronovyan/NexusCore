#include "AIAgentOrchestrator.h"
#include "MockOpenAI_API_Client.h"
#include "UIModel.h"
#include "WorkspaceManager.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <memory>

using namespace ai_editor;

// Helper function to create a temporary directory for tests
std::string createTempTestDirectory() {
    std::string tempDir = "./test_output_" + std::to_string(std::rand());
    std::filesystem::create_directories(tempDir);
    return tempDir;
}

// Helper function to clean up test directory
void cleanupTestDirectory(const std::string& dir) {
    if (std::filesystem::exists(dir)) {
        std::filesystem::remove_all(dir);
    }
}

// A very simple test framework
#define TEST_CASE(name) \
    std::cout << "Running test: " << name << std::endl; \
    bool test_passed = true; \
    try {

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << message << std::endl; \
        test_passed = false; \
    }

#define END_TEST \
    } catch (const std::exception& e) { \
        std::cerr << "Exception: " << e.what() << std::endl; \
        test_passed = false; \
    } \
    if (test_passed) { \
        std::cout << "Test passed!" << std::endl; \
    } else { \
        std::cout << "Test failed!" << std::endl; \
    } \
    return test_passed;

// Helper to create a mock API response with a tool call
ApiResponse createPlanResponseWithToolCall() {
    ApiResponse response;
    response.id = "chatcmpl-123";
    response.model = "gpt-4o";
    response.success = true;
    response.content = "I'll help you create a plan for your project.";
    
    ApiToolCall toolCall;
    toolCall.id = "call_abc123";
    toolCall.type = "function";
    toolCall.function_name = "propose_plan";
    toolCall.function_args = R"({
        "project_name": "Greeter",
        "language": "C++",
        "files": [
            {"name": "main.cpp", "description": "Main entry point for the application"},
            {"name": "CMakeLists.txt", "description": "CMake build configuration"}
        ],
        "steps": [
            "Create CMakeLists.txt",
            "Create main.cpp",
            "Build and test the application"
        ],
        "description": "A simple application that asks for the user's name and displays a greeting."
    })";
    
    response.tool_calls.push_back(toolCall);
    return response;
}

// Helper to create a mock API response with an abstract preview tool call
ApiResponse createAbstractPreviewResponseWithToolCall() {
    ApiResponse response;
    response.id = "chatcmpl-456";
    response.model = "gpt-4o";
    response.success = true;
    response.content = "Here's a preview of the application.";
    
    ApiToolCall toolCall;
    toolCall.id = "call_preview456";
    toolCall.type = "function";
    toolCall.function_name = "provide_abstract_preview";
    toolCall.function_args = R"({
        "overview": "A console application that asks for the user's name and displays a greeting.",
        "core_functionality": ["Get user input", "Display personalized greeting"],
        "user_interaction": "The user will be prompted to enter their name, and the program will respond with a greeting.",
        "technical_highlights": ["C++ standard I/O", "String manipulation"],
        "next_planned_file_to_generate": "CMakeLists.txt"
    })";
    
    response.tool_calls.push_back(toolCall);
    return response;
}

// Test functions
bool testAIAgentOrchestratorInitialPrompt() {
    TEST_CASE("AIAgentOrchestrator handles initial prompt")
    
    // Create test dependencies
    UIModel uiModel;
    auto mockApiClient = std::make_unique<MockOpenAI_API_Client>();
    std::string testDir = createTempTestDirectory();
    WorkspaceManager workspaceManager(testDir);
    
    // Set up the mock response for the initial prompt
    mockApiClient->setNextResponse(createPlanResponseWithToolCall());
    auto* mockClientPtr = mockApiClient.get(); // Keep a raw pointer for assertions
    
    // Create the orchestrator with mock dependencies
    AIAgentOrchestrator orchestrator(std::move(mockApiClient), uiModel, workspaceManager);
    
    // Call the orchestrator with a user prompt
    std::string userPrompt = "Create a simple C++ greeter application";
    orchestrator.handleSubmitUserPrompt(userPrompt);
    
    // Verify the orchestrator sent the expected request to the API
    TEST_ASSERT(mockClientPtr->getChatCompletionCallCount() > 0, "Messages should be sent to API");
    
    // Verify the state transition
    TEST_ASSERT(orchestrator.getCurrentState() == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_FEEDBACK_ON_PLAN, 
                "Orchestrator should transition to AWAITING_USER_FEEDBACK_ON_PLAN state");
    
    // Verify the UI model was updated
    TEST_ASSERT(uiModel.chatHistory.size() > 0, "UI chat history should be updated");
    TEST_ASSERT(uiModel.projectFiles.size() == 2, "Project files should be added to UI model");
    
    // Clean up test directory
    cleanupTestDirectory(testDir);
    
    END_TEST
}

bool testAIAgentOrchestratorHandlesFeedback() {
    TEST_CASE("AIAgentOrchestrator handles user feedback")
    
    // Create test dependencies
    UIModel uiModel;
    auto mockApiClient = std::make_unique<MockOpenAI_API_Client>();
    std::string testDir = createTempTestDirectory();
    WorkspaceManager workspaceManager(testDir);
    
    // Set up the mock responses
    std::vector<ApiResponse> responses = {
        createPlanResponseWithToolCall(),         // For initial prompt
        createAbstractPreviewResponseWithToolCall() // For feedback
    };
    mockApiClient->setNextResponses(responses);
    auto* mockClientPtr = mockApiClient.get(); // Keep a raw pointer for assertions
    
    // Create the orchestrator with mock dependencies
    AIAgentOrchestrator orchestrator(std::move(mockApiClient), uiModel, workspaceManager);
    
    // Call the orchestrator with a user prompt to set up the state
    orchestrator.handleSubmitUserPrompt("Create a simple C++ greeter application");
    
    // Verify we're in the correct state after initial prompt
    TEST_ASSERT(orchestrator.getCurrentState() == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_FEEDBACK_ON_PLAN, 
                "Orchestrator should be in AWAITING_USER_FEEDBACK_ON_PLAN state after initial prompt");
    
    // Call the orchestrator with user feedback
    std::string userFeedback = "Looks good, but can we add a loop to greet the user multiple times?";
    orchestrator.handleSubmitUserFeedback(userFeedback);
    
    // Verify the orchestrator sent the expected requests to the API
    TEST_ASSERT(mockClientPtr->getChatCompletionCallCount() == 2, "Two message sequences should be sent to API");
    
    // Verify the state transition
    TEST_ASSERT(orchestrator.getCurrentState() == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_APPROVAL_OF_PREVIEW, 
                "Orchestrator should transition to AWAITING_USER_APPROVAL_OF_PREVIEW state");
    
    // Verify the UI model was updated
    TEST_ASSERT(uiModel.chatHistory.size() > 0, "UI chat history should be updated");
    
    // Clean up test directory
    cleanupTestDirectory(testDir);
    
    END_TEST
}

int main() {
    // Initialize random seed for temp directory names
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Run the tests
    bool allPassed = true;
    
    allPassed &= testAIAgentOrchestratorInitialPrompt();
    allPassed &= testAIAgentOrchestratorHandlesFeedback();
    
    // Add more tests here...
    
    // Report overall result
    if (allPassed) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed!" << std::endl;
        return 1;
    }
} 