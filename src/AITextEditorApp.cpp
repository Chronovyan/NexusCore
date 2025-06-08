#include "AppDebugLog.h"
#include <iostream>
#include <string>
#include <cstdlib>
// Include GLEW before any other OpenGL-related headers
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include "UIModel.h"
#include "OpenAI_API_Client.h"
#include "MockOpenAI_API_Client.h"
#include "WorkspaceManager.h"
#include "AIAgentOrchestrator.h"
#include "AIManager.h"
#include "tutorials/TutorialManager.hpp"
#include "tutorials/TutorialUIController.hpp"
#include "tutorials/TutorialProgressTracker.hpp"

// Include other necessary headers for your application

using namespace ai_editor;

// Forward declarations
bool saveApiKeyToEnvFile(const std::string& apiKey, const std::string& filePath);
std::string readApiKeyFromEnvFile(const std::string& filePath);
void renderModelSelectionDialog(UIModel& uiModel, AIManager& aiManager);
void renderTutorialUI(UIModel& uiModel, TutorialUIController& tutorialController);
void renderTutorialBrowser(UIModel& uiModel, TutorialUIController& tutorialController);

// Function to read API key from .env file
std::string readApiKeyFromEnvFile(const std::string& filePath) {
    std::ifstream envFile(filePath);
    if (!envFile.is_open()) {
        LOG_DEBUG("Could not open .env file for reading");
        return "";
    }
    
    std::string line;
    std::regex apiKeyRegex("^OPENAI_API_KEY=(.+)$");
    std::smatch match;
    
    while (std::getline(envFile, line)) {
        if (std::regex_search(line, match, apiKeyRegex) && match.size() > 1) {
            return match[1].str();
        }
    }
    
    LOG_DEBUG("No API key found in .env file");
    return "";
}

