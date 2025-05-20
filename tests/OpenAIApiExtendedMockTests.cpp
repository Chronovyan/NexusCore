#include <gtest/gtest.h>
#include "MockOpenAI_API_Client.h"
#include "OpenAI_API_Client_types.h"
#include <vector>

using namespace ai_editor;

// Test fixture for OpenAI API extended mock tests
class OpenAIApiExtendedMockTest : public ::testing::Test {
protected:
    MockOpenAI_API_Client mock;
    
    // Sample model data for tests
    std::vector<ApiModelInfo> createSampleModels() {
        std::vector<ApiModelInfo> models;
        
        ApiModelInfo gpt4;
        gpt4.id = "gpt-4o";
        gpt4.object = "model";
        gpt4.created = "1683758102";
        gpt4.owned_by = "openai";
        
        ApiModelInfo gpt35;
        gpt35.id = "gpt-3.5-turbo";
        gpt35.object = "model";
        gpt35.created = "1677610602";
        gpt35.owned_by = "openai";
        
        models.push_back(gpt4);
        models.push_back(gpt35);
        
        return models;
    }
    
    // Sample embedding data for tests
    std::vector<std::vector<float>> createSampleEmbeddings(size_t count = 1, size_t dimensions = 4) {
        std::vector<std::vector<float>> embeddings;
        
        for (size_t i = 0; i < count; ++i) {
            std::vector<float> embedding;
            for (size_t j = 0; j < dimensions; ++j) {
                embedding.push_back(0.1f * (j + 1) * (i % 2 == 0 ? 1.0f : -1.0f));
            }
            embeddings.push_back(embedding);
        }
        
        return embeddings;
    }
};

// Tests for listModels functionality
TEST_F(OpenAIApiExtendedMockTest, ListModels_Success) {
    // Configure the mock with sample models
    auto models = createSampleModels();
    mock.setSuccessModelListResponse(models);
    
    // Call the method
    ApiModelListResponse response = mock.listModels();
    
    // Verify the method was called
    EXPECT_TRUE(mock.listModels_called_);
    
    // Verify the response
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.models.size(), 2);
    EXPECT_EQ(response.models[0].id, "gpt-4o");
    EXPECT_EQ(response.models[1].id, "gpt-3.5-turbo");
    EXPECT_EQ(response.models[0].owned_by, "openai");
    EXPECT_FALSE(response.raw_json_response.empty());
}

TEST_F(OpenAIApiExtendedMockTest, ListModels_Error) {
    // Configure the mock to return an error
    mock.setErrorModelListResponse("Service unavailable", 503);
    
    // Call the method
    ApiModelListResponse response = mock.listModels();
    
    // Verify the method was called
    EXPECT_TRUE(mock.listModels_called_);
    
    // Verify the error response
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.error_message, "Service unavailable");
    EXPECT_TRUE(response.models.empty());
    EXPECT_TRUE(response.raw_json_response.find("503") != std::string::npos);
}

TEST_F(OpenAIApiExtendedMockTest, ListModels_QueuedResponses) {
    // Create two different responses to queue
    ApiModelListResponse response1;
    response1.success = true;
    response1.raw_json_response = "{\"data\":[{\"id\":\"gpt-4o\"}]}";
    ApiModelInfo model1;
    model1.id = "gpt-4o";
    response1.models.push_back(model1);
    
    ApiModelListResponse response2;
    response2.success = true;
    response2.raw_json_response = "{\"data\":[{\"id\":\"gpt-3.5-turbo\"}]}";
    ApiModelInfo model2;
    model2.id = "gpt-3.5-turbo";
    response2.models.push_back(model2);
    
    // Prime the responses in reverse order of execution
    mock.primeModelListResponse(response1);
    mock.primeModelListResponse(response2);
    
    // The first call should return the second response (FIFO queue)
    ApiModelListResponse firstResult = mock.listModels();
    EXPECT_EQ(firstResult.models[0].id, "gpt-3.5-turbo");
    
    // The second call should return the first response
    ApiModelListResponse secondResult = mock.listModels();
    EXPECT_EQ(secondResult.models[0].id, "gpt-4o");
}

// Tests for retrieveModel functionality
TEST_F(OpenAIApiExtendedMockTest, RetrieveModel_Success) {
    // Configure a specific model response
    ApiModelInfo customModel;
    customModel.id = "gpt-4o";
    customModel.object = "model";
    customModel.created = "1683758102";
    customModel.owned_by = "openai";
    customModel.permissions = {"create", "read"};
    
    mock.setModelResponse("gpt-4o", customModel);
    
    // Call the method
    ApiModelInfo response = mock.retrieveModel("gpt-4o");
    
    // Verify the method was called with the correct ID
    EXPECT_TRUE(mock.retrieveModel_called_);
    EXPECT_EQ(mock.last_retrieved_model_id_, "gpt-4o");
    
    // Verify the response matches what we configured
    EXPECT_EQ(response.id, "gpt-4o");
    EXPECT_EQ(response.owned_by, "openai");
    EXPECT_EQ(response.permissions.size(), 2);
    EXPECT_EQ(response.permissions[0], "create");
}

