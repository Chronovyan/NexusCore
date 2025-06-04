#pragma once

#include "interfaces/ITutorialFramework.hpp"
#include <mutex>
#include <unordered_map>

namespace ai_editor {

/**
 * @class Tutorial
 * @brief Implementation of the ITutorial interface
 * 
 * This class provides a concrete implementation of the ITutorial interface
 * for storing and running tutorials.
 */
class Tutorial : public ITutorial {
public:
    /**
     * @brief Constructor with tutorial info
     * 
     * @param info Information about the tutorial
     */
    explicit Tutorial(const TutorialInfo& info);
    
    /**
     * @brief Constructor with tutorial info and steps
     * 
     * @param info Information about the tutorial
     * @param steps Steps in the tutorial
     */
    Tutorial(const TutorialInfo& info, const std::vector<TutorialStep>& steps);
    
    /**
     * @brief Destructor
     */
    ~Tutorial() override = default;
    
    // ITutorial implementation
    TutorialInfo getInfo() const override;
    std::vector<TutorialStep> getSteps() const override;
    std::optional<TutorialStep> getStep(const std::string& stepId) const override;
    std::optional<TutorialStep> getStepByIndex(size_t index) const override;
    size_t getStepCount() const override;
    void registerStepVerifier(const std::string& stepId, TutorialStepVerifier verifier) override;
    void setCompletionCallback(TutorialCompletionCallback callback) override;
    
    /**
     * @brief Add a step to the tutorial
     * 
     * @param step The step to add
     * @return bool True if successful
     */
    bool addStep(const TutorialStep& step);
    
    /**
     * @brief Remove a step from the tutorial
     * 
     * @param stepId ID of the step to remove
     * @return bool True if successful
     */
    bool removeStep(const std::string& stepId);
    
    /**
     * @brief Verify if a step has been completed
     * 
     * @param stepId ID of the step to verify
     * @return bool True if the step is completed
     */
    bool verifyStep(const std::string& stepId);
    
    /**
     * @brief Check if the tutorial is valid (has necessary info and steps)
     * 
     * @return bool True if the tutorial is valid
     */
    bool isValid() const;
    
    /**
     * @brief Update tutorial information
     * 
     * @param info The updated information
     */
    void updateInfo(const TutorialInfo& info);
    
    /**
     * @brief Generate a unique ID for a step
     * 
     * @return std::string A unique step ID
     */
    std::string generateStepId() const;
    
private:
    /**
     * @brief Find the index of a step by ID
     * 
     * @param stepId ID of the step to find
     * @return int Index of the step, or -1 if not found
     */
    int findStepIndex(const std::string& stepId) const;
    
    // Member variables
    TutorialInfo info_;
    std::vector<TutorialStep> steps_;
    std::unordered_map<std::string, TutorialStepVerifier> stepVerifiers_;
    TutorialCompletionCallback completionCallback_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace ai_editor 