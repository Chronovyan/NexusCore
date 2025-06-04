#include "tutorials/Tutorial.hpp"
#include "EditorErrorReporter.h"
#include <algorithm>
#include <chrono>
#include <random>

namespace ai_editor {

Tutorial::Tutorial(const TutorialInfo& info)
    : info_(info), completionCallback_(nullptr) {
    // Ensure tutorial ID is not empty
    if (info_.id.empty()) {
        info_.id = "tutorial_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
    }
}

Tutorial::Tutorial(const TutorialInfo& info, const std::vector<TutorialStep>& steps)
    : info_(info), steps_(steps), completionCallback_(nullptr) {
    // Ensure tutorial ID is not empty
    if (info_.id.empty()) {
        info_.id = "tutorial_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    // Ensure all steps have IDs
    for (auto& step : steps_) {
        if (step.id.empty()) {
            step.id = generateStepId();
        }
    }
}

TutorialInfo Tutorial::getInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

std::vector<TutorialStep> Tutorial::getSteps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return steps_;
}

std::optional<TutorialStep> Tutorial::getStep(const std::string& stepId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& step : steps_) {
        if (step.id == stepId) {
            return step;
        }
    }
    return std::nullopt;
}

std::optional<TutorialStep> Tutorial::getStepByIndex(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index < steps_.size()) {
        return steps_[index];
    }
    return std::nullopt;
}

size_t Tutorial::getStepCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return steps_.size();
}

void Tutorial::registerStepVerifier(const std::string& stepId, TutorialStepVerifier verifier) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Make sure the step exists
    bool stepExists = false;
    for (const auto& step : steps_) {
        if (step.id == stepId) {
            stepExists = true;
            break;
        }
    }
    
    if (!stepExists) {
        ERROR_REPORTER.reportError("Tutorial::registerStepVerifier", 
            "Attempted to register verifier for non-existent step: " + stepId);
        return;
    }
    
    stepVerifiers_[stepId] = verifier;
}

void Tutorial::setCompletionCallback(TutorialCompletionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completionCallback_ = callback;
}

bool Tutorial::addStep(const TutorialStep& step) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Ensure step has an ID
    TutorialStep newStep = step;
    if (newStep.id.empty()) {
        newStep.id = generateStepId();
    }
    
    // Make sure step ID is unique
    for (const auto& existingStep : steps_) {
        if (existingStep.id == newStep.id) {
            ERROR_REPORTER.reportError("Tutorial::addStep", 
                "Step ID already exists: " + newStep.id);
            return false;
        }
    }
    
    steps_.push_back(newStep);
    return true;
}

bool Tutorial::removeStep(const std::string& stepId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int index = findStepIndex(stepId);
    if (index == -1) {
        ERROR_REPORTER.reportError("Tutorial::removeStep", 
            "Step not found: " + stepId);
        return false;
    }
    
    steps_.erase(steps_.begin() + index);
    
    // Remove any associated verifier
    auto verifierIt = stepVerifiers_.find(stepId);
    if (verifierIt != stepVerifiers_.end()) {
        stepVerifiers_.erase(verifierIt);
    }
    
    return true;
}

bool Tutorial::verifyStep(const std::string& stepId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the step exists
    auto stepOptional = getStep(stepId);
    if (!stepOptional) {
        ERROR_REPORTER.reportError("Tutorial::verifyStep", 
            "Step not found: " + stepId);
        return false;
    }
    
    // Check if there's a verifier for this step
    auto verifierIt = stepVerifiers_.find(stepId);
    if (verifierIt == stepVerifiers_.end()) {
        // No verifier, assume the step is completed
        return true;
    }
    
    // Call the verifier function
    return verifierIt->second(stepOptional.value());
}

bool Tutorial::isValid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if the tutorial has a valid ID
    if (info_.id.empty()) {
        return false;
    }
    
    // Check if the tutorial has a title
    if (info_.title.empty()) {
        return false;
    }
    
    // Check if the tutorial has at least one step
    if (steps_.empty()) {
        return false;
    }
    
    // Check if all steps have IDs
    for (const auto& step : steps_) {
        if (step.id.empty()) {
            return false;
        }
    }
    
    return true;
}

void Tutorial::updateInfo(const TutorialInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Preserve the ID
    TutorialInfo newInfo = info;
    if (newInfo.id.empty()) {
        newInfo.id = info_.id;
    }
    
    info_ = newInfo;
}

std::string Tutorial::generateStepId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(1000, 9999);
    
    return "step_" + std::to_string(distrib(gen)) + "_" + 
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count() % 10000);
}

int Tutorial::findStepIndex(const std::string& stepId) const {
    for (size_t i = 0; i < steps_.size(); ++i) {
        if (steps_[i].id == stepId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace ai_editor 