TEST_F(OpenAIApiExtendedMockTest, RetrieveModel_DefaultResponse) {
    // Don't configure a specific model, let the mock use its default logic
    
    // Call the method for a model ID we haven't explicitly configured
    ApiModelInfo response = mock.retrieveModel("some-model");
    
    // Verify the method was called with the correct ID
    EXPECT_TRUE(mock.retrieveModel_called_);
    EXPECT_EQ(mock.last_retrieved_model_id_, "some-model");
    
    // Verify the default response contains the requested ID
    EXPECT_EQ(response.id, "some-model");
    EXPECT_EQ(response.object, "model");
    EXPECT_EQ(response.owned_by, "organization-owner");
}

TEST_F(OpenAIApiExtendedMockTest, RetrieveModel_Error) {
    // Configure the mock to return an error
    mock.setModelInfoSuccessResponse(false);
    mock.setModelInfoErrorMessage("Model not found");
    
    // Call the method
    ApiModelInfo response = mock.retrieveModel("nonexistent-model");
    
    // Verify the method was called
    EXPECT_TRUE(mock.retrieveModel_called_);
    EXPECT_EQ(mock.last_retrieved_model_id_, "nonexistent-model");
    
    // Verify the error response
    EXPECT_EQ(response.id, "error:Model not found");
}

// Tests for createEmbedding functionality
TEST_F(OpenAIApiExtendedMockTest, CreateEmbedding_Success) {
    // Configure the mock with sample embeddings
    auto embeddings = createSampleEmbeddings(2, 4);
    mock.setSuccessEmbeddingResponse(embeddings, "text-embedding-ada-002");
    
    // Create a request
    ApiEmbeddingRequest request;
    request.input = "Hello, world!";
    request.model = "text-embedding-ada-002";
    request.user = "test-user";
    
    // Call the method
    ApiEmbeddingResponse response = mock.createEmbedding(request);
    
    // Verify the method was called with the correct request
    EXPECT_TRUE(mock.createEmbedding_called_);
    EXPECT_EQ(mock.last_embedding_request_.input, "Hello, world!");
    EXPECT_EQ(mock.last_embedding_request_.model, "text-embedding-ada-002");
    EXPECT_EQ(mock.last_embedding_request_.user, "test-user");
    
    // Verify the response
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.model, "text-embedding-ada-002");
    EXPECT_EQ(response.object, "list");
    EXPECT_EQ(response.data.size(), 2);
    EXPECT_EQ(response.data[0].embedding.size(), 4);
    EXPECT_FLOAT_EQ(response.data[0].embedding[0], 0.1f);
    EXPECT_FLOAT_EQ(response.data[0].embedding[1], 0.2f);
    EXPECT_EQ(response.data[0].index, 0);
    EXPECT_EQ(response.data[0].object, "embedding");
    EXPECT_EQ(response.usage_prompt_tokens, 8);
    EXPECT_EQ(response.usage_total_tokens, 8);
}

TEST_F(OpenAIApiExtendedMockTest, CreateEmbedding_Error) {
    // Configure the mock to return an error
    mock.setErrorEmbeddingResponse("Invalid model", 404);
    
    // Create a request
    ApiEmbeddingRequest request;
    request.input = "Hello, world!";
    request.model = "nonexistent-model";
    
    // Call the method
    ApiEmbeddingResponse response = mock.createEmbedding(request);
    
    // Verify the method was called
    EXPECT_TRUE(mock.createEmbedding_called_);
    
    // Verify the error response
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.error_message, "Invalid model");
    EXPECT_TRUE(response.raw_json_response.find("404") != std::string::npos);
    EXPECT_TRUE(response.data.empty());
}

TEST_F(OpenAIApiExtendedMockTest, CreateEmbedding_QueuedResponses) {
    // Create responses to queue
    ApiEmbeddingResponse response1;
    response1.success = true;
    response1.model = "text-embedding-ada-002";
    response1.object = "list";
    
    ApiEmbeddingData data1;
    data1.embedding = {0.1f, 0.2f};
    data1.index = 0;
    data1.object = "embedding";
    response1.data.push_back(data1);
    
    ApiEmbeddingResponse response2;
    response2.success = true;
    response2.model = "text-embedding-ada-002";
    response2.object = "list";
    
    ApiEmbeddingData data2;
    data2.embedding = {-0.1f, -0.2f};
    data2.index = 0;
    data2.object = "embedding";
    response2.data.push_back(data2);
    
    // Prime the responses in reverse order of execution
    mock.primeEmbeddingResponse(response1);
    mock.primeEmbeddingResponse(response2);
    
    // Create a request
    ApiEmbeddingRequest request;
    request.input = "Test";
    request.model = "text-embedding-ada-002";
    
    // The first call should return the second response (FIFO queue)
    ApiEmbeddingResponse firstResult = mock.createEmbedding(request);
    EXPECT_FLOAT_EQ(firstResult.data[0].embedding[0], -0.1f);
    
    // The second call should return the first response
    ApiEmbeddingResponse secondResult = mock.createEmbedding(request);
    EXPECT_FLOAT_EQ(secondResult.data[0].embedding[0], 0.1f);
} 