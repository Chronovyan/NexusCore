#pragma once

#include "interfaces/ITutorialFramework.hpp"
#include "tutorials/TutorialProgressTracker.hpp"
#include "TutorialLoader.hpp"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

namespace ai_editor {

/**
 * @class TutorialManager
 * @brief Implementation of the ITutorialManager interface
 * 
 * This class provides a concrete implementation of the ITutorialManager
 * interface for managing tutorials and coordinating tutorial execution.
 */
class TutorialManager : public ITutorialManager {
public:
    /**
     * @brief Constructor
     */
    TutorialManager();
    
    /**
     * @brief Constructor with progress tracker
     * 
     * @param progressTracker Progress tracker to use
     */
    explicit TutorialManager(std::shared_ptr<ITutorialProgressTracker> progressTracker);
    
    /**
     * @brief Destructor
     */
    ~TutorialManager() override = default;
    
    // ITutorialManager implementation
    bool registerTutorial(std::shared_ptr<ITutorial> tutorial) override;
    bool unregisterTutorial(const std::string& tutorialId) override;
    std::shared_ptr<ITutorial> getTutorial(const std::string& tutorialId) const override;
    std::vector<std::shared_ptr<ITutorial>> getAllTutorials() const override;
    std::vector<std::shared_ptr<ITutorial>> getTutorialsByType(TutorialType type) const override;
    std::vector<std::shared_ptr<ITutorial>> getTutorialsByDifficulty(TutorialDifficulty difficulty) const override;
    std::vector<std::shared_ptr<ITutorial>> getTutorialsByTag(const std::string& tag) const override;
    std::vector<std::shared_ptr<ITutorial>> getRecommendedTutorials(size_t count) const override;
    bool startTutorial(const std::string& tutorialId) override;
    bool endCurrentTutorial(bool completed) override;
    std::shared_ptr<ITutorial> getCurrentTutorial() const override;
    std::optional<TutorialStep> getCurrentStep() const override;
    bool moveToNextStep() override;
    bool moveToPreviousStep() override;
    bool moveToStep(const std::string& stepId) override;
    void registerActionHandler(TutorialActionType actionType, TutorialActionHandler handler) override;
    std::shared_ptr<ITutorialProgressTracker> getProgressTracker() const override;
    size_t loadTutorialsFromDirectory(const std::string& directoryPath) override;
    void registerCompletionCallback(TutorialCompletionCallback callback) override;
    
    /**
     * @brief Check if a tutorial is in progress
     * 
     * @return bool True if a tutorial is in progress
     */
    bool isTutorialInProgress() const;
    
    /**
     * @brief Get the current step index
     * 
     * @return int The current step index, or -1 if no tutorial is in progress
     */
    int getCurrentStepIndex() const;
    
    /**
     * @brief Execute a tutorial action
     * 
     * @param step The step containing the action to execute
     * @return bool True if successful
     */
    bool executeAction(const TutorialStep& step);
    
    /**
     * @brief Verify if the current step is completed
     * 
     * @return bool True if the current step is completed
     */
    bool verifyCurrentStep();
    
    /**
     * @brief Get the default progress file path
     * 
     * @return std::string The default progress file path
     */
    std::string getDefaultProgressFilePath() const;
    
    /**
     * @brief Load tutorial progress from the default file
     * 
     * @return bool True if successful
     */
    bool loadProgress();
    
    /**
     * @brief Save tutorial progress to the default file
     * 
     * @return bool True if successful
     */
    bool saveProgress() const;
    
private:
    /**
     * @brief Find tutorial by ID
     * 
     * @param tutorialId ID of the tutorial to find
     * @return std::shared_ptr<ITutorial> The tutorial, or nullptr if not found
     */
    std::shared_ptr<ITutorial> findTutorial(const std::string& tutorialId) const;
    
    /**
     * @brief Get the step index by ID
     * 
     * @param tutorial The tutorial
     * @param stepId ID of the step
     * @return int The step index, or -1 if not found
     */
    int getStepIndex(const std::shared_ptr<ITutorial>& tutorial, const std::string& stepId) const;
    
    /**
     * @brief Get the first step ID of a tutorial
     * 
     * @param tutorial The tutorial
     * @return std::string The first step ID, or empty string if no steps
     */
    std::string getFirstStepId(const std::shared_ptr<ITutorial>& tutorial) const;
    
    /**
     * @brief Update the current step
     * 
     * @param stepId ID of the new current step
     * @return bool True if successful
     */
    bool updateCurrentStep(const std::string& stepId);
    
    /**
     * @brief Get the action handler for a specific action type
     * 
     * @param actionType The action type
     * @return TutorialActionHandler The handler, or nullptr if not found
     */
    TutorialActionHandler getActionHandler(TutorialActionType actionType) const;
    
    // Member variables
    std::unordered_map<std::string, std::shared_ptr<ITutorial>> tutorials_;
    std::shared_ptr<ITutorialProgressTracker> progressTracker_;
    std::unordered_map<TutorialActionType, TutorialActionHandler> actionHandlers_;
    std::string currentTutorialId_;
    std::string currentStepId_;
    TutorialCompletionCallback completionCallback_;
    TutorialLoader tutorialLoader_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace ai_editor 