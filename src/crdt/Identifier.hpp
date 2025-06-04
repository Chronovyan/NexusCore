#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ai_editor {

/**
 * @class IdentifierElement
 * @brief A single element in a position identifier
 * 
 * Represents a single element in a path-based position identifier for CRDT characters.
 */
class IdentifierElement {
public:
    /**
     * @brief Constructor
     * 
     * @param digit The digit value
     * @param clientId The client ID
     */
    IdentifierElement(uint32_t digit, const std::string& clientId);
    
    /**
     * @brief Get the digit value
     * 
     * @return uint32_t The digit value
     */
    uint32_t getDigit() const;
    
    /**
     * @brief Get the client ID
     * 
     * @return std::string The client ID
     */
    std::string getClientId() const;
    
    /**
     * @brief Compare with another element
     * 
     * @param other The other element
     * @return int -1, 0, or 1 for less than, equal, or greater than
     */
    int compareTo(const IdentifierElement& other) const;
    
    /**
     * @brief Equality operator
     * 
     * @param other The other element
     * @return bool True if equal
     */
    bool operator==(const IdentifierElement& other) const;
    
    /**
     * @brief Less than operator
     * 
     * @param other The other element
     * @return bool True if less than
     */
    bool operator<(const IdentifierElement& other) const;
    
private:
    uint32_t digit_;
    std::string clientId_;
};

/**
 * @class Identifier
 * @brief A position identifier for CRDT characters
 * 
 * Represents a path-based position identifier for CRDT characters, consisting of
 * a vector of IdentifierElement objects.
 */
class Identifier {
public:
    /**
     * @brief Default constructor
     */
    Identifier();
    
    /**
     * @brief Constructor with elements
     * 
     * @param elements The identifier elements
     */
    explicit Identifier(const std::vector<IdentifierElement>& elements);
    
    /**
     * @brief Get the elements
     * 
     * @return const std::vector<IdentifierElement>& The elements
     */
    const std::vector<IdentifierElement>& getElements() const;
    
    /**
     * @brief Compare with another identifier
     * 
     * @param other The other identifier
     * @return int -1, 0, or 1 for less than, equal, or greater than
     */
    int compareTo(const Identifier& other) const;
    
    /**
     * @brief Equality operator
     * 
     * @param other The other identifier
     * @return bool True if equal
     */
    bool operator==(const Identifier& other) const;
    
    /**
     * @brief Less than operator
     * 
     * @param other The other identifier
     * @return bool True if less than
     */
    bool operator<(const Identifier& other) const;
    
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
     * @return Identifier The deserialized identifier
     */
    static Identifier fromJson(const std::string& json);
    
    /**
     * @brief Create a new identifier
     * 
     * @param clientId The client ID
     * @return Identifier A new identifier
     */
    static Identifier create(const std::string& clientId);
    
    /**
     * @brief Create an identifier before another
     * 
     * @param after The identifier to be before
     * @param clientId The client ID
     * @return Identifier A new identifier before the given one
     */
    static Identifier before(const Identifier& after, const std::string& clientId);
    
    /**
     * @brief Create an identifier after another
     * 
     * @param before The identifier to be after
     * @param clientId The client ID
     * @return Identifier A new identifier after the given one
     */
    static Identifier after(const Identifier& before, const std::string& clientId);
    
    /**
     * @brief Create an identifier between two others
     * 
     * @param before The identifier to be after
     * @param after The identifier to be before
     * @param clientId The client ID
     * @return Identifier A new identifier between the given ones
     */
    static Identifier between(
        const Identifier& before,
        const Identifier& after,
        const std::string& clientId);
    
private:
    std::vector<IdentifierElement> elements_;
    
    /**
     * @brief Generate digits between two values
     * 
     * @param left The left value
     * @param right The right value
     * @return uint32_t A value between left and right
     */
    static uint32_t generateDigitsBetween(uint32_t left, uint32_t right);
};

} // namespace ai_editor 