#include "tutorials/TutorialUIController.hpp"
#include "UIModel.h"
#include "EditorErrorReporter.h"
#include <sstream>

namespace ai_editor {

TutorialUIController::TutorialUIController(std::shared_ptr<UIModel> uiModel)
    : uiModel_(uiModel), isTutorialVisible_(false) {
}

TutorialUIController::TutorialUIController(
    std::shared_ptr<UIModel> uiModel,
    std::shared_ptr<ITutorialManager> tutorialManager)
    : uiModel_(uiModel), tutorialManager_(tutorialManager), isTutorialVisible_(false) {
}

bool TutorialUIController::showTutorial(const std::string& tutorialId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have a tutorial manager
    if (!tutorialManager_) {
        ERROR_REPORTER.reportError("TutorialUIController::showTutorial",
            "No tutorial manager set");
        return false;
    }
    
    // Start the tutorial
    if (!tutorialManager_->startTutorial(tutorialId)) {
        ERROR_REPORTER.reportError("TutorialUIController::showTutorial",
            "Failed to start tutorial: " + tutorialId);
        return false;
    }
    
    // Update the UI
    isTutorialVisible_ = true;
    updateUI();
    
    return true;
}

bool TutorialUIController::hideTutorial() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Hide the tutorial UI
    isTutorialVisible_ = false;
    
    // If a tutorial is in progress, end it
    if (tutorialManager_ && tutorialManager_->getCurrentTutorial()) {
        tutorialManager_->endCurrentTutorial(false);
    }
    
    return true;
}

bool TutorialUIController::updateStep(const TutorialStep& step) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the tutorial UI is visible
    if (!isTutorialVisible_) {
        return false;
    }
    
    // Update the UI
    if (uiModel_) {
        // Set step information in the UI model
        uiModel_->tutorialStepTitle = step.title;
        uiModel_->tutorialStepDescription = step.description;
        
        // If the step has an action, highlight the target
        if (step.actionType && step.actionTarget) {
            highlightElement(step.actionTarget.value());
        }
    }
    
    return true;
}

bool TutorialUIController::highlightElement(const std::string& elementId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Call the highlight handler if available
    if (highlightHandler_) {
        highlightHandler_(elementId);
    }
    
    return true;
}

bool TutorialUIController::showNotification(const std::string& message, bool isError) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Call the notification handler if available
    if (notificationHandler_) {
        notificationHandler_(message, isError);
    }
    
    return true;
}

bool TutorialUIController::showTutorialBrowser() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have a tutorial manager
    if (!tutorialManager_) {
        ERROR_REPORTER.reportError("TutorialUIController::showTutorialBrowser",
            "No tutorial manager set");
        return false;
    }
    
    // Update the UI model with tutorial information
    if (uiModel_) {
        // Get all tutorials
        const auto allTutorials = tutorialManager_->getAllTutorials();
        
        // Prepare tutorial list for UI
        uiModel_->tutorialsList.clear();
        
        for (const auto& tutorial : allTutorials) {
            const auto& info = tutorial->getInfo();
            
            // Check if the tutorial is completed
            bool isCompleted = false;
            auto progress = tutorialManager_->getProgressTracker()->getProgress(info.id);
            if (progress && progress->isCompleted) {
                isCompleted = true;
            }
            
            // Add to the list
            uiModel_->tutorialsList.push_back({
                info.id,
                info.title,
                info.description,
                isCompleted,
                static_cast<int>(info.difficulty),
                info.estimatedTime,
                static_cast<int>(info.type)
            });
        }
        
        // Set browser as visible
        uiModel_->isTutorialBrowserVisible = true;
    }
    
    return true;
}

void TutorialUIController::setTutorialManager(std::shared_ptr<ITutorialManager> manager) {
    std::lock_guard<std::mutex> lock(mutex_);
    tutorialManager_ = manager;
}

void TutorialUIController::setUIModel(std::shared_ptr<UIModel> uiModel) {
    std::lock_guard<std::mutex> lock(mutex_);
    uiModel_ = uiModel;
}

