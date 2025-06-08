#include "LlamaProvider.h"
#include "EditorErrorReporter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <regex>
#include <random>

namespace fs = std::filesystem;

namespace ai_editor {

LlamaProvider::LlamaProvider(const std::string& modelPath)
    : modelPath_(modelPath)
    , currentModelId_("")
    , modelContext_(nullptr)
    , initialized_(false)
    , stopWorker_(false)
    , templateManager_(std::make_shared<PromptTemplateManager>())
    , currentTemplate_(nullptr)
{
    // Initialize default templates
    templateManager_->initializeDefaultTemplates();
    
    // Ensure the model path exists
    if (!fs::exists(modelPath)) {
        throw std::runtime_error("LlamaProvider: Model path does not exist: " + modelPath);
    }
}

LlamaProvider::~LlamaProvider()
{
    // Signal worker thread to stop
    stopWorker_ = true;
    
    // Wait for worker thread to finish
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    // Unload the model
    unloadModel();
}

bool LlamaProvider::initialize(const ProviderOptions& options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Provider already initialized",
            "Call initialize() only once"
        );
        return false;
    }
    
    options_ = options;
    
    // Scan for available models
    scanAvailableModels();
    
    // Set up default model if available
    if (!modelInfoCache_.empty() && currentModelId_.empty()) {
        auto it = modelInfoCache_.begin();
        currentModelId_ = it->first;
    }
    
    // Use specified template if provided
    if (!options.templateId.empty()) {
        setCurrentTemplate(options.templateId);
    } else {
        // Select best template for the current model
        selectBestTemplateForModel();
    }
    
    initialized_ = true;
    return true;
}

bool LlamaProvider::isInitialized() const
{
    return initialized_;
}

std::string LlamaProvider::getProviderName() const
{
    return "LLama";
}

std::vector<ModelInfo> LlamaProvider::listAvailableModels()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Cannot list models: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    // Rescan for models in case any were added or removed
    scanAvailableModels();
    
    // Convert the model cache to a vector
    std::vector<ModelInfo> models;
    models.reserve(modelInfoCache_.size());
    
    for (const auto& [_, modelInfo] : modelInfoCache_) {
        models.push_back(modelInfo);
    }
    
    return models;
}

ModelInfo LlamaProvider::getCurrentModelInfo() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Cannot get current model info: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    auto it = modelInfoCache_.find(currentModelId_);
    if (it == modelInfoCache_.end()) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Current model not found in cache: " + currentModelId_,
            "This should not happen, please report this bug"
        );
        return {};
    }
    
    return it->second;
}

bool LlamaProvider::setCurrentModel(const std::string& modelId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Cannot set model: Provider not initialized",
            "Call initialize() first"
        );
        return false;
    }
    
    // Check if the model exists
    auto it = modelInfoCache_.find(modelId);
    if (it == modelInfoCache_.end()) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Model not found: " + modelId,
            "Check available models with listAvailableModels()"
        );
        return false;
    }
    
    // Unload current model if loaded
    if (modelContext_ != nullptr) {
        unloadModel();
    }
    
    // Set new model ID
    currentModelId_ = modelId;
    
    // Select best template for the new model
    selectBestTemplateForModel();
    
    return true;
}

CompletionResponse LlamaProvider::sendCompletionRequest(
    const std::vector<Message>& messages,
    const std::vector<ToolDefinition>& tools)
{
    if (!isInitialized()) {
        return {CompletionResponse::Status::API_ERROR, "", {}, "Provider not initialized", {}};
    }
    
    std::string prompt;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if we have a current model
        if (currentModelId_.empty()) {
            return {CompletionResponse::Status::API_ERROR, "", {}, "No model selected", {}};
        }
        
        // Use template to convert messages to a prompt string if available
        if (currentTemplate_) {
            prompt = currentTemplate_->formatConversation(messages);
        } else {
            // Fall back to the default conversion method
            prompt = convertMessagesToPrompt(messages);
        }
    }
    
    // For simulation purposes, generate a response based on the prompt
    std::string response = generateSimulatedResponse(prompt, messages);
    
    // Check if we should generate a tool call
    bool shouldCallTool = shouldGenerateToolCall(prompt);
    
    CompletionResponse result;
    result.status = CompletionResponse::Status::SUCCESS;
    
    if (shouldCallTool && !tools.empty()) {
        // Extract or generate tool calls
        auto [content, toolCalls] = extractToolCalls(response, tools);
        result.content = content;
        result.toolCalls = toolCalls;
    } else {
        result.content = response;
    }
    
    // Add metadata
    result.metadata["model"] = currentModelId_;
    result.metadata["prompt_tokens"] = std::to_string(countTokens(prompt));
    result.metadata["completion_tokens"] = std::to_string(countTokens(response));
    result.metadata["total_tokens"] = std::to_string(countTokens(prompt) + countTokens(response));
    
    return result;
}

