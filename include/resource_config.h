#ifndef CHRONOVYAN_RESOURCE_CONFIG_H
#define CHRONOVYAN_RESOURCE_CONFIG_H

#include <unordered_map>
#include <string>

namespace chronovyan {

/**
 * @class ResourceConfig
 * @brief Centralized configuration system for resource optimization parameters
 * 
 * This class provides a centralized place to store, access, and modify 
 * configuration parameters used throughout the resource optimization system.
 * This replaces the use of magic numbers and hardcoded thresholds with named
 * configuration values that can be tuned and adapted for different use cases.
 */
class ResourceConfig {
public:
    /**
     * @brief Get the singleton instance of ResourceConfig
     * @return Reference to the singleton instance
     */
    static ResourceConfig& getInstance() {
        static ResourceConfig instance;
        return instance;
    }

    /**
     * @brief Get a double parameter value
     * @param key The parameter name
     * @param defaultValue The default value to return if key is not found
     * @return The parameter value, or defaultValue if not found
     */
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = m_doubleParams.find(key);
        return (it != m_doubleParams.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Set a double parameter value
     * @param key The parameter name
     * @param value The value to set
     */
    void setDouble(const std::string& key, double value) {
        m_doubleParams[key] = value;
    }

    /**
     * @brief Get an integer parameter value
     * @param key The parameter name
     * @param defaultValue The default value to return if key is not found
     * @return The parameter value, or defaultValue if not found
     */
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = m_intParams.find(key);
        return (it != m_intParams.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Set an integer parameter value
     * @param key The parameter name
     * @param value The value to set
     */
    void setInt(const std::string& key, int value) {
        m_intParams[key] = value;
    }

    /**
     * @brief Load default configuration values
     * 
     * This method initializes the configuration with sensible default values.
     * It should be called once at program startup.
     */
    void loadDefaults() {
        // Thresholds
        m_doubleParams["aethel_threshold"] = 0.75;
        m_doubleParams["chronons_threshold"] = 0.85;
        m_doubleParams["timeline_overlap_threshold"] = 0.30;
        m_doubleParams["temporal_sync_factor"] = 0.65;
        m_doubleParams["paradox_threshold"] = 0.45;
        
        // Priority factors
        m_doubleParams["high_priority_factor"] = 1.25;
        m_doubleParams["medium_priority_factor"] = 1.0;
        m_doubleParams["low_priority_factor"] = 0.75;
        
        // Resource management
        m_doubleParams["resource_decay_rate"] = 0.05;
        m_intParams["temporal_debounce_time"] = 250;
        
        // Aethel allocation thresholds
        m_doubleParams["aethel_low_usage_threshold"] = 0.3;
        m_doubleParams["aethel_moderate_usage_threshold"] = 0.6;
        m_doubleParams["aethel_high_usage_threshold"] = 0.85;
        
        // Aethel allocation factors
        m_doubleParams["aethel_low_usage_factor"] = 1.5;
        m_doubleParams["aethel_moderate_usage_factor"] = 1.2;
        m_doubleParams["aethel_high_usage_factor"] = 1.05;
        
        // Paradox risk factors
        m_doubleParams["paradox_base_risk"] = 0.1;
        m_doubleParams["paradox_complexity_weight"] = 0.25;
        m_doubleParams["paradox_branch_power"] = 0.75;
        m_doubleParams["paradox_branch_weight"] = 0.15;
        
        // Chronons efficiency thresholds
        m_doubleParams["efficiency_very_low_threshold"] = 0.4;
        m_doubleParams["efficiency_moderate_threshold"] = 0.65;
        m_doubleParams["efficiency_high_threshold"] = 0.85;
        
        // Optimization factors
        m_doubleParams["optimization_level_3_factor"] = 1.75;
        m_doubleParams["optimization_level_2_factor"] = 1.35;
        m_doubleParams["optimization_level_1_factor"] = 1.15;
        
        // Thread complexity thresholds
        m_intParams["complexity_low_threshold"] = 100;
        m_intParams["complexity_moderate_threshold"] = 500;
        m_intParams["complexity_high_threshold"] = 1000;
        
        // Thread counts
        m_intParams["threads_for_simple_operations"] = 1;
        m_intParams["threads_for_moderate_operations"] = 2;
        m_intParams["threads_for_complex_operations"] = 4;
        m_intParams["threads_for_very_complex_operations"] = 8;
        
        // Resource savings rates
        m_doubleParams["savings_rate_level_1"] = 0.1;
        m_doubleParams["savings_rate_level_2"] = 0.25;
        m_doubleParams["savings_rate_level_3"] = 0.4;
        m_doubleParams["default_savings_rate"] = 0.0;  // Default return value for no optimization
        
        // Time dilation
        m_doubleParams["dilation_threshold"] = 0.7;
        m_doubleParams["priority_modifier"] = 0.1;
        
        // Timeline compression
        m_intParams["timeline_length_small"] = 100;
        m_intParams["timeline_length_medium"] = 500;
        m_intParams["timeline_length_large"] = 1000;
        m_intParams["timeline_length_very_large"] = 5000;
        
        // Compression ratios
        m_doubleParams["compression_ratio_none"] = 0.0;
        m_doubleParams["compression_ratio_light"] = 0.3;
        m_doubleParams["compression_ratio_medium"] = 0.5;
        m_doubleParams["compression_ratio_heavy"] = 0.7;
        m_doubleParams["compression_ratio_maximum"] = 0.85;
        m_doubleParams["initial_compression_ratio"] = 0.0;  // Initial value for compression ratio
    }

private:
    // Private constructor for singleton
    ResourceConfig() {
        loadDefaults();
    }
    
    // Delete copy and move constructors and assignment operators
    ResourceConfig(const ResourceConfig&) = delete;
    ResourceConfig& operator=(const ResourceConfig&) = delete;
    ResourceConfig(ResourceConfig&&) = delete;
    ResourceConfig& operator=(ResourceConfig&&) = delete;

    std::unordered_map<std::string, double> m_doubleParams;
    std::unordered_map<std::string, int> m_intParams;
};

} // namespace chronovyan

#endif // CHRONOVYAN_RESOURCE_CONFIG_H 