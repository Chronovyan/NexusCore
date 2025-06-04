#ifndef TUTORIAL_LOADER_HPP
#define TUTORIAL_LOADER_HPP

#include "../interfaces/ITutorialFramework.hpp"
#include "Tutorial.hpp"
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace ai_editor {

/**
 * @class TutorialLoader
 * @brief Class responsible for loading tutorials from JSON files.
 */
class TutorialLoader {
public:
    /**
     * @brief Default constructor.
     */
    TutorialLoader() = default;

    /**
     * @brief Load a tutorial from a JSON file.
     * @param filePath Path to the JSON file.
     * @return Shared pointer to the loaded tutorial, or nullptr if loading failed.
     */
    std::shared_ptr<ITutorial> loadFromFile(const std::string& filePath);

    /**
     * @brief Load all tutorials from a directory.
     * @param directoryPath Path to the directory containing tutorial JSON files.
     * @return Vector of shared pointers to the loaded tutorials.
     */
    std::vector<std::shared_ptr<ITutorial>> loadFromDirectory(const std::string& directoryPath);

private:
    /**
     * @brief Convert TutorialType string to enum value.
     * @param typeStr Type string from JSON.
     * @return TutorialType enum value.
     */
    TutorialType stringToTutorialType(int typeValue);

    /**
     * @brief Parse tutorial steps from JSON.
     * @param stepsJson JSON object containing steps.
     * @return Vector of TutorialStep objects.
     */
    std::vector<TutorialStep> parseSteps(const nlohmann::json& stepsJson);
};

} // namespace ai_editor

#endif // TUTORIAL_LOADER_HPP 