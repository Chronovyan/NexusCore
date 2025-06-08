#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <unordered_map>

namespace ai_editor {

/**
 * @enum TutorialType
 * @brief Types of tutorials available in the system
 */
enum class TutorialType {
    EDITOR_BASIC,        // Basic editor functionality tutorials
    EDITOR_ADVANCED,     // Advanced editor functionality tutorials
    AI_BASIC,            // Basic AI feature tutorials
    AI_ADVANCED,         // Advanced AI feature tutorials
    PROJECT_SPECIFIC,    // Tutorials specific to the current project
    CUSTOM               // Custom tutorial types
};

/**
 * @enum TutorialDifficulty
 * @brief Difficulty levels for tutorials
 */
enum class TutorialDifficulty {
    BEGINNER,            // Easy tutorials for new users
    INTERMEDIATE,        // Moderate difficulty for users familiar with basics
    ADVANCED,            // Challenging tutorials for experienced users
    EXPERT               // Complex tutorials for power users
};

/**
 * @enum TutorialStepType
 * @brief Types of tutorial steps
 */
enum class TutorialStepType {
    INSTRUCTION,         // Provide information or instructions to the user
    ACTION,              // User needs to perform a specific action
    VERIFICATION,        // System verifies user has completed a task
    INTERACTIVE,         // User interacts with a specific UI element
    DEMONSTRATION,       // System demonstrates a feature
    QUIZ                 // Test user's knowledge with a question
};

/**
 * @enum TutorialActionType
 * @brief Types of actions that can be performed in a tutorial
 */
enum class TutorialActionType {
    KEYBOARD_INPUT,      // User needs to press specific keys
    MOUSE_CLICK,         // User needs to click on an element
    TEXT_INPUT,          // User needs to type specific text
    COMMAND_EXECUTION,   // User needs to execute a specific command
    MENU_SELECTION,      // User needs to select a menu item
    AI_INTERACTION,      // User needs to interact with an AI feature
    CUSTOM               // Custom action type
};

/**
 * @struct TutorialStep
 * @brief Represents a single step in a tutorial
 */
struct TutorialStep {
    std::string id;                     // Unique identifier for the step
    std::string title;                  // Short title for the step
    std::string description;            // Detailed description/instructions
    TutorialStepType type;              // Type of tutorial step
    std::optional<TutorialActionType> actionType; // Type of action (if applicable)
    std::optional<std::string> actionTarget;      // Target for the action (e.g., menu item, command name)
    std::optional<std::string> expectedResult;    // Expected result after action
    std::optional<std::string> verificationCode;  // Code to verify step completion
    std::unordered_map<std::string, std::string> metadata; // Additional metadata for the step
    
    // Constructors
    TutorialStep() = default;
    
    TutorialStep(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        TutorialStepType type
    ) : id(id), title(title), description(description), type(type) {}
};

/**
 * @struct TutorialInfo
 * @brief Contains metadata about a tutorial
 */
struct TutorialInfo {
    std::string id;                     // Unique identifier for the tutorial
    std::string title;                  // Tutorial title
    std::string description;            // Tutorial description
    TutorialType type;                  // Type of tutorial
    TutorialDifficulty difficulty;      // Difficulty level
    std::vector<std::string> tags;      // Tags for categorization
    std::vector<std::string> prerequisites; // IDs of tutorials that should be completed first
    std::string estimatedTime;          // Estimated time to complete (e.g., "5-10 minutes")
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
    
    // Constructors
    TutorialInfo() = default;
    