void TutorialUIController::updateUI() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have a UI model and tutorial manager
    if (!uiModel_ || !tutorialManager_) {
        return;
    }
    
    // Get the current tutorial and step
    auto tutorial = tutorialManager_->getCurrentTutorial();
    auto currentStep = tutorialManager_->getCurrentStep();
    
    // Update the UI model
    if (tutorial && currentStep) {
        // Set tutorial information
        uiModel_->tutorialTitle = tutorial->getInfo().title;
        uiModel_->tutorialDescription = getTutorialInfoText(tutorial);
        
        // Set step information
        uiModel_->tutorialStepTitle = currentStep->title;
        uiModel_->tutorialStepDescription = currentStep->description;
        
        // Set progress information
        int currentIndex = tutorialManager_->getCurrentStepIndex();
        uiModel_->tutorialProgress = getProgressText(tutorial, currentIndex);
        
        // Set visibility
        uiModel_->isTutorialVisible = isTutorialVisible_;
        
        // If the step has an action, highlight the target
        if (currentStep->actionType && currentStep->actionTarget) {
            highlightElement(currentStep->actionTarget.value());
        }
    } else {
        // No tutorial in progress, hide the UI
        uiModel_->isTutorialVisible = false;
    }
}

bool TutorialUIController::isTutorialVisible() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isTutorialVisible_;
}

void TutorialUIController::setTutorialVisible(bool visible) {
    std::lock_guard<std::mutex> lock(mutex_);
    isTutorialVisible_ = visible;
    
    if (uiModel_) {
        uiModel_->isTutorialVisible = visible;
    }
}

void TutorialUIController::registerNotificationHandler(std::function<void(const std::string&, bool)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    notificationHandler_ = handler;
}

void TutorialUIController::registerHighlightHandler(std::function<void(const std::string&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    highlightHandler_ = handler;
}

std::string TutorialUIController::getTutorialInfoText(const std::shared_ptr<ITutorial>& tutorial) const {
    const auto& info = tutorial->getInfo();
    
    std::stringstream ss;
    ss << info.description << "\n\n";
    
    // Add difficulty
    ss << "Difficulty: ";
    switch (info.difficulty) {
        case TutorialDifficulty::BEGINNER:
            ss << "Beginner";
            break;
        case TutorialDifficulty::INTERMEDIATE:
            ss << "Intermediate";
            break;
        case TutorialDifficulty::ADVANCED:
            ss << "Advanced";
            break;
        case TutorialDifficulty::EXPERT:
            ss << "Expert";
            break;
    }
    ss << "\n";
    
    // Add estimated time
    if (!info.estimatedTime.empty()) {
        ss << "Estimated time: " << info.estimatedTime << "\n";
    }
    
    // Add tags
    if (!info.tags.empty()) {
        ss << "Tags: ";
        for (size_t i = 0; i < info.tags.size(); ++i) {
            ss << info.tags[i];
            if (i < info.tags.size() - 1) {
                ss << ", ";
            }
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string TutorialUIController::getStepInfoText(const TutorialStep& step) const {
    std::stringstream ss;
    ss << step.description;
    
    // Add action information if available
    if (step.actionType && step.actionTarget) {
        ss << "\n\nAction: ";
        switch (step.actionType.value()) {
            case TutorialActionType::KEYBOARD_INPUT:
                ss << "Press " << step.actionTarget.value();
                break;
            case TutorialActionType::MOUSE_CLICK:
                ss << "Click on " << step.actionTarget.value();
                break;
            case TutorialActionType::TEXT_INPUT:
                ss << "Type " << step.actionTarget.value();
                break;
            case TutorialActionType::COMMAND_EXECUTION:
                ss << "Execute command " << step.actionTarget.value();
                break;
            case TutorialActionType::MENU_SELECTION:
                ss << "Select menu item " << step.actionTarget.value();
                break;
            case TutorialActionType::AI_INTERACTION:
                ss << "Interact with " << step.actionTarget.value();
                break;
            case TutorialActionType::CUSTOM:
                ss << step.actionTarget.value();
                break;
        }
    }
    
    // Add expected result if available
    if (step.expectedResult) {
        ss << "\n\nExpected result: " << step.expectedResult.value();
    }
    
    return ss.str();
}

std::string TutorialUIController::getProgressText(
    const std::shared_ptr<ITutorial>& tutorial, 
    int currentIndex) const {
    
    std::stringstream ss;
    ss << "Step " << (currentIndex + 1) << " of " << tutorial->getStepCount();
    
    return ss.str();
}

} // namespace ai_editor 