std::vector<float> LlamaProvider::generateEmbedding(
    const std::string& input,
    const std::optional<std::string>& modelId)
{
    if (!initialized_) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Cannot generate embedding: Provider not initialized",
            "Call initialize() first"
        );
        return {};
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Check if model is loaded
        if (!modelContext_) {
            EditorErrorReporter::reportError(
                "LlamaProvider",
                "Cannot generate embedding: Model not loaded",
                "Load a model first"
            );
            return {};
        }
        
        // In a real implementation, we would use the LLama model to generate embeddings
        // For this skeleton, we'll just return a placeholder embedding
        
        // Create a placeholder embedding (random values)
        std::vector<float> embedding(384, 0.0f);
        for (size_t i = 0; i < embedding.size(); ++i) {
            embedding[i] = static_cast<float>(rand()) / RAND_MAX;
        }
        
        // Normalize the embedding (very basic normalization)
        float sum = 0.0f;
        for (float val : embedding) {
            sum += val * val;
        }
        
        float norm = std::sqrt(sum);
        for (float& val : embedding) {
            val /= norm;
        }
        
        return embedding;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Exception generating embedding: " + std::string(e.what()),
            "Check model state"
        );
        return {};
    }
}

ProviderOptions LlamaProvider::getOptions() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return options_;
}

void LlamaProvider::setOptions(const ProviderOptions& options)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    options_ = options;
    
    // Apply model-specific options if needed
    // For LLama, we might have options like context size, temperature, etc.
    
    // If a model is specified, try to load it
    if (options.additionalOptions.count("model") > 0) {
        std::string modelId = options.additionalOptions.at("model");
        if (currentModelId_ != modelId) {
            unloadModel();
            loadModel(modelId);
            currentModelId_ = modelId;
        }
    }
}

bool LlamaProvider::supportsCapability(const std::string& capability) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return false;
    }
    
    // Check if the current model is cached
    auto it = modelInfoCache_.find(currentModelId_);
    if (it == modelInfoCache_.end()) {
        return false;
    }
    
    // Check if the capability is supported
    const auto& capabilities = it->second.capabilities;
    auto capIt = capabilities.find(capability);
    if (capIt == capabilities.end()) {
        return false;
    }
    
    // Check if the capability is enabled
    return capIt->second == "yes" || 
           capIt->second == "true" || 
           capIt->second == "supported" || 
           capIt->second == "enabled";
}

void LlamaProvider::scanAvailableModels()
{
    try {
        // Clear the cache
        modelInfoCache_.clear();
        
        // Scan for model files in the model path
        for (const auto& entry : fs::directory_iterator(modelPath_)) {
            // Check if this is a file with a supported extension
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                
                // Check for supported extensions (add more as needed)
                if (ext == ".bin" || ext == ".gguf" || ext == ".ggml") {
                    std::string modelId = entry.path().stem().string();
                    std::string modelPath = entry.path().string();
                    
                    // Create model info
                    ModelInfo info;
                    info.id = modelId;
                    info.name = modelId;
                    info.provider = "LLama";
                    info.version = "local";
                    info.isLocal = true;
                    
                    // Get file size as a rough estimate of model capacity
                    uintmax_t fileSize = fs::file_size(entry.path());
                    
                    // Estimate context window based on file size (very rough heuristic)
                    // In a real implementation, this would be determined by the model itself
                    info.contextWindowSize = static_cast<size_t>(fileSize / 1000000) * 256;
                    
                    // Set basic capabilities
                    info.capabilities["text_completion"] = "yes";
                    info.capabilities["tools"] = "limited";
                    info.capabilities["function_calling"] = "limited";
                    info.capabilities["embeddings"] = "yes";
                    info.capabilities["vision"] = "no";
                    
                    // Add additional info
                    info.additionalInfo["file_path"] = modelPath;
                    info.additionalInfo["file_size_bytes"] = std::to_string(fileSize);
                    
                    // Add to cache
                    modelInfoCache_[modelId] = info;
                }
            }
        }
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Exception scanning models: " + std::string(e.what()),
            "Check model path permissions"
        );
    }
}

