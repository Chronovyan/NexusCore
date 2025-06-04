#pragma once

#include "interfaces/IAIProvider.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace ai_editor {

/**
 * @struct PromptTemplateInfo
 * @brief Information about a prompt template
 */
struct PromptTemplateInfo {
    std::string id;                 // Unique identifier for the template
    std::string name;               // Human-readable name
    std::string description;        // Description of the template
    std::string providerType;       // Provider type this template is designed for
    std::vector<std::string> compatibleModels; // List of compatible model IDs or patterns
    bool isDefault;                 // Whether this is a default template
    bool isEditable;                // Whether this template can be edited by users
};

/**
 * @class PromptTemplate
 * @brief Manages model-specific prompt templates
 * 
 * This class provides functionality for storing, retrieving, and applying
 * prompt templates for different AI models and use cases.
 */
class PromptTemplate {
public:
    /**
     * @brief Format for system message
     */
    using SystemMessageFormat = std::function<std::string(const std::string&)>;
    
    /**
     * @brief Format for user message
     */
    using UserMessageFormat = std::function<std::string(const std::string&)>;
    
    /**
     * @brief Format for assistant message
     */
    using AssistantMessageFormat = std::function<std::string(const std::string&)>;
    
    /**
     * @brief Format for tool message
     */
    using ToolMessageFormat = std::function<std::string(const std::string&, const std::string&)>;
    
    /**
     * @brief Wrapper function to format a complete conversation
     */
    using ConversationFormatter = std::function<std::string(const std::vector<Message>&)>;

    /**
     * @brief Constructor
     * 
     * @param id Template identifier
     * @param name Human-readable name
     * @param description Description of the template
     * @param providerType Provider type this template is designed for
     * @param compatibleModels List of compatible model IDs or patterns
     * @param isDefault Whether this is a default template
     * @param isEditable Whether this template can be edited by users
     */
    PromptTemplate(
        const std::string& id,
        const std::string& name,
        const std::string& description,
        const std::string& providerType,
        const std::vector<std::string>& compatibleModels,
        bool isDefault = false,
        bool isEditable = true
    );
    
    /**
     * @brief Get template information
     * 
     * @return PromptTemplateInfo The template information
     */
    PromptTemplateInfo getInfo() const;
    
    /**
     * @brief Set system message format
     * 
     * @param format The format function
     */
    void setSystemMessageFormat(SystemMessageFormat format);
    
    /**
     * @brief Set user message format
     * 
     * @param format The format function
     */
    void setUserMessageFormat(UserMessageFormat format);
    
    /**
     * @brief Set assistant message format
     * 
     * @param format The format function
     */
    void setAssistantMessageFormat(AssistantMessageFormat format);
    
    /**
     * @brief Set tool message format
     * 
     * @param format The format function
     */
    void setToolMessageFormat(ToolMessageFormat format);
    
    /**
     * @brief Set conversation formatter
     * 
     * @param formatter The formatter function
     */
    void setConversationFormatter(ConversationFormatter formatter);
    
    /**
     * @brief Format a system message
     * 
     * @param content The message content
     * @return std::string The formatted message
     */
    std::string formatSystemMessage(const std::string& content) const;
    
    /**
     * @brief Format a user message
     * 
     * @param content The message content
     * @return std::string The formatted message
     */
    std::string formatUserMessage(const std::string& content) const;
    
    /**
     * @brief Format an assistant message
     * 
     * @param content The message content
     * @return std::string The formatted message
     */
    std::string formatAssistantMessage(const std::string& content) const;
    
    /**
     * @brief Format a tool message
     * 
     * @param content The message content
     * @param name The tool name
     * @return std::string The formatted message
     */
    std::string formatToolMessage(const std::string& content, const std::string& name) const;
    
    /**
     * @brief Format a complete conversation
     * 
     * @param messages The conversation messages
     * @return std::string The formatted conversation
     */
    std::string formatConversation(const std::vector<Message>& messages) const;
    
    /**
     * @brief Check if this template is compatible with a model
     * 
     * @param modelId The model ID to check
     * @return bool True if compatible
     */
    bool isCompatibleWithModel(const std::string& modelId) const;
    
