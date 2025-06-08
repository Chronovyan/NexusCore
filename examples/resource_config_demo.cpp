#include "resource_optimizer.h"
#include "resource_config.h"
#include "resource_config_loader.h"
#include "temporal_operation.h"
#include "timeline.h"
#include <iostream>
#include <string>

using namespace chronovyan;

int main(int argc, char* argv[]) {
    std::cout << "Chronovyan Resource Optimization Demo" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    
    // Get the resource configuration singleton
    ResourceConfig& config = ResourceConfig::getInstance();
    
    // Load configuration from file if provided
    if (argc > 1) {
        std::string configFile = argv[1];
        std::cout << "Loading configuration from: " << configFile << std::endl;
        if (!ResourceConfigLoader::loadFromFile(configFile, config)) {
            std::cerr << "Failed to load configuration. Using defaults." << std::endl;
        }
    } else {
        std::cout << "Using default configuration values." << std::endl;
    }
    
    // Create a resource optimizer
    ResourceOptimizer optimizer;
    
    // Demonstrate optimization of a temporal operation
    TemporalOperation operation;
    operation.setEfficiency(0.3); // Start with a low efficiency
    
    std::cout << "\nTemporal Operation Optimization:" << std::endl;
    std::cout << "Initial efficiency: " << operation.getEfficiency() << std::endl;
    
    optimizer.optimizeChrononsUsage(operation);
    
    std::cout << "Optimized efficiency: " << operation.getEfficiency() << std::endl;
    std::cout << "Optimization level: " << operation.getOptimizationLevel() << std::endl;
    
    // Demonstrate timeline compression
    Timeline timeline;
    timeline.setLength(750); // Medium length timeline
    
    std::cout << "\nTimeline Compression:" << std::endl;
    std::cout << "Initial length: " << timeline.getLength() << std::endl;
    std::cout << "Initial compression ratio: " << timeline.getCompressionRatio() << std::endl;
    
    optimizer.applyTimelineCompression(timeline);
    
    std::cout << "Applied compression ratio: " << timeline.getCompressionRatio() << std::endl;
    
    // Demonstrate other optimization calculations
    std::cout << "\nOther Optimization Metrics:" << std::endl;
    
    double aethelUsage = 0.5; // 50% of capacity
    double aethelCapacity = 1.0;
    double optimalAethel = optimizer.calculateOptimalAethelAllocation(aethelUsage, aethelCapacity);
    std::cout << "Optimal Aethel allocation: " << optimalAethel << " (from current: " << aethelUsage << ")" << std::endl;
    
    double temporalComplexity = 0.8; // High complexity
    int branchCount = 5; // Multiple branches
    double paradoxRisk = optimizer.calculateParadoxRisk(temporalComplexity, branchCount);
    std::cout << "Paradox risk: " << paradoxRisk << " (complexity: " << temporalComplexity 
              << ", branches: " << branchCount << ")" << std::endl;
    
    int operationComplexity = 750; // Moderate-to-high complexity
    int optimalThreads = optimizer.calculateOptimalThreads(operationComplexity);
    std::cout << "Optimal thread count: " << optimalThreads << " (complexity: " << operationComplexity << ")" << std::endl;
    
    double currentResourceUsage = 0.6; // 60% usage
    int optimizationLevel = 2; // Medium optimization
    double resourceSavings = optimizer.estimateResourceSavings(currentResourceUsage, optimizationLevel);
    std::cout << "Estimated resource savings: " << resourceSavings 
              << " (current usage: " << currentResourceUsage 
              << ", optimization level: " << optimizationLevel << ")" << std::endl;
    
    double resourceUsage = 0.8; // High resource usage
    double operationPriority = 0.7; // High priority
    bool shouldDilate = optimizer.shouldApplyTimeDilation(resourceUsage, operationPriority);
    std::cout << "Should apply time dilation: " << (shouldDilate ? "Yes" : "No") 
              << " (usage: " << resourceUsage 
              << ", priority: " << operationPriority << ")" << std::endl;
    
    return 0;
} 