bool LlamaProvider::loadModel(const std::string& modelId)
{
    try {
        // Check if the model exists in the cache
        auto it = modelInfoCache_.find(modelId);
        if (it == modelInfoCache_.end()) {
            EditorErrorReporter::reportError(
                "LlamaProvider",
                "Model not found in cache: " + modelId,
                "Scan for models first"
            );
            return false;
        }
        
        // Get the model file path
        std::string modelPath = it->second.additionalInfo.at("file_path");
        
        // In a real implementation, this would load the LLama model
        // For this skeleton, we'll just simulate loading a model
        
        // Simulate loading time based on file size
        uintmax_t fileSize = std::stoull(it->second.additionalInfo.at("file_size_bytes"));
        std::this_thread::sleep_for(std::chrono::milliseconds(fileSize / 10000000)); // Scale loading time with file size
        
        // Set modelContext_ to a non-null value to indicate that a model is loaded
        modelContext_ = this; // Just a placeholder
        
        EditorErrorReporter::reportInfo(
            "LlamaProvider",
            "Model loaded: " + modelId,
            "File: " + modelPath
        );
        
        return true;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Exception loading model: " + std::string(e.what()),
            "Check model file"
        );
        return false;
    }
}

void LlamaProvider::unloadModel()
{
    try {
        // In a real implementation, this would unload the LLama model
        // For this skeleton, we'll just clear the modelContext_
        
        if (modelContext_) {
            // Simulate unloading time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            modelContext_ = nullptr;
            
            EditorErrorReporter::reportInfo(
                "LlamaProvider",
                "Model unloaded: " + currentModelId_,
                ""
            );
        }
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Exception unloading model: " + std::string(e.what()),
            "Check model state"
        );
    }
}

std::string LlamaProvider::convertMessagesToPrompt(const std::vector<Message>& messages) const
{
    // If we have a template, use it
    if (currentTemplate_) {
        return currentTemplate_->formatConversation(messages);
    }
    
    // Otherwise, use the default implementation
    // Convert chat messages to a prompt string for LLama
    // This implementation uses a more realistic template based on common LLama chat templates
    
    std::stringstream prompt;
    
    // Check if this is likely an alpaca-style model (7B, 13B models often use this)
    bool isAlpacaStyle = currentModelId_.find("alpaca") != std::string::npos ||
                         currentModelId_.find("7b") != std::string::npos ||
                         currentModelId_.find("13b") != std::string::npos;
    
    if (isAlpacaStyle) {
        // Alpaca-style template
        prompt << "### Instruction:\n";
        
        // Extract system message if present
        std::string systemContent;
        for (const auto& message : messages) {
            if (message.role == Message::Role::SYSTEM) {
                systemContent += message.content + "\n";
            }
        }
        
        if (!systemContent.empty()) {
            prompt << systemContent << "\n";
        }
        
        // Add user and assistant messages as a conversation
        for (const auto& message : messages) {
            if (message.role == Message::Role::USER) {
                prompt << "User: " << message.content << "\n";
            } else if (message.role == Message::Role::ASSISTANT) {
                prompt << "Assistant: " << message.content << "\n";
            } else if (message.role == Message::Role::TOOL || message.role == Message::Role::FUNCTION) {
                if (message.name) {
                    prompt << "Tool (" << *message.name << "): " << message.content << "\n";
                } else {
                    prompt << "Tool: " << message.content << "\n";
                }
            }
        }
        
        prompt << "### Response:\nAssistant: ";
    } else {
        // Llama-2-chat style template for newer models
        bool firstUserMsg = true;
        
        for (const auto& message : messages) {
            switch (message.role) {
                case Message::Role::SYSTEM: {
                    prompt << "<s>[SYSTEM] " << message.content << " </s>\n";
                    break;
                }
                case Message::Role::USER: {
                    if (firstUserMsg) {
                        prompt << "<s>[INST] " << message.content << " [/INST]\n";
                        firstUserMsg = false;
                    } else {
                        prompt << "<s>[INST] " << message.content << " [/INST]\n";
                    }
                    break;
                }
                case Message::Role::ASSISTANT: {
                    prompt << message.content << " </s>\n";
                    break;
                }
                case Message::Role::TOOL:
                case Message::Role::FUNCTION: {
                    std::string toolName = message.name.value_or("unknown");
                    prompt << "<s>[TOOL] " << toolName << ": " << message.content << " [/TOOL]\n";
                    break;
                }
            }
        }
        
        // Final assistant turn prefix
        prompt << "";
    }
    
    return prompt.str();
}

