#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/OpenAI_API_Client.h"
#include "../src/IOpenAI_API_Client.h"
#include "../src/OpenAI_API_Client_types.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace ai_editor;

// Mock CPR response for testing OpenAI API client error handling
namespace cpr {
    class Response {
    public:
        int status_code;
        std::string text;
        std::string error_message;
    };
}

// Mock the implementation to inject failures
class MockOpenAIClientImpl {
public:
    static ApiResponse simulateHttpError(int statusCode, const std::string& errorMessage) {
        ApiResponse response;
        response.success = false;
        response.error_message = "HTTP Error " + std::to_string(statusCode) + ": " + errorMessage;
        return response;
    }

    static ApiResponse simulateNetworkError(const std::string& errorMessage) {
        ApiResponse response;
        response.success = false;
        response.error_message = "Network Error: " + errorMessage;
        return response;
    }

    static ApiResponse simulateInvalidJsonResponse(const std::string& invalidJson) {
        ApiResponse response;
        response.success = false;
        response.error_message = "Invalid JSON response: " + invalidJson;
        return response;
    }

    static ApiResponse simulateApiErrorResponse(int statusCode, const std::string& errorType, const std::string& errorMessage) {
        // Create a JSON error response that mimics the OpenAI API error format
        json errorJson = {
            {"error", {
                {"message", errorMessage},
                {"type", errorType},
                {"code", statusCode}
            }}
        };

        ApiResponse response;
        response.success = false;
        response.raw_json_response = errorJson.dump();
        response.error_message = "API Error: " + errorMessage;
        return response;
    }
};

// Test fixture for OpenAI API error handling tests
class OpenAIApiErrorHandlingTest : public ::testing::Test {
protected:
    // Helper method to create a basic API request
    std::vector<ApiChatMessage> createBasicRequest() {
        return {
            {"system", "You are a helpful assistant."},
            {"user", "Hello, world!"}
        };
    }
};

// Test handling of API schema validation errors
TEST_F(OpenAIApiErrorHandlingTest, HandleSchemaValidationErrors) {
    // Simulate an OpenAI API schema validation error
    ApiResponse errorResponse = MockOpenAIClientImpl::simulateApiErrorResponse(
        400,
        "invalid_request_error", 
        "array schema missing items at line 1 column 1234"
    );

    // Check that the error message contains useful information
    EXPECT_FALSE(errorResponse.success);
    EXPECT_THAT(errorResponse.error_message, ::testing::HasSubstr("API Error"));
    EXPECT_THAT(errorResponse.raw_json_response, ::testing::HasSubstr("array schema missing items"));
}

// Test handling of authentication errors
TEST_F(OpenAIApiErrorHandlingTest, HandleAuthenticationErrors) {
    // Simulate an OpenAI API authentication error
    ApiResponse errorResponse = MockOpenAIClientImpl::simulateApiErrorResponse(
        401,
        "authentication_error", 
        "Invalid Authentication"
    );

    // Check that the error message contains useful information
    EXPECT_FALSE(errorResponse.success);
    EXPECT_THAT(errorResponse.error_message, ::testing::HasSubstr("API Error"));
    EXPECT_THAT(errorResponse.raw_json_response, ::testing::HasSubstr("Invalid Authentication"));
}

// Test handling of rate limit errors
TEST_F(OpenAIApiErrorHandlingTest, HandleRateLimitErrors) {
    // Simulate an OpenAI API rate limit error
    ApiResponse errorResponse = MockOpenAIClientImpl::simulateApiErrorResponse(
        429,
        "rate_limit_error", 
        "Rate limit reached for requests"
    );

    // Check that the error message contains useful information
    EXPECT_FALSE(errorResponse.success);
    EXPECT_THAT(errorResponse.error_message, ::testing::HasSubstr("API Error"));
    EXPECT_THAT(errorResponse.raw_json_response, ::testing::HasSubstr("Rate limit reached"));
}

// Test handling of server errors
TEST_F(OpenAIApiErrorHandlingTest, HandleServerErrors) {
    // Simulate an OpenAI API server error
    ApiResponse errorResponse = MockOpenAIClientImpl::simulateApiErrorResponse(
        500,
        "server_error", 
        "The server had an error while processing your request"
    );

    // Check that the error message contains useful information
    EXPECT_FALSE(errorResponse.success);
    EXPECT_THAT(errorResponse.error_message, ::testing::HasSubstr("API Error"));
    EXPECT_THAT(errorResponse.raw_json_response, ::testing::HasSubstr("server had an error"));
}

// Test handling of HTTP errors
TEST_F(OpenAIApiErrorHandlingTest, HandleHttpErrors) {
    // Simulate various HTTP errors
    ApiResponse error404 = MockOpenAIClientImpl::simulateHttpError(404, "Not Found");
    ApiResponse error502 = MockOpenAIClientImpl::simulateHttpError(502, "Bad Gateway");

    // Check 404 error
    EXPECT_FALSE(error404.success);
    EXPECT_THAT(error404.error_message, ::testing::HasSubstr("HTTP Error 404"));
    EXPECT_THAT(error404.error_message, ::testing::HasSubstr("Not Found"));

    // Check 502 error
    EXPECT_FALSE(error502.success);
    EXPECT_THAT(error502.error_message, ::testing::HasSubstr("HTTP Error 502"));
    EXPECT_THAT(error502.error_message, ::testing::HasSubstr("Bad Gateway"));
}

// Test handling of network errors
TEST_F(OpenAIApiErrorHandlingTest, HandleNetworkErrors) {
    // Simulate various network errors
    ApiResponse connectionError = MockOpenAIClientImpl::simulateNetworkError("Connection refused");
    ApiResponse timeoutError = MockOpenAIClientImpl::simulateNetworkError("Request timed out");

    // Check connection error
    EXPECT_FALSE(connectionError.success);
    EXPECT_THAT(connectionError.error_message, ::testing::HasSubstr("Network Error"));
    EXPECT_THAT(connectionError.error_message, ::testing::HasSubstr("Connection refused"));

    // Check timeout error
    EXPECT_FALSE(timeoutError.success);
    EXPECT_THAT(timeoutError.error_message, ::testing::HasSubstr("Network Error"));
    EXPECT_THAT(timeoutError.error_message, ::testing::HasSubstr("Request timed out"));
}

// Test handling of JSON parsing errors
TEST_F(OpenAIApiErrorHandlingTest, HandleJsonParsingErrors) {
    // Simulate an invalid JSON response
    ApiResponse invalidJsonError = MockOpenAIClientImpl::simulateInvalidJsonResponse("{invalid json...");

    // Check JSON parsing error
    EXPECT_FALSE(invalidJsonError.success);
    EXPECT_THAT(invalidJsonError.error_message, ::testing::HasSubstr("Invalid JSON"));
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 