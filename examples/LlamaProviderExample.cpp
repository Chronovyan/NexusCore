#include "AIManager.h"
#include "LlamaProvider.h"
#include "EditorErrorReporter.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * This example demonstrates how to use the LlamaProvider to interact with local LLama models.
 * 
 * To run this example:
 * 1. Download a LLama model in GGUF format
 * 2. Place it in a directory
 * 3. Run the example with the path to the model directory as the first argument
 */
int main(int argc, char** argv) {
    // Check if a model path was provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_model_directory_or_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " C:/Users/YourUsername/AIModels/llama" << std::endl;
        return 1;
    }
    
    // Get the model path from command line arguments
    std::string modelPath = argv[1];
    
    // Verify the path exists
    if (!fs::exists(modelPath)) {
        std::cerr << "Error: The specified path does not exist: " << modelPath << std::endl;
        return 1;
    }
    
    std::cout << "Using model path: " << modelPath << std::endl;
    
    try {
        // Create the AI Manager
        ai_editor::AIManager aiManager;
        
        // Initialize the LLama provider
        std::cout << "Initializing LLama provider..." << std::endl;
        if (!aiManager.initializeLocalLlamaProvider(modelPath)) {
            std::cerr << "Failed to initialize LLama provider" << std::endl;
            return 1;
        }
        
        // Set as active provider
        if (!aiManager.setActiveProvider("llama")) {
            std::cerr << "Failed to set LLama as the active provider" << std::endl;
            return 1;
        }
        
        // Get the active provider
        auto provider = aiManager.getActiveProvider();
        if (!provider) {
            std::cerr << "Failed to get active provider" << std::endl;
            return 1;
        }
        
        // Print provider and model information
        auto modelInfo = provider->getCurrentModelInfo();
        std::cout << "Provider: " << provider->getProviderName() << std::endl;
        std::cout << "Model: " << modelInfo.displayName << " (" << modelInfo.modelId << ")" << std::endl;
        std::cout << "Context size: " << modelInfo.contextSize << " tokens" << std::endl;
        
        // List available models
        std::cout << "\nAvailable models:" << std::endl;
        auto models = provider->listAvailableModels();
        for (const auto& model : models) {
            std::cout << "- " << model.displayName << " (" << model.modelId << ")" << std::endl;
        }
        
        // Interactive chat loop
        std::cout << "\n=== Interactive Chat with " << modelInfo.displayName << " ===\n" << std::endl;
        std::cout << "Type your messages and press Enter. Type 'exit' to quit." << std::endl;
        
        std::vector<ai_editor::Message> conversation;
        
        // Add a system message
        ai_editor::Message systemMessage;
        systemMessage.role = ai_editor::Message::Role::SYSTEM;
        systemMessage.content = "You are a helpful AI assistant running locally on the user's machine using LLama. "
                             "Be concise and helpful in your responses.";
        conversation.push_back(systemMessage);
        
        // Chat loop
        std::string userInput;
        while (true) {
            std::cout << "\nYou: ";
            std::getline(std::cin, userInput);
            
            if (userInput == "exit") {
                break;
            }
            
            // Add user message to conversation
            ai_editor::Message userMessage;
            userMessage.role = ai_editor::Message::Role::USER;
            userMessage.content = userInput;
            conversation.push_back(userMessage);
            
            // Send completion request
            std::cout << "Thinking..." << std::endl;
            auto response = provider->sendCompletionRequest(conversation, {});
            
            if (response.status != ai_editor::CompletionResponse::Status::SUCCESS) {
                std::cerr << "Error: " << response.errorMessage << std::endl;
                continue;
            }
            
            // Display response
            std::cout << "\nAI: " << response.content << std::endl;
            
            // Add assistant message to conversation
            ai_editor::Message assistantMessage;
            assistantMessage.role = ai_editor::Message::Role::ASSISTANT;
            assistantMessage.content = response.content;
            conversation.push_back(assistantMessage);
            
            // Display token usage if available
            if (response.metadata.find("prompt_tokens") != response.metadata.end() &&
                response.metadata.find("completion_tokens") != response.metadata.end()) {
                int promptTokens = std::stoi(response.metadata.at("prompt_tokens"));
                int completionTokens = std::stoi(response.metadata.at("completion_tokens"));
                int totalTokens = promptTokens + completionTokens;
                
                std::cout << "\nTokens used: " << totalTokens 
                          << " (Prompt: " << promptTokens 
                          << ", Completion: " << completionTokens << ")" << std::endl;
            }
        }
        
        std::cout << "Exiting chat. Goodbye!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
} 