#pragma once

#include "RPGState.h"
#include "../src/AIManager.h"
#include <memory>

namespace rpg {

class RPGGame {
public:
    RPGGame(std::shared_ptr<ai_editor::AIManager> aiManager);
    
    // Game lifecycle
    bool initialize();
    void update();
    void shutdown();
    
    // Game state access
    std::shared_ptr<GameState> getState() { return state_; }
    
    // Process player input
    std::string processInput(const std::string& input);
    
    // AI content generation
    std::string generateContent(const std::string& prompt);
    
    // Game state management
    void loadDefaultWorld();
    void saveGame(const std::string& filename);
    bool loadGame(const std::string& filename);
    
    // AI generation helpers - made public for testing
    std::string generateRoomDescription(const std::string& locationName);
    std::string generateNPCResponse(const std::string& npcName, const std::string& playerInput);
    std::string generateItemDescription(const std::string& itemName);
    
private:
    std::shared_ptr<GameState> state_;
    std::shared_ptr<ai_editor::AIManager> aiManager_;
    CommandRegistry commandRegistry_;
    
    // Internal state
    bool initialized_ = false;
};

} // namespace rpg