std::vector<ToolCall> LlamaProvider::parseToolCallsFromOutput(const std::string& output) const
{
    std::vector<ToolCall> toolCalls;
    
    // Use regex to find tool calls in the format
    // `tool: <tool_name> arguments: { ... }`
    std::regex toolCallRegex(R"(tool:\s*(\w+)\s*arguments:\s*\{([^}]+)\})");
    
    std::sregex_iterator it(output.begin(), output.end(), toolCallRegex);
    std::sregex_iterator end;
    
    int callId = 1;
    while (it != end) {
        std::smatch match = *it;
        
        if (match.size() >= 3) {
            ToolCall toolCall;
            toolCall.id = "call_" + std::to_string(callId++);
            toolCall.name = match[1].str();
            
            // Format the arguments as JSON
            std::string argsText = match[2].str();
            // Simple cleanup for arguments to make it more JSON-like
            // Replace line breaks with spaces
            std::replace(argsText.begin(), argsText.end(), '\n', ' ');
            // Ensure property names are quoted
            std::regex propNameRegex(R"((\w+)\s*:)");
            argsText = std::regex_replace(argsText, propNameRegex, "\"$1\":");
            // Ensure string values are quoted
            std::regex valueRegex(R"(:(\s*)(\w+))");
            argsText = std::regex_replace(argsText, valueRegex, ":$1\"$2\"");
            
            toolCall.arguments = "{" + argsText + "}";
            toolCalls.push_back(toolCall);
        }
        
        ++it;
    }
    
    return toolCalls;
}

std::string LlamaProvider::generateSimulatedResponse(const std::string& prompt, const std::vector<Message>& messages) const
{
    // Create a more realistic simulated response based on the prompt
    
    // Extract keywords from the prompt to use in the response
    std::vector<std::string> keywords;
    std::istringstream iss(prompt);
    std::string word;
    while (iss >> word) {
        if (word.length() > 4 && 
            word != "system" && word != "user" && word != "assistant" &&
            word != "instruction" && word != "response") {
            keywords.push_back(word);
        }
    }
    
    // Look for the last user message
    std::string lastUserMessage;
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        if (it->role == Message::Role::USER) {
            lastUserMessage = it->content;
            break;
        }
    }
    
    // Generate a response based on message content type
    std::ostringstream response;
    
    // Check if it's a code request
    if (lastUserMessage.find("code") != std::string::npos || 
        lastUserMessage.find("function") != std::string::npos || 
        lastUserMessage.find("class") != std::string::npos ||
        lastUserMessage.find("implement") != std::string::npos) {
        
        // Generate a code response
        response << "I'll implement that for you. Here's the code:\n\n";
        response << "```cpp\n";
        response << "// Example implementation\n";
        response << "#include <iostream>\n";
        response << "#include <string>\n\n";
        
        if (lastUserMessage.find("class") != std::string::npos) {
            response << "class Example {\n";
            response << "private:\n";
            response << "    std::string name;\n\n";
            response << "public:\n";
            response << "    Example(const std::string& n) : name(n) {}\n\n";
            response << "    void printName() const {\n";
            response << "        std::cout << \"Name: \" << name << std::endl;\n";
            response << "    }\n";
            response << "};\n\n";
        } else {
            response << "void exampleFunction() {\n";
            response << "    std::cout << \"This is an example function\" << std::endl;\n";
            response << "    // Add implementation here\n";
            response << "}\n\n";
        }
        
        response << "int main() {\n";
        response << "    std::cout << \"Hello from local LLama model!\" << std::endl;\n";
        response << "    return 0;\n";
        response << "}\n";
        response << "```\n\n";
        
        response << "This is a basic implementation. Let me know if you need any adjustments or have questions about how it works!";
    }
    // Check if it's a question
    else if (lastUserMessage.find("?") != std::string::npos || 
             lastUserMessage.find("how") != std::string::npos || 
             lastUserMessage.find("what") != std::string::npos || 
             lastUserMessage.find("why") != std::string::npos) {
        
        response << "That's an interesting question. ";
        
        // Add some keywords from the message to make it seem more relevant
        if (!keywords.empty()) {
            response << "Based on my understanding of ";
            
            for (size_t i = 0; i < std::min(size_t(3), keywords.size()); ++i) {
                response << keywords[i];
                if (i < std::min(size_t(2), keywords.size()) - 1) {
                    response << ", ";
                } else if (i == std::min(size_t(2), keywords.size()) - 1 && keywords.size() > 1) {
                    response << " and ";
                }
            }
            
            response << ", I can provide the following explanation:\n\n";
        }
        
        response << "The concept you're asking about is fundamental to understanding this topic. ";
        response << "There are several aspects to consider:\n\n";
        response << "1. First, it's important to recognize the underlying principles.\n";
        response << "2. The practical applications demonstrate why this matters.\n";
        response << "3. Historical context helps us understand how this developed over time.\n\n";
        
        response << "I hope this explanation helps! Let me know if you'd like me to elaborate on any specific point.";
    }
    // Default general response
    else {
        response << "I understand what you're looking for. ";
        
        if (!keywords.empty()) {
            response << "Based on your interest in ";
            for (size_t i = 0; i < std::min(size_t(3), keywords.size()); ++i) {
                response << keywords[i];
                if (i < std::min(size_t(2), keywords.size()) - 1) {
                    response << ", ";
                } else if (i == std::min(size_t(2), keywords.size()) - 1 && keywords.size() > 1) {
                    response << " and ";
                }
            }
            response << ", ";
        }
        
        response << "I can provide the following information:\n\n";
        response << "This is a simulated response from a local LLama model. ";
        response << "In a real implementation, the model would generate text based on its training data ";
        response << "and the specific prompt you provided. The response would be more relevant and detailed.\n\n";
        
        response << "Local LLama models offer several advantages:\n";
        response << "- Privacy: Your data stays on your device\n";
        response << "- No internet required: Works offline\n";
        response << "- No usage costs: Run as many queries as you want\n";
        response << "- Customizability: Fine-tune for specific use cases\n\n";
        
        response << "Is there anything specific about local AI models you'd like to know more about?";
    }
    
    return response.str();
}

