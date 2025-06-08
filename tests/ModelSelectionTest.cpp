#include <gtest/gtest.h>
#include "../src/AIManager.h"
#include "../src/interfaces/IAIProvider.hpp"
#include "../src/OpenAIProvider.h"
#include "../src/LlamaProvider.h"
#include <memory>
#include <string>
#include <vector>

using namespace ai_editor;

// Mock provider for testing
class MockProvider : public IAIProvider {
public:
    MockProvider(const std::string& providerId, const std::string& modelId)
        : providerId_(providerId), modelId_(modelId), initialized_(true)
    {
        // Setup default model info
        currentModel_.id = modelId;
        currentModel_.name = "Mock Model " + modelId;
        currentModel_.provider = providerId;
        currentModel_.version = "1.0";
        currentModel_.isLocal = false;
        currentModel_.contextWindowSize = 4096;
        
        // Add a few models to the available models list
        availableModels_.push_back(currentModel_);
        
        ModelInfo model2;
        model2.id = modelId + "-2";
        model2.name = "Mock Model " + modelId + "-2";
        model2.provider = providerId;
        model2.version = "1.0";
        model2.isLocal = false;
        model2.contextWindowSize = 8192;
        availableModels_.push_back(model2);
    }
    
    bool initialize(const ProviderOptions& options) override {
        initialized_ = true;
        options_ = options;
        return true;
    }
    
    bool isInitialized() const override {
        return initialized_;
    }
    
    std::string getProviderName() const override {
        return providerId_;
    }
    
    std::vector<ModelInfo> listAvailableModels() override {
        return availableModels_;
    }
    
    ModelInfo getCurrentModelInfo() const override {
        return currentModel_;
    }
    
    bool setCurrentModel(const std::string& modelId) override {
        for (const auto& model : availableModels_) {
            if (model.id == modelId) {
                currentModel_ = model;
                modelId_ = modelId;
                return true;
            }
        }
        return false;
    }
    
    CompletionResponse sendCompletionRequest(
        const std::vector<Message>& messages,
        const std::vector<ToolDefinition>& tools = {}
    ) override {
        CompletionResponse response;
        response.success = true;
        response.content = "Mock response from " + providerId_ + " model " + modelId_;
        return response;
    }
    
    std::vector<float> generateEmbedding(
        const std::string& input,
        const std::optional<std::string>& modelId = std::nullopt
    ) override {
        return std::vector<float>{0.1f, 0.2f, 0.3f};
    }
    
    ProviderOptions getOptions() const override {
        return options_;
    }
    
    void setOptions(const ProviderOptions& options) override {
        options_ = options;
    }
    
    bool supportsCapability(const std::string& capability) const override {
        auto it = currentModel_.capabilities.find(capability);
        if (it != currentModel_.capabilities.end()) {
            return it->second == "yes" || it->second == "true";
        }
        return false;
    }
    
private:
    std::string providerId_;
    std::string modelId_;
    bool initialized_;
    ModelInfo currentModel_;
    std::vector<ModelInfo> availableModels_;
    ProviderOptions options_;
};

// Factory function for creating MockProvider instances
std::unique_ptr<IAIProvider> createMockProvider(const ProviderOptions& options) {
    std::string providerId = "mock";
    std::string modelId = "mock-model";
    
    if (options.additionalOptions.count("provider_id") > 0) {
        providerId = options.additionalOptions.at("provider_id");
    }
    
    if (options.additionalOptions.count("model_id") > 0) {
        modelId = options.additionalOptions.at("model_id");
    }
    
    return std::make_unique<MockProvider>(providerId, modelId);
}

class ModelSelectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register the mock provider type
        AIProviderFactory::registerProviderType("mock", createMockProvider);
    }
    
    void TearDown() override {
        // Nothing to do
    }
};

TEST_F(ModelSelectionTest, RegisterAndSelectProvider) {
    AIManager manager;
    
    // Register a mock provider
    ProviderOptions options1;
    options1.additionalOptions["provider_id"] = "mock1";
    options1.additionalOptions["model_id"] = "model1";
    ASSERT_TRUE(manager.registerProvider("mock", options1));
    
    // Check that the provider is registered
    EXPECT_TRUE(manager.isProviderRegistered("mock"));
    
    // Get the registered provider types
    std::vector<std::string> providerTypes = manager.getRegisteredProviderTypes();
    EXPECT_EQ(providerTypes.size(), 1);
    EXPECT_EQ(providerTypes[0], "mock");
    
    // Set the provider as active
    EXPECT_TRUE(manager.setActiveProvider("mock"));
    EXPECT_EQ(manager.getActiveProviderType(), "mock");
    
    // Get the current model info
    ModelInfo currentModel = manager.getCurrentModelInfo();
    EXPECT_EQ(currentModel.id, "model1");
    EXPECT_EQ(currentModel.provider, "mock1");
}

