#include "tutorials/TutorialManager.hpp"
#include "tutorials/Tutorial.hpp"
#include "EditorErrorReporter.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include "../ErrorReporter.h"

namespace ai_editor {

TutorialManager::TutorialManager()
    : progressTracker_(std::make_shared<TutorialProgressTracker>()),
      completionCallback_(nullptr) {
}

TutorialManager::TutorialManager(std::shared_ptr<ITutorialProgressTracker> progressTracker)
    : progressTracker_(progressTracker),
      completionCallback_(nullptr) {
    if (!progressTracker_) {
        progressTracker_ = std::make_shared<TutorialProgressTracker>();
    }
}

bool TutorialManager::registerTutorial(std::shared_ptr<ITutorial> tutorial) {
    if (!tutorial) {
        ERROR_REPORTER.reportError("TutorialManager::registerTutorial",
            "Tutorial is null");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& info = tutorial->getInfo();
    
    // Check if a tutorial with this ID already exists
    if (tutorials_.find(info.id) != tutorials_.end()) {
        ERROR_REPORTER.reportError("TutorialManager::registerTutorial",
            "Tutorial with ID already exists: " + info.id);
        return false;
    }
    
    // Add the tutorial
    tutorials_[info.id] = tutorial;
    
    // Register a completion callback
    tutorial->setCompletionCallback([this, id = info.id](const std::string&) {
        if (completionCallback_) {
            completionCallback_(id);
        }
    });
    
    return true;
}

bool TutorialManager::unregisterTutorial(const std::string& tutorialId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the tutorial exists
    auto it = tutorials_.find(tutorialId);
    if (it == tutorials_.end()) {
        ERROR_REPORTER.reportError("TutorialManager::unregisterTutorial",
            "Tutorial not found: " + tutorialId);
        return false;
    }
    
    // If this tutorial is currently in progress, end it
    if (currentTutorialId_ == tutorialId) {
        endCurrentTutorial(false);
    }
    
    // Remove the tutorial
    tutorials_.erase(it);
    
    return true;
}

std::shared_ptr<ITutorial> TutorialManager::getTutorial(const std::string& tutorialId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findTutorial(tutorialId);
}

std::vector<std::shared_ptr<ITutorial>> TutorialManager::getAllTutorials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ITutorial>> result;
    result.reserve(tutorials_.size());
    
    for (const auto& pair : tutorials_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<ITutorial>> TutorialManager::getTutorialsByType(TutorialType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ITutorial>> result;
    
    for (const auto& pair : tutorials_) {
        if (pair.second->getInfo().type == type) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<ITutorial>> TutorialManager::getTutorialsByDifficulty(TutorialDifficulty difficulty) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ITutorial>> result;
    
    for (const auto& pair : tutorials_) {
        if (pair.second->getInfo().difficulty == difficulty) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<ITutorial>> TutorialManager::getTutorialsByTag(const std::string& tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<ITutorial>> result;
    
    for (const auto& pair : tutorials_) {
        const auto& tags = pair.second->getInfo().tags;
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<ITutorial>> TutorialManager::getRecommendedTutorials(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get all tutorials
    std::vector<std::shared_ptr<ITutorial>> allTutorials;
    allTutorials.reserve(tutorials_.size());
    
    for (const auto& pair : tutorials_) {
        allTutorials.push_back(pair.second);
    }
    
    // Sort tutorials by priority:
    // 1. Uncompleted tutorials
    // 2. Beginner tutorials
    // 3. Tutorials with no prerequisites
    // 4. Tutorials with fewer steps
    std::sort(allTutorials.begin(), allTutorials.end(), [this](
        const std::shared_ptr<ITutorial>& a, 
        const std::shared_ptr<ITutorial>& b) {
        
        const auto& aInfo = a->getInfo();
        const auto& bInfo = b->getInfo();
        
        // Check if the tutorials are completed
        bool aCompleted = false;
        bool bCompleted = false;
        
        auto aProgress = progressTracker_->getProgress(aInfo.id);
        if (aProgress && aProgress->isCompleted) {
            aCompleted = true;
        }
        
        auto bProgress = progressTracker_->getProgress(bInfo.id);
        if (bProgress && bProgress->isCompleted) {
            bCompleted = true;
        }
        
        // Uncompleted tutorials first
        if (aCompleted != bCompleted) {
            return !aCompleted;
        }
        
        // Beginner tutorials first
        if (aInfo.difficulty != bInfo.difficulty) {
            return aInfo.difficulty < bInfo.difficulty;
        }
        
        // Tutorials with no prerequisites first
        if (aInfo.prerequisites.empty() != bInfo.prerequisites.empty()) {
            return aInfo.prerequisites.empty();
        }
        
        // Tutorials with fewer steps first
        return a->getStepCount() < b->getStepCount();
    });
    
    // Return the first 'count' tutorials
    std::vector<std::shared_ptr<ITutorial>> result;
    result.reserve(std::min(count, allTutorials.size()));
    
    for (size_t i = 0; i < std::min(count, allTutorials.size()); ++i) {
        result.push_back(allTutorials[i]);
    }
    
    return result;
}

bool TutorialManager::startTutorial(const std::string& tutorialId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // If another tutorial is in progress, end it
    if (!currentTutorialId_.empty()) {
        endCurrentTutorial(false);
    }
    
    // Find the tutorial
    auto tutorial = findTutorial(tutorialId);
    if (!tutorial) {
        ERROR_REPORTER.reportError("TutorialManager::startTutorial",
            "Tutorial not found: " + tutorialId);
        return false;
    }
    
    // Get the first step
    std::string firstStepId = getFirstStepId(tutorial);
    if (firstStepId.empty()) {
        ERROR_REPORTER.reportError("TutorialManager::startTutorial",
            "Tutorial has no steps: " + tutorialId);
        return false;
    }
    
    // Set the current tutorial and step
    currentTutorialId_ = tutorialId;
    currentStepId_ = firstStepId;
    
    // Initialize progress tracking
    progressTracker_->initializeProgress(tutorialId, firstStepId);
    progressTracker_->incrementAttemptCount(tutorialId);
    
    return true;
}

bool TutorialManager::endCurrentTutorial(bool completed) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a tutorial is in progress
    if (currentTutorialId_.empty()) {
        return false;
    }
    
    // Mark the tutorial as completed if requested
    if (completed) {
        progressTracker_->markTutorialCompleted(currentTutorialId_);
        
        // Call the completion callback
        if (completionCallback_) {
            completionCallback_(currentTutorialId_);
        }
    }
    
    // Clear the current tutorial
    currentTutorialId_ = "";
    currentStepId_ = "";
    
    return true;
}

std::shared_ptr<ITutorial> TutorialManager::getCurrentTutorial() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (currentTutorialId_.empty()) {
        return nullptr;
    }
    
    return findTutorial(currentTutorialId_);
}

std::optional<TutorialStep> TutorialManager::getCurrentStep() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (currentTutorialId_.empty() || currentStepId_.empty()) {
        return std::nullopt;
    }
    
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        return std::nullopt;
    }
    
    return tutorial->getStep(currentStepId_);
}

bool TutorialManager::moveToNextStep() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a tutorial is in progress
    if (currentTutorialId_.empty() || currentStepId_.empty()) {
        ERROR_REPORTER.reportError("TutorialManager::moveToNextStep",
            "No tutorial in progress");
        return false;
    }
    
    // Find the tutorial
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        ERROR_REPORTER.reportError("TutorialManager::moveToNextStep",
            "Current tutorial not found: " + currentTutorialId_);
        return false;
    }
    
    // Find the current step index
    int currentIndex = getStepIndex(tutorial, currentStepId_);
    if (currentIndex == -1) {
        ERROR_REPORTER.reportError("TutorialManager::moveToNextStep",
            "Current step not found: " + currentStepId_);
        return false;
    }
    
    // Check if there is a next step
    if (currentIndex >= static_cast<int>(tutorial->getStepCount()) - 1) {
        // This was the last step, mark the tutorial as completed
        progressTracker_->markTutorialCompleted(currentTutorialId_);
        
        // Call the completion callback
        if (completionCallback_) {
            completionCallback_(currentTutorialId_);
        }
        
        // Clear the current tutorial
        currentTutorialId_ = "";
        currentStepId_ = "";
        
        return false;
    }
    
    // Get the next step
    auto nextStep = tutorial->getStepByIndex(currentIndex + 1);
    if (!nextStep) {
        ERROR_REPORTER.reportError("TutorialManager::moveToNextStep",
            "Failed to get next step");
        return false;
    }
    
    // Mark the current step as completed
    progressTracker_->markStepCompleted(currentTutorialId_, currentStepId_);
    
    // Update the current step
    currentStepId_ = nextStep->id;
    progressTracker_->setCurrentStep(currentTutorialId_, currentStepId_);
    
    return true;
}

bool TutorialManager::moveToPreviousStep() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a tutorial is in progress
    if (currentTutorialId_.empty() || currentStepId_.empty()) {
        ERROR_REPORTER.reportError("TutorialManager::moveToPreviousStep",
            "No tutorial in progress");
        return false;
    }
    
    // Find the tutorial
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        ERROR_REPORTER.reportError("TutorialManager::moveToPreviousStep",
            "Current tutorial not found: " + currentTutorialId_);
        return false;
    }
    
