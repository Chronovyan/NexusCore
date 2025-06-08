#pragma once

#include <string>
#include <memory>
#include <vector>
#include "interfaces/ICRDT.hpp"
#include "crdt/CRDTChar.hpp"
#include "crdt/YataStrategy.hpp"

namespace ai_editor {

/**
 * @class CRDT
 * @brief Implementation of the CRDT document
 * 
 * This class provides a concrete implementation of the ICRDT interface.
 * It manages the state of a collaborative text document and handles local
 * and remote operations using a CRDT strategy.
 */
class CRDT : public ICRDT {
public:
    /**
     * @brief Constructor
     * 
     * @param clientId The client ID
     * @param strategy The CRDT strategy to use
     */
    CRDT(
        const std::string& clientId,
        std::shared_ptr<ICRDTStrategy> strategy = nullptr);
    
    /**
     * @brief Insert a character at a specific position
     * 
     * @param c The character to insert
     * @param index The index to insert at
     * @return std::shared_ptr<CRDTChar> The inserted character
     */
    std::shared_ptr<CRDTChar> localInsert(char c, size_t index) override;
    
    /**
     * @brief Delete a character at a specific index
     * 
     * @param index The index of the character to delete
     * @return bool True if the character was deleted
     */
    bool localDelete(size_t index) override;
    
    /**
     * @brief Apply a remote insert operation
     * 
     * @param character The character to insert
     * @return bool True if the operation was applied successfully
     */
    bool remoteInsert(const std::shared_ptr<CRDTChar>& character) override;
    
    /**
     * @brief Apply a remote delete operation
     * 
     * @param position The position identifier of the character to delete
     * @param clientId The client ID
     * @param clock The logical clock value
     * @return bool True if the operation was applied successfully
     */
    bool remoteDelete(
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock) override;
    
    /**
     * @brief Get the content as a string
     * 
     * @return std::string The content
     */
    std::string toString() const override;
    
    /**
     * @brief Get the client ID
     * 
     * @return std::string The client ID
     */
    std::string getClientId() const override;
    
    /**
     * @brief Get the current strategy
     * 
     * @return std::shared_ptr<ICRDTStrategy> The current strategy
     */
    std::shared_ptr<ICRDTStrategy> getStrategy() const override;
    
    /**
     * @brief Set the strategy
     * 
     * @param strategy The new strategy
     */
    void setStrategy(std::shared_ptr<ICRDTStrategy> strategy) override;
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const override;
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON representation
     * @param clientId The client ID
     * @return std::shared_ptr<CRDT> The deserialized CRDT
     */
    static std::shared_ptr<CRDT> fromJson(
        const std::string& json,
        const std::string& clientId);
    
private:
    std::string clientId_;
    std::shared_ptr<ICRDTStrategy> strategy_;
    
    /**
     * @brief Initialize the strategy if it's null
     */
    void initializeStrategyIfNeeded();
};

} // namespace ai_editor 