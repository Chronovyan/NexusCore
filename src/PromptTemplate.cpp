#include "PromptTemplate.h"
#include "EditorErrorReporter.h"
#include <regex>
#include <algorithm>

namespace ai_editor {

PromptTemplate::PromptTemplate(
    const std::string& id,
    const std::string& name,
    const std::string& description,
    const std::string& providerType,
    const std::vector<std::string>& compatibleModels,
    bool isDefault,
    bool isEditable)
    : id_(id),
      name_(name),
      description_(description),
      providerType_(providerType),
      compatibleModels_(compatibleModels),
      isDefault_(isDefault),
      isEditable_(isEditable)
{
    // Default message formats (basic formatting)
    systemMessageFormat_ = [](const std::string& content) {
        return "SYSTEM: " + content;
    };
    
    userMessageFormat_ = [](const std::string& content) {
        return "USER: " + content;
    };
    
    assistantMessageFormat_ = [](const std::string& content) {
        return "ASSISTANT: " + content;
    };
    
    toolMessageFormat_ = [](const std::string& content, const std::string& name) {
        return "TOOL (" + name + "): " + content;
    };
    
    // Default conversation formatter
    conversationFormatter_ = [this](const std::vector<Message>& messages) {
        std::string result;
        
        for (const auto& message : messages) {
            if (!result.empty()) {
                result += "\n\n";
            }
            
            switch (message.role) {
                case Message::Role::SYSTEM:
                    result += this->formatSystemMessage(message.content);
                    break;
                    
                case Message::Role::USER:
                    result += this->formatUserMessage(message.content);
                    break;
                    
                case Message::Role::ASSISTANT:
                    result += this->formatAssistantMessage(message.content);
                    break;
                    
                case Message::Role::TOOL:
                case Message::Role::FUNCTION:
                    result += this->formatToolMessage(
                        message.content, 
                        message.name.value_or("unknown")
                    );
                    break;
            }
        }
        
        return result;
    };
}

PromptTemplateInfo PromptTemplate::getInfo() const
{
    return PromptTemplateInfo{
        id_,
        name_,
        description_,
        providerType_,
        compatibleModels_,
        isDefault_,
        isEditable_
    };
}

void PromptTemplate::setSystemMessageFormat(SystemMessageFormat format)
{
    if (!isEditable_) {
        EditorErrorReporter::reportError(
            "PromptTemplate",
            "Cannot modify non-editable template: " + id_ + ". Create a new template based on this one instead.",
            3  // ERROR level
        );
        return;
    }
    
    systemMessageFormat_ = format;
}

void PromptTemplate::setUserMessageFormat(UserMessageFormat format)
{
    if (!isEditable_) {
        EditorErrorReporter::reportError(
            "PromptTemplate",
            "Cannot modify non-editable template: " + id_ + ". Create a new template based on this one instead.",
            3  // ERROR level
        );
        return;
    }
    
    userMessageFormat_ = format;
}

void PromptTemplate::setAssistantMessageFormat(AssistantMessageFormat format)
{
    if (!isEditable_) {
        EditorErrorReporter::reportError(
            "PromptTemplate",
            "Cannot modify non-editable template: " + id_ + ". Create a new template based on this one instead.",
            3  // ERROR level
        );
        return;
    }
    
    assistantMessageFormat_ = format;
}

void PromptTemplate::setToolMessageFormat(ToolMessageFormat format)
{
    if (!isEditable_) {
        EditorErrorReporter::reportError(
            "PromptTemplate",
            "Cannot modify non-editable template: " + id_ + ". Create a new template based on this one instead.",
            3  // ERROR level
        );
        return;
    }
    
    toolMessageFormat_ = format;
}

void PromptTemplate::setConversationFormatter(ConversationFormatter formatter)
{
    if (!isEditable_) {
        EditorErrorReporter::reportError(
            "PromptTemplate",
            "Cannot modify non-editable template: " + id_ + ". Create a new template based on this one instead.",
            3  // ERROR level
        );
        return;
    }
    
    conversationFormatter_ = formatter;
}

std::string PromptTemplate::formatSystemMessage(const std::string& content) const
{
    if (systemMessageFormat_) {
        return systemMessageFormat_(content);
    }
    
    return "SYSTEM: " + content;
}

std::string PromptTemplate::formatUserMessage(const std::string& content) const
{
    if (userMessageFormat_) {
        return userMessageFormat_(content);
    }
    
    return "USER: " + content;
}

std::string PromptTemplate::formatAssistantMessage(const std::string& content) const
{
    if (assistantMessageFormat_) {
        return assistantMessageFormat_(content);
    }
    
    return "ASSISTANT: " + content;
}

std::string PromptTemplate::formatToolMessage(const std::string& content, const std::string& name) const
{
    if (toolMessageFormat_) {
        return toolMessageFormat_(content, name);
    }
    
    return "TOOL (" + name + "): " + content;
}

std::string PromptTemplate::formatConversation(const std::vector<Message>& messages) const
{
    if (conversationFormatter_) {
        return conversationFormatter_(messages);
    }
    
    // Fallback implementation
    std::string result;
    
    for (const auto& message : messages) {
        if (!result.empty()) {
            result += "\n\n";
        }
        
        switch (message.role) {
            case Message::Role::SYSTEM:
                result += formatSystemMessage(message.content);
                break;
                
            case Message::Role::USER:
                result += formatUserMessage(message.content);
                break;
                
            case Message::Role::ASSISTANT:
                result += formatAssistantMessage(message.content);
                break;
                
            case Message::Role::TOOL:
            case Message::Role::FUNCTION:
                result += formatToolMessage(
                    message.content, 
                    message.name.value_or("unknown")
                );
                break;
        }
    }
    
    return result;
}

bool PromptTemplate::isCompatibleWithModel(const std::string& modelId) const
{
    // Check if the model ID is directly in the list
    if (std::find(compatibleModels_.begin(), compatibleModels_.end(), modelId) != compatibleModels_.end()) {
        return true;
    }
    
    // Check if any pattern matches the model ID
    for (const auto& pattern : compatibleModels_) {
        if (pattern.find('*') != std::string::npos) {
            // Convert glob pattern to regex
            std::string regexPattern = pattern;
            std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
            regexPattern = "^" + regexPattern + "$";
            
            std::regex re(regexPattern);
            if (std::regex_match(modelId, re)) {
                return true;
            }
        }
    }
    
    return false;
}

bool PromptTemplate::isForProvider(const std::string& providerType) const
{
    return providerType_ == providerType;
}

bool PromptTemplate::isDefault() const
{
    return isDefault_;
}

bool PromptTemplate::isEditable() const
{
    return isEditable_;
}

std::string PromptTemplate::getId() const
{
    return id_;
}

std::string PromptTemplate::getName() const
{
    return name_;
}

std::string PromptTemplate::getDescription() const
{
    return description_;
}

std::string PromptTemplate::getProviderType() const
{
    return providerType_;
}

std::vector<std::string> PromptTemplate::getCompatibleModels() const
{
    return compatibleModels_;
}

// PromptTemplateManager implementation

PromptTemplateManager::PromptTemplateManager()
{
    // Initialize with default templates
    initializeDefaultTemplates();
}

bool PromptTemplateManager::addTemplate(std::shared_ptr<PromptTemplate> templ)
{
    if (!templ) {
        EditorErrorReporter::reportError(
            "PromptTemplateManager",
            "Cannot add null template",
            3  // ERROR level
        );
        return false;
    }
    
    std::string id = templ->getId();
    
    // Check if a template with this ID already exists
    if (templates_.find(id) != templates_.end()) {
        EditorErrorReporter::reportError(
            "PromptTemplateManager",
            "Template with ID '" + id + "' already exists",
            3  // ERROR level
        );
        return false;
    }
    
    templates_[id] = templ;
    return true;
}

bool PromptTemplateManager::removeTemplate(const std::string& templateId)
{
    auto it = templates_.find(templateId);
    
    if (it == templates_.end()) {
        EditorErrorReporter::reportError(
            "PromptTemplateManager",
            "Template with ID '" + templateId + "' not found",
            3  // ERROR level
        );
        return false;
    }
    
    // Check if it's a default template
    if (it->second->isDefault()) {
        EditorErrorReporter::reportError(
            "PromptTemplateManager",
            "Cannot remove default template '" + templateId + "'",
            3  // ERROR level
        );
        return false;
    }
    
    templates_.erase(it);
    return true;
}

std::shared_ptr<PromptTemplate> PromptTemplateManager::getTemplate(const std::string& templateId) const
{
    auto it = templates_.find(templateId);
    
    if (it == templates_.end()) {
        return nullptr;
    }
    
    return it->second;
}

std::shared_ptr<PromptTemplate> PromptTemplateManager::findTemplateForModel(
    const std::string& modelId,
    const std::string& providerType) const
{
    // First, try to find an exact match
    for (const auto& [id, templ] : templates_) {
        if (templ->isForProvider(providerType) && templ->isCompatibleWithModel(modelId)) {
            return templ;
        }
    }
    
    // If no exact match, return the default template for the provider
    return getDefaultTemplateForProvider(providerType);
}

std::vector<std::shared_ptr<PromptTemplate>> PromptTemplateManager::getAllTemplates() const
{
    std::vector<std::shared_ptr<PromptTemplate>> result;
    result.reserve(templates_.size());
    
    for (const auto& [id, templ] : templates_) {
        result.push_back(templ);
    }
    
    return result;
}

std::vector<std::shared_ptr<PromptTemplate>> PromptTemplateManager::getTemplatesForProvider(
    const std::string& providerType) const
{
    std::vector<std::shared_ptr<PromptTemplate>> result;
    
    for (const auto& [id, templ] : templates_) {
        if (templ->isForProvider(providerType)) {
            result.push_back(templ);
        }
    }
    
    return result;
}

std::shared_ptr<PromptTemplate> PromptTemplateManager::getDefaultTemplateForProvider(
    const std::string& providerType) const
{
    for (const auto& [id, templ] : templates_) {
        if (templ->isForProvider(providerType) && templ->isDefault()) {
            return templ;
        }
    }
    
    return nullptr;
}

void PromptTemplateManager::initializeDefaultTemplates()
{
    // 1. OpenAI ChatGPT template
    auto openaiTemplate = std::shared_ptr<PromptTemplate>(new PromptTemplate(
        "openai-default",
        "OpenAI Default",
        "Standard template for OpenAI ChatGPT models",
        "openai",
        {"gpt-3.5-turbo*", "gpt-4*"},
        true,
        false
    ));
    
    // OpenAI doesn't need any special formatting as the API handles the roles
    openaiTemplate->setConversationFormatter([](const std::vector<Message>& messages) {
        // For OpenAI, we don't need to format the messages as they're sent directly to the API
        // This is just a placeholder as the messages will be converted to API format
        return ""; 
    });
    
    // 2. Llama-2-Chat template
    auto llama2Template = std::shared_ptr<PromptTemplate>(new PromptTemplate(
        "llama2-chat",
        "Llama-2 Chat",
        "Template for Llama-2 chat models",
        "llama",
        {"llama-2*", "*-chat", "*-7b", "*-13b", "*-70b"},
        true,
        false
    ));
    
    llama2Template->setSystemMessageFormat([](const std::string& content) {
        return "<s>[SYSTEM]\n" + content + "\n</s>";
    });
    
    llama2Template->setUserMessageFormat([](const std::string& content) {
        return "<s>[INST]\n" + content + "\n[/INST]";
    });
    
    llama2Template->setAssistantMessageFormat([](const std::string& content) {
        return content + "\n</s>";
    });
    
    llama2Template->setToolMessageFormat([](const std::string& content, const std::string& name) {
        return "<s>[TOOL] " + name + ":\n" + content + "\n[/TOOL]";
    });
    
    llama2Template->setConversationFormatter([this](const std::vector<Message>& messages) {
        std::string result;
        bool firstUserMsg = true;
        
        for (const auto& message : messages) {
            switch (message.role) {
                case Message::Role::SYSTEM: {
                    if (!result.empty()) result += "\n\n";
                    result += "<s>[SYSTEM]\n" + message.content + "\n</s>";
                    break;
                }
                case Message::Role::USER: {
                    if (!result.empty()) result += "\n\n";
                    if (firstUserMsg) {
                        result += "<s>[INST]\n" + message.content + "\n[/INST]";
                        firstUserMsg = false;
                    } else {
                        result += "<s>[INST]\n" + message.content + "\n[/INST]";
                    }
                    break;
                }
                case Message::Role::ASSISTANT: {
                    if (!result.empty() && result.size() >= 8 && result.compare(result.size() - 8, 8, "[/INST]") != 0) {
                        result += "\n\n";
                    }
                    result += message.content + "\n</s>";
                    break;
                }
                case Message::Role::TOOL:
                case Message::Role::FUNCTION: {
                    if (!result.empty()) result += "\n\n";
                    std::string toolName = message.name.value_or("unknown");
                    result += "<s>[TOOL] " + toolName + ":\n" + message.content + "\n[/TOOL]";
                    break;
                }
            }
        }
        
        return result;
    });
    
    // 3. Alpaca template
    auto alpacaTemplate = std::shared_ptr<PromptTemplate>(new PromptTemplate(
        "alpaca-style",
        "Alpaca Style",
        "Template for Alpaca-style instruction models",
        "llama",
        {"*alpaca*", "*instruct*"},
        false,
        false
    ));
    
    alpacaTemplate->setSystemMessageFormat([](const std::string& content) {
        return "### Instruction:\n" + content;
    });
    
    alpacaTemplate->setUserMessageFormat([](const std::string& content) {
        return "User: " + content;
    });
    
    alpacaTemplate->setAssistantMessageFormat([](const std::string& content) {
        return "Assistant: " + content;
    });
    
    alpacaTemplate->setToolMessageFormat([](const std::string& content, const std::string& name) {
        return "Tool (" + name + "): " + content;
    });
    
    alpacaTemplate->setConversationFormatter([this](const std::vector<Message>& messages) {
        std::string result = "### Instruction:\n";
        
        // Extract system message if present
        std::string systemContent;
        for (const auto& message : messages) {
            if (message.role == Message::Role::SYSTEM) {
                systemContent += message.content + "\n";
            }
        }
        
        if (!systemContent.empty()) {
            result += systemContent + "\n";
        }
        
        // Add user and assistant messages as a conversation
        for (const auto& message : messages) {
            if (message.role == Message::Role::USER) {
                result += "User: " + message.content + "\n";
            } else if (message.role == Message::Role::ASSISTANT) {
                result += "Assistant: " + message.content + "\n";
            } else if (message.role == Message::Role::TOOL || message.role == Message::Role::FUNCTION) {
                if (message.name) {
                    result += "Tool (" + *message.name + "): " + message.content + "\n";
                } else {
                    result += "Tool: " + message.content + "\n";
                }
            }
        }
        
        result += "### Response:\nAssistant: ";
        return result;
    });
    
    // 4. ChatML template (Claude, etc.)
    auto chatMLTemplate = std::shared_ptr<PromptTemplate>(new PromptTemplate(
        "chatml",
        "ChatML",
        "Template for models supporting the ChatML format",
        "llama",
        {"*claude*", "*mistral*", "*mixtral*"},
        false,
        false
    ));
    
    chatMLTemplate->setSystemMessageFormat([](const std::string& content) {
        return "<|im_start|>system\n" + content + "<|im_end|>";
    });
    
    chatMLTemplate->setUserMessageFormat([](const std::string& content) {
        return "<|im_start|>user\n" + content + "<|im_end|>";
    });
    
    chatMLTemplate->setAssistantMessageFormat([](const std::string& content) {
        return "<|im_start|>assistant\n" + content + "<|im_end|>";
    });
    
    chatMLTemplate->setToolMessageFormat([](const std::string& content, const std::string& name) {
        return "<|im_start|>tool " + name + "\n" + content + "<|im_end|>";
    });
    
    chatMLTemplate->setConversationFormatter([](const std::vector<Message>& messages) {
        std::string result;
        
        for (const auto& message : messages) {
            if (!result.empty()) result += "\n";
            
            switch (message.role) {
                case Message::Role::SYSTEM:
                    result += "<|im_start|>system\n" + message.content + "<|im_end|>";
                    break;
                    
                case Message::Role::USER:
                    result += "<|im_start|>user\n" + message.content + "<|im_end|>";
                    break;
                    
                case Message::Role::ASSISTANT:
                    result += "<|im_start|>assistant\n" + message.content + "<|im_end|>";
                    break;
                    
                case Message::Role::TOOL:
                case Message::Role::FUNCTION: {
                    std::string toolName = message.name.value_or("unknown");
                    result += "<|im_start|>tool " + toolName + "\n" + message.content + "<|im_end|>";
                    break;
                }
            }
        }
        
        // Add the final assistant prefix
        result += "\n<|im_start|>assistant\n";
        
        return result;
    });
    
    // Add all templates
    templates_["openai-default"] = openaiTemplate;
    templates_["llama2-chat"] = llama2Template;
    templates_["alpaca-style"] = alpacaTemplate;
    templates_["chatml"] = chatMLTemplate;
}

} // namespace ai_editor 