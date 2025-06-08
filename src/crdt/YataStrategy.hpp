#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "interfaces/ICRDT.hpp"
#include "crdt/CRDTChar.hpp"

namespace ai_editor {

/**
 * @class YataStrategy
 * @brief Implementation of the YATA algorithm for CRDTs
 * 
 * This class provides a concrete implementation of the ICRDTStrategy interface
 * using the YATA (Yet Another Text Algorithm) algorithm for conflict-free
 * replicated text editing.
 */
class YataStrategy : public ICRDTStrategy {
public:
    /**
     * @brief Constructor
     * 
     * @param clientId The client ID
     */
    explicit YataStrategy(const std::string& clientId);
    
    /**
     * @brief Insert a character at a specific position
     * 
     * @param value The character to insert
     * @param index The index to insert at
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return std::shared_ptr<CRDTChar> The inserted character
     */
    std::shared_ptr<CRDTChar> insert(
        char value,
        size_t index,
        const std::string& clientId,
        uint64_t clock) override;
    
    /**
     * @brief Delete a character at a specific index
     * 
     * @param index The index of the character to delete
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return bool True if the character was deleted
     */
    bool remove(
        size_t index,
        const std::string& clientId,
        uint64_t clock) override;
    
    /**
     * @brief Get the character at a specific index
     * 
     * @param index The index
     * @return std::shared_ptr<CRDTChar> The character at the index
     */
    std::shared_ptr<CRDTChar> at(size_t index) const override;
    
    /**
     * @brief Get the number of characters
     * 
     * @param includeDeleted Whether to include deleted characters
     * @return size_t The number of characters
     */
    size_t size(bool includeDeleted = false) const override;
    
    /**
     * @brief Get the content as a string
     * 
     * @return std::string The content
     */
    std::string toString() const override;
    
    /**
     * @brief Find a character by its position identifier
     * 
     * @param position The position identifier
     * @return std::optional<size_t> The index, if found
     */
    std::optional<size_t> findByPosition(const Identifier& position) const override;
    
    /**
     * @brief Apply a remote insert operation
     * 
     * @param character The character to insert
     * @return bool True if the operation was applied successfully
     */
    bool applyRemoteInsert(const std::shared_ptr<CRDTChar>& character) override;
    
    /**
     * @brief Apply a remote delete operation
     * 
     * @param position The position identifier of the character to delete
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return bool True if the operation was applied successfully
     */
    bool applyRemoteDelete(
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock) override;
    
    /**
     * @brief Get the current strategy name
     * 
     * @return std::string The strategy name
     */
    std::string getStrategyName() const override;
    
    /**
     * @brief Get all characters
     * 
     * @param includeDeleted Whether to include deleted characters
     * @return std::vector<std::shared_ptr<CRDTChar>> All characters
     */
    std::vector<std::shared_ptr<CRDTChar>> getAllChars(bool includeDeleted = false) const override;
    
    /**
     * @brief Get the logical clock for a client
     * 
     * @param clientId The client ID
     * @return uint64_t The logical clock value
     */
    uint64_t getClientClock(const std::string& clientId) const;
    
    /**
     * @brief Get the next logical clock for a client
     * 
     * @param clientId The client ID
     * @return uint64_t The next logical clock value
     */
    uint64_t getNextClientClock(const std::string& clientId);
    
    /**
     * @brief Get the vector clock
     * 
     * @return std::unordered_map<std::string, uint64_t> The vector clock
     */
    std::unordered_map<std::string, uint64_t> getVectorClock() const;
    
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
     * @param clientId The client ID
     * @return std::shared_ptr<YataStrategy> The deserialized strategy
     */
    static std::shared_ptr<YataStrategy> fromJson(
        const std::string& json,
        const std::string& clientId);
    
private:
    std::string clientId_;
    std::vector<std::shared_ptr<CRDTChar>> chars_;
    std::unordered_map<std::string, uint64_t> vectorClock_;
    mutable std::mutex mutex_;
    
    /**
     * @brief Find the index to insert a character
     * 
     * @param character The character to insert
     * @return size_t The index to insert at
     */
    size_t findInsertIndex(const std::shared_ptr<CRDTChar>& character) const;
    
    /**
     * @brief Generate a position between two others
     * 
     * @param index The index to insert at
     * @return Identifier The generated position
     */
    Identifier generatePositionBetween(size_t index) const;
    
    /**
     * @brief Get the character at an index
     * 
     * @param index The index
     * @param includeDeleted Whether to include deleted characters
     * @return std::shared_ptr<CRDTChar> The character at the index
     */
    std::shared_ptr<CRDTChar> getCharAt(size_t index, bool includeDeleted = false) const;
    
    /**
     * @brief Update the vector clock
     * 
     * @param clientId The client ID
     * @param clock The logical clock value
     */
    void updateVectorClock(const std::string& clientId, uint64_t clock);
};

} // namespace ai_editor 