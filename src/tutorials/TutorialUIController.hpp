#pragma once

#include "interfaces/ITutorialFramework.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <functional>

namespace ai_editor {

// Forward declarations
class UIModel;

/**
 * @class TutorialUIController
 * @brief Implementation of the ITutorialUIController interface
 * 
 * This class provides a concrete implementation of the ITutorialUIController
 * interface for controlling the tutorial UI.
 */
class TutorialUIController : public ITutorialUIController {
public:
    /**
     * @brief Constructor
     * 
     * @param uiModel UI model to use
     */
    explicit TutorialUIController(std::shared_ptr<UIModel> uiModel);
    
    /**
     * @brief Constructor with tutorial manager
     * 
     * @param uiModel UI model to use
     * @param tutorialManager Tutorial manager to use
     */
    TutorialUIController(
        std::shared_ptr<UIModel> uiModel,
        std::shared_ptr<ITutorialManager> tutorialManager);
    
    /**
     * @brief Destructor
     */
    ~TutorialUIController() override = default;
    
    // ITutorialUIController implementation
    bool showTutorial(const std::string& tutorialId) override;
    bool hideTutorial() override;
    bool updateStep(const TutorialStep& step) override;
    bool highlightElement(const std::string& elementId) override;
    bool showNotification(const std::string& message, bool isError) override;
    bool showTutorialBrowser() override;
    void setTutorialManager(std::shared_ptr<ITutorialManager> manager) override;
    
    /**
     * @brief Set UI model
     * 
     * @param uiModel UI model to use
     */
    void setUIModel(std::shared_ptr<UIModel> uiModel);
    
    /**
     * @brief Update the tutorial UI
     * 
     * Called when the tutorial state changes
     */
    void updateUI();
    
    /**
     * @brief Check if the tutorial UI is visible
     * 
     * @return bool True if the tutorial UI is visible
     */
    bool isTutorialVisible() const;
    
    /**
     * @brief Set the tutorial UI visibility
     * 
     * @param visible Whether the tutorial UI should be visible
     */
    void setTutorialVisible(bool visible);
    
    /**
     * @brief Register a notification handler
     * 
     * @param handler Function to handle notifications
     */
    void registerNotificationHandler(std::function<void(const std::string&, bool)> handler);
    
    /**
     * @brief Register a highlight handler
     * 
     * @param handler Function to handle element highlighting
     */
    void registerHighlightHandler(std::function<void(const std::string&)> handler);
    
private:
    /**
     * @brief Get tutorial info text
     * 
     * @param tutorial The tutorial
     * @return std::string Info text for the tutorial
     */
    std::string getTutorialInfoText(const std::shared_ptr<ITutorial>& tutorial) const;
    
    /**
     * @brief Get step info text
     * 
     * @param step The step
     * @return std::string Info text for the step
     */
    std::string getStepInfoText(const TutorialStep& step) const;
    
    /**
     * @brief Get progress text
     * 
     * @param tutorial The tutorial
     * @param currentIndex Current step index
     * @return std::string Progress text
     */
    std::string getProgressText(
        const std::shared_ptr<ITutorial>& tutorial, 
        int currentIndex) const;
    
    // Member variables
    std::shared_ptr<UIModel> uiModel_;
    std::shared_ptr<ITutorialManager> tutorialManager_;
    bool isTutorialVisible_;
    std::function<void(const std::string&, bool)> notificationHandler_;
    std::function<void(const std::string&)> highlightHandler_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace ai_editor 