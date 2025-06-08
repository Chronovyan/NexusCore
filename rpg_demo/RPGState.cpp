#include "RPGState.h"
#include <sstream>

namespace rpg {

// Command implementations
std::string cmdLook(GameState& state, const std::vector<std::string>& args) {
    auto location = state.getCurrentLocation();
    if (!location) {
        return "You are in the void. This shouldn't happen.";
    }
    
    std::stringstream ss;
    ss << "== " << location->name << " ==\n";
    ss << location->description << "\n\n";
    
    if (!location->objects.empty()) {
        ss << "You see: ";
        for (size_t i = 0; i < location->objects.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << location->objects[i]->name;
        }
        ss << "\n";
    }
    
    if (!location->exits.empty()) {
        ss << "Exits: ";
        bool first = true;
        for (const auto& exit : location->exits) {
            if (!first) ss << ", ";
            ss << exit.first;
            first = false;
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string cmdGo(GameState& state, const std::vector<std::string>& args) {
    if (args.empty()) {
        return "Go where?";
    }
    
    auto location = state.getCurrentLocation();
    if (!location) {
        return "You can't go anywhere from here.";
    }
    
    std::string direction = args[0];
    if (location->exits.find(direction) == location->exits.end()) {
        return "You can't go that way.";
    }
    
    std::string newLocationId = location->exits[direction];
    if (state.locations.find(newLocationId) == state.locations.end()) {
        return "You can't go that way (invalid location).";
    }
    
    state.player->currentLocationId = newLocationId;
    return cmdLook(state, {});
}

std::string cmdHelp(GameState&, const std::vector<std::string>&) {
    return R"(
Available commands:
  look/l - Look around
  go <direction> - Move in a direction (north, south, east, west, etc.)
  get/take <item> - Pick up an item
  drop <item> - Drop an item
  inventory/i - Check your inventory
  examine/x <object> - Examine an object
  talk/to <npc> - Talk to an NPC
  help - Show this help
  quit - Quit the game
)";
}

std::string cmdQuit(GameState& state, const std::vector<std::string>&) {
    state.gameRunning = false;
    return "Thanks for playing!";
}

std::string cmdExamine(GameState& state, const std::vector<std::string>& args) {
    if (args.empty()) {
        return "Examine what?";
    }
    
    std::string target = args[0];
    auto location = state.getCurrentLocation();
    
    // Check objects in the room
    for (const auto& obj : location->objects) {
        if (obj->name == target) {
            return obj->interact(state);
        }
    }
    
    // Check inventory
    if (state.player->inventory.find(target) != state.player->inventory.end()) {
        if (state.gameObjects.find(target) != state.gameObjects.end()) {
            return state.gameObjects[target]->interact(state);
        }
        return "You examine the " + target + ". It's a " + target + ".";
    }
    
    // Check the room itself
    if (location->name == target) {
        return location->interact(state);
    }
    
    return "You don't see that here.";
}

std::string cmdInventory(GameState& state, const std::vector<std::string>&) {
    if (state.player->inventory.empty()) {
        return "You're not carrying anything.";
    }
    
    std::stringstream ss;
    ss << "You are carrying:";
    for (const auto& item : state.player->inventory) {
        ss << "\n- " << item.first;
        if (item.second > 1) {
            ss << " (" << item.second << ")";
        }
    }
    return ss.str();
}

std::string cmdGet(GameState& state, const std::vector<std::string>& args) {
    if (args.empty()) {
        return "Get what?";
    }
    
    std::string itemName = args[0];
    auto location = state.getCurrentLocation();
    
    // Find the item in the room
    for (auto it = location->objects.begin(); it != location->objects.end(); ++it) {
        if ((*it)->name == itemName) {
            // Add to inventory
            state.player->inventory[itemName]++;
            
            // Remove from room
            location->objects.erase(it);
            
            return "You take the " + itemName + ".";
        }
    }
    
    return "You don't see that here.";
}

std::string cmdDrop(GameState& state, const std::vector<std::string>& args) {
    if (args.empty()) {
        return "Drop what?";
    }
    
    std::string itemName = args[0];
    auto& inventory = state.player->inventory;
    
    if (inventory.find(itemName) == inventory.end()) {
        return "You're not carrying that.";
    }
    
    // Remove from inventory
    if (--inventory[itemName] <= 0) {
        inventory.erase(itemName);
    }
    
    // Add to room
    auto location = state.getCurrentLocation();
    if (state.gameObjects.find(itemName) != state.gameObjects.end()) {
        location->objects.push_back(state.gameObjects[itemName]);
    } else {
        auto item = std::make_shared<SimpleObject>(itemName, itemName, "A " + itemName);
        state.gameObjects[itemName] = item;
        location->objects.push_back(item);
    }
    
    return "You drop the " + itemName + ".";
}

void initializeDefaultCommands(CommandRegistry& registry) {
    registry.registerCommand("look", cmdLook);
    registry.registerCommand("l", cmdLook);
    registry.registerCommand("go", cmdGo);
    registry.registerCommand("help", cmdHelp);
    registry.registerCommand("quit", cmdQuit);
    registry.registerCommand("q", cmdQuit);
    registry.registerCommand("examine", cmdExamine);
    registry.registerCommand("x", cmdExamine);
    registry.registerCommand("inventory", cmdInventory);
    registry.registerCommand("i", cmdInventory);
    registry.registerCommand("get", cmdGet);
    registry.registerCommand("take", cmdGet);
    registry.registerCommand("drop", cmdDrop);
}

} // namespace rpg
