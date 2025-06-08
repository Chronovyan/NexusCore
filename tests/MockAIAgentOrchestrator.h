#pragma once

#include "../src/AIAgentOrchestrator.h"

class MockAIAgentOrchestrator : public ai_editor::AIAgentOrchestrator {
public:
    MockAIAgentOrchestrator() = default;
    ~MockAIAgentOrchestrator() override = default;

    // Mock all the pure virtual functions from the base class
    void initialize(const std::string& configPath) override {}
    void updateCodeContext(const std::string& code) override {}
    void setActiveModel(const std::string& modelId) override {}
    std::vector<std::string> getAvailableModels() const override { return {}; }
    std::string getActiveModel() const override { return ""; }
    std::string generateCode(const std::string& prompt) override { return ""; }
    std::string explainCode(const std::string& code) override { return ""; }
    std::string refactorCode(const std::string& code, const std::string& instructions) override { return ""; }
    std::string fixErrors(const std::string& code, const std::vector<std::string>& errors) override { return ""; }
    std::string generateTests(const std::string& code) override { return ""; }
    std::string getCodeSuggestion(const std::string& context) override { return ""; }
    std::vector<std::string> getAvailableAgents() const override { return {}; }
    void setActiveAgent(const std::string& agentId) override {}
    std::string getActiveAgent() const override { return ""; }
    void registerCustomAgent(const std::string& agentId, std::unique_ptr<ai_editor::AIAgent> agent) override {}
    void removeAgent(const std::string& agentId) override {}
    bool hasAgent(const std::string& agentId) const override { return false; }
    std::vector<std::string> getAgentCapabilities(const std::string& agentId) const override { return {}; }
    void setModelParameters(const std::string& modelId, const std::string& parameters) override {}
    std::string getModelParameters(const std::string& modelId) const override { return ""; }
    void setMaxTokens(int maxTokens) override {}
    int getMaxTokens() const override { return 0; }
    void setTemperature(float temperature) override {}
    float getTemperature() const override { return 0.0f; }
    void setTopP(float topP) override {}
    float getTopP() const override { return 0.0f; }
    void setFrequencyPenalty(float penalty) override {}
    float getFrequencyPenalty() const override { return 0.0f; }
    void setPresencePenalty(float penalty) override {}
    float getPresencePenalty() const override { return 0.0f; }
    void setStopSequences(const std::vector<std::string>& stopSequences) override {}
    std::vector<std::string> getStopSequences() const override { return {}; }
    void setMaxRetries(int maxRetries) override {}
    int getMaxRetries() const override { return 0; }
    void setTimeout(int milliseconds) override {}
    int getTimeout() const override { return 0; }
    std::string getLastError() const override { return ""; }
    void clearError() override {}
    bool hasError() const override { return false; }
    void setLoggingEnabled(bool enabled) override {}
    bool isLoggingEnabled() const override { return false; }
    void setLogLevel(int level) override {}
    int getLogLevel() const override { return 0; }
    void setCacheEnabled(bool enabled) override {}
    bool isCacheEnabled() const override { return false; }
    void clearCache() override {}
    void setApiKey(const std::string& apiKey) override {}
    std::string getApiKey() const override { return ""; }
    void setApiBaseUrl(const std::string& baseUrl) override {}
    std::string getApiBaseUrl() const override { return ""; }
    void setApiVersion(const std::string& version) override {}
    std::string getApiVersion() const override { return ""; }
    void setOrganization(const std::string& orgId) override {}
    std::string getOrganization() const override { return ""; }
    void setProject(const std::string& projectId) override {}
    std::string getProject() const override { return ""; }
    void setUser(const std::string& userId) override {}
    std::string getUser() const override { return ""; }
    void setMetadata(const std::string& key, const std::string& value) override {}
    std::string getMetadata(const std::string& key) const override { return ""; }
    void removeMetadata(const std::string& key) override {}
    std::map<std::string, std::string> getAllMetadata() const override { return {}; }
    void clearMetadata() override {}
    void setDefaultModel(const std::string& modelId) override {}
    std::string getDefaultModel() const override { return ""; }
    void setDefaultAgent(const std::string& agentId) override {}
    std::string getDefaultAgent() const override { return ""; }
    void setDefaultParameters(const std::string& parameters) override {}
    std::string getDefaultParameters() const override { return ""; }
    void resetToDefaults() override {}
    std::string getVersion() const override { return "1.0.0"; }
    std::string getBuildInfo() const override { return "Mock Build"; }
    void setDebugMode(bool enabled) override {}
    bool isDebugMode() const override { return false; }
    void setVerboseLogging(bool enabled) override {}
    bool isVerboseLogging() const override { return false; }
    void setMaxConcurrentRequests(int max) override {}
    int getMaxConcurrentRequests() const override { return 1; }
    void setRequestTimeout(int milliseconds) override {}
    int getRequestTimeout() const override { return 10000; }
    void setRetryOnFailure(bool enabled) override {}
    bool getRetryOnFailure() const override { return false; }
    void setMaxRetryAttempts(int attempts) override {}
    int getMaxRetryAttempts() const override { return 0; }
    void setRetryDelay(int milliseconds) override {}
    int getRetryDelay() const override { return 0; }
    void setMaxBatchSize(int size) override {}
    int getMaxBatchSize() const override { return 0; }
    void setBatchTimeout(int milliseconds) override {}
    int getBatchTimeout() const override { return 0; }
    void setCacheSize(int size) override {}
    int getCacheSize() const override { return 0; }
    void setCacheTtl(int seconds) override {}
    int getCacheTtl() const override { return 0; }
    void setCacheDirectory(const std::string& path) override {}
    std::string getCacheDirectory() const override { return ""; }
    void setLogDirectory(const std::string& path) override {}
    std::string getLogDirectory() const override { return ""; }
    void setLogFile(const std::string& filename) override {}
    std::string getLogFile() const override { return ""; }
    void setLogRotationSize(int sizeInMb) override {}
    int getLogRotationSize() const override { return 0; }
    void setLogMaxFiles(int maxFiles) override {}
    int getLogMaxFiles() const override { return 0; }
    void setLogCompressionEnabled(bool enabled) override {}
    bool isLogCompressionEnabled() const override { return false; }
    void setLogAppendMode(bool enabled) override {}
    bool isLogAppendMode() const override { return false; }
    void setLogFlushInterval(int seconds) override {}
    int getLogFlushInterval() const override { return 0; }
    void setLogBufferSize(int size) override {}
    int getLogBufferSize() const override { return 0; }
