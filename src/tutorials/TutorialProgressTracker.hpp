#pragma once

#include "interfaces/ITutorialFramework.hpp"
#include <mutex>
#include <unordered_map>
#include <string>

namespace ai_editor {

/**
 * @class TutorialProgressTracker
 * @brief Implementation of the ITutorialProgressTracker interface
 * 
 * This class provides a concrete implementation of the ITutorialProgressTracker
 * interface for tracking and persisting tutorial progress.
 */
class TutorialProgressTracker : public ITutorialProgressTracker {
public:
    /**
     * @brief Constructor
     */
    TutorialProgressTracker();
    
    /**
     * @brief Constructor with initial progress data
     * 
     * @param initialProgress Initial progress data
     */
    explicit TutorialProgressTracker(
        const std::vector<TutorialProgressData>& initialProgress);
    
    /**
     * @brief Destructor
     */
    ~TutorialProgressTracker() override = default;
    
    // ITutorialProgressTracker implementation
    std::optional<TutorialProgressData> getProgress(
        const std::string& tutorialId) const override;
    
    bool markStepCompleted(
        const std::string& tutorialId,
        const std::string& stepId) override;
    
    bool setCurrentStep(
        const std::string& tutorialId,
        const std::string& stepId) override;
    
    bool markTutorialCompleted(
        const std::string& tutorialId) override;
    
    int incrementAttemptCount(
        const std::string& tutorialId) override;
    
    bool resetProgress(
        const std::string& tutorialId) override;
    
    std::vector<TutorialProgressData> getAllProgress() const override;
    
    bool saveToFile(const std::string& filePath) const override;
    
    bool loadFromFile(const std::string& filePath) override;
    
    /**
     * @brief Initialize progress for a tutorial
     * 
     * @param tutorialId ID of the tutorial
     * @param initialStepId ID of the initial step (optional)
     * @return bool True if successful
     */
    bool initializeProgress(const std::string& tutorialId, 
                           const std::string& initialStepId = "");
    
    /**
     * @brief Check if a tutorial has progress data
     * 
     * @param tutorialId ID of the tutorial
     * @return bool True if progress data exists
     */
    bool hasProgress(const std::string& tutorialId) const;
    
    /**
     * @brief Add metadata to tutorial progress
     * 
     * @param tutorialId ID of the tutorial
     * @param key Metadata key
     * @param value Metadata value
     * @return bool True if successful
     */
    bool addProgressMetadata(
        const std::string& tutorialId,
        const std::string& key,
        const std::string& value);
    
    /**
     * @brief Get progress metadata
     * 
     * @param tutorialId ID of the tutorial
     * @param key Metadata key
     * @return std::optional<std::string> Metadata value, if found
     */
    std::optional<std::string> getProgressMetadata(
        const std::string& tutorialId,
        const std::string& key) const;
    
    /**
     * @brief Clear all progress data
     */
    void clearAllProgress();
    
private:
    /**
     * @brief Get or create progress data for a tutorial
     * 
     * @param tutorialId ID of the tutorial
     * @return TutorialProgressData& Reference to the progress data
     */
    TutorialProgressData& getOrCreateProgressData(const std::string& tutorialId);
    
    /**
     * @brief Get the current date string
     * 
     * @return std::string Current date string
     */
    std::string getCurrentDateString() const;
    
    // Member variables
    std::unordered_map<std::string, TutorialProgressData> progressData_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace ai_editor 