TEST_F(ModelSelectionTest, MultipleProvidersAndModelSelection) {
    AIManager manager;
    
    // Register two mock providers
    ProviderOptions options1;
    options1.additionalOptions["provider_id"] = "mock1";
    options1.additionalOptions["model_id"] = "model1";
    ASSERT_TRUE(manager.registerProvider("mock", options1));
    
    ProviderOptions options2;
    options2.additionalOptions["provider_id"] = "mock2";
    options2.additionalOptions["model_id"] = "model2";
    ASSERT_TRUE(manager.registerProvider("mock2", options2));
    
    // Check that both providers are registered
    EXPECT_TRUE(manager.isProviderRegistered("mock"));
    EXPECT_TRUE(manager.isProviderRegistered("mock2"));
    
    // Get the registered provider types
    std::vector<std::string> providerTypes = manager.getRegisteredProviderTypes();
    EXPECT_EQ(providerTypes.size(), 2);
    
    // The first provider should be active by default
    EXPECT_EQ(manager.getActiveProviderType(), "mock");
    
    // List available models from the first provider
    std::vector<ModelInfo> models1 = manager.listAvailableModels();
    EXPECT_EQ(models1.size(), 2);
    EXPECT_EQ(models1[0].id, "model1");
    EXPECT_EQ(models1[1].id, "model1-2");
    
    // Switch to the second provider
    EXPECT_TRUE(manager.setActiveProvider("mock2"));
    EXPECT_EQ(manager.getActiveProviderType(), "mock2");
    
    // List available models from the second provider
    std::vector<ModelInfo> models2 = manager.listAvailableModels();
    EXPECT_EQ(models2.size(), 2);
    EXPECT_EQ(models2[0].id, "model2");
    EXPECT_EQ(models2[1].id, "model2-2");
    
    // Set a different model for the second provider
    EXPECT_TRUE(manager.setCurrentModel("model2-2"));
    
    // Check that the model was changed
    ModelInfo currentModel = manager.getCurrentModelInfo();
    EXPECT_EQ(currentModel.id, "model2-2");
    
    // Switch back to the first provider
    EXPECT_TRUE(manager.setActiveProvider("mock"));
    
    // The first provider should still have its original model
    currentModel = manager.getCurrentModelInfo();
    EXPECT_EQ(currentModel.id, "model1");
    
    // Change the model for the first provider
    EXPECT_TRUE(manager.setCurrentModel("model1-2"));
    
    // Check that the model was changed
    currentModel = manager.getCurrentModelInfo();
    EXPECT_EQ(currentModel.id, "model1-2");
}

TEST_F(ModelSelectionTest, SendCompletionWithDifferentModels) {
    AIManager manager;
    
    // Register two mock providers
    ProviderOptions options1;
    options1.additionalOptions["provider_id"] = "mock1";
    options1.additionalOptions["model_id"] = "model1";
    ASSERT_TRUE(manager.registerProvider("mock", options1));
    
    ProviderOptions options2;
    options2.additionalOptions["provider_id"] = "mock2";
    options2.additionalOptions["model_id"] = "model2";
    ASSERT_TRUE(manager.registerProvider("mock2", options2));
    
    // Create test messages
    std::vector<Message> messages;
    messages.push_back(Message(Message::Role::USER, "Test message"));
    
    // Send a request with the first provider
    EXPECT_TRUE(manager.setActiveProvider("mock"));
    CompletionResponse response1 = manager.sendCompletionRequest(messages);
    EXPECT_TRUE(response1.success);
    EXPECT_EQ(response1.content, "Mock response from mock1 model model1");
    
    // Send a request with the second provider
    EXPECT_TRUE(manager.setActiveProvider("mock2"));
    CompletionResponse response2 = manager.sendCompletionRequest(messages);
    EXPECT_TRUE(response2.success);
    EXPECT_EQ(response2.content, "Mock response from mock2 model model2");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 