    // Find the current step index
    int currentIndex = getStepIndex(tutorial, currentStepId_);
    if (currentIndex == -1) {
        ERROR_REPORTER.reportError("TutorialManager::moveToPreviousStep",
            "Current step not found: " + currentStepId_);
        return false;
    }
    
    // Check if there is a previous step
    if (currentIndex <= 0) {
        // This was the first step
        return false;
    }
    
    // Get the previous step
    auto prevStep = tutorial->getStepByIndex(currentIndex - 1);
    if (!prevStep) {
        ERROR_REPORTER.reportError("TutorialManager::moveToPreviousStep",
            "Failed to get previous step");
        return false;
    }
    
    // Update the current step
    currentStepId_ = prevStep->id;
    progressTracker_->setCurrentStep(currentTutorialId_, currentStepId_);
    
    return true;
}

bool TutorialManager::moveToStep(const std::string& stepId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a tutorial is in progress
    if (currentTutorialId_.empty()) {
        ERROR_REPORTER.reportError("TutorialManager::moveToStep",
            "No tutorial in progress");
        return false;
    }
    
    // Find the tutorial
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        ERROR_REPORTER.reportError("TutorialManager::moveToStep",
            "Current tutorial not found: " + currentTutorialId_);
        return false;
    }
    
    // Check if the step exists
    auto step = tutorial->getStep(stepId);
    if (!step) {
        ERROR_REPORTER.reportError("TutorialManager::moveToStep",
            "Step not found: " + stepId);
        return false;
    }
    
    // Update the current step
    currentStepId_ = stepId;
    progressTracker_->setCurrentStep(currentTutorialId_, currentStepId_);
    
    return true;
}

