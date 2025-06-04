#pragma once

#include <string>
#include <vector>
#include <memory>
#include "interfaces/ICRDT.hpp"
#include "crdt/Identifier.hpp"
#include "crdt/CRDTChar.hpp"

namespace ai_editor {

// Forward declaration
class ICRDTStrategy;

/**
 * @class CRDTOperation
 * @brief Base class for CRDT operations
 * 
 * This class provides the base implementation for CRDT operations.
 */
class CRDTOperation : public ICRDTOperation {
public:
    /**
     * @brief Constructor
     * 
     * @param type Operation type
     * @param clientId Client ID
     * @param clock Logical clock value
     */
    CRDTOperation(
        CRDTOperationType type,
        const std::string& clientId,
        uint64_t clock);
    
    /**
     * @brief Get the operation type
     * 
     * @return CRDTOperationType The operation type
     */
    CRDTOperationType getType() const override;
    
    /**
     * @brief Get the client ID
     * 
     * @return std::string The client ID
     */
    std::string getClientId() const override;
    
    /**
     * @brief Get the logical clock value
     * 
     * @return uint64_t The logical clock value
     */
    uint64_t getClock() const override;
    
    /**
     * @brief Apply this operation to a CRDT strategy
     * 
     * @param strategy The CRDT strategy
     * @return bool True if the operation was applied successfully
     */
    virtual bool apply(ICRDTStrategy& strategy) override = 0;
    
    /**
     * @brief Serialize the operation to JSON
     * 
     * @return std::string JSON representation
     */
    virtual std::string toJson() const override = 0;
    
    /**
     * @brief Invert this operation
     * 
     * @return std::shared_ptr<ICRDTOperation> The inverse operation
     */
    virtual std::shared_ptr<ICRDTOperation> invert() const override = 0;
    
protected:
    CRDTOperationType type_;
    std::string clientId_;
    uint64_t clock_;
};

/**
 * @class CRDTInsertOperation
 * @brief Operation for inserting a character
 */
class CRDTInsertOperation : public CRDTOperation {
public:
    /**
     * @brief Constructor
     * 
     * @param value Character value
     * @param position Position identifier
     * @param clientId Client ID
     * @param clock Logical clock value
     */
    CRDTInsertOperation(
        char value,
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock);
    
    /**
     * @brief Apply this operation to a CRDT strategy
     * 
     * @param strategy The CRDT strategy
     * @return bool True if the operation was applied successfully
     */
    bool apply(ICRDTStrategy& strategy) override;
    
    /**
     * @brief Serialize the operation to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const override;
    
    /**
     * @brief Invert this operation
     * 
     * @return std::shared_ptr<ICRDTOperation> The inverse operation
     */
    std::shared_ptr<ICRDTOperation> invert() const override;
    
    /**
     * @brief Get the character value
     * 
     * @return char The character value
     */
    char getValue() const;
    
    /**
     * @brief Get the position
     * 
     * @return const Identifier& The position
     */
    const Identifier& getPosition() const;
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON representation
     * @return std::shared_ptr<CRDTInsertOperation> The parsed operation
     */
    static std::shared_ptr<CRDTInsertOperation> fromJson(const std::string& json);
    
private:
    char value_;
    Identifier position_;
};

/**
 * @class CRDTDeleteOperation
 * @brief Operation for deleting a character
 */
class CRDTDeleteOperation : public CRDTOperation {
public:
    /**
     * @brief Constructor
     * 
     * @param position Position of the character to delete
     * @param clientId Client ID
     * @param clock Logical clock value
     */
    CRDTDeleteOperation(
        const Identifier& position,
        const std::string& clientId,
        uint64_t clock);
    
    /**
     * @brief Apply this operation to a CRDT strategy
     * 
     * @param strategy The CRDT strategy
     * @return bool True if the operation was applied successfully
     */
    bool apply(ICRDTStrategy& strategy) override;
    
    /**
     * @brief Serialize the operation to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const override;
    
    /**
     * @brief Invert this operation
     * 
     * @return std::shared_ptr<ICRDTOperation> The inverse operation
     */
    std::shared_ptr<ICRDTOperation> invert() const override;
    
    /**
     * @brief Get the position
     * 
     * @return const Identifier& The position
     */
    const Identifier& getPosition() const;
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON representation
     * @return std::shared_ptr<CRDTDeleteOperation> The parsed operation
     */
    static std::shared_ptr<CRDTDeleteOperation> fromJson(const std::string& json);
    
private:
    Identifier position_;
};

/**
 * @class CRDTCompositeOperation
 * @brief Operation composed of multiple operations
 */
class CRDTCompositeOperation : public CRDTOperation {
public:
    /**
     * @brief Constructor
     * 
     * @param operations Component operations
     * @param clientId Client ID
     * @param clock Logical clock value
     */
    CRDTCompositeOperation(
        const std::vector<std::shared_ptr<ICRDTOperation>>& operations,
        const std::string& clientId,
        uint64_t clock);
    
    /**
     * @brief Apply this operation to a CRDT strategy
     * 
     * @param strategy The CRDT strategy
     * @return bool True if the operation was applied successfully
     */
    bool apply(ICRDTStrategy& strategy) override;
    
    /**
     * @brief Serialize the operation to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const override;
    
    /**
     * @brief Invert this operation
     * 
     * @return std::shared_ptr<ICRDTOperation> The inverse operation
     */
    std::shared_ptr<ICRDTOperation> invert() const override;
    
    /**
     * @brief Get the component operations
     * 
     * @return const std::vector<std::shared_ptr<ICRDTOperation>>& The operations
     */
    const std::vector<std::shared_ptr<ICRDTOperation>>& getOperations() const;
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON representation
     * @return std::shared_ptr<CRDTCompositeOperation> The parsed operation
     */
    static std::shared_ptr<CRDTCompositeOperation> fromJson(const std::string& json);
    
private:
    std::vector<std::shared_ptr<ICRDTOperation>> operations_;
};

} // namespace ai_editor 