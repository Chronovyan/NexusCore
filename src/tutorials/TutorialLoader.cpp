#include "TutorialLoader.hpp"
#include "../ErrorReporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace ai_editor {

std::shared_ptr<ITutorial> TutorialLoader::loadFromFile(const std::string& filePath) {
    try {
        // Open and read the JSON file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            ERROR_REPORTER.report(ErrorReporter::Severity::ERROR, "Failed to open tutorial file: " + filePath);
            return nullptr;
        }

        // Parse JSON
        nlohmann::json tutorialJson;
        file >> tutorialJson;
        file.close();

        // Extract tutorial info
        TutorialInfo info;
        info.id = tutorialJson["id"];
        info.title = tutorialJson["title"];
        info.description = tutorialJson["description"];
        info.type = stringToTutorialType(tutorialJson["type"]);
        info.difficulty = tutorialJson["difficulty"];
        info.estimatedTime = tutorialJson["estimatedTime"];

        // Extract tags if present
        if (tutorialJson.contains("tags") && tutorialJson["tags"].is_array()) {
            for (const auto& tag : tutorialJson["tags"]) {
                info.tags.push_back(tag);
            }
        }

        // Parse steps
        auto steps = parseSteps(tutorialJson["steps"]);

        // Create and return the tutorial
        auto tutorial = std::make_shared<Tutorial>(info);
        for (const auto& step : steps) {
            tutorial->addStep(step);
        }

        return tutorial;
    }
    catch (const std::exception& e) {
        ERROR_REPORTER.report(ErrorReporter::Severity::ERROR, 
                             "Error loading tutorial from file: " + filePath + 
                             ", error: " + e.what());
        return nullptr;
    }
}

std::vector<std::shared_ptr<ITutorial>> TutorialLoader::loadFromDirectory(const std::string& directoryPath) {
    std::vector<std::shared_ptr<ITutorial>> tutorials;
    
    try {
        // Iterate through all JSON files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.path().extension() == ".json") {
                auto tutorial = loadFromFile(entry.path().string());
                if (tutorial) {
                    tutorials.push_back(tutorial);
                }
            }
        }
    }
    catch (const std::exception& e) {
        ERROR_REPORTER.report(ErrorReporter::Severity::ERROR, 
                             "Error loading tutorials from directory: " + directoryPath + 
                             ", error: " + e.what());
    }
    
    return tutorials;
}

TutorialType TutorialLoader::stringToTutorialType(int typeValue) {
    switch (typeValue) {
        case 0:
            return TutorialType::BEGINNER;
        case 1:
            return TutorialType::INTERMEDIATE;
        case 2:
            return TutorialType::ADVANCED;
        default:
            return TutorialType::BEGINNER;
    }
}

std::vector<TutorialStep> TutorialLoader::parseSteps(const nlohmann::json& stepsJson) {
    std::vector<TutorialStep> steps;
    
    if (!stepsJson.is_array()) {
        ERROR_REPORTER.report(ErrorReporter::Severity::ERROR, "Tutorial steps is not an array");
        return steps;
    }
    
    for (const auto& stepJson : stepsJson) {
        TutorialStep step;
        
        // Extract required fields
        step.id = stepJson["id"];
        step.title = stepJson["title"];
        step.description = stepJson["description"];
        
        // Extract required actions if present
        if (stepJson.contains("required_actions") && stepJson["required_actions"].is_array()) {
            for (const auto& action : stepJson["required_actions"]) {
                step.requiredActions.push_back(action);
            }
        }
        
        steps.push_back(step);
    }
    
    return steps;
}

} // namespace ai_editor 