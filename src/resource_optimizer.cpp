#include "resource_optimizer.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

namespace chronovyan {

ResourceOptimizer::ResourceOptimizer() : 
    m_config(ResourceConfig::getInstance())
{
    // Constructor only needs to get reference to the config singleton
}

double ResourceOptimizer::calculateOptimalAethelAllocation(double currentUsage, double maxCapacity) {
    // Use named parameters from the config system instead of magic numbers
    if (currentUsage < maxCapacity * m_config.getDouble("aethel_low_usage_threshold")) {
        // Low usage, can allocate more
        return currentUsage * m_config.getDouble("aethel_low_usage_factor");
    } else if (currentUsage < maxCapacity * m_config.getDouble("aethel_moderate_usage_threshold")) {
        // Moderate usage, allocate carefully
        return currentUsage * m_config.getDouble("aethel_moderate_usage_factor");
    } else if (currentUsage < maxCapacity * m_config.getDouble("aethel_high_usage_threshold")) {
        // High usage, allocate conservatively
        return currentUsage * m_config.getDouble("aethel_high_usage_factor");
    } else {
        // Critical usage, restrict allocation
        return currentUsage;
    }
}

double ResourceOptimizer::calculateParadoxRisk(double temporalComplexity, double branchCount) {
    // Use named parameters from the config system
    double baseRisk = m_config.getDouble("paradox_base_risk");
    double complexityFactor = temporalComplexity * m_config.getDouble("paradox_complexity_weight");
    double branchFactor = pow(branchCount, m_config.getDouble("paradox_branch_power")) * 
                          m_config.getDouble("paradox_branch_weight");
    
    return baseRisk + complexityFactor + branchFactor;
}

void ResourceOptimizer::optimizeChrononsUsage(TemporalOperation& operation) {
    double currentEfficiency = operation.getEfficiency();
    
    // Apply efficiency improvements based on configurable thresholds
    if (currentEfficiency < m_config.getDouble("efficiency_very_low_threshold")) {
        // Very inefficient operation
        operation.setOptimizationLevel(static_cast<int>(OptimizationLevel::AGGRESSIVE));
        operation.applyOptimizationFactor(m_config.getDouble("optimization_level_3_factor"));
    } else if (currentEfficiency < m_config.getDouble("efficiency_moderate_threshold")) {
        // Moderately efficient
        operation.setOptimizationLevel(static_cast<int>(OptimizationLevel::MODERATE));
        operation.applyOptimizationFactor(m_config.getDouble("optimization_level_2_factor"));
    } else if (currentEfficiency < m_config.getDouble("efficiency_high_threshold")) {
        // Fairly efficient
        operation.setOptimizationLevel(static_cast<int>(OptimizationLevel::LIGHT));
        operation.applyOptimizationFactor(m_config.getDouble("optimization_level_1_factor"));
    } else {
        // Already very efficient
        operation.setOptimizationLevel(static_cast<int>(OptimizationLevel::NONE));
    }
}

int ResourceOptimizer::calculateOptimalThreads(int operationComplexity) {
    // Determine thread count based on complexity with configurable thresholds
    if (operationComplexity < m_config.getInt("complexity_low_threshold")) {
        return m_config.getInt("threads_for_simple_operations");
    } else if (operationComplexity < m_config.getInt("complexity_moderate_threshold")) {
        return m_config.getInt("threads_for_moderate_operations");
    } else if (operationComplexity < m_config.getInt("complexity_high_threshold")) {
        return m_config.getInt("threads_for_complex_operations");
    } else {
        return m_config.getInt("threads_for_very_complex_operations");
    }
}

double ResourceOptimizer::estimateResourceSavings(double currentUsage, int optimizationLevel) {
    // Different savings rates from configuration for each optimization level
    switch (optimizationLevel) {
        case static_cast<int>(OptimizationLevel::NONE):
            return m_config.getDouble("default_savings_rate");  // No optimization
        case static_cast<int>(OptimizationLevel::LIGHT):
            return currentUsage * m_config.getDouble("savings_rate_level_1");
        case static_cast<int>(OptimizationLevel::MODERATE):
            return currentUsage * m_config.getDouble("savings_rate_level_2");
        case static_cast<int>(OptimizationLevel::AGGRESSIVE):
            return currentUsage * m_config.getDouble("savings_rate_level_3");
        default:
            return m_config.getDouble("default_savings_rate");
    }
}

bool ResourceOptimizer::shouldApplyTimeDilation(double resourceUsage, double operationPriority) {
    // Configurable thresholds for time dilation decision
    double dilationThreshold = m_config.getDouble("dilation_threshold");
    double priorityModifier = m_config.getDouble("priority_modifier");
    
    double adjustedThreshold = dilationThreshold + (operationPriority * priorityModifier);
    
    return resourceUsage > adjustedThreshold;
}

void ResourceOptimizer::applyTimelineCompression(Timeline& timeline) {
    double compressionRatio = m_config.getDouble("initial_compression_ratio");
    
    // Determine compression ratio based on timeline length using configurable thresholds
    int timelineLength = timeline.getLength();
    if (timelineLength < m_config.getInt("timeline_length_small")) {
        compressionRatio = m_config.getDouble("compression_ratio_none");
    } else if (timelineLength < m_config.getInt("timeline_length_medium")) {
        compressionRatio = m_config.getDouble("compression_ratio_light");
    } else if (timelineLength < m_config.getInt("timeline_length_large")) {
        compressionRatio = m_config.getDouble("compression_ratio_medium");
    } else if (timelineLength < m_config.getInt("timeline_length_very_large")) {
        compressionRatio = m_config.getDouble("compression_ratio_heavy");
    } else {
        compressionRatio = m_config.getDouble("compression_ratio_maximum");
    }
    
    // NOTE: Timeline::compress method doesn't exist yet, this is a placeholder for future implementation
    // timeline.compress(compressionRatio);
    
    // For now, just log the compression ratio that would be applied
    std::cout << "Would compress timeline to " << (compressionRatio * 100) << "% of original size" << std::endl;
}

} // namespace chronovyan 