#include "AppDebugLog.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include "UIModel.h"
#include "OpenAI_API_Client.h"
#include "WorkspaceManager.h"
#include "AIAgentOrchestrator.h"

// Include other necessary headers for your application

using namespace ai_editor;

// Function to read API key from .env file
std::string readApiKeyFromEnvFile(const std::string& filePath) {
    std::ifstream envFile(filePath);
    std::string apiKey;
    
    if (!envFile.is_open()) {
        LOG_ERROR("Could not open .env file: " + filePath);
        return "";
    }
    
    std::string line;
    std::regex keyPattern("OPENAI_API_KEY=(.*)");
    
    while (std::getline(envFile, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::smatch matches;
        if (std::regex_search(line, matches, keyPattern) && matches.size() > 1) {
            apiKey = matches[1].str();
            break;
        }
    }
    
    return apiKey;
}

// Simple callback for text input when user clicks "Send"
void handleSendButton(UIModel& uiModel, AIAgentOrchestrator& orchestrator) {
    // Check if there's any text to send
    if (uiModel.userInputBuffer[0] == '\0') {
        return;
    }
    
    // Get the user input
    std::string userInput = uiModel.userInputBuffer;
    
    // Clear the input buffer
    uiModel.userInputBuffer[0] = '\0';
    
    // Check the orchestrator state to determine which method to call
    AIAgentOrchestrator::OrchestratorState state = orchestrator.getCurrentState();
    
    // Route to the appropriate handler based on the current state
    if (state == AIAgentOrchestrator::OrchestratorState::IDLE) {
        // Initial prompt - starting a new conversation
        orchestrator.handleSubmitUserPrompt(userInput);
    } 
    else if (state == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_FEEDBACK_ON_PLAN ||
             state == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_CLARIFICATION_BEFORE_PLAN ||
             state == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_CLARIFICATION) {
        // User is responding to a plan or clarification questions
        orchestrator.handleSubmitUserFeedback(userInput);
    }
    else if (state == AIAgentOrchestrator::OrchestratorState::AWAITING_USER_APPROVAL_OF_PREVIEW) {
        // Handle user approval of the preview
        if (userInput == "approve preview" || userInput == "yes" || 
            userInput == "proceed" || userInput == "approve" ||
            userInput == "looks good" || userInput == "looks good, proceed") {
            // User approved the preview, trigger file generation
            orchestrator.handleSubmitUserApprovalOfPreview(userInput);
        } else {
            // Treat any other input as feedback requesting changes
            orchestrator.handleSubmitUserFeedback(userInput);
        }
    }
    else {
        // For any other state, just show a message that we can't process this now
        uiModel.AddSystemMessage("Cannot process input in the current state: " + 
                               std::to_string(static_cast<int>(state)));
    }
}

// Error callback for GLFW
static void glfw_error_callback(int error, const char* description) {
    LOG_ERROR("GLFW Error " + std::to_string(error) + ": " + std::string(description));
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int argc, char** argv) {
    // Initialize debug logging at the earliest point
    LOG_INIT("AITextEditor");
    LOG_DEBUG("Application starting");
    
    try {
        // Read API key from .env file in project root
        LOG_DEBUG("Reading API key from .env file");
        std::string apiKeyStr = readApiKeyFromEnvFile(".env");
        
        // Fall back to environment variable if .env file read fails
        if (apiKeyStr.empty()) {
            LOG_DEBUG("API key not found in .env file, falling back to environment variable");
            
            #ifdef _MSC_VER
            // Windows-specific safe environment variable handling
            char* apiKey = nullptr;
            size_t len = 0;
            errno_t err = _dupenv_s(&apiKey, &len, "OPENAI_API_KEY");
            
            if (err != 0 || apiKey == nullptr) {
                LOG_ERROR("OPENAI_API_KEY environment variable not set");
                std::cerr << "ERROR: OPENAI_API_KEY not found in .env file or environment variables!" << std::endl;
                std::cerr << "Please set your OpenAI API key in .env file or as an environment variable." << std::endl;
                return 1;
            }
            
            // Store a copy of the key
            apiKeyStr = std::string(apiKey);
            
            // Free the memory allocated by _dupenv_s
            free(apiKey);
            apiKey = nullptr;
            #else
            // Standard getenv for other platforms
            char* apiKey = std::getenv("OPENAI_API_KEY");
            if (!apiKey) {
                LOG_ERROR("OPENAI_API_KEY environment variable not set");
                std::cerr << "ERROR: OPENAI_API_KEY not found in .env file or environment variables!" << std::endl;
                std::cerr << "Please set your OpenAI API key in .env file or as an environment variable." << std::endl;
                return 1;
            }
            
            // Store a copy of the key
            apiKeyStr = std::string(apiKey);
            #endif
        }
        
        if (apiKeyStr.empty()) {
            LOG_ERROR("API key is empty");
            std::cerr << "ERROR: API key is empty!" << std::endl;
            return 1;
        }
        
        LOG_DEBUG("API key found");
        
        // Setup window
        LOG_DEBUG("Setting up GLFW window");
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            LOG_ERROR("Failed to initialize GLFW");
            return 1;
        }
        
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        
        // Create window with graphics context
        LOG_DEBUG("Creating GLFW window");
        GLFWwindow* window = glfwCreateWindow(1280, 720, "AI-First TextEditor", NULL, NULL);
        if (window == NULL) {
            LOG_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return 1;
        }
        
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync
        LOG_DEBUG("GLFW window created successfully");
        
        // Setup Dear ImGui context
        LOG_DEBUG("Setting up ImGui context");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        // Enable Docking if ImGui version supports it
        #ifdef ImGuiConfigFlags_DockingEnable
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        #endif
        
        // Setup ImGui style
        ImGui::StyleColorsDark();
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
        LOG_DEBUG("ImGui initialized successfully");
        
        // Initialize UI Model and other components
        LOG_DEBUG("Initializing UI model and components");
        UIModel uiModel;
        
        // Create OpenAI client
        LOG_DEBUG("Creating OpenAI API client");
        OpenAI_API_Client apiClient(apiKeyStr);
        
        // Create workspace manager
        LOG_DEBUG("Creating workspace manager");
        WorkspaceManager workspaceManager("./workspace");
        
        // Create AI Agent Orchestrator
        LOG_DEBUG("Creating AI Agent Orchestrator");
        AIAgentOrchestrator orchestrator(apiClient, uiModel, workspaceManager);
        LOG_DEBUG("Components initialized successfully");
        
        // Add initial welcome message
        ChatMessage welcomeMsg(ChatMessage::Sender::SYSTEM, "System", "Welcome to AI-First TextEditor!");
        uiModel.chatHistory.push_back(welcomeMsg);
        
        // Main loop
        LOG_DEBUG("Starting main application loop");
        while (!glfwWindowShouldClose(window)) {
            // Poll and handle events
            glfwPollEvents();
            
            // Start the ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Create a window that takes the full screen
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
            ImGui::Begin("AI TextEditor", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
            
            // Render Menu Bar
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Project")) {
                        // Handle new project
                    }
                    if (ImGui::MenuItem("Open Project")) {
                        // Handle open project
                    }
                    if (ImGui::MenuItem("Save")) {
                        // Handle save
                    }
                    if (ImGui::MenuItem("Save As...")) {
                        // Handle save as
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit")) {
                        glfwSetWindowShouldClose(window, true);
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                        // Handle undo
                    }
                    if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                        // Handle redo
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                        // Handle cut
                    }
                    if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                        // Handle copy
                    }
                    if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                        // Handle paste
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("View")) {
                    if (ImGui::MenuItem("Project Files", NULL, true)) {
                        // Toggle project files visibility
                    }
                    if (ImGui::MenuItem("Chat", NULL, true)) {
                        // Toggle chat visibility
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Settings")) {
                    if (ImGui::MenuItem("API Key Settings")) {
                        // Open API key settings
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Help")) {
                    if (ImGui::MenuItem("About")) {
                        // Show about dialog
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            
            // Split the window into two panels
            float leftPanelWidth = 250.0f;
            
            // Left panel for project files
            ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, 0), true);
            
            ImGui::Text("Project Files");
            ImGui::Separator();
            
            // Display project files
            for (const auto& file : uiModel.projectFiles) {
                if (ImGui::Selectable(file.filename.c_str())) {
                    // Handle file selection
                }
                if (ImGui::IsItemHovered() && !file.description.empty()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", file.description.c_str());
                    ImGui::EndTooltip();
                }
            }
            
            ImGui::EndChild();
            
            // Right panel with chat interface
            ImGui::SameLine();
            ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
            
            // Simple chat interface
            ImGui::Text("Chat with AI:");
            
            // Display chat history
            ImGui::BeginChild("ChatHistory", ImVec2(0, ImGui::GetContentRegionAvail().y - 70), true);
            for (const auto& message : uiModel.chatHistory) {
                if (message.senderType == ChatMessage::Sender::USER) {
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "User: %s", message.text.c_str());
                } else if (message.senderType == ChatMessage::Sender::AI) {
                    ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "AI: %s", message.text.c_str());
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "System: %s", message.text.c_str());
                }
                ImGui::Separator();
            }
            ImGui::EndChild();
            
            // Input text field
            static char inputText[1024] = "";
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
            bool enter_pressed = ImGui::InputText("##Input", inputText, IM_ARRAYSIZE(inputText), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            bool send_clicked = ImGui::Button("Send");
            
            if (enter_pressed || send_clicked) {
                if (strlen(inputText) > 0) {
                    LOG_DEBUG("User input: " + std::string(inputText));
                    
                    // Add user message to chat history
                    ChatMessage userMsg(ChatMessage::Sender::USER, "You", inputText);
                    uiModel.chatHistory.push_back(userMsg);
                    
                    // Process user input through orchestrator
                    orchestrator.handleSubmitUserPrompt(inputText);
                    
                    // Clear input field
                    inputText[0] = '\0';
                }
            }
            
            ImGui::EndChild(); // End right panel
            
            ImGui::End();
            
            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window);
        }
        
        // Cleanup
        LOG_DEBUG("Application closing, cleaning up resources");
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window);
        glfwTerminate();
        LOG_DEBUG("Cleanup completed");
        
        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception occurred: " + std::string(e.what()));
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        LOG_ERROR("Unknown exception occurred");
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