    TutorialInfo(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        TutorialType type,
        TutorialDifficulty difficulty,
        const std::string& estimatedTime = ""
    ) : id(id), title(title), description(description), 
        type(type), difficulty(difficulty), estimatedTime(estimatedTime) {}
};

/**
 * @struct TutorialProgressData
 * @brief Stores progress information for a tutorial
 */
struct TutorialProgressData {
    std::string tutorialId;             // ID of the tutorial
    std::string currentStepId;          // ID of the current step
    std::vector<std::string> completedSteps; // IDs of completed steps
    bool isCompleted;                   // Whether the tutorial is completed
    int attemptCount;                   // Number of times user attempted this tutorial
    std::string lastAttemptDate;        // Date of last attempt
    std::unordered_map<std::string, std::string> metadata; // Additional progress metadata
    
    // Constructor
    TutorialProgressData() : isCompleted(false), attemptCount(0) {}
    
    TutorialProgressData(const std::string& tutorialId)
        : tutorialId(tutorialId), isCompleted(false), attemptCount(0) {}
};

/**
 * @typedef TutorialStepVerifier
 * @brief Function type for verifying tutorial step completion
 */
using TutorialStepVerifier = std::function<bool(const TutorialStep&)>;

/**
 * @typedef TutorialActionHandler
 * @brief Function type for handling tutorial actions
 */
using TutorialActionHandler = std::function<bool(const TutorialStep&)>;

/**
 * @typedef TutorialCompletionCallback
 * @brief Function type for tutorial completion callbacks
 */
using TutorialCompletionCallback = std::function<void(const std::string&)>;

/**
 * @interface ITutorial
 * @brief Interface for a tutorial
 */
class ITutorial {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ITutorial() = default;
    
    /**
     * @brief Get tutorial information
     * 
     * @return TutorialInfo Information about the tutorial
     */
    virtual TutorialInfo getInfo() const = 0;
    
    /**
     * @brief Get all steps in the tutorial
     * 
     * @return std::vector<TutorialStep> All tutorial steps
     */
    virtual std::vector<TutorialStep> getSteps() const = 0;
    
    /**
     * @brief Get a specific step by ID
     * 
     * @param stepId ID of the step to get
     * @return std::optional<TutorialStep> The step, if found
     */
    virtual std::optional<TutorialStep> getStep(const std::string& stepId) const = 0;
    
    /**
     * @brief Get a specific step by index
     * 
     * @param index Index of the step to get
     * @return std::optional<TutorialStep> The step, if found
     */
    virtual std::optional<TutorialStep> getStepByIndex(size_t index) const = 0;
    
    /**
     * @brief Get the total number of steps
     * 
     * @return size_t Number of steps
     */
    virtual size_t getStepCount() const = 0;
    
    /**
     * @brief Register a step verification function
     * 
     * @param stepId ID of the step
     * @param verifier Function to verify step completion
     */
    virtual void registerStepVerifier(
        const std::string& stepId,
        TutorialStepVerifier verifier) = 0;
    
    /**
     * @brief Set completion callback
     * 
     * @param callback Function to call when tutorial is completed
     */
    virtual void setCompletionCallback(TutorialCompletionCallback callback) = 0;
};

/**
 * @interface ITutorialProgressTracker
 * @brief Interface for tracking tutorial progress
 */
class ITutorialProgressTracker {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ITutorialProgressTracker() = default;
    
    /**
     * @brief Get progress data for a tutorial
     * 
     * @param tutorialId ID of the tutorial
     * @return std::optional<TutorialProgressData> Progress data, if found
     */
    virtual std::optional<TutorialProgressData> getProgress(
        const std::string& tutorialId) const = 0;
    
    /**
     * @brief Mark a step as completed
     * 
     * @param tutorialId ID of the tutorial
     * @param stepId ID of the step
     * @return bool True if successful
     */
    virtual bool markStepCompleted(
        const std::string& tutorialId,
        const std::string& stepId) = 0;
    
    /**
     * @brief Set the current step for a tutorial
     * 
     * @param tutorialId ID of the tutorial
     * @param stepId ID of the current step
     * @return bool True if successful
     */
    virtual bool setCurrentStep(
        const std::string& tutorialId,
        const std::string& stepId) = 0;
    
