#include "UIModel.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

namespace ai_editor {

// Global state
UIModel g_uiModel;
bool g_shouldSetFocus = false;  // Flag to set focus on input field

// Error callback for GLFW
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Helper function to render the conversation view panel
void RenderConversationPanel() {
    ImGui::BeginChild("ConversationView", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 4), true);
    
    // Render chat messages
    for (const auto& message : g_uiModel.chatHistory) {
        // Determine color based on sender
        ImVec4 nameColor;
        switch (message.senderType) {
            case ChatMessage::Sender::AI:
                nameColor = ImVec4(0.2f, 0.7f, 0.2f, 1.0f); // Green for AI
                break;
            case ChatMessage::Sender::USER:
                nameColor = ImVec4(0.2f, 0.2f, 0.8f, 1.0f); // Blue for User
                break;
            case ChatMessage::Sender::SYSTEM:
                nameColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // Red for System
                break;
        }
        
        // Display sender with color
        ImGui::TextColored(nameColor, "%s:", message.senderName.c_str());
        
        // Display message text (wrapped)
        ImGui::SameLine();
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 20);
        ImGui::TextUnformatted(message.text.c_str());
        ImGui::PopTextWrapPos();
        
        ImGui::Separator();
    }
    
    // Auto-scroll to bottom when a new message is added
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f || g_uiModel.aiIsProcessing) {
        ImGui::SetScrollHereY(1.0f);  // Scroll to bottom
    }
    
    ImGui::EndChild();
}

// Helper function to simulate an AI response based on user input
std::string SimulateAIResponse(const std::string& userInput) {
    // In a real app, this would call the AI model
    // For now, we'll just provide different responses based on simple patterns
    
    // Convert to lowercase for case-insensitive comparison
    std::string lowerInput = userInput;
    for (auto& c : lowerInput) c = std::tolower(c);
    
    if (lowerInput.find("hello") != std::string::npos || 
        lowerInput.find("hi") != std::string::npos) {
        return "Hello! I'm your AI assistant. How can I help you with your coding project today?";
    }
    else if (lowerInput.find("create") != std::string::npos && lowerInput.find("file") != std::string::npos) {
        // Simulate creating a new file
        if (lowerInput.find("header") != std::string::npos || lowerInput.find(".h") != std::string::npos) {
            g_uiModel.AddProjectFile("myclass.h", ProjectFile::Status::GENERATING, "C++ header file");
            
            // Simulate that after a delay, the file would be generated
            g_uiModel.AddSystemMessage("File 'myclass.h' has been created");
            
            return "I'm creating a new header file called 'myclass.h' for you. What should this class do?";
        }
        else if (lowerInput.find("cpp") != std::string::npos || lowerInput.find(".cpp") != std::string::npos) {
            g_uiModel.AddProjectFile("myclass.cpp", ProjectFile::Status::GENERATING, "C++ implementation file");
            
            // Simulate that after a delay, the file would be generated
            g_uiModel.AddSystemMessage("File 'myclass.cpp' has been created");
            
            return "I'm creating a new C++ implementation file called 'myclass.cpp'. This will contain the implementation of your class.";
        }
        else {
            return "I can help you create files for your project. What kind of file would you like to create? For example, a .cpp file or a header (.h) file?";
        }
    }
    else if (lowerInput.find("help") != std::string::npos) {
        return "I can assist with coding tasks, project organization, and explaining concepts. What specific help do you need?";
    }
    else if (lowerInput.find("thank") != std::string::npos) {
        return "You're welcome! Let me know if you need anything else.";
    }
    else if (lowerInput.find("code") != std::string::npos || 
             lowerInput.find("function") != std::string::npos ||
             lowerInput.find("class") != std::string::npos) {
        // If user asks for code generation, simulate adding a file
        if (lowerInput.find("class") != std::string::npos) {
            g_uiModel.AddProjectFile("exampleclass.h", ProjectFile::Status::GENERATING, "Example C++ class");
            
            return "I can help you write a class. I've started creating an 'exampleclass.h' file. "
                   "Would you like a simple class or something more complex with inheritance?";
        }
        else {
            return "I can help you write code. Could you describe what functionality you need in more detail?";
        }
    }
    
    // Default response
    return "I understand you said: \"" + userInput + "\". How would you like me to help with this?";
}

