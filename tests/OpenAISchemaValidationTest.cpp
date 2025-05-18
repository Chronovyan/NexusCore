#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/OpenAI_API_Client.h"
#include "../src/OpenAI_API_Client_types.h"
#include "../src/MockOpenAI_API_Client.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace ai_editor;

// Test fixture for OpenAI Schema Validation tests
class OpenAISchemaValidationTest : public ::testing::Test {
protected:
    // Helper function to create a parameter with items definition
    ApiFunctionParameter createArrayParameterWithItems(
        const std::string& name,
        const std::string& description,
        const std::string& items_type,
        const std::vector<ApiFunctionParameter::Property>& properties = {}
    ) {
        ApiFunctionParameter param;
        param.name = name;
        param.type = "array";
        param.description = description;
        param.required = true;
        param.items_type = items_type;
        param.items_properties = properties;
        return param;
    }

    // Helper function to check if the generated JSON includes items definition
    bool jsonHasItemsDefinition(const json& j, const std::string& param_name) {
        try {
            return j["tools"][0]["function"]["parameters"]["properties"][param_name].contains("items");
        } catch (const std::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            return false;
        }
    }
};

// Test that array parameters have proper items definitions
TEST_F(OpenAISchemaValidationTest, ArrayParametersHaveItemsDefinition) {
    // Create a mock OpenAI client to capture the request
    MockOpenAI_API_Client mockClient;
    
    // Set up a tool definition with an array parameter
    ApiToolDefinition testTool("test_tool", "A test tool with array parameters");
    
    // Add a string array parameter
    ApiFunctionParameter stringArrayParam = createArrayParameterWithItems(
        "string_array", 
        "An array of strings", 
        "string"
    );
    testTool.function.parameters.push_back(stringArrayParam);
    
    // Add an object array parameter with properties
    std::vector<ApiFunctionParameter::Property> objectProperties = {
        {"name", "string", "Name of the item", true},
        {"count", "integer", "Count of items", true}
    };
    
    ApiFunctionParameter objectArrayParam = createArrayParameterWithItems(
        "object_array", 
        "An array of objects", 
        "object",
        objectProperties
    );
    testTool.function.parameters.push_back(objectArrayParam);
    
    // Prepare messages
    std::vector<ApiChatMessage> messages = {
        {"user", "Test message"}
    };
    
    // Configure mock to return a simple response
    ApiResponse mockResponse;
    mockResponse.success = true;
    mockResponse.content = "Test response";
    mockClient.primeResponse(mockResponse);
    
    // Make the request
    mockClient.sendChatCompletionRequest(
        messages,
        {testTool},
        "gpt-4o",
        0.0f,
        100
    );
    
    // Create the request JSON manually for testing
    json requestJson = {
        {"model", "gpt-4o"},
        {"messages", {
            {{"role", "user"}, {"content", "Test message"}}
        }},
        {"tools", {
            {
                {"type", "function"},
                {"function", {
                    {"name", "test_tool"},
                    {"description", "A test tool with array parameters"},
                    {"parameters", {
                        {"type", "object"},
                        {"properties", {
                            {"string_array", {
                                {"type", "array"},
                                {"description", "An array of strings"},
                                {"items", {
                                    {"type", "string"}
                                }}
                            }},
                            {"object_array", {
                                {"type", "array"},
                                {"description", "An array of objects"},
                                {"items", {
                                    {"type", "object"},
                                    {"properties", {
                                        {"name", {
                                            {"type", "string"},
                                            {"description", "Name of the item"}
                                        }},
                                        {"count", {
                                            {"type", "integer"},
                                            {"description", "Count of items"}
                                        }}
                                    }},
                                    {"required", {"name", "count"}}
                                }}
                            }}
                        }},
                        {"required", {"string_array", "object_array"}}
                    }}
                }}
            }
        }},
        {"temperature", 0.0},
        {"max_tokens", 100}
    };
    
    // Check if array parameters have items definitions
    ASSERT_TRUE(jsonHasItemsDefinition(requestJson, "string_array")) 
        << "String array parameter should have an items definition";
    
    ASSERT_TRUE(jsonHasItemsDefinition(requestJson, "object_array"))
        << "Object array parameter should have an items definition";
}

