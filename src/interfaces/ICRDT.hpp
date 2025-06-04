#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <functional>

namespace ai_editor {

// Forward declarations
class CRDTChar;
class Identifier;

/**
 * @struct Identifier
 * @brief Unique position identifier for a character in a CRDT
 */
struct Identifier {
    std::vector<std::pair<int, std::string>> path; // Array of (position, client ID) pairs
    
    // Comparison operators
    bool operator==(const Identifier& other) const;
    bool operator!=(const Identifier& other) const;
    bool operator<(const Identifier& other) const;
    bool operator>(const Identifier& other) const;
    bool operator<=(const Identifier& other) const;
    bool operator>=(const Identifier& other) const;
    
    /**
     * @brief Create an identifier between two others
     * 
     * @param left Left boundary identifier
     * @param right Right boundary identifier
     * @param clientId Client ID to use
     * @return Identifier A new identifier between left and right
     */
    static Identifier generateBetween(
        const Identifier& left,
        const Identifier& right,
        const std::string& clientId);
    
    /**
     * @brief Create a new identifier from a JSON string
     * 
     * @param json JSON representation
     * @return Identifier The deserialized identifier
     */
    static Identifier fromJson(const std::string& json);
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const;
};

/**
 * @struct CRDTChar
 * @brief A character in the CRDT
 */
struct CRDTChar {
    char value;                // The character value
    Identifier position;       // Position identifier
    std::string clientId;      // ID of the client that created this character
    uint64_t clock;            // Logical clock value when this character was created
    bool deleted;              // Whether this character is deleted (tombstone)
    
    // Constructor
    CRDTChar(
        char value,
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock,
        bool deleted = false);
    
    // Comparison operators
    bool operator==(const CRDTChar& other) const;
    bool operator!=(const CRDTChar& other) const;
    bool operator<(const CRDTChar& other) const;
    bool operator>(const CRDTChar& other) const;
    
    /**
     * @brief Create a new character from a JSON string
     * 
     * @param json JSON representation
     * @return CRDTChar The deserialized character
     */
    static CRDTChar fromJson(const std::string& json);
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const;
};

/**
 * @enum CRDTOperationType
 * @brief Types of CRDT operations
 */
enum class CRDTOperationType {
    INSERT,       // Insert a character
    DELETE,       // Delete a character
    FORMAT,       // Apply formatting to characters
    COMPOSITE     // Multiple operations as one
};

/**
 * @class ICRDTStrategy
 * @brief Interface for CRDT conflict resolution strategies
 * 
 * This interface defines the methods that any CRDT conflict resolution
 * strategy must implement. Different strategies may have different
 * approaches to handling concurrent operations.
 */
class ICRDTStrategy {
public:
    virtual ~ICRDTStrategy() = default;
    
    /**
     * @brief Insert a character at a specific position
     * 
     * @param value The character to insert
     * @param index The index to insert at
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return std::shared_ptr<CRDTChar> The inserted character
     */
    virtual std::shared_ptr<CRDTChar> insert(
        char value,
        size_t index,
        const std::string& clientId,
        uint64_t clock) = 0;
    
    /**
     * @brief Delete a character at a specific index
     * 
     * @param index The index of the character to delete
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return bool True if the character was deleted
     */
    virtual bool remove(
        size_t index,
        const std::string& clientId,
        uint64_t clock) = 0;
    
    /**
     * @brief Get the character at a specific index
     * 
     * @param index The index
     * @return std::shared_ptr<CRDTChar> The character at the index
     */
    virtual std::shared_ptr<CRDTChar> at(size_t index) const = 0;
    
    /**
     * @brief Get the number of characters
     * 
     * @param includeDeleted Whether to include deleted characters
     * @return size_t The number of characters
     */
    virtual size_t size(bool includeDeleted = false) const = 0;
    
    /**
     * @brief Get the content as a string
     * 
     * @return std::string The content
     */
    virtual std::string toString() const = 0;
    
    /**
     * @brief Find a character by its position identifier
     * 
     * @param position The position identifier
     * @return std::optional<size_t> The index, if found
     */
    virtual std::optional<size_t> findByPosition(const Identifier& position) const = 0;
    
    /**
     * @brief Apply a remote insert operation
     * 
     * @param character The character to insert
     * @return bool True if the operation was applied successfully
     */
    virtual bool applyRemoteInsert(const std::shared_ptr<CRDTChar>& character) = 0;
    