void TutorialManager::registerActionHandler(TutorialActionType actionType, TutorialActionHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!handler) {
        // If the handler is null, remove it
        actionHandlers_.erase(actionType);
    } else {
        // Add or update the handler
        actionHandlers_[actionType] = handler;
    }
}

std::shared_ptr<ITutorialProgressTracker> TutorialManager::getProgressTracker() const {
    return progressTracker_;
}

std::vector<std::shared_ptr<ITutorial>> TutorialManager::loadTutorialsFromDirectory(const std::string& directoryPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Use the tutorial loader to load tutorials from the directory
        std::vector<std::shared_ptr<ITutorial>> loadedTutorials = tutorialLoader.loadFromDirectory(directoryPath);
        
        // Register each loaded tutorial
        for (const auto& tutorial : loadedTutorials) {
            registerTutorial(tutorial);
        }
        
        // Return the loaded tutorials
        return loadedTutorials;
    } 
    catch (const std::exception& e) {
        ERROR_REPORTER.report(ErrorReporter::Severity::ERROR, 
                             "Error loading tutorials from directory: " + directoryPath + 
                             ", error: " + e.what());
        return {};
    }
}

void TutorialManager::registerCompletionCallback(TutorialCompletionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completionCallback_ = callback;
}

bool TutorialManager::isTutorialInProgress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !currentTutorialId_.empty();
}