    /**
     * @brief Mark a tutorial as completed
     * 
     * @param tutorialId ID of the tutorial
     * @return bool True if successful
     */
    virtual bool markTutorialCompleted(
        const std::string& tutorialId) = 0;
    
    /**
     * @brief Increment attempt count for a tutorial
     * 
     * @param tutorialId ID of the tutorial
     * @return int New attempt count
     */
    virtual int incrementAttemptCount(
        const std::string& tutorialId) = 0;
    
    /**
     * @brief Reset progress for a tutorial
     * 
     * @param tutorialId ID of the tutorial
     * @return bool True if successful
     */
    virtual bool resetProgress(
        const std::string& tutorialId) = 0;
    
    /**
     * @brief Get all tutorials with progress data
     * 
     * @return std::vector<TutorialProgressData> All progress data
     */
    virtual std::vector<TutorialProgressData> getAllProgress() const = 0;
    
    /**
     * @brief Save progress data to file
     * 
     * @param filePath Path to save the progress data
     * @return bool True if successful
     */
    virtual bool saveToFile(const std::string& filePath) const = 0;
    
    /**
     * @brief Load progress data from file
     * 
     * @param filePath Path to load the progress data from
     * @return bool True if successful
     */
    virtual bool loadFromFile(const std::string& filePath) = 0;
};

/**
 * @interface ITutorialManager
 * @brief Interface for managing tutorials
 */
class ITutorialManager {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ITutorialManager() = default;
    
    /**
     * @brief Register a tutorial
     * 
     * @param tutorial The tutorial to register
     * @return bool True if successful
     */
    virtual bool registerTutorial(std::shared_ptr<ITutorial> tutorial) = 0;
    
    /**
     * @brief Unregister a tutorial
     * 
     * @param tutorialId ID of the tutorial to unregister
     * @return bool True if successful
     */
    virtual bool unregisterTutorial(const std::string& tutorialId) = 0;
    
    /**
     * @brief Get a tutorial by ID
     * 
     * @param tutorialId ID of the tutorial
     * @return std::shared_ptr<ITutorial> The tutorial, if found
     */
    virtual std::shared_ptr<ITutorial> getTutorial(
        const std::string& tutorialId) const = 0;
    
    /**
     * @brief Get all registered tutorials
     * 
     * @return std::vector<std::shared_ptr<ITutorial>> All tutorials
     */
    virtual std::vector<std::shared_ptr<ITutorial>> getAllTutorials() const = 0;
    
    /**
     * @brief Get tutorials by type
     * 
     * @param type Type of tutorials to get
     * @return std::vector<std::shared_ptr<ITutorial>> Matching tutorials
     */
    virtual std::vector<std::shared_ptr<ITutorial>> getTutorialsByType(
        TutorialType type) const = 0;
    
    /**
     * @brief Get tutorials by difficulty
     * 
     * @param difficulty Difficulty level
     * @return std::vector<std::shared_ptr<ITutorial>> Matching tutorials
     */
    virtual std::vector<std::shared_ptr<ITutorial>> getTutorialsByDifficulty(
        TutorialDifficulty difficulty) const = 0;
    
    /**
     * @brief Get tutorials by tag
     * 
     * @param tag Tag to filter by
     * @return std::vector<std::shared_ptr<ITutorial>> Matching tutorials
     */
    virtual std::vector<std::shared_ptr<ITutorial>> getTutorialsByTag(
        const std::string& tag) const = 0;
    
    /**
     * @brief Get recommended tutorials based on user progress
     * 
     * @param count Maximum number of tutorials to recommend
     * @return std::vector<std::shared_ptr<ITutorial>> Recommended tutorials
     */
    virtual std::vector<std::shared_ptr<ITutorial>> getRecommendedTutorials(
        size_t count = 5) const = 0;
    