    /**
     * @brief Apply a remote delete operation
     * 
     * @param position The position identifier of the character to delete
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return bool True if the operation was applied successfully
     */
    virtual bool applyRemoteDelete(
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock) = 0;
    
    /**
     * @brief Get the current strategy name
     * 
     * @return std::string The strategy name
     */
    virtual std::string getStrategyName() const = 0;
    
    /**
     * @brief Get all characters
     * 
     * @param includeDeleted Whether to include deleted characters
     * @return std::vector<std::shared_ptr<CRDTChar>> All characters
     */
    virtual std::vector<std::shared_ptr<CRDTChar>> getAllChars(bool includeDeleted = false) const = 0;
};

/**
 * @interface ICRDTOperation
 * @brief Interface for CRDT operations
 */
class ICRDTOperation {
public:
    virtual ~ICRDTOperation() = default;
    
    /**
     * @brief Get the operation type
     * 
     * @return CRDTOperationType The operation type
     */
    virtual CRDTOperationType getType() const = 0;
    
    /**
     * @brief Get the client ID
     * 
     * @return std::string The client ID
     */
    virtual std::string getClientId() const = 0;
    
    /**
     * @brief Get the logical clock value
     * 
     * @return uint64_t The logical clock value
     */
    virtual uint64_t getClock() const = 0;
    
    /**
     * @brief Apply this operation to a CRDT strategy
     * 
     * @param strategy The CRDT strategy
     * @return bool True if the operation was applied successfully
     */
    virtual bool apply(ICRDTStrategy& strategy) = 0;
    
    /**
     * @brief Serialize the operation to JSON
     * 
     * @return std::string JSON representation
     */
    virtual std::string toJson() const = 0;
    
    /**
     * @brief Create an operation from JSON
     * 
     * @param json JSON representation
     * @return std::shared_ptr<ICRDTOperation> The deserialized operation
     */
    static std::shared_ptr<ICRDTOperation> fromJson(const std::string& json);
    
    /**
     * @brief Invert this operation
     * 
     * @return std::shared_ptr<ICRDTOperation> The inverse operation
     */
    virtual std::shared_ptr<ICRDTOperation> invert() const = 0;
};

/**
 * @class ICRDT
 * @brief Interface for CRDT document
 * 
 * This interface defines the methods that a CRDT document must implement.
 * A CRDT document manages the state of a collaborative text document and
 * handles local and remote operations.
 */
class ICRDT {
public:
    virtual ~ICRDT() = default;
    
    /**
     * @brief Insert a character at a specific position
     * 
     * @param c The character to insert
     * @param index The index to insert at
     * @return std::shared_ptr<CRDTChar> The inserted character
     */
    virtual std::shared_ptr<CRDTChar> localInsert(char c, size_t index) = 0;
    
    /**
     * @brief Delete a character at a specific index
     * 
     * @param index The index of the character to delete
     * @return bool True if the character was deleted
     */
    virtual bool localDelete(size_t index) = 0;
    
    /**
     * @brief Apply a remote insert operation
     * 
     * @param character The character to insert
     * @return bool True if the operation was applied successfully
     */
    virtual bool remoteInsert(const std::shared_ptr<CRDTChar>& character) = 0;
    
    /**
     * @brief Apply a remote delete operation
     * 
     * @param position The position identifier of the character to delete
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return bool True if the operation was applied successfully
     */
    virtual bool remoteDelete(
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock) = 0;
    
    /**
     * @brief Get the content as a string
     * 
     * @return std::string The content
     */
    virtual std::string toString() const = 0;
    
    /**
     * @brief Get the client ID
     * 
     * @return std::string The client ID
     */
    virtual std::string getClientId() const = 0;
    
    /**
     * @brief Get the current strategy
     * 
     * @return std::shared_ptr<ICRDTStrategy> The current strategy
     */
    virtual std::shared_ptr<ICRDTStrategy> getStrategy() const = 0;
    
    /**
     * @brief Set the strategy
     * 
     * @param strategy The new strategy
     */
    virtual void setStrategy(std::shared_ptr<ICRDTStrategy> strategy) = 0;
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    virtual std::string toJson() const = 0;
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON representation
     * @param clientId The client ID
     * @return std::shared_ptr<ICRDT> The deserialized CRDT
     */
    static std::shared_ptr<ICRDT> fromJson(
        const std::string& json,
        const std::string& clientId);
};

} // namespace ai_editor 