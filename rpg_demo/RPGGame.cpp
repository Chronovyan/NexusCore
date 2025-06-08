#include "RPGGame.h"
#include "RPGState.h"
#include <fstream>
#include <sstream>
#include <random>

namespace rpg {

RPGGame::RPGGame(std::shared_ptr<ai_editor::AIManager> aiManager)
    : aiManager_(aiManager) {
    state_ = std::make_shared<GameState>();
    initializeDefaultCommands(commandRegistry_);
}

bool RPGGame::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Load or generate the game world
    loadDefaultWorld();
    
    // Set up the player
    state_->player->currentLocationId = "start_room";
    
    // Add some default items
    auto sword = std::make_shared<SimpleObject>("rusty_sword", "rusty sword", "An old, rusty sword that has seen better days.");
    state_->gameObjects["rusty_sword"] = sword;
    
    auto potion = std::make_shared<SimpleObject>("health_potion", "health potion", "A red potion that restores health when drunk.");
    state_->gameObjects["health_potion"] = potion;
    
    // Add items to starting room
    if (state_->locations.find("start_room") != state_->locations.end()) {
        state_->locations["start_room"]->objects.push_back(sword);
        state_->locations["start_room"]->objects.push_back(potion);
    }
    
    initialized_ = true;
    return true;
}

void RPGGame::update() {
    // Game update logic goes here
}

void RPGGame::shutdown() {
    // Cleanup resources
}

std::string RPGGame::processInput(const std::string& input) {
    if (input.empty()) {
        return "";
    }
    
    // Process the command
    std::string result = commandRegistry_.executeCommand(input, *state_);
    
    // If the command wasn't recognized, try to handle it with AI
    if (result.find("don't understand") != std::string::npos) {
        // This is where we could use AI to interpret natural language commands
        // For now, we'll just return the default message
    }
    
    return result;
}

std::string RPGGame::generateContent(const std::string& prompt) {
    if (!aiManager_) {
        return "AI not available. Could not generate content.";
    }
    
    // Store the prompt for reference
    state_->lastAIPrompt = prompt;
    
    try {
        // For now, use a simple mock response since we don't have access to the full AI provider
        // In a real implementation, this would use the AIManager's methods to generate content
        return "Generated content for: " + prompt;
    } catch (const std::exception& e) {
        return "Error generating content: " + std::string(e.what());
    }
}