// Function to save API key to .env file
bool saveApiKeyToEnvFile(const std::string& apiKey, const std::string& filePath) {
    try {
        // First, read the existing contents
        std::ifstream envFile(filePath);
        bool fileExists = envFile.good();
        
        std::vector<std::string> lines;
        std::string line;
        bool keyFound = false;
        std::regex apiKeyRegex("^OPENAI_API_KEY=.*$");
        
        if (fileExists) {
            while (std::getline(envFile, line)) {
                if (std::regex_match(line, apiKeyRegex)) {
                    // Replace the existing API key line
                    lines.push_back("OPENAI_API_KEY=" + apiKey);
                    keyFound = true;
                } else {
                    lines.push_back(line);
                }
            }
            envFile.close();
        }
        
        // If the key wasn't found in the existing file, add it
        if (!keyFound) {
            lines.push_back("OPENAI_API_KEY=" + apiKey);
        }
        
        // Write back to the file
        std::ofstream envFileOut(filePath);
        if (!envFileOut.is_open()) {
            LOG_ERROR("Failed to open .env file for writing");
            return false;
        }
        
        for (const auto& l : lines) {
            envFileOut << l << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while saving API key to .env file: " + std::string(e.what()));
        return false;
    }
}

// Simple callback for text input when user clicks "Send"
void handleSendButton(UIModel& uiModel, AIAgentOrchestrator& orchestrator) {
    try {
        // Check if input is empty
        if (uiModel.userInputBuffer[0] == '\0') {
            LOG_DEBUG("Empty input, ignoring");
            return;
        }
        
        // Get the user input
        std::string userInput = uiModel.userInputBuffer;
        
        // Clear the input buffer
        uiModel.userInputBuffer[0] = '\0';
        
        // Process the input with the orchestrator
        if (orchestrator.getState() == AIAgentOrchestrator::State::IDLE) {
            // Initial prompt
            LOG_DEBUG("Handling initial prompt: " + userInput);
            orchestrator.handleSubmitUserPrompt(userInput);
        } else if (orchestrator.getState() == AIAgentOrchestrator::State::AWAITING_APPROVAL) {
            // Approve/reject
            LOG_DEBUG("Handling approval/rejection: " + userInput);
            orchestrator.handleUserFeedback(userInput);
        } else if (orchestrator.getState() == AIAgentOrchestrator::State::EXECUTING_TASK) {
            // Feedback during execution
            LOG_DEBUG("Handling feedback during execution: " + userInput);
            orchestrator.handleUserFeedbackDuringExecution(userInput);
        } else if (orchestrator.getState() == AIAgentOrchestrator::State::AI_ERROR) {
            // Try to recover from error
            LOG_DEBUG("Trying to recover from error with: " + userInput);
            orchestrator.resetState();
            orchestrator.handleSubmitUserPrompt(userInput);
        } else {
            // Unhandled state
            LOG_WARNING("Unhandled state in handleSendButton: " + 
                        std::to_string(static_cast<int>(orchestrator.getState())));
            uiModel.AddSystemMessage("Cannot process input in the current state: " +
                        std::to_string(static_cast<int>(orchestrator.getState())));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in handleSendButton: " + std::string(e.what()));
        uiModel.AddSystemMessage("Error processing input: " + std::string(e.what()));
    }
}

void renderModelSelectionDialog(UIModel& uiModel, AIManager& aiManager) {
    if (!uiModel.showModelSelectionDialog) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), 
                           ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);
    
    if (ImGui::Begin("AI Model Selection", &uiModel.showModelSelectionDialog)) {
        // Get all available provider types
        std::vector<std::string> providerTypes = aiManager.getAvailableProviderTypesList();
        
        // Provider selection
        const char* currentProvider = uiModel.currentProviderType.empty() ? 
                                     "Select Provider" : uiModel.currentProviderType.c_str();
        
        if (ImGui::BeginCombo("Provider", currentProvider)) {
            for (int i = 0; i < providerTypes.size(); i++) {
                const bool isSelected = (uiModel.selectedProviderIndex == i);
                if (ImGui::Selectable(providerTypes[i].c_str(), isSelected)) {
                    uiModel.selectedProviderIndex = i;
                    
                    // Load models for this provider
                    uiModel.availableModels = aiManager.listAvailableModels(providerTypes[i]);
                    uiModel.selectedModelIndex = -1; // Reset model selection
                }
                
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        // Model selection - only show if a provider is selected
        if (uiModel.selectedProviderIndex >= 0 && uiModel.selectedProviderIndex < providerTypes.size()) {
            ImGui::Separator();
            ImGui::Text("Select Model:");
            
            // Create a child window for scrollable list
            ImGui::BeginChild("ModelsScrollRegion", ImVec2(0, 250), true);
            
            for (int i = 0; i < uiModel.availableModels.size(); i++) {
                const ModelInfo& model = uiModel.availableModels[i];
                
                const bool isSelected = (uiModel.selectedModelIndex == i);
                std::string modelDisplay = model.name + " (" + model.id + ")";
                
                if (model.isLocal) {
                    modelDisplay += " [Local]";
                }
                
                if (ImGui::Selectable(modelDisplay.c_str(), isSelected)) {
                    uiModel.selectedModelIndex = i;
                }
                
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
                
                // Show model details on hover
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("ID: %s", model.id.c_str());
                    ImGui::Text("Provider: %s", model.provider.c_str());
                    ImGui::Text("Version: %s", model.version.c_str());
                    ImGui::Text("Local: %s", model.isLocal ? "Yes" : "No");
                    ImGui::Text("Context Window: %zu tokens", model.contextWindowSize);
                    
                    if (!model.capabilities.empty()) {
                        ImGui::Separator();
                        ImGui::Text("Capabilities:");
                        for (const auto& [cap, support] : model.capabilities) {
                            ImGui::Text("  %s: %s", cap.c_str(), support.c_str());
                        }
                    }
                    
                    ImGui::EndTooltip();
                }
            }
            
            ImGui::EndChild();
        }
        
        ImGui::Separator();
        
        // Show currently active model
        ImGui::Text("Current Model: %s", uiModel.GetCurrentModelDisplayName().c_str());
        
        // Apply button - only enabled if a model is selected
        bool canApply = uiModel.selectedProviderIndex >= 0 && 
                       uiModel.selectedModelIndex >= 0 && 
                       uiModel.selectedModelIndex < uiModel.availableModels.size();
        
        if (!canApply) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        
        if (ImGui::Button("Apply", ImVec2(120, 0)) && canApply) {
            std::string providerType = providerTypes[uiModel.selectedProviderIndex];
            std::string modelId = uiModel.availableModels[uiModel.selectedModelIndex].id;
            
            // Set the model in AIManager
            if (aiManager.setActiveProvider(providerType) && 
                aiManager.setCurrentModel(modelId)) {
                
                // Update the UI model
                uiModel.SetCurrentModel(providerType, modelId);
                uiModel.showModelSelectionDialog = false;
            } else {
                uiModel.AddSystemMessage("Failed to set model: " + modelId);
            }
        }
        
        if (!canApply) {
            ImGui::PopStyleVar();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            uiModel.showModelSelectionDialog = false;
        }
        
        ImGui::End();
    }
}

// Function to render the tutorial UI
void renderTutorialUI(UIModel& uiModel, TutorialUIController& tutorialController) {
    if (!tutorialController.isTutorialVisible()) {
        return;
    }
    
    // Calculate the appropriate position and size for the tutorial panel
    ImVec2 windowPos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.9f);
    ImVec2 windowSize = ImVec2(ImGui::GetIO().DisplaySize.x * 0.8f, ImGui::GetIO().DisplaySize.y * 0.2f);
    
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    
    if (ImGui::Begin("Tutorial", nullptr, 
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoCollapse)) {
        
        // Display tutorial content
        ImGui::TextWrapped("%s", uiModel.tutorialStepContent.c_str());
        
        ImGui::Separator();
        
        // Navigation buttons
        if (ImGui::Button("Previous", ImVec2(120, 0))) {
            // Just use the controller's UI update method
            tutorialController.updateUI();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Next", ImVec2(120, 0))) {
            // Just use the controller's UI update method
            tutorialController.updateUI();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Close Tutorial")) {
            tutorialController.hideTutorial();
        }
    }
    ImGui::End();
}

