#pragma once

#include "interfaces/IDiffEngine.hpp"
#include "interfaces/IMergeEngine.hpp"
#include <memory>

/**
 * @class DiffMergeFactory
 * @brief Factory for creating diff and merge engines
 * 
 * This class provides static methods for creating instances of diff and merge engines.
 * It serves as a central point for creating and configuring these components.
 */
class DiffMergeFactory {
public:
    /**
     * @brief Create a diff engine
     * 
     * @return A shared pointer to an IDiffEngine
     */
    static IDiffEnginePtr createDiffEngine();
    
    /**
     * @brief Create a merge engine
     * 
     * @param diffEngine An optional diff engine to use (if null, a new one will be created)
     * @return A shared pointer to an IMergeEngine
     */
    static IMergeEnginePtr createMergeEngine(IDiffEnginePtr diffEngine = nullptr);
}; 