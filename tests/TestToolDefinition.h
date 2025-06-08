#pragma once

#include <string>

// Simple ToolDefinition structure for testing
struct ToolDefinition {
    std::string name;
    std::string description;
    std::string schema;
    
    ToolDefinition() = default;
    ToolDefinition(const std::string& name, const std::string& desc, const std::string& schema = "")
        : name(name), description(desc), schema(schema) {}
};