int TutorialManager::getCurrentStepIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (currentTutorialId_.empty() || currentStepId_.empty()) {
        return -1;
    }
    
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        return -1;
    }
    
    return getStepIndex(tutorial, currentStepId_);
}

bool TutorialManager::executeAction(const TutorialStep& step) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the step has an action
    if (!step.actionType) {
        return true; // No action to execute
    }
    
    // Get the action handler
    auto handler = getActionHandler(step.actionType.value());
    if (!handler) {
        ERROR_REPORTER.reportError("TutorialManager::executeAction",
            "No handler registered for action type: " + std::to_string(static_cast<int>(step.actionType.value())));
        return false;
    }
    
    // Execute the action
    return handler(step);
}

bool TutorialManager::verifyCurrentStep() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if a tutorial is in progress
    if (currentTutorialId_.empty() || currentStepId_.empty()) {
        return false;
    }
    
    // Find the tutorial
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        return false;
    }
    
    // Verify the step
    auto castedTutorial = std::dynamic_pointer_cast<Tutorial>(tutorial);
    if (!castedTutorial) {
        ERROR_REPORTER.reportError("TutorialManager::verifyCurrentStep",
            "Failed to cast tutorial to concrete type");
        return false;
    }
    
    return castedTutorial->verifyStep(currentStepId_);
}

std::string TutorialManager::getDefaultProgressFilePath() const {
    return "tutorial_progress.json";
}

bool TutorialManager::loadProgress() {
    return progressTracker_->loadFromFile(getDefaultProgressFilePath());
}

bool TutorialManager::saveProgress() const {
    return progressTracker_->saveToFile(getDefaultProgressFilePath());
}

std::shared_ptr<ITutorial> TutorialManager::findTutorial(const std::string& tutorialId) const {
    auto it = tutorials_.find(tutorialId);
    if (it == tutorials_.end()) {
        return nullptr;
    }
    
    return it->second;
}

int TutorialManager::getStepIndex(const std::shared_ptr<ITutorial>& tutorial, const std::string& stepId) const {
    const auto& steps = tutorial->getSteps();
    
    for (size_t i = 0; i < steps.size(); ++i) {
        if (steps[i].id == stepId) {
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

std::string TutorialManager::getFirstStepId(const std::shared_ptr<ITutorial>& tutorial) const {
    const auto& steps = tutorial->getSteps();
    
    if (steps.empty()) {
        return "";
    }
    
    return steps[0].id;
}

bool TutorialManager::updateCurrentStep(const std::string& stepId) {
    // Find the tutorial
    auto tutorial = findTutorial(currentTutorialId_);
    if (!tutorial) {
        return false;
    }
    
    // Check if the step exists
    auto step = tutorial->getStep(stepId);
    if (!step) {
        return false;
    }
    
    // Update the current step
    currentStepId_ = stepId;
    progressTracker_->setCurrentStep(currentTutorialId_, currentStepId_);
    
    return true;
}

TutorialActionHandler TutorialManager::getActionHandler(TutorialActionType actionType) const {
    auto it = actionHandlers_.find(actionType);
    if (it == actionHandlers_.end()) {
        return nullptr;
    }
    
    return it->second;
}

} // namespace ai_editor 