void RPGGame::loadDefaultWorld() {
    // Create a simple starting area
    auto startRoom = std::make_shared<Location>(
        "start_room",
        "Old Dungeon Cell",
        "A damp and musty dungeon cell. The stone walls are covered in moss, and a single torch flickers weakly on the wall. "
        "The iron bars on the door are rusted but still strong. There's a wooden bench and a small hole in the corner."
    );
    
    // Add some objects to the room
    auto torch = std::make_shared<SimpleObject>(
        "torch",
        "Rusty Torch",
        "An old, rusty torch mounted on the wall. It provides just enough light to see by."
    );
    
    auto door = std::make_shared<SimpleObject>(
        "door",
        "Wooden Door",
        "A heavy wooden door with iron bands. It looks like it could be opened."
    );
    
    auto bench = std::make_shared<SimpleObject>(
        "bench",
        "Wooden Bench",
        "A simple wooden bench, worn smooth from years of use."
    );
    
    auto hole = std::make_shared<SimpleObject>(
        "hole",
        "Small Hole",
        "A small hole in the wall, just big enough to reach into."
    );
    
    startRoom->objects.push_back(torch);
    startRoom->objects.push_back(door);
    startRoom->objects.push_back(bench);
    startRoom->objects.push_back(hole);
    
    // Save the location
    state_->locations[startRoom->id] = startRoom;
    
    // Set player's starting location
    state_->player->currentLocationId = startRoom->id;
    
    // Create a corridor
    auto corridor = std::make_shared<Location>(
        "corridor",
        "Dungeon Corridor",
        "A long, dark corridor stretches before you. The air is musty and damp."
    );
    
    // Add objects to corridor
    auto barrel = std::make_shared<SimpleObject>(
        "barrel",
        "Wooden Barrel",
        "A wooden barrel, probably used to store water or food."
    );
    
    auto crate = std::make_shared<SimpleObject>(
        "crate",
        "Old Crate",
        "A wooden crate that looks like it might contain something useful."
    );
    
    corridor->objects.push_back(barrel);
    corridor->objects.push_back(crate);
    corridor->exits["south"] = "start_room";
    corridor->exits["east"] = "treasure_room";
    
    // Create a treasure room
    auto treasureRoom = std::make_shared<Location>(
        "treasure_room",
        "Treasure Room",
        "A small room filled with dusty chests and cobwebs. The air is thick with the smell of old wood and rust."
    );
    
    auto chest = std::make_shared<SimpleObject>(
        "chest",
        "Old Chest",
        "A large wooden chest with iron reinforcements. It looks ancient but still sturdy."
    );
    
    auto skeleton = std::make_shared<SimpleObject>(
        "skeleton",
        "Ancient Skeleton",
        "The remains of a long-dead adventurer, still clutching a rusty sword."
    );
    
    treasureRoom->objects.push_back(chest);
    treasureRoom->objects.push_back(skeleton);
    treasureRoom->exits["west"] = "corridor";
    
    // Create a guard room
    auto guardRoom = std::make_shared<Location>(
        "guard_room",
        "Guard Room",
        "A small room with a wooden table and a couple of rickety chairs. The remains of a meal are scattered on the table. "
        "A rusty suit of armor stands in the corner, and a door leads further into the dungeon."
    );
    
    auto armor = std::make_shared<SimpleObject>(
        "armor",
        "Rusty Armor",
        "A rusty suit of armor, probably left here by a long-forgotten guard."
    );
    
    auto table = std::make_shared<SimpleObject>(
        "table",
        "Wooden Table",
        "A simple wooden table, covered in the remains of a meal."
    );
    
    auto chair = std::make_shared<SimpleObject>(
        "chair",
        "Rickety Chair",
        "A rickety wooden chair, probably used by the guards."
    );
    
    guardRoom->objects.push_back(armor);
    guardRoom->objects.push_back(table);
    guardRoom->objects.push_back(chair);
    guardRoom->exits["west"] = "corridor";
    
    // Set up connections between locations
    startRoom->exits["north"] = "corridor";
    corridor->exits["south"] = "start_room";
    corridor->exits["east"] = "treasure_room";
    corridor->exits["west"] = "guard_room";
    treasureRoom->exits["west"] = "corridor";
    guardRoom->exits["east"] = "corridor";
    
    // Save all locations
    state_->locations[corridor->id] = corridor;
    state_->locations[treasureRoom->id] = treasureRoom;
    state_->locations[guardRoom->id] = guardRoom;
    
    // Generate descriptions for the rooms using AI
    if (aiManager_) {
        for (auto& [id, location] : state_->locations) {
            location->description = generateRoomDescription(location->name);
        }
    }
}

void RPGGame::saveGame(const std::string& filename) {
    // In a real implementation, this would save the game state to a file
    // For now, we'll just create a placeholder
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Game saved to " << filename << std::endl;
    }
}

bool RPGGame::loadGame(const std::string& filename) {
    // In a real implementation, this would load the game state from a file
    // For now, we'll just reset to the default world
    loadDefaultWorld();
    return true;
}

std::string RPGGame::generateRoomDescription(const std::string& locationName) {
    return "You are in the " + locationName + ". It's a mysterious place filled with secrets waiting to be discovered.";
}

std::string RPGGame::generateNPCResponse(const std::string& npcName, const std::string& playerInput) {
    return "\"I'm not sure how to respond to that,\" says " + npcName + " thoughtfully.";
}

std::string RPGGame::generateItemDescription(const std::string& itemName) {
    return "A " + itemName + " lies here, its purpose and history lost to time.";
}

} // namespace rpg
