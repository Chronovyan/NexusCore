#include "tutorials/TutorialProgressTracker.hpp"
#include "EditorErrorReporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ai_editor {

// Helper functions for JSON serialization
nlohmann::json serializeProgressData(const TutorialProgressData& data) {
    nlohmann::json json;
    json["tutorialId"] = data.tutorialId;
    json["currentStepId"] = data.currentStepId;
    json["completedSteps"] = data.completedSteps;
    json["isCompleted"] = data.isCompleted;
    json["attemptCount"] = data.attemptCount;
    json["lastAttemptDate"] = data.lastAttemptDate;
    json["metadata"] = data.metadata;
    return json;
}

TutorialProgressData deserializeProgressData(const nlohmann::json& json) {
    TutorialProgressData data;
    data.tutorialId = json["tutorialId"].get<std::string>();
    data.currentStepId = json["currentStepId"].get<std::string>();
    data.completedSteps = json["completedSteps"].get<std::vector<std::string>>();
    data.isCompleted = json["isCompleted"].get<bool>();
    data.attemptCount = json["attemptCount"].get<int>();
    data.lastAttemptDate = json["lastAttemptDate"].get<std::string>();
    data.metadata = json["metadata"].get<std::unordered_map<std::string, std::string>>();
    return data;
}

TutorialProgressTracker::TutorialProgressTracker() = default;

TutorialProgressTracker::TutorialProgressTracker(
    const std::vector<TutorialProgressData>& initialProgress) {
    
    for (const auto& progress : initialProgress) {
        progressData_[progress.tutorialId] = progress;
    }
}

std::optional<TutorialProgressData> TutorialProgressTracker::getProgress(
    const std::string& tutorialId) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = progressData_.find(tutorialId);
    if (it == progressData_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool TutorialProgressTracker::markStepCompleted(
    const std::string& tutorialId,
    const std::string& stepId) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreateProgressData(tutorialId);
    
    // Check if the step is already completed
    if (std::find(data.completedSteps.begin(), data.completedSteps.end(), stepId) 
        != data.completedSteps.end()) {
        return true;
    }
    
    // Add the step to the completed steps
    data.completedSteps.push_back(stepId);
    data.lastAttemptDate = getCurrentDateString();
    
    return true;
}

bool TutorialProgressTracker::setCurrentStep(
    const std::string& tutorialId,
    const std::string& stepId) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreateProgressData(tutorialId);
    data.currentStepId = stepId;
    data.lastAttemptDate = getCurrentDateString();
    
    return true;
}

bool TutorialProgressTracker::markTutorialCompleted(
    const std::string& tutorialId) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreateProgressData(tutorialId);
    data.isCompleted = true;
    data.lastAttemptDate = getCurrentDateString();
    
    return true;
}

int TutorialProgressTracker::incrementAttemptCount(
    const std::string& tutorialId) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreateProgressData(tutorialId);
    data.attemptCount++;
    data.lastAttemptDate = getCurrentDateString();
    
    return data.attemptCount;
}

bool TutorialProgressTracker::resetProgress(
    const std::string& tutorialId) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = progressData_.find(tutorialId);
    if (it == progressData_.end()) {
        return false;
    }
    
    // Reset progress data while preserving metadata
    auto metadata = it->second.metadata;
    it->second = TutorialProgressData(tutorialId);
    it->second.metadata = metadata;
    it->second.lastAttemptDate = getCurrentDateString();
    
    return true;
}

std::vector<TutorialProgressData> TutorialProgressTracker::getAllProgress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TutorialProgressData> result;
    result.reserve(progressData_.size());
    
    for (const auto& pair : progressData_) {
        result.push_back(pair.second);
    }
    
    return result;
}

bool TutorialProgressTracker::saveToFile(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        nlohmann::json json;
        json["version"] = 1;
        
        // Serialize all progress data
        nlohmann::json progressArray = nlohmann::json::array();
        for (const auto& pair : progressData_) {
            progressArray.push_back(serializeProgressData(pair.second));
        }
        
        json["progress"] = progressArray;
        
        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            ERROR_REPORTER.reportError("TutorialProgressTracker::saveToFile", 
                "Failed to open file: " + filePath);
            return false;
        }
        
        file << json.dump(4); // Pretty-print with 4-space indentation
        return true;
    }
    catch (const std::exception& e) {
        ERROR_REPORTER.reportError("TutorialProgressTracker::saveToFile", 
            "Exception while saving progress data: " + std::string(e.what()));
        return false;
    }
}

bool TutorialProgressTracker::loadFromFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Read from file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            ERROR_REPORTER.reportError("TutorialProgressTracker::loadFromFile", 
                "Failed to open file: " + filePath);
            return false;
        }
        
        nlohmann::json json;
        file >> json;
        
        // Check version
        if (!json.contains("version") || json["version"].get<int>() != 1) {
            ERROR_REPORTER.reportError("TutorialProgressTracker::loadFromFile", 
                "Unsupported progress file version");
            return false;
        }
        
        // Deserialize progress data
        progressData_.clear();
        for (const auto& item : json["progress"]) {
            TutorialProgressData data = deserializeProgressData(item);
            progressData_[data.tutorialId] = data;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        ERROR_REPORTER.reportError("TutorialProgressTracker::loadFromFile", 
            "Exception while loading progress data: " + std::string(e.what()));
        return false;
    }
}

bool TutorialProgressTracker::initializeProgress(
    const std::string& tutorialId, 
    const std::string& initialStepId) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if progress already exists
    auto it = progressData_.find(tutorialId);
    if (it != progressData_.end()) {
        // Progress already exists, update only if initial step is provided
        if (!initialStepId.empty()) {
            it->second.currentStepId = initialStepId;
            it->second.lastAttemptDate = getCurrentDateString();
        }
        return true;
    }
    
    // Create new progress
    TutorialProgressData data(tutorialId);
    data.currentStepId = initialStepId;
    data.lastAttemptDate = getCurrentDateString();
    progressData_[tutorialId] = data;
    
    return true;
}

bool TutorialProgressTracker::hasProgress(const std::string& tutorialId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return progressData_.find(tutorialId) != progressData_.end();
}

bool TutorialProgressTracker::addProgressMetadata(
    const std::string& tutorialId,
    const std::string& key,
    const std::string& value) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = getOrCreateProgressData(tutorialId);
    data.metadata[key] = value;
    
    return true;
}

std::optional<std::string> TutorialProgressTracker::getProgressMetadata(
    const std::string& tutorialId,
    const std::string& key) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = progressData_.find(tutorialId);
    if (it == progressData_.end()) {
        return std::nullopt;
    }
    
    auto metadataIt = it->second.metadata.find(key);
    if (metadataIt == it->second.metadata.end()) {
        return std::nullopt;
    }
    
    return metadataIt->second;
}

void TutorialProgressTracker::clearAllProgress() {
    std::lock_guard<std::mutex> lock(mutex_);
    progressData_.clear();
}

TutorialProgressData& TutorialProgressTracker::getOrCreateProgressData(
    const std::string& tutorialId) {
    
    auto it = progressData_.find(tutorialId);
    if (it == progressData_.end()) {
        // Create new progress data
        TutorialProgressData data(tutorialId);
        data.lastAttemptDate = getCurrentDateString();
        progressData_[tutorialId] = data;
    }
    
    return progressData_[tutorialId];
}

std::string TutorialProgressTracker::getCurrentDateString() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace ai_editor 