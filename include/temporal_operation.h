#ifndef CHRONOVYAN_TEMPORAL_OPERATION_H
#define CHRONOVYAN_TEMPORAL_OPERATION_H

namespace chronovyan {

/**
 * @class TemporalOperation
 * @brief Represents a temporal operation within the Chronovyan system
 */
class TemporalOperation {
public:
    /**
     * @brief Default constructor
     */
    TemporalOperation() : m_efficiency(0.5), m_optimizationLevel(0) {}
    
    /**
     * @brief Get the current efficiency of the temporal operation
     * @return Efficiency value between 0.0 and 1.0
     */
    double getEfficiency() const { return m_efficiency; }
    
    /**
     * @brief Set the efficiency of the temporal operation
     * @param efficiency New efficiency value between 0.0 and 1.0
     */
    void setEfficiency(double efficiency) { m_efficiency = efficiency; }
    
    /**
     * @brief Get the current optimization level
     * @return Integer representing the optimization level (0-3)
     */
    int getOptimizationLevel() const { return m_optimizationLevel; }
    
    /**
     * @brief Set the optimization level
     * @param level New optimization level (0-3)
     */
    void setOptimizationLevel(int level) { m_optimizationLevel = level; }
    
    /**
     * @brief Applies an optimization factor to improve efficiency
     * @param factor The factor to apply (typically > 1.0)
     */
    void applyOptimizationFactor(double factor) {
        m_efficiency *= factor;
        if (m_efficiency > 1.0) {
            m_efficiency = 1.0;
        }
    }
    
private:
    double m_efficiency;        ///< The efficiency of the operation (0.0-1.0)
    int m_optimizationLevel;    ///< The level of optimization applied (0-3)
};

} // namespace chronovyan

#endif // CHRONOVYAN_TEMPORAL_OPERATION_H 