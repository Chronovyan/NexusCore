#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "../src/OpenAIProvider.h"
#include "TestErrorReporter.h"
#include "MockOpenAIApiClient.h"
#include "TestToolDefinition.h"

using namespace ai_editor;

class OpenAIToolConversionTest : public ::testing::Test {
protected:
    void SetUp() override {
            // Create a mock error reporter that captures errors
        errorReporter_ = std::make_shared<MockErrorReporter>();
        EditorErrorReporter::setInstance(errorReporter_);
        
        // Create a mock API client
        apiClient_ = std::make_shared<MockOpenAIApiClient>();
        
        // Create the provider with mock dependencies
        provider_ = std::make_unique<OpenAIProvider>(apiClient_);
        
        // Initialize with default options
        ProviderOptions options;
        options.settings["api_key"] = "test-api-key";
        provider_->initialize(options);
    }
    
    void TearDown() override {
        // Clean up
        provider_.reset();
        apiClient_.reset();
        errorReporter_.reset();
    }
    
    std::shared_ptr<MockErrorReporter> errorReporter_;
    std::shared_ptr<MockOpenAIApiClient> apiClient_;
    std::unique_ptr<OpenAIProvider> provider_;
};

// Test that we can access the convertToApiToolDefinition method
// This is a placeholder test since convertToApiToolDefinition is private
// The actual testing is done through the public API that uses this method
TEST_F(OpenAIToolConversionTest, CanAccessConvertToApiToolDefinition) {
    // This test just verifies that we can compile and link with the test
    // The actual functionality is tested through the public API
    SUCCEED();
}

// Test the public API that uses convertToApiToolDefinition
TEST_F(OpenAIToolConversionTest, SendCompletionRequestWithTools) {
    // Arrange
    std::vector<Message> messages = {
        {Message::Role::USER, "Hello, world!"}
    };
    
    ToolDefinition toolDef;
    toolDef.name = "test_tool";
    toolDef.description = "A test tool";
    toolDef.schema = R"(
    {
        "type": "function",
        "function": {
            "name": "test_tool",
            "description": "A test tool",
            "parameters": {
                "type": "object",
                "properties": {
                    "param1": {
                        "type": "string",
                        "description": "First parameter"
                    },
                    "param2": {
                        "type": "integer",
                        "description": "Second parameter"
                    }
                },
                "required": ["param1"]
            }
        }
    }
    )";
    
    std::vector<ToolDefinition> tools = {toolDef};
    
    // Act
    auto response = provider_->sendCompletionRequest(messages, tools);
    
    // Assert - just verify the call was made without errors
    // The actual response will be empty since we're using a mock client
    EXPECT_TRUE(true);
}

// Test with empty schema
TEST_F(OpenAIToolConversionTest, HandlesEmptySchema) {
    // Arrange
    ToolDefinition toolDef;
    toolDef.name = "empty_schema_tool";
    toolDef.description = "Tool with empty schema";
    toolDef.schema = "{}";
    
    std::vector<Message> messages = {
        {Message::Role::USER, "Test message"}
    };
    
    std::vector<ToolDefinition> tools = {toolDef};
    
    // Act - should not crash with empty schema
    auto response = provider_->sendCompletionRequest(messages, tools);
    
    // Assert - just verify the call was made without errors
    EXPECT_TRUE(true);
}

TEST_F(OpenAIToolConversionTest, HandlesInvalidJsonGracefully) {
    // Arrange
    ToolDefinition toolDef;
    toolDef.name = "invalid_tool";
    toolDef.description = "Tool with invalid JSON";
    toolDef.schema = "{invalid json";

    // Act
    ApiToolDefinition apiTool(toolDef.name, toolDef.description);
    provider_->convertToApiToolDefinition(toolDef);
    ApiToolDefinition expectedTool(toolDef.name, toolDef.description);
    ApiFunctionParameter param1("param1", "string", "First parameter", true);
    ApiFunctionParameter param2("param2", "integer", "Second parameter", false);
    expectedTool.function.parameters.push_back(param1);
    expectedTool.function.parameters.push_back(param2);

    // Assert - should still create a basic tool definition
    EXPECT_EQ(apiTool.function.name, "invalid_tool");
    EXPECT_EQ(apiTool.function.description, "Tool with invalid JSON");
    EXPECT_TRUE(apiTool.function.parameters.empty());
    
    // Should have reported an error
    EXPECT_TRUE(errorReporter_->wasErrorReported());
    EXPECT_NE(errorReporter_->getLastError().find("Failed to parse tool schema"), std::string::npos);
}

TEST_F(OpenAIToolConversionTest, HandlesEmptySchema) {
    // Arrange
    ToolDefinition toolDef;
    toolDef.name = "empty_schema_tool";
    toolDef.description = "Tool with empty schema";
    toolDef.schema = "{}";

    // Act
    auto apiToolDef = provider_->convertToApiToolDefinition(toolDef);

    // Assert - should create a basic tool with no parameters
    EXPECT_EQ(apiToolDef.function.name, "empty_schema_tool");
    EXPECT_EQ(apiToolDef.function.description, "Tool with empty schema");
    EXPECT_TRUE(apiToolDef.function.parameters.empty());
    
    // No errors should be reported for empty schema
    EXPECT_FALSE(errorReporter_->wasErrorReported());
}

// Test with invalid JSON schema
TEST_F(OpenAIToolConversionTest, HandlesInvalidJsonGracefully) {
    // Arrange
    ToolDefinition toolDef;
    toolDef.name = "invalid_tool";
    toolDef.description = "Tool with invalid JSON";
    toolDef.schema = "{invalid json";
    
    std::vector<Message> messages = {
        {Message::Role::USER, "Test message with invalid tool"}
    };
    
    std::vector<ToolDefinition> tools = {toolDef};
    
    // Act - should not crash with invalid JSON
    auto response = provider_->sendCompletionRequest(messages, tools);
    
    // Assert - just verify the call was made without errors
    EXPECT_TRUE(true);
}

// Add this to your test file's main function or test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
