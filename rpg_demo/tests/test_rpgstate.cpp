#include <gtest/gtest.h>
#include "../RPGState.h"

using namespace rpg;

// Test fixture for RPGState tests
class RPGStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test objects
        testLocation = std::make_shared<Location>("test_loc", "Test Location", "A test location");
        testObject = std::make_shared<SimpleObject>("test_obj", "Test Object", "A test object");
        
        // Set up a simple game state
        state.locations[testLocation->id] = testLocation;
        state.gameObjects[testObject->id] = testObject;
        state.player->currentLocationId = testLocation->id;
    }

    // Test objects
    GameState state;
    std::shared_ptr<Location> testLocation;
    std::shared_ptr<GameObject> testObject;
};

// Test basic game object creation
TEST_F(RPGStateTest, GameObjectCreation) {
    SimpleObject obj("obj1", "Test Object", "A test object");
    
    EXPECT_EQ(obj.id, "obj1");
    EXPECT_EQ(obj.name, "Test Object");
    EXPECT_EQ(obj.description, "A test object");
    
    // Test interaction
    std::string result = obj.interact(state);
    EXPECT_NE(result.find("A test object"), std::string::npos);
}

// Test location management
TEST_F(RPGStateTest, LocationManagement) {
    // Test adding a location
    auto loc = std::make_shared<Location>("loc1", "Location 1", "First location");
    state.locations[loc->id] = loc;
    
    EXPECT_NE(state.locations.find("loc1"), state.locations.end());
    EXPECT_EQ(state.locations["loc1"]->name, "Location 1");
}

// Test player movement between locations
TEST_F(RPGStateTest, PlayerMovement) {
    // Create a second location
    auto loc2 = std::make_shared<Location>("loc2", "Location 2", "Second location");
    state.locations[loc2->id] = loc2;
    
    // Connect the locations
    testLocation->exits["east"] = "loc2";
    loc2->exits["west"] = testLocation->id;
    
    // Test initial position
    EXPECT_EQ(state.player->currentLocationId, testLocation->id);
    
    // Test moving to a new location
    state.player->currentLocationId = "loc2";
    EXPECT_EQ(state.player->currentLocationId, "loc2");
    
    // Verify we can get the current location
    auto currentLoc = state.getCurrentLocation();
    ASSERT_NE(currentLoc, nullptr);
    EXPECT_EQ(currentLoc->id, "loc2");
}

// Test object interaction
TEST_F(RPGStateTest, ObjectInteraction) {
    // Add test object to the location
    testLocation->objects.push_back(testObject);
    
    // Test examining the object
    std::string result = testObject->interact(state);
    EXPECT_FALSE(result.empty());
    
    // Test examining the location
    result = testLocation->interact(state);
    EXPECT_FALSE(result.empty());
}

// Test command registry
TEST(CommandRegistryTest, CommandExecution) {
    CommandRegistry registry;
    GameState state;
    
    // Test registering and executing a command
    bool commandExecuted = false;
    registry.registerCommand("test", [&](GameState&, const std::vector<std::string>&) {
        commandExecuted = true;
        return "Command executed";
    });
    
    std::string result = registry.executeCommand("test", state);
    EXPECT_TRUE(commandExecuted);
    EXPECT_EQ(result, "Command executed");
    
    // Test unknown command
    result = registry.executeCommand("unknown", state);
    EXPECT_NE(result.find("don't understand"), std::string::npos);
}

// Test default commands
TEST(DefaultCommandsTest, HelpCommand) {
    CommandRegistry registry;
    GameState state;
    initializeDefaultCommands(registry);
    
    std::string result = registry.executeCommand("help", state);
    EXPECT_NE(result.find("Available commands:"), std::string::npos);
}

// Test look command
TEST(DefaultCommandsTest, LookCommand) {
    CommandRegistry registry;
    initializeDefaultCommands(registry);
    
    // Set up test state
    GameState state;
    auto location = std::make_shared<Location>("test_loc", "Test Location", "A test location");
    state.locations["test_loc"] = location;
    state.player->currentLocationId = "test_loc";
    
    // Add an object to the location
    auto obj = std::make_shared<SimpleObject>("test_obj", "Test Object", "A test object");
    location->objects.push_back(obj);
    
    // Test look command
    std::string result = registry.executeCommand("look", state);
    EXPECT_NE(result.find("Test Location"), std::string::npos);
    EXPECT_NE(result.find("A test location"), std::string::npos);
    EXPECT_NE(result.find("Test Object"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
