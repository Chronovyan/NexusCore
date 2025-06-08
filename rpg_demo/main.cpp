#include "RPGGame.h"
#include <iostream>
#include <memory>
#include <string>

// Forward declaration of the AIManager initialization function
// This would normally be in a header file
std::shared_ptr<ai_editor::AIManager> initializeAIManager();

int main() {
    std::cout << "=== AI-Assisted Text RPG ===\n";
    std::cout << "Type 'help' for a list of commands.\n\n";
    
    // Initialize the AI manager
    auto aiManager = initializeAIManager();
    if (!aiManager) {
        std::cerr << "Warning: AI features will be limited. Some content may not generate properly.\n";
    }
    
    // Create and initialize the game
    rpg::RPGGame game(aiManager);
    if (!game.initialize()) {
        std::cerr << "Failed to initialize the game.\n";
        return 1;
    }
    
    // Main game loop
    bool running = true;
    while (running) {
        // Display current location and description
        auto state = game.getState();
        auto location = state->getCurrentLocation();
        
        if (location) {
            std::cout << "\n=== " << location->name << " ===\n";
            std::cout << location->description << "\n\n";
            
            // List objects in the room
            if (!location->objects.empty()) {
                std::cout << "You see: ";
                for (size_t i = 0; i < location->objects.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << location->objects[i]->name;
                }
                std::cout << "\n";
            }
            
            // List exits
            if (!location->exits.empty()) {
                std::cout << "Exits: ";
                bool first = true;
                for (const auto& exit : location->exits) {
                    if (!first) std::cout << ", ";
                    std::cout << exit.first;
                    first = false;
                }
                std::cout << "\n";
            }
        }
        
        // Get player input
        std::cout << "\n> ";
        std::string input;
        std::getline(std::cin, input);
        
        // Process input
        if (input == "quit" || input == "q") {
            running = false;
            continue;
        }
        
        std::string result = game.processInput(input);
        if (!result.empty()) {
            std::cout << "\n" << result << "\n";
        }
        
        // Check if the game should end
        if (!state->gameRunning) {
            running = false;
        }
    }
    
    std::cout << "\nThanks for playing!\n";
    return 0;
}

// Simple implementation that returns nullptr for now
// In a real implementation, this would set up the AI manager with API keys, etc.
std::shared_ptr<ai_editor::AIManager> initializeAIManager() {
    // In a real implementation, we would initialize the AI manager here
    // For now, we'll just return nullptr to indicate no AI is available
    return nullptr;
}