    /**
     * @brief Check if this template is for a specific provider
     * 
     * @param providerType The provider type to check
     * @return bool True if for this provider
     */
    bool isForProvider(const std::string& providerType) const;
    
    /**
     * @brief Check if this is a default template
     * 
     * @return bool True if default
     */
    bool isDefault() const;
    
    /**
     * @brief Check if this template can be edited
     * 
     * @return bool True if editable
     */
    bool isEditable() const;
    
    /**
     * @brief Get the template ID
     * 
     * @return std::string The template ID
     */
    std::string getId() const;
    
    /**
     * @brief Get the template name
     * 
     * @return std::string The template name
     */
    std::string getName() const;
    
    /**
     * @brief Get the template description
     * 
     * @return std::string The template description
     */
    std::string getDescription() const;
    
    /**
     * @brief Get the provider type
     * 
     * @return std::string The provider type
     */
    std::string getProviderType() const;
    
    /**
     * @brief Get compatible models
     * 
     * @return std::vector<std::string> The compatible models
     */
    std::vector<std::string> getCompatibleModels() const;

private:
    // Template information
    std::string id_;
    std::string name_;
    std::string description_;
    std::string providerType_;
    std::vector<std::string> compatibleModels_;
    bool isDefault_;
    bool isEditable_;
    
    // Format functions
    SystemMessageFormat systemMessageFormat_;
    UserMessageFormat userMessageFormat_;
    AssistantMessageFormat assistantMessageFormat_;
    ToolMessageFormat toolMessageFormat_;
    ConversationFormatter conversationFormatter_;
};

/**
 * @class PromptTemplateManager
 * @brief Manages a collection of prompt templates
 * 
 * This class provides functionality for storing, retrieving, and managing
 * prompt templates for different AI models and use cases.
 */
class PromptTemplateManager {
public:
    /**
     * @brief Constructor
     */
    PromptTemplateManager();
    
    /**
     * @brief Add a template
     * 
     * @param template The template to add
     * @return bool True if added successfully
     */
    bool addTemplate(std::shared_ptr<PromptTemplate> templ);
    
    /**
     * @brief Remove a template
     * 
     * @param templateId The ID of the template to remove
     * @return bool True if removed successfully
     */
    bool removeTemplate(const std::string& templateId);
    
    /**
     * @brief Get a template by ID
     * 
     * @param templateId The template ID
     * @return std::shared_ptr<PromptTemplate> The template, or nullptr if not found
     */
    std::shared_ptr<PromptTemplate> getTemplate(const std::string& templateId) const;
    
    /**
     * @brief Find the best template for a model
     * 
     * @param modelId The model ID
     * @param providerType The provider type
     * @return std::shared_ptr<PromptTemplate> The best template, or nullptr if none found
     */
    std::shared_ptr<PromptTemplate> findTemplateForModel(
        const std::string& modelId,
        const std::string& providerType) const;
    
    /**
     * @brief Get all templates
     * 
     * @return std::vector<std::shared_ptr<PromptTemplate>> All templates
     */
    std::vector<std::shared_ptr<PromptTemplate>> getAllTemplates() const;
    
    /**
     * @brief Get templates for a provider
     * 
     * @param providerType The provider type
     * @return std::vector<std::shared_ptr<PromptTemplate>> Templates for the provider
     */
    std::vector<std::shared_ptr<PromptTemplate>> getTemplatesForProvider(
        const std::string& providerType) const;
    
    /**
     * @brief Get default template for a provider
     * 
     * @param providerType The provider type
     * @return std::shared_ptr<PromptTemplate> The default template, or nullptr if none
     */
    std::shared_ptr<PromptTemplate> getDefaultTemplateForProvider(
        const std::string& providerType) const;
    
    /**
     * @brief Initialize with default templates
     */
    void initializeDefaultTemplates();

private:
    // Templates by ID
    std::map<std::string, std::shared_ptr<PromptTemplate>> templates_;
};

} // namespace ai_editor 