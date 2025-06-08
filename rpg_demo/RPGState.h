#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <sstream>
#include <deque>

namespace rpg {

// Forward declarations
class GameState;

// Basic game object
// Forward declaration
class GameState;

class GameObject {
public:
    std::string id;
    std::string name;
    std::string description;
    std::map<std::string, std::string> properties;

    GameObject(const std::string& id, const std::string& name, const std::string& desc = "")
        : id(id), name(name), description(desc) {}

    virtual ~GameObject() = default;
    virtual std::string interact(GameState& state) = 0;
};

// Simple concrete implementation for testing
class SimpleObject : public GameObject {
public:
    SimpleObject(const std::string& id, const std::string& name, const std::string& desc = "")
        : GameObject(id, name, desc) {}

    std::string interact(GameState& state) override {
        return "You see " + description;
    }
};

// Location in the game world
class Location : public GameObject {
public:
    std::map<std::string, std::string> exits; // direction -> locationId
    std::vector<std::shared_ptr<GameObject>> objects;

    Location(const std::string& id, const std::string& name, const std::string& desc = "")
        : GameObject(id, name, desc) {}

    std::string interact(GameState& state) override {
        return description;
    }
};

// Player character
class Player : public GameObject {
public:
    std::string currentLocationId;
    std::map<std::string, int> stats;
    std::map<std::string, int> inventory; // itemId -> count

    Player() : GameObject("player", "Player", "The player character") {}

    std::string interact(GameState& state) override {
        return "You look at yourself. " + description;
    }
};

// Game state
class GameState {
public:
    std::shared_ptr<Player> player;
    std::map<std::string, std::shared_ptr<Location>> locations;
    std::map<std::string, std::shared_ptr<GameObject>> gameObjects;
    std::vector<std::string> messageLog;
    bool gameRunning = true;

    // AI generation context
    std::string worldContext;
    std::string lastAIPrompt;
    std::string lastAIResponse;

    GameState() {
        player = std::make_shared<Player>();
        gameObjects["player"] = player;
    }

    void addMessage(const std::string& message) {
        messageLog.push_back(message);
        // Keep log at a reasonable size
        if (messageLog.size() > 100) {
            messageLog.erase(messageLog.begin());
        }
    }

    std::shared_ptr<Location> getCurrentLocation() {
        if (locations.find(player->currentLocationId) != locations.end()) {
            return locations[player->currentLocationId];
        }
        return nullptr;
    }
};

// Command handler function type
using CommandHandler = std::function<std::string(GameState&, const std::vector<std::string>&)>;

// Command registry
class CommandRegistry {
public:
    std::map<std::string, CommandHandler> commands;

    void registerCommand(const std::string& name, CommandHandler handler) {
        commands[name] = handler;
    }

    std::string executeCommand(const std::string& input, GameState& state) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(input);
        
        while (tokenStream >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            return "Please enter a command.";
        }

        std::string command = tokens[0];
        tokens.erase(tokens.begin());

        if (commands.find(command) != commands.end()) {
            return commands[command](state, tokens);
        }

        return "I don't understand that command. Type 'help' for a list of commands.";
    }
};

// Initialize default commands
void initializeDefaultCommands(CommandRegistry& registry);

} // namespace rpg