// Helper function to render the chat input panel
void RenderChatInputPanel() {
    // Set keyboard focus if needed
    if (g_shouldSetFocus) {
        ImGui::SetKeyboardFocusHere();
        g_shouldSetFocus = false;
    }
    
    // Chat input text field
    ImGui::PushItemWidth(-100); // Make text field fill available width minus button width
    bool input_submitted = ImGui::InputTextMultiline(
        "##ChatInput", 
        g_uiModel.userInputBuffer, 
        sizeof(g_uiModel.userInputBuffer),
        ImVec2(-70, ImGui::GetFrameHeightWithSpacing() * 3),
        ImGuiInputTextFlags_EnterReturnsTrue
    );
    ImGui::PopItemWidth();
    
    // Send button
    ImGui::SameLine();
    bool send_clicked = ImGui::Button("Send", ImVec2(60, ImGui::GetFrameHeightWithSpacing() * 3));
    
    // Process input if Send button clicked or Enter pressed
    if ((input_submitted || send_clicked) && g_uiModel.userInputBuffer[0] != '\0') {
        // Store the user input for processing
        std::string userInput = g_uiModel.userInputBuffer;
        
        // Add user message to conversation using the helper method
        g_uiModel.AddUserMessage(userInput);
        
        // Clear input buffer
        g_uiModel.userInputBuffer[0] = '\0';
        
        // Set focus for next input
        g_shouldSetFocus = true;
        
        // In a real app, we would trigger AI processing here
        g_uiModel.aiIsProcessing = true;
        g_uiModel.currentGlobalStatus = "Processing...";
        
        // For demo purposes, simulate AI response
        std::string aiResponse = SimulateAIResponse(userInput);
        
        // Add AI response to the conversation using the helper method
        g_uiModel.AddAIMessage(aiResponse);
        
        // Reset processing state
        g_uiModel.aiIsProcessing = false;
        g_uiModel.currentGlobalStatus = "Idle";
    }
}

// Helper function to render the file list sidebar
void RenderFileListSidebar() {
    ImGui::BeginChild("FileListSidebar", ImVec2(200, -ImGui::GetFrameHeightWithSpacing()), true);
    
    ImGui::Text("Files:");
    ImGui::Separator();
    
    // Render file list
    for (const auto& file : g_uiModel.projectFiles) {
        // Different colors based on file status
        ImVec4 statusColor;
        if (file.status == "Modified") {
            statusColor = ImVec4(0.9f, 0.6f, 0.1f, 1.0f); // Orange for modified
        } else if (file.status == "Planned") {
            statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray for planned
        } else if (file.status == "New") {
            statusColor = ImVec4(0.1f, 0.6f, 0.1f, 1.0f); // Green for new
        } else if (file.status == "Generating...") {
            statusColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f); // Yellow for generating
        } else if (file.status == "Error") {
            statusColor = ImVec4(0.9f, 0.1f, 0.1f, 1.0f); // Red for error
        } else {
            statusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for default
        }
        
        // File entry with status color
        ImGui::TextColored(statusColor, "%s", file.filename.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 70);
        ImGui::TextColored(statusColor, "[%s]", file.status.c_str());
        
        // Display description as a tooltip if available
        if (!file.description.empty() && ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(file.description.c_str());
            ImGui::EndTooltip();
        }
    }
    
    ImGui::EndChild();
}

// Helper function to render the global status display
void RenderGlobalStatus() {
    // Status bar at the bottom
    ImGui::Separator();
    
    // Display different status colors based on AI processing state
    ImVec4 statusColor;
    if (g_uiModel.aiIsProcessing) {
        statusColor = ImVec4(0.9f, 0.6f, 0.1f, 1.0f); // Orange when processing
    } else {
        statusColor = ImVec4(0.1f, 0.6f, 0.1f, 1.0f); // Green when idle
    }
    
    ImGui::TextColored(statusColor, "Status: %s", g_uiModel.currentGlobalStatus.c_str());
}

} // namespace ai_editor

int main(int argc, char** argv) {
    using namespace ai_editor;
    
    // Setup GLFW error callback
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return 1;
    }

    // Setup OpenGL context
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "AI-First TextEditor - MVP", nullptr, nullptr
    );
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create main window with full size
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
        ImGui::Begin("AI-First TextEditor", nullptr, 
                    ImGuiWindowFlags_NoTitleBar | 
                    ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoCollapse | 
                    ImGuiWindowFlags_MenuBar);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Project")) {}
                if (ImGui::MenuItem("Open Project...")) {}
                if (ImGui::MenuItem("Save")) {}
                if (ImGui::MenuItem("Save As...")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo")) {}
                if (ImGui::MenuItem("Redo")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Cut")) {}
                if (ImGui::MenuItem("Copy")) {}
                if (ImGui::MenuItem("Paste")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Horizontal layout: File List Sidebar on the left, Main panel on the right
        RenderFileListSidebar();
        
        ImGui::SameLine();
        
        // Main panel with vertical layout for conversation view and chat input
        ImGui::BeginGroup();
        {
            // Conversation view panel
            RenderConversationPanel();
            
            // Chat input panel
            RenderChatInputPanel();
            
            // Global Status
            RenderGlobalStatus();
        }
        ImGui::EndGroup();

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
} 