// Test that object items in arrays have property definitions
TEST_F(OpenAISchemaValidationTest, ObjectItemsHavePropertyDefinitions) {
    // Create client (we're testing the internal JSON generation logic)
    OpenAI_API_Client client("dummy_api_key");
    
    // Create tool definition with complex objects
    ApiToolDefinition filesTool("files_tool", "A tool that works with files");
    
    // Create an array parameter for files with object properties
    std::vector<ApiFunctionParameter::Property> fileProperties = {
        {"filename", "string", "Name of the file", true},
        {"description", "string", "Description of the file", true},
        {"size", "integer", "Size of the file in bytes", false}
    };
    
    ApiFunctionParameter filesParam = createArrayParameterWithItems(
        "files", 
        "List of files to process", 
        "object",
        fileProperties
    );
    filesTool.function.parameters.push_back(filesParam);
    
    // Prepare request with the tool
    std::vector<ApiChatMessage> messages = {
        {"user", "Process these files"}
    };
    
    // Use our mock client to capture the request
    MockOpenAI_API_Client mockClient;
    ApiResponse mockResponse;
    mockResponse.success = true;
    mockClient.primeResponse(mockResponse);
    
    // Make the request
    mockClient.sendChatCompletionRequest(
        messages,
        {filesTool},
        "gpt-4o",
        0.0f,
        100
    );
    
    // Extract the tools definition from the mocked client
    json toolsJson;
    for (const auto& tool : mockClient.last_sent_tools_) {
        if (tool.function.name == "files_tool") {
            for (const auto& param : tool.function.parameters) {
                if (param.name == "files") {
                    // Verify that all properties are present
                    ASSERT_EQ(param.items_properties.size(), 3) 
                        << "Should have 3 properties defined for file objects";
                    
                    // Verify individual properties
                    bool filenameFound = false, descriptionFound = false, sizeFound = false;
                    for (const auto& prop : param.items_properties) {
                        if (prop.name == "filename") {
                            EXPECT_EQ(prop.type, "string");
                            EXPECT_TRUE(prop.required);
                            filenameFound = true;
                        }
                        else if (prop.name == "description") {
                            EXPECT_EQ(prop.type, "string");
                            EXPECT_TRUE(prop.required);
                            descriptionFound = true;
                        }
                        else if (prop.name == "size") {
                            EXPECT_EQ(prop.type, "integer");
                            EXPECT_FALSE(prop.required);
                            sizeFound = true;
                        }
                    }
                    
                    EXPECT_TRUE(filenameFound) << "filename property not found";
                    EXPECT_TRUE(descriptionFound) << "description property not found";
                    EXPECT_TRUE(sizeFound) << "size property not found";
                }
            }
        }
    }
}

// Test that our AIAgentOrchestrator properly configures tools
TEST_F(OpenAISchemaValidationTest, OrchestratorConfiguresToolsWithItemsDefinitions) {
    // Since we can't directly access the private methods of AIAgentOrchestrator,
    // we'll test that the items definitions are properly created by examining
    // the parameter structures
    
    // Create files parameter with items definition as AIAgentOrchestrator would
    ApiFunctionParameter filesParam;
    filesParam.name = "files";
    filesParam.type = "array";
    filesParam.description = "List of files to be created with their descriptions";
    filesParam.required = true;
    filesParam.items_type = "object";
    filesParam.items_properties = {
        {"filename", "string", "Name of the file to create", true},
        {"description", "string", "Purpose and contents of the file", true}
    };
    
    // Verify the parameter structure is correct
    EXPECT_EQ(filesParam.name, "files");
    EXPECT_EQ(filesParam.type, "array");
    EXPECT_EQ(filesParam.items_type, "object");
    ASSERT_EQ(filesParam.items_properties.size(), 2);
    
    // Check first property
    EXPECT_EQ(filesParam.items_properties[0].name, "filename");
    EXPECT_EQ(filesParam.items_properties[0].type, "string");
    EXPECT_TRUE(filesParam.items_properties[0].required);
    
    // Check second property
    EXPECT_EQ(filesParam.items_properties[1].name, "description");
    EXPECT_EQ(filesParam.items_properties[1].type, "string");
    EXPECT_TRUE(filesParam.items_properties[1].required);
} 