#ifndef UI_MODEL_H
#define UI_MODEL_H

#include <string>
#include <vector>

namespace ai_editor {

// Represents a chat message in the conversation view
struct ChatMessage {
    enum class Sender { USER, AI, SYSTEM };
    
    Sender senderType;
    std::string senderName;
    std::string text;
    
    // Constructor for easy creation
    ChatMessage(Sender type, const std::string& name, const std::string& content)
        : senderType(type), senderName(name), text(content) {}
};

// Represents a file in the project
struct ProjectFile {
    std::string filename;
    std::string status;  // e.g., "Modified", "Planned", "New", etc.
    std::string description;
    
    // Constructor for easy creation
    ProjectFile(const std::string& name, const std::string& fileStatus, const std::string& desc = "")
        : filename(name), status(fileStatus), description(desc) {}
};

// The central data model for the UI
struct UIModel {
    // Chat conversation history
    std::vector<ChatMessage> chatHistory;
    
    // Global status message
    std::string currentGlobalStatus;
    
    // Files in the project
    std::vector<ProjectFile> projectFiles;
    
    // Buffer for user input text (for ImGui)
    char userInputBuffer[4096] = {0};
    
    // Flag to indicate if AI is currently processing
    bool aiIsProcessing = false;
    
    // Constructor with default initialization
    UIModel() : currentGlobalStatus("Idle") {
        // Add a welcome message
        chatHistory.emplace_back(
            ChatMessage::Sender::AI,
            "AI",
            "Hello! How can I help you design your project today?"
        );
        
        // Add some example files
        projectFiles.emplace_back("main.cpp", "Planned");
        projectFiles.emplace_back("CMakeLists.txt", "Planned");
    }
};

} // namespace ai_editor

#endif // UI_MODEL_H 