// Function to render the tutorial browser
void renderTutorialBrowser(UIModel& uiModel, TutorialUIController& tutorialController) {
    // Only render if browser is visible
    if (!uiModel.isTutorialBrowserVisible) return;
    
    // Calculate the appropriate position and size for the browser window
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = 600.0f;
    float windowHeight = 400.0f;
    
    // Center the window on the screen
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 
                           ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Appearing);
    
    if (ImGui::Begin("Tutorial Browser", &uiModel.isTutorialBrowserVisible, 
                    ImGuiWindowFlags_NoCollapse)) {
        
        // Search box
        ImGui::Text("Search Tutorials:");
        ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##TutorialSearch", uiModel.tutorialSearchBuffer, 
                        IM_ARRAYSIZE(uiModel.tutorialSearchBuffer));
        ImGui::PopItemWidth();
        
        // Filter tabs
        if (ImGui::BeginTabBar("TutorialFilterTabs")) {
            if (ImGui::BeginTabItem("All")) {
                // Show all tutorials
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Getting Started")) {
                // Show beginner tutorials
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Advanced")) {
                // Show advanced tutorials
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Completed")) {
                // Show completed tutorials
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        // Tutorial list
        ImGui::BeginChild("TutorialsList", ImVec2(0, ImGui::GetContentRegionAvail().y - 50), true);
        
        // Display tutorial items
        std::string searchStr = uiModel.tutorialSearchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        
        for (const auto& tutorial : uiModel.tutorialsList) {
            // Filter by search string if present
            if (!searchStr.empty()) {
                std::string titleLower = tutorial.title;
                std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), 
                               [](unsigned char c){ return std::tolower(c); });
                
                std::string descLower = tutorial.description;
                std::transform(descLower.begin(), descLower.end(), descLower.begin(), 
                               [](unsigned char c){ return std::tolower(c); });
                
                if (titleLower.find(searchStr) == std::string::npos && 
                    descLower.find(searchStr) == std::string::npos) {
                    continue; // Skip this tutorial as it doesn't match the search
                }
            }
            
            // Create a unique identifier for the selectable item
            std::string itemId = "##" + tutorial.id;
            
            // Create a color based on whether the tutorial is completed
            ImVec4 itemColor = tutorial.isCompleted ? 
                              ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : 
                              ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Text, itemColor);
            if (ImGui::Selectable((tutorial.title + itemId).c_str(), false)) {
                // Start the selected tutorial
                tutorialController.showTutorial(tutorial.id);
                uiModel.isTutorialBrowserVisible = false;
            }
            ImGui::PopStyleColor();
            
            // Display tutorial info on hover
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Title: %s", tutorial.title.c_str());
                ImGui::Text("Difficulty: %d/5", tutorial.difficulty);
                ImGui::Text("Estimated Time: %s", tutorial.estimatedTime.c_str());
                ImGui::Text("Status: %s", tutorial.isCompleted ? "Completed" : "Not Completed");
                ImGui::Separator();
                ImGui::TextWrapped("%s", tutorial.description.c_str());
                ImGui::EndTooltip();
            }
            
            // Display tutorial metadata
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                              "[Difficulty: %d/5, Time: %s]", 
                              tutorial.difficulty, 
                              tutorial.estimatedTime.c_str());
            
            // Show completion status with a colored circle
            ImGui::SameLine(ImGui::GetWindowWidth() - 40);
            if (tutorial.isCompleted) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓");
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "○");
            }
            
            // Display a short description
            ImGui::TextWrapped("%s", tutorial.description.c_str());
            ImGui::Separator();
        }
        
        ImGui::EndChild(); // End tutorials list
        
        // Bottom buttons
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            uiModel.isTutorialBrowserVisible = false;
        }
        
        ImGui::End(); // End tutorial browser window
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
        
        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            LOG_ERROR("Failed to initialize GLEW");
            return 1;
        }
        
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
        
        // Create AIManager
        LOG_DEBUG("Creating AIManager");
        AIManager aiManager;
        
        // Initialize the AI Manager
        bool aiInitialized = false;
        try {
            // Attempt to initialize OpenAI provider if API key is available
            if (!apiKeyStr.empty()) {
                std::map<std::string, std::string> optionsMap = {
                    {"api_key", apiKeyStr},
                    {"model", "gpt-4-turbo-preview"} // Default model
                };
                
                if (aiManager.initializeProvider("openai", optionsMap)) {
                    aiManager.setActiveProvider("openai");
                    aiInitialized = true;
                    LOG_INFO("OpenAI provider initialized successfully");
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in AIManager initialization: " + std::string(e.what()));
        }
        
        if (!aiInitialized) {
            // Show a warning if no API provider is configured
            uiModel.showApiKeyDialog = true;
            LOG_WARNING("No AI provider configured, showing API key dialog");
        }
        
        // Try to read API key from .env file
        std::string savedApiKey = readApiKeyFromEnvFile(".env");
        if (!savedApiKey.empty()) {
            LOG_DEBUG("Found API key in .env file");
            uiModel.SetApiKey(savedApiKey);
            
            // Register OpenAI provider with the API key
            ProviderOptions openaiOptions;
            openaiOptions.additionalOptions["api_key"] = savedApiKey;
            if (aiManager.registerProvider("openai", openaiOptions)) {
                aiManager.setActiveProvider("openai");
                
                // Get available models and set the default model
                std::vector<ModelInfo> openaiModels = aiManager.listAvailableModels();
                if (!openaiModels.empty()) {
                    std::string defaultModelId = openaiModels[0].id;
                    if (aiManager.setCurrentModel(defaultModelId)) {
                        uiModel.SetCurrentModel("openai", defaultModelId);
                        uiModel.UpdateAvailableModels(openaiModels);
                    }
                }
            }
        }
        
        // Create workspace manager
        LOG_DEBUG("Creating workspace manager");
        WorkspaceManager workspaceManager("./workspace");
        
        // Create tutorial components
        LOG_DEBUG("Creating tutorial components");
        auto tutorialProgressTracker = std::make_shared<TutorialProgressTracker>();
        auto tutorialManager = std::make_shared<TutorialManager>(tutorialProgressTracker);
        
        // Create AI Agent Orchestrator
        LOG_DEBUG("Creating AI Agent Orchestrator with AIManager");
        AIAgentOrchestrator orchestrator(aiManager, uiModel, workspaceManager);
        
        // Create Tutorial UI Controller
        LOG_DEBUG("Creating Tutorial UI Controller");
        std::shared_ptr<UIModel> uiModelPtr = std::make_shared<UIModel>();
        *uiModelPtr = uiModel; // Copy the contents of uiModel to the shared_ptr
        TutorialUIController tutorialController(uiModelPtr, tutorialManager);
        
        // Initialize tutorial progress from saved data
        LOG_DEBUG("Loading tutorial progress");
        std::string progressFilePath = "tutorial_progress.json";
        if (std::filesystem::exists(progressFilePath)) {
            tutorialProgressTracker->loadFromFile(progressFilePath);
        }
        
        // Load available tutorials
        LOG_DEBUG("Loading tutorials");
        tutorialManager->loadTutorialsFromDirectory("./tutorials");
        
        // Get available tutorials
        std::vector<TutorialListItem> tutorialsList;
        auto tutorials = tutorialManager->getAllTutorials();
        for (const auto& tutorial : tutorials) {
            if (tutorial) {
                TutorialListItem item;
                item.id = tutorial->getId();
                item.title = tutorial->getTitle();
                item.description = tutorial->getDescription();
                item.difficulty = static_cast<int>(tutorial->getDifficulty());
                item.completedSteps = 0;
                item.totalSteps = tutorial->getStepCount();
                
                // Check if we have progress data for this tutorial
                auto progress = tutorialProgressTracker->getProgress(item.id);
                if (progress.has_value()) {
                    const auto& progressData = progress.value();
                    item.completedSteps = progressData.completedSteps;
                }
                
                tutorialsList.push_back(item);
            }
        }
        uiModel.tutorials = tutorialsList;
        
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
                        uiModel.showApiKeyDialog = true;
                        LOG_DEBUG("API Key Settings menu item clicked, showApiKeyDialog set to true");
                    }
                    if (ImGui::MenuItem("AI Model Selection")) {
                        // Open model selection dialog
                        uiModel.showModelSelectionDialog = true;
                        
                        // Refresh the list of available models from all providers
                        std::vector<std::string> providerTypes = aiManager.getAvailableProviderTypesList();
                        std::vector<ModelInfo> allModels;
                        
                        for (const auto& providerType : providerTypes) {
                            std::vector<ModelInfo> providerModels = aiManager.listAvailableModels(providerType);
                            allModels.insert(allModels.end(), providerModels.begin(), providerModels.end());
                        }
                        
                        uiModel.UpdateAvailableModels(allModels);
                        
                        LOG_DEBUG("AI Model Selection menu item clicked, showModelSelectionDialog set to true");
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
            
            // Display current model in the status bar
            ImGui::Text("Current Model: %s", uiModel.GetCurrentModelDisplayName().c_str());
            
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
            
            // Add Tutorials button
            ImGui::Separator();
            if (ImGui::Button("Tutorials", ImVec2(leftPanelWidth - 20, 0))) {
                tutorialController.showTutorialBrowser();
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
                    handleSendButton(uiModel, orchestrator);
                    
                    // Clear input field
                    inputText[0] = '\0';
                }
            }
            
            ImGui::EndChild(); // End right panel
            
            ImGui::End();
            
            // API Key Settings dialog - this is a separate window that appears when showApiKeyDialog is true
            if (uiModel.showApiKeyDialog) {
                ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
                
                if (ImGui::Begin("API Key Settings", &uiModel.showApiKeyDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Enter your OpenAI API Key:");
                    ImGui::InputText("##apikey", uiModel.apiKeyBuffer, IM_ARRAYSIZE(uiModel.apiKeyBuffer), ImGuiInputTextFlags_Password);
                    
                    ImGui::Spacing();
                    ImGui::Text("API keys start with 'sk-' and are ~50 characters long.");
                    ImGui::Text("You can find your API key in the OpenAI dashboard.");
                    
                    ImGui::Spacing();
                    if (ImGui::Button("Save", ImVec2(120, 0))) {
                        std::string newKey = uiModel.apiKeyBuffer;
                        if (uiModel.SetApiKey(newKey)) {
                            // Register or update OpenAI provider with the new key
                            ProviderOptions openaiOptions;
                            openaiOptions.additionalOptions["api_key"] = newKey;
                            
                            if (aiManager.isProviderRegistered("openai")) {
                                aiManager.setProviderOptions("openai", openaiOptions);
                            } else {
                                aiManager.registerProvider("openai", openaiOptions);
                                aiManager.setActiveProvider("openai");
                            }
                            
                            // Save to .env file
                            if (saveApiKeyToEnvFile(newKey, ".env")) {
                                LOG_DEBUG("API key saved to .env file");
                                uiModel.AddSystemMessage("API key saved successfully to .env file.");
                            } else {
                                LOG_ERROR("Failed to save API key to .env file");
                                uiModel.AddSystemMessage("Warning: API key set but could not save to .env file.");
                            }
                            
                            uiModel.showApiKeyDialog = false;
                        }
                    }
                    
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        uiModel.showApiKeyDialog = false;
                    }
                    
                    ImGui::End();
                }
            }

            // Render the model selection dialog
            renderModelSelectionDialog(uiModel, aiManager);
            
            // Render tutorial UI and browser if visible
            renderTutorialUI(uiModel, tutorialController);
            renderTutorialBrowser(uiModel, tutorialController);

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
        
        // Save tutorial progress before exit
        LOG_DEBUG("Saving tutorial progress");
        tutorialProgressTracker->saveToFile(progressFilePath);
        
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
