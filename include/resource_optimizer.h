#ifndef CHRONOVYAN_RESOURCE_OPTIMIZER_H
#define CHRONOVYAN_RESOURCE_OPTIMIZER_H

#include "temporal_operation.h"
#include "timeline.h"
#include "resource_config.h"

namespace chronovyan {

/**
 * @class ResourceOptimizer
 * @brief Optimizes resource usage for temporal operations and timelines
 *
 * This class is responsible for analyzing and optimizing resource usage in temporal
 * operations, calculating optimal resource allocations, and estimating resource
 * requirements for different operations.
 */
class ResourceOptimizer {
public:
    /**
     * @brief Enumeration for optimization levels
     * 
     * This enum replaces magic numbers with named constants to improve code readability
     * and maintainability. Each level represents a different degree of optimization:
     * - NONE: No optimization
     * - LIGHT: Light optimization
     * - MODERATE: Moderate optimization
     * - AGGRESSIVE: Aggressive optimization
     */
    enum class OptimizationLevel {
        NONE = 0,       // No optimization
        LIGHT = 1,      // Light optimization
        MODERATE = 2,   // Moderate optimization
        AGGRESSIVE = 3  // Aggressive optimization
    };
    
    /**
     * @brief Default constructor
     */
    ResourceOptimizer();

    /**
     * @brief Calculates the optimal aethel allocation for a given usage and capacity
     * @param currentUsage The current amount of aethel being used
     * @param maxCapacity The maximum aethel capacity available
     * @return The optimal aethel allocation
     */
    double calculateOptimalAethelAllocation(double currentUsage, double maxCapacity);

    /**
     * @brief Calculates the paradox risk based on temporal complexity and branch count
     * @param temporalComplexity The complexity value of the temporal operation
     * @param branchCount The number of timeline branches
     * @return The calculated paradox risk (0.0 to 1.0)
     */
    double calculateParadoxRisk(double temporalComplexity, double branchCount);

    /**
     * @brief Optimizes chronons usage for a temporal operation
     * @param operation The temporal operation to optimize
     */
    void optimizeChrononsUsage(TemporalOperation& operation);

    /**
     * @brief Calculates the optimal number of threads for parallel execution
     * @param operationComplexity The complexity value of the operation
     * @return The optimal number of threads
     */
    int calculateOptimalThreads(int operationComplexity);

    /**
     * @brief Estimates resource savings from applying optimization
     * @param currentUsage The current resource usage
     * @param optimizationLevel The level of optimization to apply (0-3)
     * @return The estimated resource savings
     */
    double estimateResourceSavings(double currentUsage, int optimizationLevel);

    /**
     * @brief Determines if time dilation should be applied based on resource usage
     * @param resourceUsage The current resource usage (0.0 to 1.0)
     * @param operationPriority The priority of the operation (0.0 to 1.0)
     * @return True if time dilation should be applied, false otherwise
     */
    bool shouldApplyTimeDilation(double resourceUsage, double operationPriority);

    /**
     * @brief Applies timeline compression to optimize storage
     * @param timeline The timeline to compress
     */
    void applyTimelineCompression(Timeline& timeline);

private:
    // Reference to the configuration system
    ResourceConfig& m_config;
};

} // namespace chronovyan

#endif // CHRONOVYAN_RESOURCE_OPTIMIZER_H 