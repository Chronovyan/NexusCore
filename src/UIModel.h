#ifndef UI_MODEL_H
#define UI_MODEL_H

#include <string>
#include <vector>
#include <ctime>

namespace ai_editor {

// Represents a chat message in the conversation view
struct ChatMessage {
    enum class Sender { USER, AI, SYSTEM };
    
    Sender senderType;
    std::string senderName;
    std::string text;
    std::time_t timestamp;
    
    // Constructor for easy creation
    ChatMessage(Sender type, const std::string& name, const std::string& content)
        : senderType(type), senderName(name), text(content), timestamp(std::time(nullptr)) {}
};

// Represents a file in the project
struct ProjectFile {
    enum class Status { PLANNED, GENERATING, GENERATED, MODIFIED, ERROR };
    
    std::string filename;
    std::string status;  // e.g., "Modified", "Planned", "New", etc.
    std::string description;
    
    // Constructor for easy creation
    ProjectFile(const std::string& name, const std::string& fileStatus, const std::string& desc = "")
        : filename(name), status(fileStatus), description(desc) {}
        
    // Helper method to convert Status enum to string
    static std::string StatusToString(Status status) {
        switch (status) {
            case Status::PLANNED: return "Planned";
            case Status::GENERATING: return "Generating...";
            case Status::GENERATED: return "Generated";
            case Status::MODIFIED: return "Modified";
            case Status::ERROR: return "Error";
            default: return "Unknown";
        }
    }
};

// The central data model for the UI
struct UIModel {
    // Chat conversation history
    std::vector<ChatMessage> chatHistory;
    
    // Global status message
    std::string currentGlobalStatus;
    
    // Files in the project
    std::vector<ProjectFile> projectFiles;
    
    // OpenAI API key
    std::string apiKey;
    bool apiKeyValid = false;
    char apiKeyBuffer[1024] = {0};
    
    // Buffer for user input text (for ImGui)
    char userInputBuffer[4096] = {0};
    
    // Flag to indicate if AI is currently processing
    bool aiIsProcessing = false;
    
    // Constructor with default initialization
    UIModel() : currentGlobalStatus("Idle") {
        // Add a welcome message
        chatHistory.emplace_back(
            ChatMessage::Sender::SYSTEM,
            "System",
            "Welcome to AI-First TextEditor! Please enter your OpenAI API key in the settings to enable AI features."
        );
        
        // Add some example files
        projectFiles.emplace_back(
            "main.cpp", 
            ProjectFile::StatusToString(ProjectFile::Status::PLANNED),
            "Main entry point for the application"
        );
        
        projectFiles.emplace_back(
            "CMakeLists.txt", 
            ProjectFile::StatusToString(ProjectFile::Status::PLANNED),
            "Build configuration file"
        );
        
        projectFiles.emplace_back(
            "README.md", 
            ProjectFile::StatusToString(ProjectFile::Status::GENERATING),
            "Project documentation"
        );
    }
    
    // Add a user message and return it
    ChatMessage& AddUserMessage(const std::string& text) {
        chatHistory.emplace_back(ChatMessage::Sender::USER, "You", text);
        return chatHistory.back();
    }
    
    // Add an AI message and return it
    ChatMessage& AddAIMessage(const std::string& text) {
        chatHistory.emplace_back(ChatMessage::Sender::AI, "AI", text);
        return chatHistory.back();
    }
    
    // Add a system message and return it
    ChatMessage& AddSystemMessage(const std::string& text) {
        chatHistory.emplace_back(ChatMessage::Sender::SYSTEM, "System", text);
        return chatHistory.back();
    }
    
    // Add a project file and return it
    ProjectFile& AddProjectFile(const std::string& filename, ProjectFile::Status status, const std::string& description = "") {
        projectFiles.emplace_back(filename, ProjectFile::StatusToString(status), description);
        return projectFiles.back();
    }
    
    // Validate and set the API key
    bool SetApiKey(const std::string& key) {
        // Simple validation: API key should be non-empty and at least 20 chars
        if (key.length() >= 20) {
            apiKey = key;
            apiKeyValid = true;
            AddSystemMessage("API key saved. You can now use AI features.");
            return true;
        }
        
        AddSystemMessage("Invalid API key format. Please check your key and try again.");
        apiKeyValid = false;
        return false;
    }
};

} // namespace ai_editor

#endif // UI_MODEL_H 