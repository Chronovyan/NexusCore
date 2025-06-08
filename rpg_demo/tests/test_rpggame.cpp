#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <map>
#include "../RPGGame.h"
#include "../RPGState.h"
#include "../../src/AIManager.h"
#include "../../src/interfaces/IAIProvider.hpp"

using namespace ai_editor;

using namespace rpg;

// Mock AIProvider for testing
class MockAIProvider : public IAIProvider {
public:
    MockAIProvider() {
        // Initialize with default options
        options_.temperature = 0.7f;
        options_.maxTokens = 2000;
    }
    
    // IAIProvider implementation
    bool initialize(const ProviderOptions& options) override { 
        options_ = options;
        return true; 
    }
    
    bool isInitialized() const override { return true; }
    
    std::string getProviderName() const override { return "MockAI"; }
    
    std::vector<ModelInfo> listAvailableModels() override { 
        return {}; 
    }
    
    ModelInfo getCurrentModelInfo() const override { 
        return {"mock-model", "Mock Model", "MockAI", "1.0", {}, false, 2048, {}}; 
    }
    
    bool setCurrentModel(const std::string&) override { return true; }
    
    CompletionResponse sendCompletionRequest(
        const std::vector<Message>& messages,
        const std::vector<ToolDefinition>& = {}
    ) override {
        if (messages.empty()) {
            return {CompletionResponse::Status::API_ERROR, "", {}, "No messages provided"};
        }
        
        // Get the last user message
        const auto& lastMessage = messages.back();
        lastPrompt = lastMessage.content;
        
        CompletionResponse response;
        response.status = CompletionResponse::Status::SUCCESS;
        response.content = "Generated content for: " + lastPrompt.substr(0, 20) + "...";
        return response;
    }
    
    std::vector<float> generateEmbedding(
        const std::string&,
        const std::optional<std::string>& = std::nullopt
    ) override {
        return {};
    }
    
    ProviderOptions getOptions() const override { return options_; }
    
    void setOptions(const ProviderOptions& options) { options_ = options; }
    
    bool supportsCapability(const std::string&) const override { return false; }
    
    std::shared_ptr<PromptTemplate> getCurrentTemplate() const { 
        return nullptr; 
    }
    
    bool setCurrentTemplate(const std::string&) { return false; }
    
    std::vector<std::string> getAvailableTemplates() const { return {}; }
    
    // Test helpers
    std::string lastPrompt;
    
private:
    ProviderOptions options_;
};

// Mock AIManager for testing
class MockAIManager : public AIManager {
public:
    MockAIManager() {
        // Set up a mock provider
        mockProvider_ = std::make_shared<MockAIProvider>();
    }
    
    ~MockAIManager() = default;
    
    // Implement required AIManager methods
    void registerProvider(const std::string& type, ProviderCreatorFunc creator) {
        // No-op for testing
    }
    
    std::shared_ptr<IAIProvider> createProvider(
        const std::string& type,
        const std::map<std::string, std::string>& options) {
        return std::static_pointer_cast<IAIProvider>(mockProvider_);
    }
    
    bool initializeProvider(
        const std::string& type,
        const std::map<std::string, std::string>& options) {
        return true;
    }
    
    std::string getAvailableProviderTypes() const {
        return "mock";
    }
    
    std::vector<std::string> getAvailableProviderTypesList() const {
        return {"mock"};
    }
    
    std::shared_ptr<IAIProvider> getActiveProvider() const {
        return std::static_pointer_cast<IAIProvider>(mockProvider_);
    }
    
    std::string lastPrompt() const {
        if (auto provider = std::dynamic_pointer_cast<MockAIProvider>(mockProvider_)) {
            return provider->lastPrompt;
        }
        return "";
    }
    
private:
    std::shared_ptr<MockAIProvider> mockProvider_;
};

// Test fixture for RPGGame tests
class RPGGameTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockAIManager_ = std::make_shared<MockAIManager>();
        game = std::make_unique<RPGGame>(mockAIManager_);
        // Don't call initialize() here to test initialization separately
    }
    
    std::unique_ptr<RPGGame> game;
    std::shared_ptr<MockAIManager> mockAIManager_;
    
    // Helper function to create a temporary file for testing
    std::string createTempFile(const std::string& content = "") {
        std::string filename = std::tmpnam(nullptr);
        std::ofstream file(filename);
        file << content;
        file.close();
        return filename;
    }
};

