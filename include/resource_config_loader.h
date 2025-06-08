#ifndef CHRONOVYAN_RESOURCE_CONFIG_LOADER_H
#define CHRONOVYAN_RESOURCE_CONFIG_LOADER_H

#include "resource_config.h"
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace chronovyan {

/**
 * @class ResourceConfigLoader
 * @brief Utility class for loading configuration values from files
 */
class ResourceConfigLoader {
public:
    /**
     * @brief Load configuration from a file
     * @param filepath Path to the configuration file
     * @param config Reference to the ResourceConfig to update
     * @return True if successful, false otherwise
     * 
     * The file format is simple key-value pairs:
     * key_name = value
     * 
     * Lines starting with # are comments.
     * Blank lines are ignored.
     */
    static bool loadFromFile(const std::string& filepath, ResourceConfig& config) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open configuration file: " << filepath << std::endl;
            return false;
        }
        
        std::string line;
        int lineNumber = 0;
        
        while (std::getline(file, line)) {
            lineNumber++;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Parse key=value pair
            size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos) {
                std::cerr << "Warning: Line " << lineNumber << " is not a valid key-value pair: " << line << std::endl;
                continue;
            }
            
            std::string key = trim(line.substr(0, equalsPos));
            std::string valueStr = trim(line.substr(equalsPos + 1));
            
            if (key.empty()) {
                std::cerr << "Warning: Line " << lineNumber << " has an empty key: " << line << std::endl;
                continue;
            }
            
            // Try to parse as double first
            try {
                double doubleValue = std::stod(valueStr);
                
                // Check if it's an integer (no decimal part)
                if (doubleValue == std::floor(doubleValue) && valueStr.find('.') == std::string::npos) {
                    int intValue = static_cast<int>(doubleValue);
                    config.setInt(key, intValue);
                    std::cout << "Loaded int config: " << key << " = " << intValue << std::endl;
                } else {
                    config.setDouble(key, doubleValue);
                    std::cout << "Loaded double config: " << key << " = " << doubleValue << std::endl;
                }
            } catch (const std::invalid_argument&) {
                std::cerr << "Warning: Line " << lineNumber << " has an invalid value: " << valueStr << std::endl;
            } catch (const std::out_of_range&) {
                std::cerr << "Warning: Line " << lineNumber << " has a value out of range: " << valueStr << std::endl;
            }
        }
        
        return true;
    }
    
private:
    /**
     * @brief Trim whitespace from beginning and end of string
     * @param str The string to trim
     * @return The trimmed string
     */
    static std::string trim(const std::string& str) {
        const std::string whitespace = " \t\n\r\f\v";
        const auto start = str.find_first_not_of(whitespace);
        
        if (start == std::string::npos) {
            return ""; // String is all whitespace
        }
        
        const auto end = str.find_last_not_of(whitespace);
        const auto length = end - start + 1;
        
        return str.substr(start, length);
    }
};

} // namespace chronovyan

#endif // CHRONOVYAN_RESOURCE_CONFIG_LOADER_H 