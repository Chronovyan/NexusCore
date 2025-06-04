#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include "crdt/Identifier.hpp"

namespace ai_editor {

/**
 * @class CRDTChar
 * @brief A character in the CRDT document
 * 
 * Represents a single character in a CRDT document, with a value, position,
 * client ID, logical clock, and deletion status.
 */
class CRDTChar {
public:
    /**
     * @brief Constructor
     * 
     * @param value The character value
     * @param position The position identifier
     * @param clientId The client ID
     * @param clock The logical clock value
     * @param deleted Whether the character is deleted
     */
    CRDTChar(
        char value,
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock,
        bool deleted = false);
    
    /**
     * @brief Get the character value
     * 
     * @return char The character value
     */
    char getValue() const;
    
    /**
     * @brief Get the position identifier
     * 
     * @return const Identifier& The position identifier
     */
    const Identifier& getPosition() const;
    
    /**
     * @brief Get the client ID
     * 
     * @return const std::string& The client ID
     */
    const std::string& getClientId() const;
    
    /**
     * @brief Get the logical clock
     * 
     * @return uint64_t The logical clock value
     */
    uint64_t getClock() const;
    
    /**
     * @brief Check if the character is deleted
     * 
     * @return bool True if deleted
     */
    bool isDeleted() const;
    
    /**
     * @brief Mark the character as deleted
     * 
     * @param deleted The new deletion status
     */
    void markDeleted(bool deleted);
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const;
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON representation
     * @return std::shared_ptr<CRDTChar> The deserialized character
     */
    static std::shared_ptr<CRDTChar> fromJson(const std::string& json);
    
    /**
     * @brief Equality operator
     * 
     * @param other The other character
     * @return bool True if equal
     */
    bool operator==(const CRDTChar& other) const;
    
    /**
     * @brief Less than operator (compares positions)
     * 
     * @param other The other character
     * @return bool True if this position is less than other's
     */
    bool operator<(const CRDTChar& other) const;
    
private:
    char value_;
    Identifier position_;
    std::string clientId_;
    uint64_t clock_;
    bool deleted_;
};

} // namespace ai_editor 