bool LlamaProvider::shouldGenerateToolCall(const std::string& prompt) const
{
    // Determine if we should generate a tool call based on the prompt content
    return prompt.find("tool") != std::string::npos || 
           prompt.find("function") != std::string::npos ||
           prompt.find("API") != std::string::npos ||
           prompt.find("action") != std::string::npos ||
           prompt.find("execute") != std::string::npos;
}

std::pair<std::string, std::vector<ToolCall>> LlamaProvider::extractToolCalls(
    const std::string& content, 
    const std::vector<ToolDefinition>& tools) const
{
    // This is a simulation of extracting tool calls from model output
    // In a real implementation, this would parse the actual model output
    
    // For simulation, we'll generate a tool call for one of the provided tools
    std::vector<ToolCall> toolCalls;
    std::string remainingContent = content;
    
    // Only proceed if we have tools
    if (!tools.empty()) {
        // Randomly select a tool
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> toolDist(0, tools.size() - 1);
        int selectedToolIndex = toolDist(gen);
        
        const auto& selectedTool = tools[selectedToolIndex];
        
        // Create a tool call
        ToolCall toolCall;
        toolCall.id = "call_" + std::to_string(gen());
        toolCall.name = selectedTool.name;
        
        // Create simplified arguments based on the tool schema
        // For a real implementation, we would parse the schema and generate valid arguments
        std::string schema = selectedTool.schema;
        
        if (schema.find("string") != std::string::npos) {
            toolCall.arguments = R"({"text": "example value"})";
        } else if (schema.find("number") != std::string::npos) {
            toolCall.arguments = R"({"value": 42})";
        } else if (schema.find("boolean") != std::string::npos) {
            toolCall.arguments = R"({"flag": true})";
        } else {
            toolCall.arguments = R"({"param": "value"})";
        }
        
        toolCalls.push_back(toolCall);
        
        // Modify the content to indicate tool usage
        std::ostringstream modifiedContent;
        modifiedContent << remainingContent.substr(0, remainingContent.length() / 2);
        modifiedContent << "\n\nI'll use the " << selectedTool.name << " tool to help with this task.\n\n";
        
        remainingContent = modifiedContent.str();
    }
    
    return {remainingContent, toolCalls};
}