    /**
     * @brief Start a tutorial
     * 
     * @param tutorialId ID of the tutorial to start
     * @return bool True if successful
     */
    virtual bool startTutorial(const std::string& tutorialId) = 0;
    
    /**
     * @brief End the current tutorial
     * 
     * @param completed Whether the tutorial was completed
     * @return bool True if successful
     */
    virtual bool endCurrentTutorial(bool completed = false) = 0;
    
    /**
     * @brief Get the current tutorial
     * 
     * @return std::shared_ptr<ITutorial> The current tutorial, if any
     */
    virtual std::shared_ptr<ITutorial> getCurrentTutorial() const = 0;
    
    /**
     * @brief Get the current tutorial step
     * 
     * @return std::optional<TutorialStep> The current step, if any
     */
    virtual std::optional<TutorialStep> getCurrentStep() const = 0;
    
    /**
     * @brief Move to the next step in the current tutorial
     * 
     * @return bool True if successful
     */
    virtual bool moveToNextStep() = 0;
    
    /**
     * @brief Move to the previous step in the current tutorial
     * 
     * @return bool True if successful
     */
    virtual bool moveToPreviousStep() = 0;
    
    /**
     * @brief Move to a specific step in the current tutorial
     * 
     * @param stepId ID of the step to move to
     * @return bool True if successful
     */
    virtual bool moveToStep(const std::string& stepId) = 0;
    
    /**
     * @brief Register an action handler
     * 
     * @param actionType Type of action
     * @param handler Function to handle the action
     */
    virtual void registerActionHandler(
        TutorialActionType actionType,
        TutorialActionHandler handler) = 0;
    
    /**
     * @brief Get the progress tracker
     * 
     * @return std::shared_ptr<ITutorialProgressTracker> The progress tracker
     */
    virtual std::shared_ptr<ITutorialProgressTracker> getProgressTracker() const = 0;
    
    /**
     * @brief Load tutorials from directory
     * 
     * @param directoryPath Path to directory containing tutorial files
     * @return size_t Number of tutorials loaded
     */
    virtual size_t loadTutorialsFromDirectory(
        const std::string& directoryPath) = 0;
    
    /**
     * @brief Register a tutorial completion callback
     * 
     * @param callback Function to call when any tutorial is completed
     */
    virtual void registerCompletionCallback(
        TutorialCompletionCallback callback) = 0;
};

/**
 * @interface ITutorialUIController
 * @brief Interface for controlling the tutorial UI
 */
class ITutorialUIController {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ITutorialUIController() = default;
    
    /**
     * @brief Show the tutorial UI
     * 
     * @param tutorialId ID of the tutorial to show
     * @return bool True if successful
     */
    virtual bool showTutorial(const std::string& tutorialId) = 0;
    
    /**
     * @brief Hide the tutorial UI
     * 
     * @return bool True if successful
     */
    virtual bool hideTutorial() = 0;
    
    /**
     * @brief Update the tutorial UI with the current step
     * 
     * @param step The current step
     * @return bool True if successful
     */
    virtual bool updateStep(const TutorialStep& step) = 0;
    
    /**
     * @brief Highlight a UI element for the current step
     * 
     * @param elementId ID of the element to highlight
     * @return bool True if successful
     */
    virtual bool highlightElement(const std::string& elementId) = 0;
    
    /**
     * @brief Show a tutorial notification
     * 
     * @param message The notification message
     * @param isError Whether it's an error notification
     * @return bool True if successful
     */
    virtual bool showNotification(
        const std::string& message,
        bool isError = false) = 0;
    
    /**
     * @brief Show the tutorial browser
     * 
     * @return bool True if successful
     */
    virtual bool showTutorialBrowser() = 0;
    
    /**
     * @brief Set tutorial manager
     * 
     * @param manager The tutorial manager to use
     */
    virtual void setTutorialManager(std::shared_ptr<ITutorialManager> manager) = 0;
};

} // namespace ai_editor 