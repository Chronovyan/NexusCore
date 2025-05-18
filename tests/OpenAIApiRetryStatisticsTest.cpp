#include <gtest/gtest.h>
#include "../src/MockOpenAI_API_Client.h"
#include "../src/OpenAI_API_Client.h"
#include "../src/EditorError.h"
#include "../src/OpenAI_API_Client_types.h"
#include <chrono>
#include <iostream>

namespace ai_editor {

class OpenAIApiRetryStatisticsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug logging for these tests
        ErrorReporter::debugLoggingEnabled = true;
    }
    
    void TearDown() override {
        // Reset logging
        ErrorReporter::debugLoggingEnabled = false;
    }
};

TEST_F(OpenAIApiRetryStatisticsTest, RetryStatisticsCapture) {
    // Create a mock client
    MockOpenAI_API_Client mockClient;
    
    // Ensure retries are enabled
    mockClient.enableRetries(true);
    
    // Set a custom retry policy
    ApiRetryPolicy policy;
    policy.maxRetries = 3;
    policy.initialBackoff = std::chrono::milliseconds(10); // Fast for testing
    policy.maxBackoff = std::chrono::milliseconds(50);
    policy.retryOnRateLimit = true;
    mockClient.setRetryPolicy(policy);
    
    // Simulate different retry scenarios
    
    // Case 1: Rate limit retry that succeeds
    mockClient.simulateRetries(2, "rate limiting", true);
    std::vector<ApiChatMessage> request1 = {
        {"user", "Test message for rate limit retry"}
    };
    mockClient.setResponseContent("Success after rate limit retry");
    ApiResponse response1 = mockClient.callChatCompletionEndpoint(request1);
    EXPECT_TRUE(response1.success);
    
    // Case 2: Server error retry that fails
    mockClient.simulateRetries(3, "server error", false);
    std::vector<ApiChatMessage> request2 = {
        {"user", "Test message for server error retry"}
    };
    mockClient.setErrorResponse("Internal Server Error", 500);
    ApiResponse response2 = mockClient.callChatCompletionEndpoint(request2);
    EXPECT_FALSE(response2.success);
    
    // Case 3: Network error retry that succeeds
    mockClient.simulateRetries(1, "network error", true);
    std::vector<ApiChatMessage> request3 = {
        {"user", "Test message for network error retry"}
    };
    mockClient.setResponseContent("Success after network retry");
    ApiResponse response3 = mockClient.callChatCompletionEndpoint(request3);
    EXPECT_TRUE(response3.success);
    
    // Get retry statistics
    RetryStatistics::Stats stats = mockClient.getRetryStatistics();
    
    // Verify statistics
    EXPECT_EQ(3, stats.totalRequestsWithRetries);
    EXPECT_EQ(6, stats.totalRetryAttempts); // 2 + 3 + 1
    EXPECT_EQ(2, stats.successfulRetriedRequests); // Case 1 and 3
    EXPECT_EQ(1, stats.failedAfterRetries); // Case 2
    
    // Get report for debugging - skip this part as Stats doesn't have a report member
    // and we don't have direct access to the RetryStatistics object to call getReport()
    std::cout << "Retry Statistics Report - Stats collected:" << std::endl;
    std::cout << "  Total requests with retries: " << stats.totalRequestsWithRetries << std::endl;
    std::cout << "  Total retry attempts: " << stats.totalRetryAttempts << std::endl;
    std::cout << "  Successful after retries: " << stats.successfulRetriedRequests << std::endl;
    std::cout << "  Failed after retries: " << stats.failedAfterRetries << std::endl;
    
    // Verify report contains our retry reasons via the retryReasonCounts map
    EXPECT_NE(stats.retryReasonCounts.find("rate limiting"), stats.retryReasonCounts.end());
    EXPECT_NE(stats.retryReasonCounts.find("server error"), stats.retryReasonCounts.end());
    EXPECT_NE(stats.retryReasonCounts.find("network error"), stats.retryReasonCounts.end());
    
    // Test reset functionality
    mockClient.resetRetryStatistics();
    RetryStatistics::Stats resetStats = mockClient.getRetryStatistics();
    EXPECT_EQ(0, resetStats.totalRequestsWithRetries);
    EXPECT_EQ(0, resetStats.totalRetryAttempts);
    EXPECT_EQ(0, resetStats.successfulRetriedRequests);
    EXPECT_EQ(0, resetStats.failedAfterRetries);
}

TEST_F(OpenAIApiRetryStatisticsTest, DisabledRetriesNoStatistics) {
    // Create a mock client with retries disabled
    MockOpenAI_API_Client mockClient;
    mockClient.enableRetries(false);
    
    // Attempt to simulate a retry (but it should not count since retries are disabled)
    mockClient.simulateRetries(3, "test reason", false);
    std::vector<ApiChatMessage> request = {
        {"user", "Test message with retries disabled"}
    };
    mockClient.setErrorResponse("Error", 500);
    ApiResponse response = mockClient.callChatCompletionEndpoint(request);
    
    // Get statistics - should be zero since retries are disabled
    RetryStatistics::Stats stats = mockClient.getRetryStatistics();
    EXPECT_EQ(0, stats.totalRequestsWithRetries);
    EXPECT_EQ(0, stats.totalRetryAttempts);
}

} // namespace ai_editor 