int LlamaProvider::countTokens(const std::string& text) const
{
    // This is a very simplified token counting method
    // In a real implementation, you would use the actual tokenizer
    
    // Count words as a rough approximation of tokens
    int count = 0;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        // Count punctuation as separate tokens
        for (char c : word) {
            if (ispunct(c)) {
                count++;
            }
        }
        count++;
    }
    
    return count;
}

// Factory function for creating LlamaProvider instances
std::unique_ptr<IAIProvider> createLlamaProvider(const ProviderOptions& options)
{
    // Extract model path from options
    std::string modelPath;
    
    if (options.additionalOptions.count("model_path") > 0) {
        modelPath = options.additionalOptions.at("model_path");
    } else {
        // Try to get from environment variable
        const char* envModelPath = std::getenv("LLAMA_MODEL_PATH");
        if (envModelPath) {
            modelPath = envModelPath;
        } else {
            EditorErrorReporter::reportError(
                "LlamaProvider",
                "No model path provided",
                "Set 'model_path' in additionalOptions or LLAMA_MODEL_PATH environment variable"
            );
            return nullptr;
        }
    }
    
    try {
        // Create and initialize the provider
        auto provider = std::make_unique<LlamaProvider>(modelPath);
        
        if (!provider->initialize(options)) {
            return nullptr;
        }
        
        return provider;
    } catch (const std::exception& e) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Failed to create provider: " + std::string(e.what()),
            "Check model path and options"
        );
        return nullptr;
    }
}

// Register the LlamaProvider with the AIProviderFactory
void registerLlamaProvider()
{
    AIProviderFactory::registerProviderType("llama", createLlamaProvider);
}

std::shared_ptr<PromptTemplate> LlamaProvider::getCurrentTemplate() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return currentTemplate_;
}

bool LlamaProvider::setCurrentTemplate(const std::string& templateId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get the template from the manager
    auto template_ = templateManager_->getTemplate(templateId);
    
    if (!template_) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Template not found: " + templateId,
            "Check available templates with getAvailableTemplates()"
        );
        return false;
    }
    
    // Check if the template is compatible with this provider
    if (!template_->isForProvider("llama")) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Template not compatible with provider: " + templateId,
            "Use a template designed for LLama models"
        );
        return false;
    }
    
    // If we have a current model, check if the template is compatible
    if (!currentModelId_.empty() && !template_->isCompatibleWithModel(currentModelId_)) {
        EditorErrorReporter::reportError(
            "LlamaProvider",
            "Template not compatible with model: " + templateId,
            "Select a template compatible with " + currentModelId_
        );
        return false;
    }
    
    // Set the template
    currentTemplate_ = template_;
    
    // Update options
    options_.templateId = templateId;
    
    return true;
}

std::vector<std::string> LlamaProvider::getAvailableTemplates() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    
    // Get all templates for this provider
    auto templates = templateManager_->getTemplatesForProvider("llama");
    
    // If we have a current model, filter for compatible templates
    if (!currentModelId_.empty()) {
        for (const auto& templ : templates) {
            if (templ->isCompatibleWithModel(currentModelId_)) {
                result.push_back(templ->getId());
            }
        }
    } else {
        // Otherwise, return all templates for this provider
        for (const auto& templ : templates) {
            result.push_back(templ->getId());
        }
    }
    
    return result;
}

bool LlamaProvider::selectBestTemplateForModel()
{
    // If we have a current model, find the best template for it
    if (!currentModelId_.empty()) {
        auto bestTemplate = templateManager_->findTemplateForModel(currentModelId_, "llama");
        
        if (bestTemplate) {
            currentTemplate_ = bestTemplate;
            options_.templateId = bestTemplate->getId();
            return true;
        }
    }
    
    // If no model or no compatible template, use the default template
    auto defaultTemplate = templateManager_->getDefaultTemplateForProvider("llama");
    
    if (defaultTemplate) {
        currentTemplate_ = defaultTemplate;
        options_.templateId = defaultTemplate->getId();
        return true;
    }
    
    // No template available
    currentTemplate_ = nullptr;
    options_.templateId = "";
    return false;
}

} // namespace ai_editor 