// Test game initialization
TEST_F(RPGGameTest, Initialization) {
    // Game should not be initialized yet
    EXPECT_FALSE(game->getState()->player);
    
    // Initialize the game
    EXPECT_TRUE(game->initialize());
    
    auto state = game->getState();
    ASSERT_NE(state, nullptr);
    
    // Player should be created
    EXPECT_NE(state->player, nullptr);
    
    // Starting location should be set
    EXPECT_FALSE(state->player->currentLocationId.empty());
    EXPECT_NE(state->locations.find(state->player->currentLocationId), state->locations.end());
    
    // Should have some locations loaded
    EXPECT_FALSE(state->locations.empty());
    
    // Should have some game objects loaded
    EXPECT_FALSE(state->gameObjects.empty());
}

// Test command processing
TEST_F(RPGGameTest, CommandProcessing) {
    // First initialize the game
    game->initialize();
    
    // Test look command
    std::string lookResult = game->processInput("look");
    EXPECT_FALSE(lookResult.empty());
    
    // Test go command (should fail if no direction specified)
    std::string goResult = game->processInput("go");
    EXPECT_NE(goResult.find("where"), std::string::npos);
    
    // Test invalid command
    std::string invalidResult = game->processInput("invalid_command");
    EXPECT_NE(invalidResult.find("don't understand"), std::string::npos);
}

// Test world loading
TEST_F(RPGGameTest, WorldLoading) {
    // Initialize the game
    game->initialize();
    auto state = game->getState();
    
    // Check that the starting room exists and has the correct properties
    auto startRoom = state->locations.find("start_room");
    ASSERT_NE(startRoom, state->locations.end());
    EXPECT_EQ(startRoom->second->name, "Old Dungeon Cell");
    
    // Check that some objects are in the starting room
    EXPECT_FALSE(startRoom->second->objects.empty());
    
    // Check that the player is in the starting room
    EXPECT_EQ(state->player->currentLocationId, "start_room");
}

// Test AI content generation
TEST_F(RPGGameTest, AIContentGeneration) {
    // Test room description generation
    std::string roomDesc = game->generateRoomDescription("test_room");
    EXPECT_FALSE(roomDesc.empty());
    EXPECT_NE(roomDesc.find("test_room"), std::string::npos);
    
    // Test NPC response generation
    std::string npcResponse = game->generateNPCResponse("test_npc", "Hello");
    EXPECT_FALSE(npcResponse.empty());
    EXPECT_NE(npcResponse.find("test_npc"), std::string::npos);
    
    // Test item description generation
    std::string itemDesc = game->generateItemDescription("test_item");
    EXPECT_FALSE(itemDesc.empty());
    EXPECT_NE(itemDesc.find("test_item"), std::string::npos);
}

// Test game saving and loading
TEST_F(RPGGameTest, GamePersistence) {
    // Initialize the game
    game->initialize();
    
    // Create a test save file
    std::string saveFile = createTempFile();
    
    // Save the game
    game->saveGame(saveFile);
    
    // Verify the file was created and has content
    std::ifstream testFile(saveFile);
    EXPECT_TRUE(testFile.good());
    std::string content((std::istreambuf_iterator<char>(testFile)), 
                       std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    
    // Test loading the game
    RPGGame newGame(std::make_shared<MockAIManager>());
    EXPECT_TRUE(newGame.loadGame(saveFile));
    
    // Clean up
    std::remove(saveFile.c_str());
}

// Test game state management
TEST_F(RPGGameTest, GameStateManagement) {
    // Test that the game starts uninitialized
    EXPECT_FALSE(game->getState()->player);
    
    // Initialize the game
    EXPECT_TRUE(game->initialize());
    
    // Test that the game is now initialized
    EXPECT_TRUE(game->getState()->player);
    
    // Test shutdown
    game->shutdown();
    // Note: Our current shutdown is a no-op, but we can still test it doesn't crash
    SUCCEED();
}
